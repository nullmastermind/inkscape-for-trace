// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for CSS styles
 */
/* Authors:
 *   Kamalpreet Kaur Grewal
 *   Tavmjong Bah
 *   Jabiertxof
 *
 * Copyright (C) Kamalpreet Kaur Grewal 2016 <grewalkamal005@gmail.com>
 * Copyright (C) Tavmjong Bah 2017 <tavmjong@free.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "styledialog.h"
#include "attribute-rel-svg.h"
#include "attributes.h"
#include "document-undo.h"
#include "inkscape.h"
#include "io/resource.h"
#include "selection.h"
#include "style-internal.h"
#include "style.h"
#include "svg/svg-color.h"
#include "ui/icon-loader.h"
#include "ui/widget/iconrenderer.h"
#include "verbs.h"
#include "xml/attribute-record.h"
#include "xml/node-observer.h"
#include "xml/sp-css-attr.h"

#include <map>
#include <regex>
#include <utility>

#include <gdk/gdkkeysyms.h>
#include <glibmm/i18n.h>

// G_MESSAGES_DEBUG=DEBUG_STYLEDIALOG  gdb ./inkscape
// #define DEBUG_STYLEDIALOG
// #define G_LOG_DOMAIN "STYLEDIALOG"

using Inkscape::DocumentUndo;

/**
 * This macro is used to remove spaces around selectors or any strings when
 * parsing is done to update XML style element or row labels in this dialog.
 */
#define REMOVE_SPACES(x)                                                                                               \
    x.erase(0, x.find_first_not_of(' '));                                                                              \
    x.erase(x.find_last_not_of(' ') + 1);

namespace Inkscape {

/**
 * Get the first <style> element's first text node. If no such node exists and
 * `create_if_missing` is false, then return NULL.
 *
 * Only finds <style> elements in root or in root-level <defs>.
 */
XML::Node *get_first_style_text_node(XML::Node *root, bool create_if_missing)
{
    static GQuark const CODE_svg_style = g_quark_from_static_string("svg:style");
    static GQuark const CODE_svg_defs = g_quark_from_static_string("svg:defs");

    XML::Node *styleNode = nullptr;
    XML::Node *textNode = nullptr;

    if (!root) {
        return nullptr;
    }

    for (auto *node = root->firstChild(); node; node = node->next()) {
        if (node->code() == CODE_svg_defs) {
            textNode = get_first_style_text_node(node, false);
            if (textNode != nullptr) {
                return textNode;
            }
        }

        if (node->code() == CODE_svg_style) {
            styleNode = node;
            break;
        }
    }

    if (styleNode == nullptr) {
        if (!create_if_missing)
            return nullptr;

        styleNode = root->document()->createElement("svg:style");
        root->addChild(styleNode, nullptr);
        Inkscape::GC::release(styleNode);
    }

    for (auto *node = styleNode->firstChild(); node; node = node->next()) {
        if (node->type() == XML::NodeType::TEXT_NODE) {
            textNode = node;
            break;
        }
    }

    if (textNode == nullptr) {
        if (!create_if_missing)
            return nullptr;

        textNode = root->document()->createTextNode("");
        styleNode->appendChild(textNode);
        Inkscape::GC::release(textNode);
    }

    return textNode;
}

namespace UI {
namespace Dialog {

// Keeps a watch on style element
class StyleDialog::NodeObserver : public Inkscape::XML::NodeObserver {
  public:
    NodeObserver(StyleDialog *styledialog)
        : _styledialog(styledialog)
    {
        g_debug("StyleDialog::NodeObserver: Constructor");
    };

    void notifyContentChanged(Inkscape::XML::Node &node, Inkscape::Util::ptr_shared old_content,
                              Inkscape::Util::ptr_shared new_content) override;

    StyleDialog *_styledialog;
};


void StyleDialog::NodeObserver::notifyContentChanged(Inkscape::XML::Node & /*node*/,
                                                     Inkscape::Util::ptr_shared /*old_content*/,
                                                     Inkscape::Util::ptr_shared /*new_content*/)
{

    g_debug("StyleDialog::NodeObserver::notifyContentChanged");
    _styledialog->_updating = false;
    _styledialog->readStyleElement();
}


// Keeps a watch for new/removed/changed nodes
// (Must update objects that selectors match.)
class StyleDialog::NodeWatcher : public Inkscape::XML::NodeObserver {
  public:
    NodeWatcher(StyleDialog *styledialog)
        : _styledialog(styledialog)
    {
        g_debug("StyleDialog::NodeWatcher: Constructor");
    };

    void notifyChildAdded(Inkscape::XML::Node & /*node*/, Inkscape::XML::Node &child,
                          Inkscape::XML::Node * /*prev*/) override
    {
            _styledialog->_nodeAdded(child);
    }

    void notifyChildRemoved(Inkscape::XML::Node & /*node*/, Inkscape::XML::Node &child,
                            Inkscape::XML::Node * /*prev*/) override
    {
            _styledialog->_nodeRemoved(child);
    }
    /*     void notifyContentChanged(Inkscape::XML::Node &node,
                                          Inkscape::Util::ptr_shared old_content,
                                          Inkscape::Util::ptr_shared new_content) override{
            if ( _styledialog && _repr && _textNode == node) {
                _styledialog->_stylesheetChanged( node );
            }
        };
     */
    void notifyAttributeChanged(Inkscape::XML::Node &node, GQuark qname, Util::ptr_shared /*old_value*/,
                                Util::ptr_shared /*new_value*/) override
    {
        static GQuark const CODE_id = g_quark_from_static_string("id");
        static GQuark const CODE_class = g_quark_from_static_string("class");
        static GQuark const CODE_style = g_quark_from_static_string("style");

        if (qname == CODE_id || qname == CODE_class || qname == CODE_style) {
            _styledialog->_nodeChanged(node);
        }
    }

    StyleDialog *_styledialog;
};

void StyleDialog::_nodeAdded(Inkscape::XML::Node &node)
{
    readStyleElement();
}

void StyleDialog::_nodeRemoved(Inkscape::XML::Node &repr)
{
    if (_textNode == &repr) {
        _textNode = nullptr;
    }

    readStyleElement();
}

void StyleDialog::_nodeChanged(Inkscape::XML::Node &object)
{
    g_debug("StyleDialog::_nodeChanged");
    readStyleElement();
}

/* void
StyleDialog::_stylesheetChanged( Inkscape::XML::Node &repr ) {
    std::cout << "Style tag modified" << std::endl;
    readStyleElement();
} */

/**
 * Constructor
 * A treeview and a set of two buttons are added to the dialog. _addSelector
 * adds selectors to treeview. _delSelector deletes the selector from the dialog.
 * Any addition/deletion of the selectors updates XML style element accordingly.
 */
StyleDialog::StyleDialog()
    : DialogBase("/dialogs/style", SP_VERB_DIALOG_STYLE)
    , _updating(false)
    , _textNode(nullptr)
    , _scroolpos(0)
    , _deleted_pos(0)
    , _deletion(false)
{
    g_debug("StyleDialog::StyleDialog");

    m_nodewatcher.reset(new StyleDialog::NodeWatcher(this));
    m_styletextwatcher.reset(new StyleDialog::NodeObserver(this));

    // Pack widgets
    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _styleBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    _styleBox.set_valign(Gtk::ALIGN_START);
    _scrolledWindow.add(_styleBox);
    _vadj = _scrolledWindow.get_vadjustment();
    _vadj->signal_value_changed().connect(sigc::mem_fun(*this, &StyleDialog::_vscrool));
    _mainBox.set_orientation(Gtk::ORIENTATION_VERTICAL);

    pack_start(_mainBox, Gtk::PACK_EXPAND_WIDGET);
}

void StyleDialog::_vscrool()
{
    if (!_scroollock) {
        _scroolpos = _vadj->get_value();
    } else {
        _vadj->set_value(_scroolpos);
        _scroollock = false;
    }
}

Glib::ustring StyleDialog::fixCSSSelectors(Glib::ustring selector)
{
    g_debug("SelectorsDialog::fixCSSSelectors");
    REMOVE_SPACES(selector);
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[,]+", selector);
    Glib::ustring my_selector = selector + " {"; // Parsing fails sometimes without '{'. Fix me
    CRSelector *cr_selector = cr_selector_parse_from_buf((guchar *)my_selector.c_str(), CR_UTF_8);
    for (auto token : tokens) {
        REMOVE_SPACES(token);
        std::vector<Glib::ustring> subtokens = Glib::Regex::split_simple("[ ]+", token);
        for (auto subtoken : subtokens) {
            REMOVE_SPACES(subtoken);
            Glib::ustring my_selector = subtoken + " {"; // Parsing fails sometimes without '{'. Fix me
            CRSelector *cr_selector = cr_selector_parse_from_buf((guchar *)my_selector.c_str(), CR_UTF_8);
            gchar *selectorchar = reinterpret_cast<gchar *>(cr_selector_to_string(cr_selector));
            if (selectorchar) {
                Glib::ustring toadd = Glib::ustring(selectorchar);
                g_free(selectorchar);
                if (toadd[0] != '.' && toadd[0] != '#' && toadd.size() > 1) {
                    auto i = std::min(toadd.find("#"), toadd.find("."));
                    Glib::ustring tag = toadd;
                    if (i != std::string::npos) {
                        tag = tag.substr(0, i);
                    }
                    if (!SPAttributeRelSVG::isSVGElement(tag)) {
                        if (tokens.size() == 1) {
                            tag = "." + tag;
                            return tag;
                        } else {
                            return "";
                        }
                    }
                }
            }
        }
    }
    if (cr_selector) {
        return selector;
    }
    return "";
}

/**
 * Class destructor
 */
StyleDialog::~StyleDialog()
{
    g_debug("StyleDialog::~StyleDialog");
    _document_replaced_connection.disconnect();
    _selection_changed_connection.disconnect();
}

void StyleDialog::_reload() { readStyleElement(); }

/**
 * @return Inkscape::XML::Node* pointing to a style element's text node.
 * Returns the style element's text node. If there is no style element, one is created.
 * Ditto for text node.
 */
Inkscape::XML::Node *StyleDialog::_getStyleTextNode(bool create_if_missing)
{
    g_debug("StyleDialog::_getStyleTextNoded");

    auto textNode = Inkscape::get_first_style_text_node(m_root, create_if_missing);

    if (_textNode != textNode) {
        if (_textNode) {
            _textNode->removeObserver(*m_styletextwatcher);
        }

        _textNode = textNode;

        if (_textNode) {
            _textNode->addObserver(*m_styletextwatcher);
        }
    }

    return textNode;
}


Glib::RefPtr<Gtk::TreeModel> StyleDialog::_selectTree(Glib::ustring selector)
{
    g_debug("StyleDialog::_selectTree");

    Gtk::Label *selectorlabel;
    Glib::RefPtr<Gtk::TreeModel> model;
    for (auto fullstyle : _styleBox.get_children()) {
        Gtk::Box *style = dynamic_cast<Gtk::Box *>(fullstyle);
        for (auto stylepart : style->get_children()) {
            switch (style->child_property_position(*stylepart)) {
                case 0: {
                    Gtk::Box *selectorbox = dynamic_cast<Gtk::Box *>(stylepart);
                    for (auto styleheader : selectorbox->get_children()) {
                        if (!selectorbox->child_property_position(*styleheader)) {
                            selectorlabel = dynamic_cast<Gtk::Label *>(styleheader);
                        }
                    }
                    break;
                }
                case 1: {
                    Glib::ustring wdg_selector = selectorlabel->get_text();
                    if (wdg_selector == selector) {
                        Gtk::TreeView *treeview = dynamic_cast<Gtk::TreeView *>(stylepart);
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

void StyleDialog::setCurrentSelector(Glib::ustring current_selector)
{
    g_debug("StyleDialog::setCurrentSelector");
    _current_selector = current_selector;
    readStyleElement();
}

// copied from style.cpp:1499
static bool is_url(char const *p)
{
    if (p == nullptr)
        return false;
    /** \todo
     * FIXME: I'm not sure if this applies to SVG as well, but CSS2 says any URIs
     * in property values must start with 'url('.
     */
    return (g_ascii_strncasecmp(p, "url(", 4) == 0);
}

/**
 * Fill the Gtk::TreeStore from the svg:style element.
 */
void StyleDialog::readStyleElement()
{
    g_debug("StyleDialog::readStyleElement");

    if (_updating)
        return; // Don't read if we wrote style element.
    _updating = true;
    _scroollock = true;
    Inkscape::XML::Node *textNode = _getStyleTextNode();
    SPDocument *document = SP_ACTIVE_DOCUMENT;

    // Get content from style text node.
    std::string content = (textNode && textNode->content()) ? textNode->content() : "";

    // Remove end-of-lines (check it works on Windoze).
    content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());

    // Remove comments (/* xxx */)

    bool breakme = false;
    size_t start = content.find("/*");
    size_t open = content.find("{", start + 1);
    size_t close = content.find("}", start + 1);
    size_t end = content.find("*/", close + 1);
    while (!breakme) {
        if (open == std::string::npos || close == std::string::npos || end == std::string::npos) {
            breakme = true;
            break;
        }
        while (open < close) {
            open = content.find("{", close + 1);
            close = content.find("}", close + 1);
            end = content.find("*/", close + 1);
            size_t reopen = content.find("{", close + 1);
            if (open == std::string::npos || end == std::string::npos || end < reopen) {
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
    _owner_style.clear();
    // If text node is empty, return (avoids problem with negative below).

    for (auto child : _styleBox.get_children()) {
        _styleBox.remove(*child);
        delete child;
    }
    Inkscape::Selection *selection = _desktop->getSelection();
    SPObject *obj = nullptr;
    if (selection->objects().size() == 1) {
        obj = selection->objects().back();
    }
    if (!obj) {
        obj = _desktop->getDocument()->getXMLDialogSelectedObject();
        if (obj && !obj->getRepr()) {
            obj = nullptr; // treat detached object as no selection
        }
    }

    auto gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "dialog-css.glade");
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
    css_tree->get_style_context()->add_class("style_element");
    Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(_mColumns);
    css_tree->set_model(store);
    css_selector_event_add->signal_button_release_event().connect(
        sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *, Glib::ustring, gint>(
            sigc::mem_fun(*this, &StyleDialog::_addRow), store, css_tree, "style_properties", selectorpos));
    Inkscape::UI::Widget::IconRenderer *addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
    addRenderer->add_icon("edit-delete");
    int addCol = css_tree->append_column(" ", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = css_tree->get_column(addCol);
    if (col) {
        addRenderer->signal_activated().connect(
            sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(sigc::mem_fun(*this, &StyleDialog::_onPropDelete), store));
    }
    Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
    label->property_placeholder_text() = _("property");
    label->property_editable() = true;
    label->signal_edited().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *>(
        sigc::mem_fun(*this, &StyleDialog::_nameEdited), store, css_tree));
    label->signal_editing_started().connect(sigc::mem_fun(*this, &StyleDialog::_startNameEdit));
    addCol = css_tree->append_column(" ", *label) - 1;
    col = css_tree->get_column(addCol);
    if (col) {
        col->set_resizable(true);
        col->add_attribute(label->property_text(), _mColumns._colName);
    }
    Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
    value->property_placeholder_text() = _("value");
    value->property_editable() = true;
    value->signal_edited().connect(
        sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(sigc::mem_fun(*this, &StyleDialog::_valueEdited), store));
    value->signal_editing_started().connect(
        sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(sigc::mem_fun(*this, &StyleDialog::_startValueEdit), store));
    addCol = css_tree->append_column(" ", *value) - 1;
    col = css_tree->get_column(addCol);
    if (col) {
        col->add_attribute(value->property_text(), _mColumns._colValue);
        col->set_expand(true);
        col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
    }
    Inkscape::UI::Widget::IconRenderer *urlRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
    urlRenderer->add_icon("empty-icon");
    urlRenderer->add_icon("edit-redo");
    int urlCol = css_tree->append_column(" ", *urlRenderer) - 1;
    Gtk::TreeViewColumn *urlcol = css_tree->get_column(urlCol);
    if (urlcol) {
        urlcol->set_min_width(40);
        urlcol->set_max_width(40);
        urlRenderer->signal_activated().connect(sigc::bind(sigc::mem_fun(*this, &StyleDialog::_onLinkObj), store));
        urlcol->add_attribute(urlRenderer->property_icon(), _mColumns._colLinked);
    }
    std::map<Glib::ustring, Glib::ustring> attr_prop;
    Gtk::TreeModel::Path path;
    bool empty = true;
    if (obj && obj->getRepr()->attribute("style")) {
        Glib::ustring style = obj->getRepr()->attribute("style");
        attr_prop = parseStyle(style);
        for (auto iter : obj->style->properties()) {
            if (attr_prop.count(iter->name())) {
                auto value = attr_prop[iter->name()];
                empty = false;
                Gtk::TreeModel::Row row = *(store->prepend());
                row[_mColumns._colSelector] = "style_properties";
                row[_mColumns._colSelectorPos] = 0;
                row[_mColumns._colActive] = true;
                row[_mColumns._colName] = iter->name();
                row[_mColumns._colValue] = value;
                row[_mColumns._colStrike] = false;
                row[_mColumns._colOwner] = Glib::ustring("Current value");
                row[_mColumns._colHref] = nullptr;
                row[_mColumns._colLinked] = false;
                if (is_url(value.c_str())) {
                    auto id = value.substr(5, value.size() - 6);
                    SPObject *elemref = nullptr;
                    if ((elemref = document->getObjectById(id.c_str()))) {
                        row[_mColumns._colHref] = elemref;
                        row[_mColumns._colLinked] = true;
                    }
                }
                _addOwnerStyle(iter->name(), "style attribute");
            }
        }
        // this is to fix a bug on cairo win:
        // https://gitlab.freedesktop.org/cairo/cairo/issues/338
        // TODO: check if inkscape min cairo version has applied the patch proposed and remove (3 times)
        if (empty) {
            css_tree->hide();
        }
        _styleBox.pack_start(*css_selector_container, Gtk::PACK_EXPAND_WIDGET);
    }
    selectorpos++;
    if (tokens.size() == 0) {
        _updating = false;
        return;
    }
    for (unsigned i = 0; i < tokens.size() - 1; i += 2) {
        Glib::ustring selector = tokens[i];
        REMOVE_SPACES(selector); // Remove leading/trailing spaces
        // Get list of objects selector matches
        std::vector<Glib::ustring> selectordata = Glib::Regex::split_simple(";", selector);
        Glib::ustring selector_orig = selector;
        if (!selectordata.empty()) {
            selector = selectordata.back();
        }
        std::vector<SPObject *> objVec = _getObjVec(selector);
        if (obj) {
            bool stop = true;
            for (auto objel : objVec) {
                if (objel == obj) {
                    stop = false;
                }
            }
            if (stop) {
                _updating = false;
                selectorpos++;
                continue;
            }
        }
        if (!obj && _current_selector != "" && _current_selector != selector) {
            _updating = false;
            selectorpos++;
            continue;
        }
        if (!obj) {
            bool present = false;
            for (auto objv : objVec) {
                for (auto objsel : selection->objects()) {
                    if (objv == objsel) {
                        present = true;
                        break;
                    }
                }
                if (present) {
                    break;
                }
            }
            if (!present) {
                _updating = false;
                selectorpos++;
                continue;
            }
        }
        Glib::ustring properties;
        // Check to make sure we do have a value to match selector.
        if ((i + 1) < tokens.size()) {
            properties = tokens[i + 1];
        } else {
            std::cerr << "StyleDialog::readStyleElement: Missing values "
                         "for last selector!"
                      << std::endl;
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
        Gtk::EventBox *css_selector_event_box;
        _builder->get_widget("CSSSelectorEventBox", css_selector_event_box);
        Gtk::Entry *css_edit_selector;
        _builder->get_widget("CSSEditSelector", css_edit_selector);
        Gtk::EventBox *css_selector_event_add;
        _builder->get_widget("CSSSelectorEventAdd", css_selector_event_add);
        css_selector_event_add->add_events(Gdk::BUTTON_RELEASE_MASK);
        css_selector->set_text(selector);
        Gtk::TreeView *css_tree;
        _builder->get_widget("CSSTree", css_tree);
        css_tree->get_style_context()->add_class("style_sheet");
        Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(_mColumns);
        css_tree->set_model(store);
        // I comment this feature, is working but seems obscure to undertand
        // the user can edit selector name in current implementation
        /* css_selector_event_box->signal_button_release_event().connect(
            sigc::bind(sigc::mem_fun(*this, &StyleDialog::_selectorStartEdit), css_selector, css_edit_selector));
        css_edit_selector->signal_key_press_event().connect(sigc::bind(
            sigc::mem_fun(*this, &StyleDialog::_selectorEditKeyPress), store, css_selector, css_edit_selector));
        css_edit_selector->signal_activate().connect(
            sigc::bind(sigc::mem_fun(*this, &StyleDialog::_selectorActivate), store, css_selector, css_edit_selector));
         */
        Inkscape::UI::Widget::IconRenderer *addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
        addRenderer->add_icon("edit-delete");
        int addCol = css_tree->append_column(" ", *addRenderer) - 1;
        Gtk::TreeViewColumn *col = css_tree->get_column(addCol);
        if (col) {
            addRenderer->signal_activated().connect(
                sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(sigc::mem_fun(*this, &StyleDialog::_onPropDelete), store));
        }
        Gtk::CellRendererToggle *isactive = Gtk::manage(new Gtk::CellRendererToggle());
        isactive->property_activatable() = true;
        addCol = css_tree->append_column(" ", *isactive) - 1;
        col = css_tree->get_column(addCol);
        if (col) {
            col->add_attribute(isactive->property_active(), _mColumns._colActive);
            isactive->signal_toggled().connect(
                sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(sigc::mem_fun(*this, &StyleDialog::_activeToggled), store));
        }
        Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
        label->property_placeholder_text() = _("property");
        label->property_editable() = true;
        label->signal_edited().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *>(
            sigc::mem_fun(*this, &StyleDialog::_nameEdited), store, css_tree));
        label->signal_editing_started().connect(sigc::mem_fun(*this, &StyleDialog::_startNameEdit));
        addCol = css_tree->append_column(" ", *label) - 1;
        col = css_tree->get_column(addCol);
        if (col) {
            col->set_resizable(true);
            col->add_attribute(label->property_text(), _mColumns._colName);
        }
        Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
        value->property_editable() = true;
        value->property_placeholder_text() = _("value");
        value->signal_edited().connect(
            sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(sigc::mem_fun(*this, &StyleDialog::_valueEdited), store));
        value->signal_editing_started().connect(
            sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(sigc::mem_fun(*this, &StyleDialog::_startValueEdit), store));
        addCol = css_tree->append_column(" ", *value) - 1;
        col = css_tree->get_column(addCol);
        if (col) {
            col->add_attribute(value->property_text(), _mColumns._colValue);
            col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
        }
        Glib::ustring style = properties;
        Glib::ustring comments = "";
        while (style.find("/*") != std::string::npos) {
            size_t beg = style.find("/*");
            size_t end = style.find("*/");
            if (end != std::string::npos && beg != std::string::npos) {
                comments = comments.append(style, beg + 2, end - beg - 2);
                style = style.erase(beg, end - beg + 2);
            }
        }
        std::map<Glib::ustring, Glib::ustring> attr_prop_styleshet = parseStyle(style);
        std::map<Glib::ustring, Glib::ustring> attr_prop_styleshet_comments = parseStyle(comments);
        std::map<Glib::ustring, std::pair<Glib::ustring, bool>> result_props;
        for (auto styled : attr_prop_styleshet) {
            result_props[styled.first] = std::make_pair(styled.second, true);
        }
        for (auto styled : attr_prop_styleshet_comments) {
            result_props[styled.first] = std::make_pair(styled.second, false);
        }
        empty = true;
        css_selector_event_add->signal_button_release_event().connect(
            sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *, Glib::ustring, gint>(
                sigc::mem_fun(*this, &StyleDialog::_addRow), store, css_tree, selector_orig, selectorpos));
        if (obj) {
            for (auto iter : result_props) {
                empty = false;
                Gtk::TreeIter iterstore = store->append();
                Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iterstore;
                Gtk::TreeModel::Row row = *(iterstore);
                row[_mColumns._colSelector] = selector_orig;
                row[_mColumns._colSelectorPos] = selectorpos;
                row[_mColumns._colActive] = iter.second.second;
                row[_mColumns._colName] = iter.first;
                row[_mColumns._colValue] = iter.second.first;
                const Glib::ustring value = row[_mColumns._colValue];
                if (iter.second.second) {
                    Glib::ustring val = "";
                    for (auto iterprop : obj->style->properties()) {
                        if (iterprop->style_src != SPStyleSrc::UNSET && iterprop->name() == iter.first) {
                            val = iterprop->get_value();
                            break;
                        }
                    }
                    guint32 r1 = 0; // if there's no color, return black
                    r1 = sp_svg_read_color(value.c_str(), r1);
                    guint32 r2 = 0; // if there's no color, return black
                    r2 = sp_svg_read_color(val.c_str(), r2);
                    if (attr_prop.count(iter.first) || (value != val && (r1 == 0 || r1 != r2))) {
                        row[_mColumns._colStrike] = true;
                        row[_mColumns._colOwner] = Glib::ustring("");
                    } else {
                        row[_mColumns._colStrike] = false;
                        row[_mColumns._colOwner] = Glib::ustring("Current value");
                        _addOwnerStyle(iter.first, selector);
                    }
                } else {
                    row[_mColumns._colStrike] = true;
                    Glib::ustring tooltiptext = _("This value is commented out.");
                    row[_mColumns._colOwner] = tooltiptext;
                }
            }
        } else {
            for (auto iter : result_props) {
                empty = false;
                Gtk::TreeModel::Row row = *(store->prepend());
                row[_mColumns._colSelector] = selector_orig;
                row[_mColumns._colSelectorPos] = selectorpos;
                row[_mColumns._colActive] = iter.second.second;
                row[_mColumns._colName] = iter.first;
                row[_mColumns._colValue] = iter.second.first;
                row[_mColumns._colStrike] = false;
                row[_mColumns._colOwner] = Glib::ustring("Stylesheet value");
            }
        }
        if (empty) {
            css_tree->hide();
        }
        _styleBox.pack_start(*css_selector_container, Gtk::PACK_EXPAND_WIDGET);
        selectorpos++;
    }
    try {
        _builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for filter effect dialog.");
        return;
    }
    _builder->get_widget("CSSSelector", css_selector);
    css_selector->set_text("element.attributes");
    _builder->get_widget("CSSSelectorContainer", css_selector_container);
    _builder->get_widget("CSSSelectorEventAdd", css_selector_event_add);
    css_selector_event_add->add_events(Gdk::BUTTON_RELEASE_MASK);
    store = Gtk::TreeStore::create(_mColumns);
    _builder->get_widget("CSSTree", css_tree);
    css_tree->get_style_context()->add_class("style_attribute");
    css_tree->set_model(store);
    css_selector_event_add->signal_button_release_event().connect(
        sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *, Glib::ustring, gint>(
            sigc::mem_fun(*this, &StyleDialog::_addRow), store, css_tree, "attributes", selectorpos));
    bool hasattributes = false;
    empty = true;
    if (obj) {
        for (auto iter : obj->style->properties()) {
            if (iter->style_src != SPStyleSrc::UNSET) {
                auto key = iter->id();
                if (key != SPAttr::FONT && key != SPAttr::D && key != SPAttr::MARKER) {
                    const gchar *attr = obj->getRepr()->attribute(iter->name().c_str());
                    if (attr) {
                        if (!hasattributes) {
                            Inkscape::UI::Widget::IconRenderer *addRenderer =
                                manage(new Inkscape::UI::Widget::IconRenderer());
                            addRenderer->add_icon("edit-delete");
                            int addCol = css_tree->append_column(" ", *addRenderer) - 1;
                            Gtk::TreeViewColumn *col = css_tree->get_column(addCol);
                            if (col) {
                                addRenderer->signal_activated().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(
                                    sigc::mem_fun(*this, &StyleDialog::_onPropDelete), store));
                            }
                            Gtk::CellRendererText *label = Gtk::manage(new Gtk::CellRendererText());
                            label->property_placeholder_text() = _("property");
                            label->property_editable() = true;
                            label->signal_edited().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore>, Gtk::TreeView *>(
                                sigc::mem_fun(*this, &StyleDialog::_nameEdited), store, css_tree));
                            label->signal_editing_started().connect(sigc::mem_fun(*this, &StyleDialog::_startNameEdit));
                            addCol = css_tree->append_column(" ", *label) - 1;
                            col = css_tree->get_column(addCol);
                            if (col) {
                                col->set_resizable(true);
                                col->add_attribute(label->property_text(), _mColumns._colName);
                            }
                            Gtk::CellRendererText *value = Gtk::manage(new Gtk::CellRendererText());
                            value->property_placeholder_text() = _("value");
                            value->property_editable() = true;
                            value->signal_edited().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(
                                sigc::mem_fun(*this, &StyleDialog::_valueEdited), store));
                            value->signal_editing_started().connect(sigc::bind<Glib::RefPtr<Gtk::TreeStore>>(
                                sigc::mem_fun(*this, &StyleDialog::_startValueEdit), store));

                            addCol = css_tree->append_column(" ", *value) - 1;
                            col = css_tree->get_column(addCol);
                            if (col) {
                                col->add_attribute(value->property_text(), _mColumns._colValue);
                                col->add_attribute(value->property_strikethrough(), _mColumns._colStrike);
                            }
                        }
                        empty = false;
                        Gtk::TreeIter iterstore = store->prepend();
                        Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iterstore;
                        Gtk::TreeModel::Row row = *(iterstore);
                        row[_mColumns._colSelector] = "attributes";
                        row[_mColumns._colSelectorPos] = selectorpos;
                        row[_mColumns._colActive] = true;
                        row[_mColumns._colName] = iter->name();
                        row[_mColumns._colValue] = attr;
                        if (_owner_style.find(iter->name()) != _owner_style.end()) {
                            row[_mColumns._colStrike] = true;
                            Glib::ustring tooltiptext = Glib::ustring("");
                            row[_mColumns._colOwner] = tooltiptext;
                        } else {
                            row[_mColumns._colStrike] = false;
                            row[_mColumns._colOwner] = Glib::ustring("Current value");
                            _addOwnerStyle(iter->name(), "inline attributes");
                        }
                        hasattributes = true;
                    }
                }
            }
        }
        if (empty) {
            css_tree->hide();
        }
        if (!hasattributes) {
            for (auto widg : css_selector_container->get_children()) {
                delete widg;
            }
        }
        _styleBox.pack_start(*css_selector_container, Gtk::PACK_EXPAND_WIDGET);
    }
    for (auto selector : _styleBox.get_children()) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(&selector[0]);
        if (box) {
            std::vector<Gtk::Widget *> childs = box->get_children();
            if (childs.size() > 1) {
                Gtk::TreeView *css_tree = dynamic_cast<Gtk::TreeView *>(childs[1]);
                if (css_tree) {
                    Glib::RefPtr<Gtk::TreeModel> model = css_tree->get_model();
                    if (model) {
                        model->foreach_iter(sigc::mem_fun(*this, &StyleDialog::_on_foreach_iter));
                    }
                }
            }
        }
    }
    if (obj) {
        obj->style->readFromObject(obj);
        obj->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
    }
    _mainBox.show_all_children();
    _updating = false;
}

bool StyleDialog::_selectorStartEdit(GdkEventButton *event, Gtk::Label *selector, Gtk::Entry *selector_edit)
{
    g_debug("StyleDialog::_selectorStartEdit");
    if (event->type == GDK_BUTTON_RELEASE && event->button == 1) {
        selector->hide();
        selector_edit->set_text(selector->get_text());
        selector_edit->show();
    }
    return false;
}

/* void StyleDialog::_selectorActivate(Glib::RefPtr<Gtk::TreeStore> store, Gtk::Label *selector, Gtk::Entry
*selector_edit)
{
    g_debug("StyleDialog::_selectorEditKeyPress");
    Glib::ustring newselector = fixCSSSelectors(selector_edit->get_text());
    if (newselector.empty()) {
        selector_edit->get_style_context()->add_class("system_error_color");
        return;
    }
    _writeStyleElement(store, selector->get_text(), selector_edit->get_text());
} */

bool StyleDialog::_selectorEditKeyPress(GdkEventKey *event, Glib::RefPtr<Gtk::TreeStore> store, Gtk::Label *selector,
                                        Gtk::Entry *selector_edit)
{
    g_debug("StyleDialog::_selectorEditKeyPress");
    switch (event->keyval) {
        case GDK_KEY_Escape:
            selector->show();
            selector_edit->hide();
            selector_edit->get_style_context()->remove_class("system_error_color");
            break;
    }
    return false;
}

bool StyleDialog::_on_foreach_iter(const Gtk::TreeModel::iterator &iter)
{
    g_debug("StyleDialog::_on_foreach_iter");

    Gtk::TreeModel::Row row = *(iter);
    Glib::ustring owner = row[_mColumns._colOwner];
    if (owner.empty()) {
        Glib::ustring value = _owner_style[row[_mColumns._colName]];
        Glib::ustring tooltiptext = Glib::ustring(_("Invalid property set"));
        if (!value.empty()) {
            tooltiptext = Glib::ustring::compose(_("Used in %1"), _owner_style[row[_mColumns._colName]]);
        }
        row[_mColumns._colOwner] = tooltiptext;
    }
    return false;
}

void StyleDialog::_onLinkObj(Glib::ustring path, Glib::RefPtr<Gtk::TreeStore> store)
{
    g_debug("StyleDialog::_onLinkObj");

    Gtk::TreeModel::Row row = *store->get_iter(path);
    if (row && row[_mColumns._colLinked]) {
        SPObject *linked = row[_mColumns._colHref];
        if (linked) {
            Inkscape::Selection *selection = _desktop->getSelection();
            _desktop->getDocument()->setXMLDialogSelectedObject(linked);
            selection->clear();
            selection->set(linked);
        }
    }
}

/**
 * @brief StyleDialog::_onPropDelete
 * @param event
 * @return true
 * Delete the attribute from the style
 */
void StyleDialog::_onPropDelete(Glib::ustring path, Glib::RefPtr<Gtk::TreeStore> store)
{
    g_debug("StyleDialog::_onPropDelete");
    Gtk::TreeModel::Row row = *store->get_iter(path);
    if (row) {
        Glib::ustring selector = row[_mColumns._colSelector];
        row[_mColumns._colName] = "";
        _deleted_pos = row[_mColumns._colSelectorPos];
        store->erase(row);
        _deletion = true;
        _writeStyleElement(store, selector);
    }
}

void StyleDialog::_addOwnerStyle(Glib::ustring name, Glib::ustring selector)
{
    g_debug("StyleDialog::_addOwnerStyle");

    if (_owner_style.find(name) == _owner_style.end()) {
        _owner_style[name] = selector;
    }
}


/**
 * @brief StyleDialog::parseStyle
 *
 * Convert a style string into a vector map. This should be moved to style.cpp
 *
 */
std::map<Glib::ustring, Glib::ustring> StyleDialog::parseStyle(Glib::ustring style_string)
{
    g_debug("StyleDialog::parseStyle");

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
void StyleDialog::_writeStyleElement(Glib::RefPtr<Gtk::TreeStore> store, Glib::ustring selector,
                                     Glib::ustring new_selector)
{
    g_debug("StyleDialog::_writeStyleElemen");
    if (_updating) {
        return;
    }
    _scroollock = true;
    Inkscape::Selection *selection = _desktop->getSelection();
    SPObject *obj = nullptr;
    if (selection->objects().size() == 1) {
        obj = selection->objects().back();
    }
    if (!obj) {
        obj = _desktop->getDocument()->getXMLDialogSelectedObject();
    }
    if (selection->objects().size() < 2 && !obj) {
        readStyleElement();
        return;
    }
    _updating = true;
    gint selectorpos = 0;
    std::string styleContent = "";
    if (selector != "style_properties" && selector != "attributes") {
        if (!new_selector.empty()) {
            selector = new_selector;
        }
        std::vector<Glib::ustring> selectordata = Glib::Regex::split_simple(";", selector);
        for (auto selectoritem : selectordata) {
            if (selectordata[selectordata.size() - 1] == selectoritem) {
                selector = selectoritem;
            } else {
                styleContent = styleContent + selectoritem + ";\n";
            }
        }
        styleContent.append("\n").append(selector.raw()).append(" { \n");
    }
    selectorpos = _deleted_pos;
    for (auto &row : store->children()) {
        selector = row[_mColumns._colSelector];
        selectorpos = row[_mColumns._colSelectorPos];
        const char *opencomment = "";
        const char *closecomment = "";
        if (selector != "style_properties" && selector != "attributes") {
            opencomment = row[_mColumns._colActive] ? "    " : "  /*";
            closecomment = row[_mColumns._colActive] ? "\n" : "*/\n";
        }
        Glib::ustring const &name = row[_mColumns._colName];
        Glib::ustring const &value = row[_mColumns._colValue];
        if (!(name.empty() && value.empty())) {
            styleContent = styleContent + opencomment + name.raw() + ":" + value.raw() + ";" + closecomment;
        }
    }
    if (selector != "style_properties" && selector != "attributes") {
        styleContent = styleContent + "}";
    }
    if (selector == "style_properties") {
        _updating = true;
        obj->getRepr()->setAttribute("style", styleContent);
        _updating = false;
    } else if (selector == "attributes") {
        for (auto iter : obj->style->properties()) {
            auto key = iter->id();
            if (key != SPAttr::FONT && key != SPAttr::D && key != SPAttr::MARKER) {
                const gchar *attr = obj->getRepr()->attribute(iter->name().c_str());
                if (attr) {
                    _updating = true;
                    obj->getRepr()->removeAttribute(iter->name());
                    _updating = false;
                }
            }
        }
        for (auto &row : store->children()) {
            Glib::ustring const &name = row[_mColumns._colName];
            Glib::ustring const &value = row[_mColumns._colValue];
            if (!(name.empty() && value.empty())) {
                _updating = true;
                obj->getRepr()->setAttribute(name, value);
                _updating = false;
            }
        }
    } else if (!selector.empty()) { // styleshet
        // We could test if styleContent is empty and then delete the style node here but there is no
        // harm in keeping it around ...

        std::string pos = std::to_string(selectorpos);
        std::string selectormatch = "(";
        for (; selectorpos > 1; selectorpos--) {
            selectormatch = selectormatch + "[^}]*?}";
        }
        selectormatch = selectormatch + ")([^}]*?})((.|\n)*)";

        Inkscape::XML::Node *textNode = _getStyleTextNode(true);
        std::regex e(selectormatch.c_str());
        std::string content = (textNode->content() ? textNode->content() : "");
        std::string result;
        std::regex_replace(std::back_inserter(result), content.begin(), content.end(), e, "$1" + styleContent + "$3");
        bool empty = false;
        if (result.empty()) {
            empty = true;
            result = "* > .inkscapehacktmp{}";
        }
        textNode->setContent(result.c_str());
        if (empty) {
            textNode->setContent("");
        }
    }
    _updating = false;
    readStyleElement();
    SPDocument *document = SP_ACTIVE_DOCUMENT;
    for (auto iter : document->getObjectsBySelector(selector)) {
        iter->style->readFromObject(iter);
        iter->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
    }
    DocumentUndo::done(SP_ACTIVE_DOCUMENT, SP_VERB_DIALOG_STYLE, _("Edited style element."));

    g_debug("StyleDialog::_writeStyleElement(): | %s |", styleContent.c_str());
}

bool StyleDialog::_addRow(GdkEventButton *evt, Glib::RefPtr<Gtk::TreeStore> store, Gtk::TreeView *css_tree,
                          Glib::ustring selector, gint pos)
{
    g_debug("StyleDialog::_addRow");

    if (evt->type == GDK_BUTTON_RELEASE && evt->button == 1) {
        Gtk::TreeIter iter = store->prepend();
        Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
        Gtk::TreeModel::Row row = *(iter);
        row[_mColumns._colSelector] = selector;
        row[_mColumns._colSelectorPos] = pos;
        row[_mColumns._colActive] = true;
        row[_mColumns._colName] = "";
        row[_mColumns._colValue] = "";
        row[_mColumns._colStrike] = false;
        gint col = 2;
        if (pos < 1) {
            col = 1;
        }
        css_tree->show();
        css_tree->set_cursor(path, *(css_tree->get_column(col)), true);
        grab_focus();
        return true;
    }
    return false;
}

void StyleDialog::_setAutocompletion(Gtk::Entry *entry, SPStyleEnum const cssenum[])
{
    g_debug("StyleDialog::_setAutocompletion");

    Glib::RefPtr<Gtk::ListStore> completionModel = Gtk::ListStore::create(_mCSSData);
    Glib::RefPtr<Gtk::EntryCompletion> entry_completion = Gtk::EntryCompletion::create();
    entry_completion->set_model(completionModel);
    entry_completion->set_text_column (_mCSSData._colCSSData);
    entry_completion->set_minimum_key_length(0);
    entry_completion->set_popup_completion(true);
    gint counter = 0;
    const char * key = cssenum[counter].key;
    while (key) {
        Gtk::TreeModel::Row row = *(completionModel->prepend());
        row[_mCSSData._colCSSData] = Glib::ustring(key);
        counter++;
        key = cssenum[counter].key;
    }
    entry->set_completion(entry_completion);
}
/*Harcode values non in enum*/
void StyleDialog::_setAutocompletion(Gtk::Entry *entry, Glib::ustring name)
{
    g_debug("StyleDialog::_setAutocompletion");

    Glib::RefPtr<Gtk::ListStore> completionModel = Gtk::ListStore::create(_mCSSData);
    Glib::RefPtr<Gtk::EntryCompletion> entry_completion = Gtk::EntryCompletion::create();
    entry_completion->set_model(completionModel);
    entry_completion->set_text_column(_mCSSData._colCSSData);
    entry_completion->set_minimum_key_length(0);
    entry_completion->set_popup_completion(true);
    if (name == "paint-order") {
        Gtk::TreeModel::Row row = *(completionModel->append());
        row[_mCSSData._colCSSData] = Glib::ustring("fill markers stroke");
        row = *(completionModel->append());
        row[_mCSSData._colCSSData] = Glib::ustring("fill stroke markers");
        row = *(completionModel->append());
        row[_mCSSData._colCSSData] = Glib::ustring("stroke markers fill");
        row = *(completionModel->append());
        row[_mCSSData._colCSSData] = Glib::ustring("stroke fill markers");
        row = *(completionModel->append());
        row[_mCSSData._colCSSData] = Glib::ustring("markers fill stroke");
        row = *(completionModel->append());
        row[_mCSSData._colCSSData] = Glib::ustring("markers stroke fill");
    }
    entry->set_completion(entry_completion);
}

void
StyleDialog::_startValueEdit(Gtk::CellEditable* cell, const Glib::ustring& path, Glib::RefPtr<Gtk::TreeStore> store)
{
    g_debug("StyleDialog::_startValueEdit");
    _deletion = false;
    _scroollock = true;
    Gtk::TreeModel::Row row = *store->get_iter(path);
    if (row) {
        Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(cell);
        Glib::ustring name = row[_mColumns._colName];
        if (name == "paint-order") {
            _setAutocompletion(entry, name);
        } else if (name == "fill-rule") {
            _setAutocompletion(entry, enum_fill_rule);
        } else if (name == "stroke-linecap") {
            _setAutocompletion(entry, enum_stroke_linecap);
        } else if (name == "stroke-linejoin") {
            _setAutocompletion(entry, enum_stroke_linejoin);
        } else if (name == "font-style") {
            _setAutocompletion(entry, enum_font_style);
        } else if (name == "font-variant") {
            _setAutocompletion(entry, enum_font_variant);
        } else if (name == "font-weight") {
            _setAutocompletion(entry, enum_font_weight);
        } else if (name == "font-stretch") {
            _setAutocompletion(entry, enum_font_stretch);
        } else if (name == "font-variant-position") {
            _setAutocompletion(entry, enum_font_variant_position);
        } else if (name == "text-align") {
            _setAutocompletion(entry, enum_text_align);
        } else if (name == "text-transform") {
            _setAutocompletion(entry, enum_text_transform);
        } else if (name == "text-anchor") {
            _setAutocompletion(entry, enum_text_anchor);
        } else if (name == "white-space") {
            _setAutocompletion(entry, enum_white_space);
        } else if (name == "direction") {
            _setAutocompletion(entry, enum_direction);
        } else if (name == "baseline-shift") {
            _setAutocompletion(entry, enum_baseline_shift);
        } else if (name == "visibility") {
            _setAutocompletion(entry, enum_visibility);
        } else if (name == "overflow") {
            _setAutocompletion(entry, enum_overflow);
        } else if (name == "display") {
            _setAutocompletion(entry, enum_display);
        } else if (name == "shape-rendering") {
            _setAutocompletion(entry, enum_shape_rendering);
        } else if (name == "color-rendering") {
            _setAutocompletion(entry, enum_color_rendering);
        } else if (name == "overflow") {
            _setAutocompletion(entry, enum_overflow);
        } else if (name == "clip-rule") {
            _setAutocompletion(entry, enum_clip_rule);
        } else if (name == "color-interpolation") {
            _setAutocompletion(entry, enum_color_interpolation);
        }
        entry->signal_key_release_event().connect(
            sigc::bind(sigc::mem_fun(*this, &StyleDialog::_onValueKeyReleased), entry));
        entry->signal_key_press_event().connect(
            sigc::bind(sigc::mem_fun(*this, &StyleDialog::_onValueKeyPressed), entry));
    }
}

void StyleDialog::_startNameEdit(Gtk::CellEditable *cell, const Glib::ustring &path)
{
    _deletion = false;
    g_debug("StyleDialog::_startNameEdit");
    _scroollock = true;
    Glib::RefPtr<Gtk::ListStore> completionModel = Gtk::ListStore::create(_mCSSData);
    Glib::RefPtr<Gtk::EntryCompletion> entry_completion = Gtk::EntryCompletion::create();
    entry_completion->set_model(completionModel);
    entry_completion->set_text_column(_mCSSData._colCSSData);
    entry_completion->set_minimum_key_length(1);
    entry_completion->set_popup_completion(true);
    for (auto prop : sp_attribute_name_list(true)) {
        Gtk::TreeModel::Row row = *(completionModel->append());
        row[_mCSSData._colCSSData] = prop;
    }
    Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(cell);
    entry->set_completion(entry_completion);
    entry->signal_key_release_event().connect(
        sigc::bind(sigc::mem_fun(*this, &StyleDialog::_onNameKeyReleased), entry));
    entry->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &StyleDialog::_onNameKeyPressed), entry));
}


gboolean sp_styledialog_store_move_to_next(gpointer data)
{
    StyleDialog *styledialog = reinterpret_cast<StyleDialog *>(data);
    if (!styledialog->_deletion) {
        auto selection = styledialog->_current_css_tree->get_selection();
        Gtk::TreeIter iter = *(selection->get_selected());
        Gtk::TreeModel::Path model = (Gtk::TreeModel::Path)iter;
        if (model == styledialog->_current_path) {
            styledialog->_current_css_tree->set_cursor(styledialog->_current_path, *styledialog->_current_value_col,
                                                       true);
        }
    }
    return FALSE;
}

/**
 * @brief StyleDialog::nameEdited
 * @param event
 * @return
 * Called when the name is edited in the TreeView editable column
 */
void StyleDialog::_nameEdited(const Glib::ustring &path, const Glib::ustring &name, Glib::RefPtr<Gtk::TreeStore> store,
                              Gtk::TreeView *css_tree)
{
    g_debug("StyleDialog::_nameEdited");

    _scroollock = true;
    Gtk::TreeModel::Row row = *store->get_iter(path);
    _current_path = (Gtk::TreeModel::Path)*store->get_iter(path);

    if (row) {
        _current_css_tree = css_tree;
        Glib::ustring finalname = name;
        auto i = finalname.find_first_of(";:=");
        if (i != std::string::npos) {
            finalname.erase(i, name.size() - i);
        }
        gint pos = row[_mColumns._colSelectorPos];
        bool write = false;
        if (row[_mColumns._colName] != finalname && row[_mColumns._colValue] != "") {
            write = true;
        }
        Glib::ustring selector = row[_mColumns._colSelector];
        Glib::ustring value = row[_mColumns._colValue];
        bool is_attr = selector == "attributes";
        Glib::ustring old_name = row[_mColumns._colName];
        row[_mColumns._colName] = finalname;
        if (finalname.empty() && value.empty()) {
            _deleted_pos = row[_mColumns._colSelectorPos];
            store->erase(row);
        }
        gint col = 3;
        if (pos < 1 || is_attr) {
            col = 2;
        }
        _current_value_col = css_tree->get_column(col);
        if (write && old_name != name) {
            _writeStyleElement(store, selector);
            /*
            I think is better comment this, is enoght update on value change
            if (selector != "style_properties" && selector != "attributes") {
            std::vector<SPObject *> objs = _getObjVec(selector);
            for (auto obj : objs){
                Glib::ustring css_str = "";
                SPCSSAttr *css = sp_repr_css_attr_new();
                sp_repr_css_attr_add_from_string(css, obj->getRepr()->attribute("style"));
                css->removeAttribute(name);
                sp_repr_css_write_string(css, css_str);
                obj->getRepr()->setAttributeOrRemoveIfEmpty("style", css_str);
                obj->style->readFromObject(obj);
                obj->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            }
            } */
        } else {
            g_timeout_add(50, &sp_styledialog_store_move_to_next, this);
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
void StyleDialog::_valueEdited(const Glib::ustring &path, const Glib::ustring &value,
                               Glib::RefPtr<Gtk::TreeStore> store)
{
    g_debug("StyleDialog::_valueEdited");

    _scroollock = true;

    Gtk::TreeModel::Row row = *store->get_iter(path);
    if (row) {
        Glib::ustring finalvalue = value;
        auto i = std::min(finalvalue.find(";"), finalvalue.find(":"));
        if (i != std::string::npos) {
            finalvalue.erase(i, finalvalue.size() - i);
        }
        Glib::ustring old_value = row[_mColumns._colValue];
        if (old_value == finalvalue) {
            return;
        }
        row[_mColumns._colValue] = finalvalue;
        Glib::ustring selector = row[_mColumns._colSelector];
        Glib::ustring name = row[_mColumns._colName];
        if (name.empty() && finalvalue.empty()) {
            _deleted_pos = row[_mColumns._colSelectorPos];
            store->erase(row);
        }
        _writeStyleElement(store, selector);
        if (selector != "style_properties" && selector != "attributes") {
            std::vector<SPObject *> objs = _getObjVec(selector);
            for (auto obj : objs) {
                Glib::ustring css_str = "";
                SPCSSAttr *css = sp_repr_css_attr_new();
                sp_repr_css_attr_add_from_string(css, obj->getRepr()->attribute("style"));
                css->removeAttribute(name);
                sp_repr_css_write_string(css, css_str);
                obj->getRepr()->setAttribute("style", css_str);
                obj->style->readFromObject(obj);
                obj->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            }
        }
    }
}

void StyleDialog::_activeToggled(const Glib::ustring &path, Glib::RefPtr<Gtk::TreeStore> store)
{
    g_debug("StyleDialog::_activeToggled");

    _scroollock = true;
    Gtk::TreeModel::Row row = *store->get_iter(path);
    if (row) {
        row[_mColumns._colActive] = !row[_mColumns._colActive];
        Glib::ustring selector = row[_mColumns._colSelector];
        _writeStyleElement(store, selector);
    }
}

bool StyleDialog::_onNameKeyPressed(GdkEventKey *event, Gtk::Entry *entry)
{
    g_debug("StyleDialog::_onNameKeyReleased");
    bool ret = false;
    switch (event->keyval) {
        case GDK_KEY_Tab:
        case GDK_KEY_KP_Tab:
            entry->editing_done();
            ret = true;
            break;
    }
    return ret;
}

bool StyleDialog::_onNameKeyReleased(GdkEventKey *event, Gtk::Entry *entry)
{
    g_debug("StyleDialog::_onNameKeyReleased");
    bool ret = false;
    switch (event->keyval) {
        case GDK_KEY_equal:
        case GDK_KEY_colon:
            entry->editing_done();
            ret = true;
            break;
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
        case GDK_KEY_semicolon: {
            Glib::ustring text = entry->get_text();
            auto i = std::min(text.find(";"), text.find(":"));
            if (i != std::string::npos) {
                entry->editing_done();
                ret = true;
            }
            break;
        }
    }
    return ret;
}

bool StyleDialog::_onValueKeyPressed(GdkEventKey *event, Gtk::Entry *entry)
{
    g_debug("StyleDialog::_onValueKeyReleased");
    bool ret = false;
    switch (event->keyval) {
        case GDK_KEY_Tab:
        case GDK_KEY_KP_Tab:
            entry->editing_done();
            ret = true;
            break;
    }
    return ret;
}

bool StyleDialog::_onValueKeyReleased(GdkEventKey *event, Gtk::Entry *entry)
{
    g_debug("StyleDialog::_onValueKeyReleased");
    bool ret = false;
    switch (event->keyval) {
        case GDK_KEY_semicolon:
            entry->editing_done();
            ret = true;
            break;
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
        case GDK_KEY_colon: {
            Glib::ustring text = entry->get_text();
            auto i = std::min(text.find(";"), text.find(":"));
            if (i != std::string::npos) {
                entry->editing_done();
                ret = true;
            }
            break;
        }
    }
    return ret;
}

/**
 * Update the watchers on objects.
 */
void StyleDialog::_updateWatchers(SPDesktop *desktop)

{
    g_debug("StyleDialog::_updateWatchers");

    if (_textNode) {
        _textNode->removeObserver(*m_styletextwatcher);
        _textNode = nullptr;
    }

    if (m_root) {
        m_root->removeSubtreeObserver(*m_nodewatcher);
        m_root = nullptr;
    }

    if (desktop) {
        m_root = desktop->getDocument()->getReprRoot();
        m_root->addSubtreeObserver(*m_nodewatcher);
    }
}


/**
 * @param selector: a valid CSS selector string.
 * @return objVec: a vector of pointers to SPObject's the selector matches.
 * Return a vector of all objects that selector matches.
 */
std::vector<SPObject *> StyleDialog::_getObjVec(Glib::ustring selector)
{
    g_debug("StyleDialog::_getObjVec");

    g_assert(selector.find(";") == Glib::ustring::npos);

    return _desktop->getDocument()->getObjectsBySelector(selector);
}

void StyleDialog::_closeDialog(Gtk::Dialog *textDialogPtr) { textDialogPtr->response(Gtk::RESPONSE_OK); }


/**
 * Handle document replaced. (Happens when a default document is immediately replaced by another
 * document in a new window.)
 */
void StyleDialog::_handleDocumentReplaced(SPDesktop *desktop, SPDocument * /* document */)
{
    g_debug("StyleDialog::handleDocumentReplaced()");

    _selection_changed_connection.disconnect();
    _updateWatchers(desktop);

    if (!desktop)
        return;

    _selection_changed_connection =
        desktop->getSelection()->connectChanged(sigc::hide(sigc::mem_fun(this, &StyleDialog::_handleSelectionChanged)));

    readStyleElement();
}


/*
 * When a dialog is floating, it is connected to the active desktop.
 */
void StyleDialog::update()
{
    if (!_app) {
        std::cerr << "UndoHistory::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();
    if (!desktop || _desktop == desktop) {
        return;
    }

    _desktop = desktop;
    _document_replaced_connection.disconnect();

    if (desktop) {
        _document_replaced_connection =
        desktop->connectDocumentReplaced(sigc::mem_fun(this, &StyleDialog::_handleDocumentReplaced));
    }

    _handleDocumentReplaced(desktop, nullptr);
}

/*
 * Handle a change in which objects are selected in a document.
 */
void StyleDialog::_handleSelectionChanged()
{
    g_debug("StyleDialog::_handleSelectionChanged()");
    _scroolpos = 0;
    _vadj->set_value(0);
    // Sometimes the selection changes because inkscape is closing.
    if (getDesktop()) {
        readStyleElement();
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
