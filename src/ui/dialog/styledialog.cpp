// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for CSS selectors
 */
/* Authors:
 *   Kamalpreet Kaur Grewal
 *   Tavmjong Bah
 *
 * Copyright (C) Kamalpreet Kaur Grewal 2016 <grewalkamal005@gmail.com>
 * Copyright (C) Tavmjong Bah 2017 <tavmjong@free.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "styledialog.h"
#include "attributes.h"
#include "verbs.h"
#include "selection.h"
#include "attribute-rel-svg.h"
#include "inkscape.h"
#include "document-undo.h"
#include "selection.h"
#include "style-internal.h"
#include "style.h"
#include "io/resource.h"
#include "ui/icon-loader.h"
#include "ui/widget/iconrenderer.h"

#include "xml/attribute-record.h"
#include "xml/node-observer.h"

#include <regex>
#include <map>
#include <utility>

#include <glibmm/i18n.h>

//#define DEBUG_STYLEDIALOG
//#define G_LOG_DOMAIN "STYLEDIALOG"

using Inkscape::DocumentUndo;
using Inkscape::Util::List;
using Inkscape::XML::AttributeRecord;

/**
 * This macro is used to remove spaces around selectors or any strings when
 * parsing is done to update XML style element or row labels in this dialog.
 */
#define REMOVE_SPACES(x) x.erase(0, x.find_first_not_of(' ')); \
    x.erase(x.find_last_not_of(' ') + 1);

namespace Inkscape {
namespace UI {
namespace Dialog {

// Keeps a watch on style element
class StyleDialog::NodeObserver : public Inkscape::XML::NodeObserver {
public:
    NodeObserver(StyleDialog* styledialog) :
        _styledialog(styledialog)
    {
        g_debug("StyleDialog::NodeObserver: Constructor");
    };

    void notifyContentChanged(Inkscape::XML::Node &node,
                                      Inkscape::Util::ptr_shared old_content,
                                      Inkscape::Util::ptr_shared new_content) override;

    StyleDialog * _styledialog;
};


void
StyleDialog::NodeObserver::notifyContentChanged(
    Inkscape::XML::Node &/*node*/,
    Inkscape::Util::ptr_shared /*old_content*/,
    Inkscape::Util::ptr_shared /*new_content*/ ) {

    g_debug("StyleDialog::NodeObserver::notifyContentChanged");
    _styledialog->_updating = false;
    _styledialog->_readStyleElement();
}


// Keeps a watch for new/removed/changed nodes
// (Must update objects that selectors match.)
class StyleDialog::NodeWatcher : public Inkscape::XML::NodeObserver {
public:
    NodeWatcher(StyleDialog* styledialog, Inkscape::XML::Node *repr) :
        _styledialog(styledialog),
        _repr(repr)
    {
        g_debug("StyleDialog::NodeWatcher: Constructor");
    };

    void notifyChildAdded( Inkscape::XML::Node &/*node*/,
                                   Inkscape::XML::Node &child,
                                   Inkscape::XML::Node */*prev*/ ) override
    {
        if ( _styledialog && _repr ) {
            _styledialog->_nodeAdded( child );
        }
    }

    void notifyChildRemoved( Inkscape::XML::Node &/*node*/,
                                     Inkscape::XML::Node &child,
                                     Inkscape::XML::Node */*prev*/ ) override
    {
        if ( _styledialog && _repr ) {
            _styledialog->_nodeRemoved( child );
        }
    }
/*     void notifyContentChanged(Inkscape::XML::Node &node,
                                      Inkscape::Util::ptr_shared old_content,
                                      Inkscape::Util::ptr_shared new_content) override{
        if ( _styledialog && _repr && _textNode == node) {
            _styledialog->_stylesheetChanged( node );
        }
    };
 */
    void notifyAttributeChanged( Inkscape::XML::Node &node,
                                         GQuark qname,
                                         Util::ptr_shared /*old_value*/,
                                         Util::ptr_shared /*new_value*/ ) override {
        if ( _styledialog && _repr ) {

            // For the moment only care about attributes that are directly used in selectors.
            const gchar * cname = g_quark_to_string (qname);
            Glib::ustring name;
            if (cname) {
                name = cname;
            }

            if ( name == "id" || name == "class" || name == "style" ) {
                _styledialog->_nodeChanged( node );
            }
            SPObject *obj = SP_ACTIVE_DOCUMENT->getObjectById(node.attribute("id"));
            if (obj) {
                for (auto iter : obj->style->properties()) {
                    if (iter->style_src != SP_STYLE_SRC_UNSET) {
                        if( iter->name == name) {
                            _styledialog->_nodeChanged( node );
                        }
                    }
                }
            }
        }
    }

    StyleDialog * _styledialog;
    Inkscape::XML::Node * _repr;  // Need to track if document changes.
};

void
StyleDialog::_nodeAdded( Inkscape::XML::Node &node ) {

    StyleDialog::NodeWatcher *w = new StyleDialog::NodeWatcher (this, &node);
    node.addObserver (*w);
    _nodeWatchers.push_back(w);

    _readStyleElement();
}

void
StyleDialog::_nodeRemoved( Inkscape::XML::Node &repr ) {

    for (auto it = _nodeWatchers.begin(); it != _nodeWatchers.end(); ++it) {
        if ( (*it)->_repr == &repr ) {
            (*it)->_repr->removeObserver (**it);
            _nodeWatchers.erase( it );
            break;
        }
    }
    _readStyleElement();
}

void
StyleDialog::_nodeChanged( Inkscape::XML::Node &object ) {
    _readStyleElement();
}

/* void
StyleDialog::_stylesheetChanged( Inkscape::XML::Node &repr ) {
    std::cout << "Style tag modified" << std::endl;
    _readStyleElement();
} */

/**
 * Constructor
 * A treeview and a set of two buttons are added to the dialog. _addSelector
 * adds selectors to treeview. _delSelector deletes the selector from the dialog.
 * Any addition/deletion of the selectors updates XML style element accordingly.
 */
StyleDialog::StyleDialog() :
    UI::Widget::Panel("/dialogs/style", SP_VERB_DIALOG_STYLE),
    _updating(false),
    _textNode(nullptr),
    _desktopTracker()
{
    g_debug("StyleDialog::StyleDialog");
    // Pack widgets
    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _styleBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    _styleBox.set_valign(Gtk::ALIGN_START);
    _scrolledWindow.add(_styleBox);
    Gtk::Box *alltoggler =  Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    Gtk::Label *infotoggler =  Gtk::manage(new Gtk::Label(_("Edit Full Stylesheet")));
    _all_css = Gtk::manage(new Gtk::Switch());
    _all_css->get_style_context()->add_class("switchclean");
    _all_css->set_margin_right(5);
    _all_css->property_active().signal_changed().connect(sigc::mem_fun(*this, &StyleDialog::_reload));
    alltoggler->pack_start(*_all_css, false, false, 0);
    alltoggler->pack_start(*infotoggler, false, false, 0);
    _all_css->set_active(false);
    _mainBox.pack_start(*alltoggler, false, false, 0);
    _mainBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    _getContents()->pack_start(_mainBox, Gtk::PACK_EXPAND_WIDGET);
    // Document & Desktop
    _desktop_changed_connection = _desktopTracker.connectDesktopChanged(
        sigc::mem_fun(*this, &StyleDialog::_handleDesktopChanged) );
    _desktopTracker.connect(GTK_WIDGET(gobj()));

    _document_replaced_connection = getDesktop()->connectDocumentReplaced(
        sigc::mem_fun(this, &StyleDialog::_handleDocumentReplaced));
    
    _selection_changed_connection = getDesktop()->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &StyleDialog::_handleSelectionChanged)));
    // Add watchers
    _updateWatchers();

    // Load tree
    _readStyleElement();
}

/**
 * Class destructor
 */
StyleDialog::~StyleDialog()
{
    g_debug("StyleDialog::~StyleDialog");
    _desktop_changed_connection.disconnect();
    _document_replaced_connection.disconnect();
    _selection_changed_connection.disconnect();
}

void 
StyleDialog::_reload()
{
    _readStyleElement();
}

/**
 * @return Inkscape::XML::Node* pointing to a style element's text node.
 * Returns the style element's text node. If there is no style element, one is created.
 * Ditto for text node.
 */
Inkscape::XML::Node* StyleDialog::_getStyleTextNode()
{

    Inkscape::XML::Node *styleNode = nullptr;
    Inkscape::XML::Node *textNode = nullptr;

    Inkscape::XML::Node *root = SP_ACTIVE_DOCUMENT->getReprRoot();
    for (unsigned i = 0; i < root->childCount(); ++i) {
        if (Glib::ustring(root->nthChild(i)->name()) == "svg:style") {

            styleNode = root->nthChild(i);

            for (unsigned j = 0; j < styleNode->childCount(); ++j) {
                if (styleNode->nthChild(j)->type() == Inkscape::XML::TEXT_NODE) {
                    textNode = styleNode->nthChild(j);
                }
            }

            if (textNode == nullptr) {
                // Style element found but does not contain text node!
                std::cerr << "StyleDialog::_getStyleTextNode(): No text node!" << std::endl;
                textNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");
                styleNode->appendChild(textNode);
                Inkscape::GC::release(textNode);
            }
        }
    }

    if (styleNode == nullptr) {
        // Style element not found, create one
        styleNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createElement("svg:style");
        textNode  = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");

        styleNode->appendChild(textNode);
        Inkscape::GC::release(textNode);

        root->addChild(styleNode, nullptr);
        Inkscape::GC::release(styleNode);
    }

    if (_textNode != textNode) {
        _textNode = textNode;
        NodeObserver *no = new NodeObserver(this);
        textNode->addObserver(*no);
    }

    return textNode;
}

Glib::RefPtr< Gtk::TreeModel > StyleDialog::_selectTree(Glib::ustring selector)
{
    Gtk::Label * selectorlabel;
    Glib::RefPtr< Gtk::TreeModel > model;
    for (auto fullstyle:_styleBox.get_children()){
        Gtk::Box *style = dynamic_cast<Gtk::Box *>(fullstyle);
        for (auto stylepart:style->get_children()){
            switch (style->child_property_position(*stylepart)) {
                case 0:
                    {
                        Gtk::Box *selectorbox = dynamic_cast<Gtk::Box *>(stylepart);
                        for (auto styleheader:selectorbox->get_children()){
                            if (!selectorbox->child_property_position(*styleheader)) {
                                selectorlabel = dynamic_cast<Gtk::Label *>(styleheader);
                            }
                        }
                        break;
                    }
                case 1:
                    {
                        Glib::ustring wdg_selector = selectorlabel->get_text();
                        if (wdg_selector == selector){
                            Gtk::TreeView* treeview = dynamic_cast<Gtk::TreeView *>(stylepart);
                            if (treeview) {
                                return treeview->get_model();
                            }
                        }
                        break;
                    }
                default:
                    break;
            }
        }
    }
    return model;
}

/**
 * Fill the Gtk::TreeStore from the svg:style element.
 */
void StyleDialog::_readStyleElement()
{
    g_debug("StyleDialog::_readStyleElement");

    if (_updating) return; // Don't read if we wrote style element.
    _updating = true;

    Inkscape::XML::Node * textNode = _getStyleTextNode();
    if (textNode == nullptr) {
        std::cerr << "StyleDialog::_readStyleElement: No text node!" << std::endl;
    }

    // Get content from style text node.
    std::string content = (textNode->content() ? textNode->content() : "");

    // Remove end-of-lines (check it works on Windoze).
    content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());

    // Remove comments (/* xxx */)

    bool breakme = false;
    size_t start = content.find("/*");
    size_t open = content.find("{", start + 1);
    size_t close = content.find("}", start + 1);
    size_t end = content.find("*/", close + 1);
    while(!breakme) {
        if (open == std::string::npos ||
            close ==  std::string::npos ||
            end ==  std::string::npos)
        {   
            breakme = true;
            break;
        }
        while (open < close) {
            open = content.find("{", close + 1);
            close = content.find("}", close + 1);
            end = content.find("*/", close + 1);
            size_t reopen = content.find("{", close + 1);
            if (open == std::string::npos ||
                end ==  std::string::npos ||
                end < reopen)
            {   
                if (end < reopen) {
                    content = content.erase(start, end - start + 2);
                } else {
                    breakme = true;
                }
                break;
            }
        }
        start = content.find("/*", start + 1);
        open = content.find("{", start + 1);
        close = content.find("}", start + 1);
        end = content.find("*/", close + 1);
    }

    // First split into selector/value chunks.
    // An attempt to use Glib::Regex failed. A C++11 version worked but
    // reportedly has problems on Windows. Using split_simple() is simpler
    // and probably faster.
    //
    // Glib::RefPtr<Glib::Regex> regex1 =
    //   Glib::Regex::create("([^\\{]+)\\{([^\\{]+)\\}");
    //
    // Glib::MatchInfo minfo;
    // regex1->match(content, minfo);

    // Split on curly brackets. Even tokens are selectors, odd are values.
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[}{]", content);

    // If text node is empty, return (avoids problem with negative below).

    for (auto child:_styleBox.get_children()) {
        _styleBox.remove(*child);
        delete child;
    }
    Inkscape::Selection* selection = getDesktop()->getSelection();
    SPObject *obj = nullptr;
    if(selection->objects().size() == 1) {
        obj = selection->objects().back();
    }

    Glib::ustring gladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-css.ui");
    Glib::RefPtr<Gtk::Builder> _builder;
    try {
        _builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }
    gint selectorpos = 0;
    Gtk::Box *css_selector_container;
    _builder->get_widget("CSSSelectorContainer", css_selector_container);
    Gtk::Label *css_selector;
    _builder->get_widget("CSSSelector", css_selector);
    Gtk::EventBox *css_selector_event_add;
    _builder->get_widget("CSSSelectorEventAdd", css_selector_event_add);
    css_selector_event_add->add_events(Gdk::BUTTON_RELEASE_MASK);
    css_selector->set_text("element");
    Gtk::TreeView *css_tree;
    _builder->get_widget("CSSTree", css_tree);
    Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(_mColumns);
    css_tree->set_model(store);
    // We need to handle comments on SPStyle to activate 
    /* Gtk::CellRendererToggle *active = Gtk::manage(new Gtk::CellRendererToggle);
    int addCol = css_tree->append_column("", *active) - 1;
    Gtk::TreeViewColumn *col = css_tree->get_column(addCol);
    if (col) {
        col->add_attribute(active->property_active(), _mColumns._colActive);
    }  */
    
    css_selector_event_add->signal_button_release_event().connect(
            sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *, Glib::ustring, gint>(
                sigc::mem_fun(*this, &StyleDialog::_addRow), store, css_tree, "style_properties", selectorpos));
    Inkscape::UI::Widget::IconRenderer * addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
    addRenderer->add_icon("edit-delete");
    int addCol = css_tree->append_column("Delete row", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = css_tree->get_column(addCol);
    if (col) {
        addRenderer->signal_activated().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore> >(sigc::mem_fun(*this, &StyleDialog::_onPropDelete), store));
    }
    Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
    label->property_placeholder_text() = _("property");
    label->property_editable() = true;
    label->signal_edited().connect(sigc::bind< Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *>(sigc::mem_fun(*this, &StyleDialog::_nameEdited), store, css_tree));
    addCol = css_tree->append_column("CSS Property", *label) - 1;
    col = css_tree->get_column(addCol);
    if (col) {
        col->add_attribute(label->property_text(), _mColumns._colName);
    }
    Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
    value->property_placeholder_text() = _("value");
    value->property_editable() = true;
    value->signal_edited().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore> >(sigc::mem_fun(*this, &StyleDialog::_valueEdited), store));
    css_tree->set_focus_vadjustment(_scrolledWindow.get_vadjustment());
    addCol = css_tree->append_column("CSS Value", *value) - 1;
    col = css_tree->get_column(addCol);
    if (col) {
        col->add_attribute(value->property_text(), _mColumns._colValue);
        col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
    }
    std::map<Glib::ustring, Glib::ustring> attr_prop;
    if (obj || _all_css->get_active()) {
        Gtk::TreeModel::Path path;
        if(!_all_css->get_active() && obj && obj->getRepr()->attribute("style")) {
            Glib::ustring style = obj->getRepr()->attribute("style");
            attr_prop = parseStyle(style);
            for (auto iter : obj->style->properties()) {
                if (attr_prop.count(iter->name)) {
                    Gtk::TreeModel::Row row = *(store->append());
                    row[_mColumns._colSelector] = "style_properties";
                    row[_mColumns._colSelectorPos] = 0;
                    row[_mColumns._colActive] = true;
                    row[_mColumns._colName] = iter->name;
                    row[_mColumns._colValue] = iter->get_value();
                    row[_mColumns._colStrike] = false;
                }
            }
        _styleBox.pack_start(*css_selector_container, Gtk::PACK_EXPAND_WIDGET);
        }
        selectorpos ++;
        try {
            _builder = Gtk::Builder::create_from_file(gladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for filter effect dialog");
            return;
        }
        _builder->get_widget("CSSSelector", css_selector);
        css_selector->set_text("element.attributes");
        _builder->get_widget("CSSSelectorContainer", css_selector_container);
        _builder->get_widget("CSSSelectorEventAdd", css_selector_event_add);
        css_selector_event_add->add_events(Gdk::BUTTON_RELEASE_MASK);
        store = Gtk::TreeStore::create(_mColumns);
        _builder->get_widget("CSSTree", css_tree);
        css_tree->set_model(store);
        css_selector_event_add->signal_button_release_event().connect(
        sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *, Glib::ustring, gint>(
        sigc::mem_fun(*this, &StyleDialog::_addRow), store, css_tree, "attributes", selectorpos));
        bool hasattributes = false;
        if(!_all_css->get_active()) {
            for (auto iter : obj->style->properties()) {
                if (iter->style_src != SP_STYLE_SRC_UNSET) {
                    if( iter->name != "font" && iter->name != "d" && iter->name != "marker") {
                        const gchar *attr = obj->getRepr()->attribute(iter->name.c_str());
                        if (attr) {
                            if (!hasattributes) {
                                Inkscape::UI::Widget::IconRenderer * addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
                                addRenderer->add_icon("edit-delete");
                                int addCol = css_tree->append_column("Delete row", *addRenderer) - 1;
                                Gtk::TreeViewColumn *col = css_tree->get_column(addCol);
                                if (col) {
                                    addRenderer->signal_activated().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore> >(sigc::mem_fun(*this, &StyleDialog::_onPropDelete), store));
                                }
                                Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
                                label->property_placeholder_text() = _("property");
                                label->property_editable() = true;
                                label->signal_edited().connect(sigc::bind< Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView * >(sigc::mem_fun(*this, &StyleDialog::_nameEdited), store, css_tree));
                                addCol = css_tree->append_column("CSS Property", *label) - 1;
                                col = css_tree->get_column(addCol);
                                if (col) {
                                    col->add_attribute(label->property_text(), _mColumns._colName);
                                }
                                Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
                                value->property_placeholder_text() = _("value");
                                value->property_editable() = true;
                                value->signal_edited().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore> >(sigc::mem_fun(*this, &StyleDialog::_valueEdited), store));
                                css_tree->set_focus_vadjustment(_scrolledWindow.get_vadjustment());
                                addCol = css_tree->append_column("CSS Value", *value) - 1;
                                col = css_tree->get_column(addCol);
                                if (col) {
                                    col->add_attribute(value->property_text(), _mColumns._colValue);
                                    col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
                                }
                            }
                            Gtk::TreeModel::Row row = *(store->append());
                            row[_mColumns._colSelector] = "attributes";
                            row[_mColumns._colSelectorPos] = 1;
                            row[_mColumns._colActive] = true;
                            row[_mColumns._colName] = iter->name;
                            row[_mColumns._colValue] = attr;
                            if (attr_prop.count(iter->name)) {
                                row[_mColumns._colStrike] = true;
                            } else {
                                row[_mColumns._colStrike] = false;
                            }
                            hasattributes = true;
                        }
                    }
                }
            }
            if (!hasattributes) {
                for (auto widg:css_selector_container->get_children()) {
                    delete widg;
                }
            }
            _styleBox.pack_start(*css_selector_container, Gtk::PACK_EXPAND_WIDGET);
        }
        selectorpos ++;
        if (tokens.size() == 0) {
            _updating = false;
            return;
        }
        for (unsigned i = 0; i < tokens.size()-1; i += 2) {
            Glib::ustring selector = tokens[i];
            REMOVE_SPACES(selector); // Remove leading/trailing spaces
            std::vector<Glib::ustring> tokensplus = Glib::Regex::split_simple("[,]+", selector);
            for (auto tok : tokensplus) {
                REMOVE_SPACES(tok);
            }
            // Get list of objects selector matches
            std::vector<SPObject *> objVec = _getObjVec( selector );
            if (!_all_css->get_active()) {
                bool stop = true;
                for (auto objel:objVec) {
                    if (objel->getId() == obj->getId()){
                        stop = false;
                    }
                }
                if (stop) {
                    _updating = false;
                    selectorpos ++;
                    continue;
                }
            }
            Glib::ustring properties;
            // Check to make sure we do have a value to match selector.
            if ((i+1) < tokens.size()) {
                properties = tokens[i+1];
            } else {
                std::cerr << "StyleDialog::_readStyleElement: Missing values "
                    "for last selector!" << std::endl;
            }
            Glib::RefPtr<Gtk::Builder> _builder;
            try {
                _builder = Gtk::Builder::create_from_file(gladefile);
            } catch (const Glib::Error &ex) {
                g_warning("Glade file loading failed for filter effect dialog");
                return;
            }
            Gtk::Box *css_selector_container;
            _builder->get_widget("CSSSelectorContainer", css_selector_container);
            Gtk::Label *css_selector;
            _builder->get_widget("CSSSelector", css_selector);
            Gtk::EventBox *css_selector_event_add;
            _builder->get_widget("CSSSelectorEventAdd", css_selector_event_add);
            css_selector_event_add->add_events(Gdk::BUTTON_RELEASE_MASK);
            css_selector->set_text(selector);
            Gtk::TreeView *css_tree;
            _builder->get_widget("CSSTree", css_tree);
            Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(_mColumns);
            css_tree->set_model(store);
            Inkscape::UI::Widget::IconRenderer * addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
            addRenderer->add_icon("edit-delete");
            int addCol = css_tree->append_column("Delete row", *addRenderer) - 1;
            Gtk::TreeViewColumn *col = css_tree->get_column(addCol);
            if (col) {
                addRenderer->signal_activated().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore> >(sigc::mem_fun(*this, &StyleDialog::_onPropDelete), store));
            } 
            Gtk::CellRendererToggle *active = Gtk::manage(new Gtk::CellRendererToggle);
            addCol = css_tree->append_column("Active Property", *active) - 1;
            col = css_tree->get_column(addCol);
            if (col) {
                col->add_attribute(active->property_active(), _mColumns._colActive);
                active->signal_toggled().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore> >(sigc::mem_fun(*this, &StyleDialog::_activeToggled), store));
            }
            //col->set_cell_data_func(*active, sigc::mem_fun(*this, &StyleDialog::_hideRootToggle));
            Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
            label->property_placeholder_text() = _("property");
            label->property_editable() = true;
            label->signal_edited().connect(sigc::bind< Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView * >(sigc::mem_fun(*this, &StyleDialog::_nameEdited), store, css_tree));
            addCol = css_tree->append_column("CSS Selector", *label) - 1;
            col = css_tree->get_column(addCol);
            if (col) {
                col->add_attribute(label->property_text(), _mColumns._colName);
            }
            Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
            value->property_editable() = true;
            value->signal_edited().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore> >(sigc::mem_fun(*this, &StyleDialog::_valueEdited), store));
            value->property_placeholder_text() = _("value");
            css_tree->set_focus_vadjustment(_scrolledWindow.get_vadjustment());
            addCol = css_tree->append_column("CSS Value", *value) - 1;
            col = css_tree->get_column(addCol);
            if (col) {
                col->add_attribute(value->property_text(), _mColumns._colValue);
                col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
            }
            Glib::ustring style = properties;
            Glib::ustring comments = "";
            while(style.find("/*") != std::string::npos) {
                size_t beg = style.find("/*");
                size_t end = style.find("*/");
                if (end !=  std::string::npos &&
                    beg !=  std::string::npos) 
                {
                    comments = comments.append(style, beg + 2, end - beg - 2);
                    style = style.erase(beg, end - beg + 2);
                }
            }
            std::map<Glib::ustring, Glib::ustring> attr_prop_styleshet = parseStyle(style);
            css_selector_event_add->signal_button_release_event().connect(
            sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *, Glib::ustring, gint>(
            sigc::mem_fun(*this, &StyleDialog::_addRow), store, css_tree, selector, selectorpos));
            if (!_all_css->get_active()) {
                for (auto iter : obj->style->properties()) {
                    if (iter->style_src != SP_STYLE_SRC_UNSET) {
                        if (attr_prop_styleshet.count(iter->name)) {
                            Gtk::TreeModel::Row row = *(store->append());
                            row[_mColumns._colSelector] = selector;
                            row[_mColumns._colSelectorPos] = selectorpos;
                            row[_mColumns._colActive] = true;
                            row[_mColumns._colName] = iter->name;
                            row[_mColumns._colValue] = attr_prop_styleshet[iter->name];
                            if (attr_prop.count(iter->name) || row[_mColumns._colValue] != iter->get_value()) {
                                row[_mColumns._colStrike] = true;
                            } else {
                                row[_mColumns._colStrike] = false;
                            }
                        }
                    }
                }
            } else {
                for (auto iter : attr_prop_styleshet) {
                    Gtk::TreeModel::Row row = *(store->append());
                    row[_mColumns._colSelector] = selector;
                    row[_mColumns._colSelectorPos] = selectorpos;
                    row[_mColumns._colActive] = true;
                    row[_mColumns._colName] = iter.first;
                    row[_mColumns._colValue] = iter.second;
                    row[_mColumns._colStrike] = false;
                }
            }
            std::map<Glib::ustring, Glib::ustring> attr_prop_styleshet_comments = parseStyle(comments);
            
            for (auto iter : attr_prop_styleshet_comments) {
                if (!attr_prop_styleshet.count(iter.first)) {
                    Gtk::TreeModel::Row row = *(store->append());
                    row[_mColumns._colSelector] = selector;
                    row[_mColumns._colSelectorPos] = selectorpos;
                    row[_mColumns._colActive] = false;
                    row[_mColumns._colName] = iter.first;
                    row[_mColumns._colValue] = iter.second;
                    row[_mColumns._colStrike] = true;
                }
            }
            _styleBox.pack_start(*css_selector_container, Gtk::PACK_EXPAND_WIDGET);
            selectorpos ++;
        }
    }
    _mainBox.show_all_children();
    if (obj) {
        obj->style->readFromObject(obj);
        obj->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
    }
    _updating = false;
}

/**
 * @brief StyleDialog::_onPropDelete
 * @param event
 * @return true
 * Delete the attribute from the style
 */
void 
StyleDialog::_onPropDelete(Glib::ustring path, Glib::RefPtr<Gtk::TreeStore> store)
{
    Gtk::TreeModel::Row row = *store->get_iter(path);
    if (row) {
        _updating = true; //to avoid a crash on deleting an obsolete widget
        Glib::ustring selector = row[_mColumns._colSelector];
        row[_mColumns._colName] = ""; 
        store->erase(row);
        _updating = false;
        _writeStyleElement(store, selector);
    }
}

/**
 * @brief StyleDialog::parseStyle
 *
 * Convert a style string into a vector map. This should be moved to style.cpp
 *
 */
std::map<Glib::ustring, Glib::ustring> 
StyleDialog::parseStyle(Glib::ustring style_string)
{
    std::map<Glib::ustring, Glib::ustring> ret;

    REMOVE_SPACES(style_string); // We'd use const, but we need to trip spaces
    std::vector<Glib::ustring> props = r_props->split(style_string);

    for (auto token : props) {
        REMOVE_SPACES(token);

        if (token.empty())
            break;
        std::vector<Glib::ustring> pair = r_pair->split(token);

        if (pair.size() > 1) {
            ret[pair[0]] = pair[1];
        }
    }
    return ret;
}


/**
 * Update the content of the style element as selectors (or objects) are added/removed.
 */
void StyleDialog::_writeStyleElement(Glib::RefPtr<Gtk::TreeStore> store, Glib::ustring selector)
{
    if (_updating) {
        return;
    }
    Inkscape::Selection* selection = getDesktop()->getSelection();
    SPObject *obj = nullptr;
    if (selection->objects().size() == 1) {
        obj = selection->objects().back();
    }
    if (!obj && !_all_css->get_active()) {
        _readStyleElement();
        return;
    }
    _updating = true;
    gint selectorpos = 0;
    std::string styleContent = "";
    if (selector != "style_properties" &&
        selector != "attributes") 
    {
        styleContent = "\n" + selector + " { \n";
    }
    for (auto& row: store->children()) {
        selector = row[_mColumns._colSelector];
        selectorpos = row[_mColumns._colSelectorPos];
        Glib::ustring opencomment ="";
        Glib::ustring closecomment = "" ;
        if (selector != "style_properties" &&
            selector != "attributes") {
            opencomment = row[_mColumns._colActive] ? "    " : "  /*";
            closecomment = row[_mColumns._colActive] ? "\n" : "*/\n" ;
        }
        Glib::ustring name = row[_mColumns._colName];        
        Glib::ustring value = row[_mColumns._colValue];
        if (!(name.empty() && value.empty())){
            styleContent = styleContent + opencomment + name + ":" + value + ";" + closecomment;
        }
    }
    if (selector != "style_properties" &&
        selector != "attributes" ) {
        styleContent = styleContent + "}";
    }
    if (selector == "style_properties") {
        obj->getRepr()->setAttribute("style",styleContent, false);
    } else if (selector == "attributes") {
        for (auto iter : obj->style->properties()) {
            if (iter->style_src != SP_STYLE_SRC_UNSET) {
                if( iter->name != "font" && iter->name != "d" && iter->name != "marker") {
                    const gchar *attr = obj->getRepr()->attribute(iter->name.c_str());
                    if (attr) {
                        obj->getRepr()->setAttribute(iter->name.c_str(), nullptr);
                    }
                }
            }
        }
        for (auto& row: store->children()) {
            Glib::ustring name = row[_mColumns._colName];        
            Glib::ustring value = row[_mColumns._colValue];          
            if (!(name.empty() && value.empty())){
                obj->getRepr()->setAttribute(name.c_str(), value);
            }
        }
    } else if (!selector.empty()) { //styleshet
        // We could test if styleContent is empty and then delete the style node here but there is no
        // harm in keeping it around ...
        SP_ACTIVE_DOCUMENT->setStyleSheet(nullptr);
        std::string pos = std::to_string(selectorpos);
        std::string selectormatch = "(";
        for (selectorpos; selectorpos > 2; selectorpos--) {
            selectormatch = selectormatch + "[^}]*?}";
        }
        selectormatch = selectormatch + ")([^}]*?})((.|\n)*)";
        Inkscape::XML::Node *textNode = _getStyleTextNode();
        std::regex e (selectormatch.c_str());
        std::string content = (textNode->content() ? textNode->content() : "");
        std::string result;
        std::regex_replace (std::back_inserter(result), content.begin(), content.end(), e, "$1" + styleContent + "$3");
        textNode->setContent(result.c_str());
    }
    _readStyleElement();
    DocumentUndo::done(SP_ACTIVE_DOCUMENT, SP_VERB_DIALOG_STYLE, _("Edited style element."));

    _updating = false;
    g_debug("StyleDialog::_writeStyleElement(): | %s |", styleContent.c_str());
}

bool StyleDialog::_addRow(GdkEventButton *evt, Glib::RefPtr<Gtk::TreeStore> store, Gtk::TreeView *css_tree, Glib::ustring selector, gint pos) {
    Gtk::TreeIter iter = store->append();
    Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
    Gtk::TreeModel::Row row = *(iter);
    row[_mColumns._colSelector] = selector;
    row[_mColumns._colSelectorPos] = pos;
    row[_mColumns._colActive] = true;
    row[_mColumns._colName] =  "";
    row[_mColumns._colValue] = "";
    row[_mColumns._colStrike] = false;
    gint col = 2;
    if (pos < 2 ){
        col = 1;
    }
    css_tree->set_cursor(path, *(css_tree->get_column(col)), true);
    grab_focus();
    return false;
}

/**
 * @brief StyleDialog::nameEdited
 * @param event
 * @return
 * Called when the name is edited in the TreeView editable column
 */
void StyleDialog::_nameEdited (const Glib::ustring& path, const Glib::ustring& name, Glib::RefPtr<Gtk::TreeStore> store, Gtk::TreeView *css_tree)
{
    Gtk::TreeModel::Row row = *store->get_iter(path);
    Gtk::TreeModel::Path pathel = (Gtk::TreeModel::Path)*store->get_iter(path);

    if(row) {
        gint pos = row[_mColumns._colSelectorPos];
        bool write = false;
        if (row[_mColumns._colName] != name && row[_mColumns._colValue] != "") {
            write = true;
        }
        Glib::ustring value = row[_mColumns._colValue];
        Glib::ustring selector = row[_mColumns._colSelector];
        row[_mColumns._colName] = name;
        if(name.empty() && value.empty()) {
            store->erase(row);
        }
        gint col = 3;
        if (pos < 2 ){
            col = 2;
        }
        if (write) {
            _writeStyleElement(store, selector);
        } else {
            css_tree->set_cursor(pathel, *(css_tree->get_column(col)), true);
            grab_focus();
        }
    }
}


/**
 * @brief StyleDialog::valueEdited
 * @param event
 * @return
 * Called when the value is edited in the TreeView editable column
 */
void StyleDialog::_valueEdited(const Glib::ustring& path, const Glib::ustring& value, Glib::RefPtr<Gtk::TreeStore> store)
{
    Gtk::TreeModel::Row row = *store->get_iter(path);
    if(row) {
        row[_mColumns._colValue] = value;
        Glib::ustring selector = row[_mColumns._colSelector];
        Glib::ustring name = row[_mColumns._colName];
        if(name.empty() && value.empty()) {
            store->erase(row);
        }
        _writeStyleElement(store, selector);
    }
}

void StyleDialog::_activeToggled(const Glib::ustring& path, Glib::RefPtr<Gtk::TreeStore> store)
{
    Gtk::TreeModel::Row row = *store->get_iter(path);
    if(row) {
        row[_mColumns._colActive] = !row[_mColumns._colActive];
        Glib::ustring selector = row[_mColumns._colSelector];
        _writeStyleElement(store, selector);
    }
}

void StyleDialog::_addWatcherRecursive(Inkscape::XML::Node *node) {

    g_debug("StyleDialog::_addWatcherRecursive()");

    StyleDialog::NodeWatcher *w = new StyleDialog::NodeWatcher(this, node);
    node->addObserver(*w);
    _nodeWatchers.push_back(w);

    for (unsigned i = 0; i < node->childCount(); ++i) {
        _addWatcherRecursive(node->nthChild(i));
    }
}

/**
 * Update the watchers on objects.
 */
void StyleDialog::_updateWatchers()
{
    _updating = true;

    // Remove old document watchers
    while (!_nodeWatchers.empty()) {
        StyleDialog::NodeWatcher *w = _nodeWatchers.back();
        w->_repr->removeObserver(*w);
        _nodeWatchers.pop_back();
        delete w;
    }

    // Recursively add new watchers
    Inkscape::XML::Node *root = SP_ACTIVE_DOCUMENT->getReprRoot();
    _addWatcherRecursive(root);

    g_debug("StyleDialog::_updateWatchers(): %d", (int)_nodeWatchers.size());

    _updating = false;
}


/**
 * @param selector: a valid CSS selector string.
 * @return objVec: a vector of pointers to SPObject's the selector matches.
 * Return a vector of all objects that selector matches.
 */
std::vector<SPObject *> StyleDialog::_getObjVec(Glib::ustring selector) {

    std::vector<SPObject *> objVec = SP_ACTIVE_DOCUMENT->getObjectsBySelector( selector );

    g_debug("StyleDialog::_getObjVec: | %s |", selector.c_str());
    for (auto& obj: objVec) {
        g_debug("  %s", obj->getId() ? obj->getId() : "null");
    }

    return objVec;
}

void StyleDialog::_closeDialog(Gtk::Dialog *textDialogPtr) { textDialogPtr->response(Gtk::RESPONSE_OK); }


/**
 * Handle document replaced. (Happens when a default document is immediately replaced by another
 * document in a new window.)
 */
void
StyleDialog::_handleDocumentReplaced(SPDesktop *desktop, SPDocument * /* document */)
{
    g_debug("StyleDialog::handleDocumentReplaced()");

    _selection_changed_connection.disconnect();

    _selection_changed_connection = desktop->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &StyleDialog::_handleSelectionChanged)));

    _updateWatchers();
    _readStyleElement();
}


/*
 * When a dialog is floating, it is connected to the active desktop.
 */
void
StyleDialog::_handleDesktopChanged(SPDesktop* desktop) {
    g_debug("StyleDialog::handleDesktopReplaced()");

    if (getDesktop() == desktop) {
        // This will happen after construction of dialog. We've already
        // set up signals so just return.
        return;
    }

    _selection_changed_connection.disconnect();
    _document_replaced_connection.disconnect();

    setDesktop( desktop );

    _selection_changed_connection = desktop->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &StyleDialog::_handleSelectionChanged)));
    _document_replaced_connection = desktop->connectDocumentReplaced(
        sigc::mem_fun(this, &StyleDialog::_handleDocumentReplaced));

    _updateWatchers();
    _readStyleElement();
}


/*
 * Handle a change in which objects are selected in a document.
 */
void
StyleDialog::_handleSelectionChanged() {
    g_debug("StyleDialog::_handleSelectionChanged()");
    _readStyleElement();
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
