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

#include <gtkmm/builder.h>
#include <glibmm/i18n.h>
#include <glibmm/regex.h>

#include <map>
#include <utility>

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
    _styledialog->_filterRow();
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
    _filterRow();
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
    _filterRow();
}

void
StyleDialog::_nodeChanged( Inkscape::XML::Node &object ) {

    _readStyleElement();
    _filterRow();
}

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
    _filterRow();
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

void StyleDialog::_hideRootToggle( Gtk::CellRenderer* renderer, const Gtk::TreeModel::iterator& iter)
{
    //Get the value from the model and show it appropriately in the view:
    Gtk::CellRendererToggle* toggle = dynamic_cast<Gtk::CellRendererToggle*>(renderer);
    Gtk::TreeModel::Row row = *iter;
    Gtk::TreeModel::iterator parent = row->parent();
    if (parent) {
        toggle->set_visible(true);
    } else {
        toggle->set_visible(false);
    }
}

/**
 * Fill the Gtk::TreeStore from the svg:style element.
 */
void StyleDialog::_readStyleElement()
{
    g_debug("StyleDialog::_readStyleElement: updating %s", (_updating ? "true" : "false"));

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
    if (tokens.size() == 0) {
        _updating = false;
        return;
    }

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
    Gtk::Box *CSSSelectorContainer;
    _builder->get_widget("CSSSelectorContainer", CSSSelectorContainer);
    Gtk::Label *CSSSelector;
    _builder->get_widget("CSSSelector", CSSSelector);
    Gtk::Label *CSSSelectorAdd;
    _builder->get_widget("CSSSelectorAdd", CSSSelectorAdd);
    Gtk::Label *CSSSelectorFilled;
    _builder->get_widget("CSSSelectorFilled", CSSSelectorFilled);
    CSSSelector->set_text("element");
    Gtk::TreeView *CSSTree;
    _builder->get_widget("CSSTree", CSSTree);
    Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(_mColumns);
    CSSTree->set_model(store);
    Gtk::CellRendererToggle *active = Gtk::manage(new Gtk::CellRendererToggle);
    int addCol = CSSTree->append_column("", *active) - 1;
    Gtk::TreeViewColumn *col = CSSTree->get_column(addCol);
    if (col) {
        col->add_attribute(active->property_active(), _mColumns._colActive);
    }
    //col->set_cell_data_func(*active, sigc::mem_fun(*this, &StyleDialog::_hideRootToggle));
    CSSTree->set_headers_visible(false);
    Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
    CSSTree->set_reorderable(false);
    label->property_editable() = true;
    addCol = CSSTree->append_column("CSS Selector", *label) - 1;
    col = CSSTree->get_column(addCol);
    if (col) {
        col->add_attribute(label->property_text(), _mColumns._colLabel);
    }
    Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
    CSSTree->set_reorderable(false);
    value->property_editable() = true;
    addCol = CSSTree->append_column("CSS Selector", *value) - 1;
    col = CSSTree->get_column(addCol);
    if (col) {
        col->add_attribute(value->property_text(), _mColumns._colValue);
        col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
    }
    bool contract = true;
    std::map<Glib::ustring, Glib::ustring> attr_prop;
    if (obj) {
        Glib::ustring style = obj->getRepr()->attribute("style");
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
        attr_prop = parseStyle(style);
        for (auto iter : obj->style->properties()) {
            if (attr_prop.count(iter->name)) {
                Gtk::TreeModel::Row row = *(store->append());
                row[_mColumns._colActive] = true;
                row[_mColumns._colSelector] = "style_property";
                row[_mColumns._colLabel] = iter->name;
                row[_mColumns._colValue] = iter->get_value();
                row[_mColumns._colStrike] = false;
                contract = false;
            }
        }
        std::map<Glib::ustring, Glib::ustring> attr_prop_comments = parseStyle(comments);
        for (auto iter : attr_prop_comments) {
            if (!attr_prop.count(iter.first)) {
                Gtk::TreeModel::Row row = *(store->append());
                row[_mColumns._colActive] = false;
                row[_mColumns._colSelector] = "style_property";
                row[_mColumns._colLabel] = iter.first;
                row[_mColumns._colValue] = iter.second;
                row[_mColumns._colStrike] = true;
                contract = false;
            }
        }
        if (contract) {
            CSSSelectorAdd->show();
            CSSSelectorFilled->hide();
        } else {
            CSSSelectorAdd->hide();
            CSSSelectorFilled->show(); 
        }
        _styleBox.pack_start(*CSSSelectorContainer, Gtk::PACK_EXPAND_WIDGET);
        bool hasattributes = false;
        contract = true;
        Glib::RefPtr<Gtk::Builder> _builder;
        try {
            _builder = Gtk::Builder::create_from_file(gladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for filter effect dialog");
            return;
        }
        Gtk::Box *CSSSelectorContainer;
        _builder->get_widget("CSSSelectorContainer", CSSSelectorContainer);
        Gtk::Label *CSSSelectorAdd;
        _builder->get_widget("CSSSelectorAdd", CSSSelectorAdd);
        Gtk::Label *CSSSelectorFilled;
        _builder->get_widget("CSSSelectorFilled", CSSSelectorFilled);
        Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(_mColumns);
        for (auto iter : obj->style->properties()) {
            if (iter->style_src != SP_STYLE_SRC_UNSET) {
                if( iter->name != "font" && iter->name != "marker") {
                    const gchar *attr = obj->getRepr()->attribute(iter->name.c_str());
                    if (attr) {
                        if (!hasattributes) {
                            Gtk::Label *CSSSelector;
                            _builder->get_widget("CSSSelector", CSSSelector);
                            CSSSelector->set_text("element.attributes");
                            Gtk::TreeView *CSSTree;
                            _builder->get_widget("CSSTree", CSSTree);
                            CSSTree->set_model(store);
                            Gtk::CellRendererToggle *active = Gtk::manage(new Gtk::CellRendererToggle);
                            int addCol = CSSTree->append_column("", *active) - 1;
                            Gtk::TreeViewColumn *col = CSSTree->get_column(addCol);
                            if (col) {
                                col->add_attribute(active->property_active(), _mColumns._colActive);
                            }
                            //col->set_cell_data_func(*active, sigc::mem_fun(*this, &StyleDialog::_hideRootToggle));
                            CSSTree->set_headers_visible(false);
                            Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
                            CSSTree->set_reorderable(false);
                            label->property_editable() = true;
                            addCol = CSSTree->append_column("CSS Selector", *label) - 1;
                            col = CSSTree->get_column(addCol);
                            if (col) {
                                col->add_attribute(label->property_text(), _mColumns._colLabel);
                            }
                            Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
                            CSSTree->set_reorderable(false);
                            value->property_editable() = true;
                            addCol = CSSTree->append_column("CSS Selector", *value) - 1;
                            col = CSSTree->get_column(addCol);
                            if (col) {
                                col->add_attribute(value->property_text(), _mColumns._colValue);
                                col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
                            }
                        }
                        Gtk::TreeModel::Row row = *(store->append());
                        row[_mColumns._colActive] = true;
                        row[_mColumns._colSelector] = "attribute";
                        row[_mColumns._colLabel] = iter->name;
                        row[_mColumns._colValue] = attr;
                        if (attr_prop.count(iter->name)) {
                            row[_mColumns._colStrike] = true;
                        } else {
                            row[_mColumns._colStrike] = false;
                        }
                        contract = false;
                        hasattributes = true;
                    }
                }
            }
        }
        if (contract) {
            CSSSelectorAdd->show();
            CSSSelectorFilled->hide();
        } else {
            CSSSelectorAdd->hide();
            CSSSelectorFilled->show(); 
        }
        _styleBox.pack_start(*CSSSelectorContainer, Gtk::PACK_EXPAND_WIDGET);
    }
    if (obj) {
        for (unsigned i = 0; i < tokens.size()-1; i += 2) {
            Glib::ustring selector = tokens[i];
            REMOVE_SPACES(selector); // Remove leading/trailing spaces
            std::vector<Glib::ustring> tokensplus = Glib::Regex::split_simple("[,]+", selector);
            for (auto tok : tokensplus) {
                REMOVE_SPACES(tok);
    /*             if (tok.find(" ") != -1 || tok.erase(0, 1).find(".") != -1) {
                    colType = UNHANDLED;
                } */
            }
            // Get list of objects selector matches
            std::vector<SPObject *> objVec = _getObjVec( selector );

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
            Gtk::Box *CSSSelectorContainer;
            _builder->get_widget("CSSSelectorContainer", CSSSelectorContainer);
            Gtk::Label *CSSSelector;
            _builder->get_widget("CSSSelector", CSSSelector);
            Gtk::Label *CSSSelectorAdd;
            _builder->get_widget("CSSSelectorAdd", CSSSelectorAdd);
            Gtk::Label *CSSSelectorFilled;
            _builder->get_widget("CSSSelectorFilled", CSSSelectorFilled);
            CSSSelector->set_text(selector);
            Gtk::TreeView *CSSTree;
            _builder->get_widget("CSSTree", CSSTree);
            Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(_mColumns);
            CSSTree->set_model(store);
            Gtk::CellRendererToggle *active = Gtk::manage(new Gtk::CellRendererToggle);
            int addCol = CSSTree->append_column("", *active) - 1;
            Gtk::TreeViewColumn *col = CSSTree->get_column(addCol);
            if (col) {
                col->add_attribute(active->property_active(), _mColumns._colActive);
            }
            //col->set_cell_data_func(*active, sigc::mem_fun(*this, &StyleDialog::_hideRootToggle));
            CSSTree->set_headers_visible(false);
            Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
            CSSTree->set_reorderable(false);
            label->property_editable() = true;
            addCol = CSSTree->append_column("CSS Selector", *label) - 1;
            col = CSSTree->get_column(addCol);
            if (col) {
                col->add_attribute(label->property_text(), _mColumns._colLabel);
            }
            Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
            CSSTree->set_reorderable(false);
            value->property_editable() = true;
            addCol = CSSTree->append_column("CSS Selector", *value) - 1;
            col = CSSTree->get_column(addCol);
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
            contract = true;
            for (auto iter : obj->style->properties()) {
                if (iter->style_src != SP_STYLE_SRC_UNSET) {
                    if (attr_prop_styleshet.count(iter->name)) {
                        Gtk::TreeModel::Row row = *(store->append());
                        row[_mColumns._colActive] = true;
                        row[_mColumns._colSelector] = selector;
                        row[_mColumns._colLabel] = iter->name;
                        row[_mColumns._colValue] = attr_prop_styleshet[iter->name];
                        if (attr_prop.count(iter->name) || row[_mColumns._colValue] != iter->get_value()) {
                            row[_mColumns._colStrike] = true;
                        } else {
                            row[_mColumns._colStrike] = false;
                        }
                        contract = false;
                    }
                }
            }
            std::map<Glib::ustring, Glib::ustring> attr_prop_styleshet_comments = parseStyle(comments);
            
            for (auto iter : attr_prop_styleshet_comments) {
                if (!attr_prop_styleshet.count(iter.first)) {
                    Gtk::TreeModel::Row row = *(store->append());
                    row[_mColumns._colActive] = false;
                    row[_mColumns._colSelector] = selector;
                    row[_mColumns._colLabel] = iter.first;
                    row[_mColumns._colValue] = iter.second;
                    row[_mColumns._colStrike] = true;
                    contract = false;
                }
            }
            if (contract) {
                CSSSelectorAdd->show();
                CSSSelectorFilled->hide();
            } else {
                CSSSelectorAdd->hide();
                CSSSelectorFilled->show(); 
            }
            _styleBox.pack_start(*CSSSelectorContainer, Gtk::PACK_EXPAND_WIDGET);
        }
    }
    _mainBox.show_all_children();
    _updating = false;
}

/**
 * @brief StyleDialog::parseStyle
 *
 * Convert a style string into a vector map. This should be moved to style.cpp
 *
 */
std::map<Glib::ustring, Glib::ustring> StyleDialog::parseStyle(Glib::ustring style_string)
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
void StyleDialog::_writeStyleElement()
{
    if (_updating) {
        return;
    }
    _updating = true;

    Glib::ustring styleContent;
/*     for (auto& row: _store->children()) {
        Glib::ustring selector = row[_mColumns._colData];
        if (!row[_mColumns._colExpand]) {
            selector = selector.erase(selector.size()-1);
        } */
        /*
                REMOVE_SPACES(selector);
        /*         size_t len = selector.size();
                if(selector[len-1] == ','){
                    selector.erase(len-1);
                }
                row[_mColumns._colData] =  selector; */
      /*   styleContent = styleContent + selector + " { " + row[_mColumns._colProperties] + " }\n";
    } */
    // We could test if styleContent is empty and then delete the style node here but there is no
    // harm in keeping it around ...

    Inkscape::XML::Node *textNode = _getStyleTextNode();
    textNode->setContent(styleContent.c_str());

    DocumentUndo::done(SP_ACTIVE_DOCUMENT, SP_VERB_DIALOG_STYLE, _("Edited style element."));

    _updating = false;
    g_debug("StyleDialog::_writeStyleElement(): | %s |", styleContent.c_str());
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
 * @param sel
 * @return This function returns a comma separated list of ids for objects in input vector.
 * It is used in creating an 'id' selector. It relies on objects having 'id's.
 */
Glib::ustring StyleDialog::_getIdList(std::vector<SPObject*> sel)
{
    Glib::ustring str;
    for (auto& obj: sel) {
        str += "#" + Glib::ustring(obj->getId()) + ", ";
    }
    if (!str.empty()) {
        str.erase(str.size()-1); // Remove space at end. c++11 has pop_back() but not ustring.
        str.erase(str.size()-1); // Remove comma at end.
    }
    return str;
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

// -------------------------------------------------------------------

class PropertyData
{
public:
    PropertyData() = default;;
    PropertyData(Glib::ustring name) : _name(std::move(name)) {};

    void _setSheetValue(Glib::ustring value) { _sheetValue = value; };
    void _setAttrValue(Glib::ustring value)  { _attrValue  = value; };
    Glib::ustring _getName()       { return _name;       };
    Glib::ustring _getSheetValue() { return _sheetValue; };
    Glib::ustring _getAttrValue()  { return _attrValue;  };

private:
    Glib::ustring _name;
    Glib::ustring _sheetValue;
    Glib::ustring _attrValue;
};

// -------------------------------------------------------------------


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
    _filterRow();
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
    _filterRow();
}


/*
 * Handle a change in which objects are selected in a document.
 */
void
StyleDialog::_handleSelectionChanged() {
    g_debug("StyleDialog::_handleSelectionChanged()");
    _readStyleElement();
    _filterRow();
}

/**
 * This function selects the row in treeview corresponding to an object selected
 * in the drawing. If more than one row matches, the first is chosen.
 */
void StyleDialog::_filterRow()
{
    g_debug("StyleDialog::_selectRow: updating: %s", (_updating ? "true" : "false"));
    if (_updating || !getDesktop()) return; // Avoid updating if we have set row via dialog.
    if (SP_ACTIVE_DESKTOP != getDesktop()) {
        std::cerr << "StyleDialog::_selectRow: SP_ACTIVE_DESKTOP != getDesktop()" << std::endl;
        return;
    }
    //Gtk::TreeModel::Children children = _store->children();
    Inkscape::Selection* selection = getDesktop()->getSelection();
    SPObject *obj = nullptr;
    if(selection->objects().size() == 1) {
        obj = selection->objects().back();
    }

    for (auto *child : _styleBox.get_children()) {
        Gtk::Box *childbox = dynamic_cast<Gtk::Box *>(child);
        std::vector<Gtk::Widget *> selectorwrapper = childbox->get_children();
        Gtk::Box *childwrapperbox = dynamic_cast<Gtk::Box *>(selectorwrapper[0]);
        std::vector<Gtk::Widget *> childselectorwrapper = childwrapperbox->get_children();
        Gtk::Label *selectorlabel = dynamic_cast<Gtk::Label *>(childselectorwrapper[0]);
        Glib::ustring selector = selectorlabel->get_text();
        if (selector == "element"){
            child->show();
            childbox->show_all_children();
            continue;
        }
        if (selector.empty()) {
            return;
        }
        std::vector<SPObject *> objVec = _getObjVec( selector );
        if (obj) {
            for (auto & i : objVec) {
                if (obj->getId() == i->getId()) {
                    child->show();
                    childbox->show_all_children();
                    break;
                } else {
                    child->hide();
                }
            }
        } else {
            child->hide();
        }
    }
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
