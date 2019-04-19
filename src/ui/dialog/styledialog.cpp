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

#include "message-context.h"
#include "message-stack.h"
#include "selection.h"
#include "style-internal.h"
#include "style.h"

#include "ui/icon-loader.h"
#include "ui/widget/iconrenderer.h"
#include "verbs.h"
#include "xml/attribute-record.h"
#include "xml/node-event-vector.h"
#include <glibmm/i18n.h>

#include <glibmm/i18n.h>

static void on_attr_changed(Inkscape::XML::Node *repr, const gchar *name, const gchar * /*old_value*/,
                            const gchar *new_value, bool /*is_interactive*/, gpointer data)
{
    CSS_DIALOG(data)->onAttrChanged(repr, name, new_value);
    CSS_DIALOG(data)->styledialog = new Inkscape::UI::Dialog::StyleDialog();
}

Inkscape::XML::NodeEventVector css_repr_events = {
    nullptr,                  /* child_added */
    nullptr,                  /* child_removed */
    on_attr_changed, nullptr, /* content_changed */
    nullptr                   /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * Constructor
 * A treeview whose each row corresponds to a CSS property of selector selected.
 * New CSS property can be added by clicking '+' at bottom of the CSS pane. '-'
 * in front of the CSS property row can be clicked to delete the CSS property.
 * Besides clicking on an already selected property row makes the property editable
 * and clicking 'Enter' updates the property with changes reflected in the
 * drawing.
 */
StyleDialog::StyleDialog(bool stylemode) :
    UI::Widget::Panel("/dialogs/style", SP_VERB_DIALOG_STYLE),
    _updating(false),
    _textNode(nullptr),
    _desktopTracker(),
    _stylemode(stylemode)
{
    g_debug("StyleDialog::StyleDialog");

    // Tree
    Inkscape::UI::Widget::IconRenderer * addRenderer = manage(
                new Inkscape::UI::Widget::IconRenderer() );
    addRenderer->add_icon("edit-delete");
    addRenderer->add_icon("list-add");
    addRenderer->add_icon("object-locked");
    
    _store = TreeStore::create(this);
    _modelfilter = Gtk::TreeModelFilter::create(_store);
    _modelfilter->set_visible_column(_mColumns._colVisible);
    _treeView.set_model(_modelfilter);
    _treeView.set_headers_visible(true);
    _treeView.enable_model_drag_source();
    _treeView.enable_model_drag_dest( Gdk::ACTION_MOVE );
    int addCol = _treeView.append_column("", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if ( col ) {
        col->add_attribute(addRenderer->property_icon(), _mColumns._colType);
    }
    _treeView.append_column("CSS Selector", _mColumns._colSelector);
    _treeView.set_expander_column(*(_treeView.get_column(1)));

    // Pack widgets
    _paned.set_orientation(Gtk::ORIENTATION_VERTICAL);
    _paned.pack1(_mainBox, Gtk::SHRINK);
    _mainBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    if (!_stylemode) {
        _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
        _scrolledWindow.add(_treeView);
        _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        create = manage( new Gtk::Button() );
        _styleButton(*create, "list-add", "Add a new CSS Selector");
        create->signal_clicked().connect(sigc::mem_fun(*this, &StyleDialog::_addSelector));

        del = manage( new Gtk::Button() );
        _styleButton(*del, "list-remove", "Remove a CSS Selector");
        del->signal_clicked().connect(sigc::mem_fun(*this, &StyleDialog::_delSelector));
        del->set_sensitive(false);
        _mainBox.pack_end(_buttonBox, Gtk::PACK_SHRINK);

        _buttonBox.pack_start(*create, Gtk::PACK_SHRINK);
        _buttonBox.pack_start(*del, Gtk::PACK_SHRINK);
    } else {
        _mainBox.pack_start(_treeView, Gtk::PACK_EXPAND_WIDGET);
    }
    _getContents()->pack_start(_paned, Gtk::PACK_EXPAND_WIDGET);


    // Signal handlers
    _treeView.signal_button_release_event().connect(   // Needs to be release, not press.
        sigc::mem_fun(*this,  &StyleDialog::_handleButtonEvent),
        false);

    _treeView.signal_button_release_event().connect_notify(
        sigc::mem_fun(*this, &StyleDialog::_buttonEventsSelectObjs),
        false);

    _treeView.signal_row_expanded().connect(sigc::mem_fun(*this, &StyleDialog::_rowExpand));

    _treeView.signal_row_collapsed().connect(sigc::mem_fun(*this, &StyleDialog::_rowCollapse));

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

    Inkscape::UI::Widget::IconRenderer *addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
    addRenderer->add_icon("edit-delete");
    addRenderer->signal_activated().connect(sigc::mem_fun(*this, &StyleDialog::onPropertyDelete));

    if (!_stylemode && !_store->children().empty()) {
        del->set_sensitive(true);
    }

    int addCol = _treeView.append_column("", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if (col) {
        col->add_attribute(addRenderer->property_visible(), _cssColumns.deleteButton);
        Gtk::Image *add_icon = Gtk::manage(sp_get_icon_image("list-add", Gtk::ICON_SIZE_SMALL_TOOLBAR));
        col->set_clickable(true);
        col->set_widget(*add_icon);
        add_icon->set_tooltip_text(_("Add a new style property"));
        add_icon->show();
        // This gets the GtkButton inside the GtkBox, inside the GtkAlignment, inside the GtkImage icon.
        auto button = add_icon->get_parent()->get_parent()->get_parent();
        // Assign the button event so that create happens BEFORE delete. If this code
        // isn't in this exact way, the onAttrDelete is called when the header lines are pressed.
        button->signal_button_release_event().connect(sigc::mem_fun(*this, &StyleDialog::onPropertyCreate), false);
    }

    Gtk::CellRendererText *renderer = Gtk::manage(new Gtk::CellRendererText());
    _treeView.set_reorderable(false);
    renderer->property_editable() = true;
    int nameColNum = _treeView.append_column("Property", *renderer) - 1;
    _propCol = _treeView.get_column(nameColNum);
    if (_propCol) {
        _propCol->add_attribute(renderer->property_text(), _cssColumns.label);
        _propCol->add_attribute(renderer->property_foreground_rgba(), _cssColumns.label_color);
    }
    renderer->signal_edited().connect(sigc::mem_fun(*this, &CssDialog::nameEdited));
    renderer = Gtk::manage(new Gtk::CellRendererText());
    renderer->property_editable() = true;
    int attrColNum = _treeView.append_column("Value", *renderer) - 1;
    _attrCol = _treeView.get_column(attrColNum);
    if (_attrCol) {
        _attrCol->add_attribute(renderer->property_text(), _cssColumns._styleAttrVal);
        _attrCol->add_attribute(renderer->property_foreground_rgba(), _cssColumns.attr_color);
    }
    renderer->signal_edited().connect(sigc::mem_fun(*this, &CssDialog::valueEdited));
    renderer = Gtk::manage(new Gtk::CellRendererText());
    renderer->property_editable() = true;

    // Set the initial sort column (and direction) to place real attributes at the top.
    _store->set_sort_column(_cssColumns.label, Gtk::SORT_ASCENDING);

    _getContents()->pack_start(*_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);

    css_reset_context(0);
    setDesktop(getDesktop());
}

/**
 * @brief StyleDialog::~StyleDialog
 * Class destructor
 */
StyleDialog::~StyleDialog()
{
    setDesktop(nullptr);
    _repr = nullptr;
    _message_changed_connection.disconnect();
    _message_context = nullptr;
    _message_stack = nullptr;
    _message_changed_connection.~connection();
}

void StyleDialog::_set_status_message(Inkscape::MessageType /*type*/, const gchar *message, GtkWidget *widget)
{
    if (widget) {
        gtk_label_set_markup(GTK_LABEL(widget), message ? message : "");
    }
}


/**
 * @brief StyleDialog::setDesktop
 * @param desktop
 * This function sets the 'desktop' for the CSS pane.
 */
void StyleDialog::setDesktop(SPDesktop* desktop)
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
    while(content.find("/*") != std::string::npos) {
        size_t start = content.find("/*");
        content.erase(start, (content.find("*/", start) - start) +2);
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
    std::cout << content << std::endl;
    std::cout << "aaaaaaaaaaaaaaaaaaaaaaaaaa" << std::endl;
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[}{]", content);

    // If text node is empty, return (avoids problem with negative below).
    if (tokens.size() == 0) {
        _updating = false;
        return;
    }
    std::vector<std::pair<Glib::ustring, bool>> expanderstatus;
    for (unsigned i = 0; i < tokens.size() - 1; i += 2) {
        Glib::ustring selector = tokens[i];
        REMOVE_SPACES(selector); // Remove leading/trailing spaces
        for (auto &row : _store->children()) {
            Glib::ustring selectorold = row[_mColumns._colSelector];
            if (selectorold == selector) {
                expanderstatus.emplace_back(selector, row[_mColumns._colExpand]);
            }
        }
    }
    _store->clear();

    for (unsigned i = 0; i < tokens.size()-1; i += 2) {

        Glib::ustring selector = tokens[i];
        REMOVE_SPACES(selector); // Remove leading/trailing spaces
        std::vector<Glib::ustring> tokensplus = Glib::Regex::split_simple("[,]+", selector);
        coltype colType = SELECTOR;
        for (auto tok : tokensplus) {
            REMOVE_SPACES(tok);
            if (tok.find(" ") != -1 || tok.erase(0, 1).find(".") != -1) {
                colType = UNHANDLED;
            }
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
        REMOVE_SPACES(properties);
        bool colExpand = false;
        for (auto rowstatus : expanderstatus) {
            if (selector == rowstatus.first) {
                colExpand = rowstatus.second;
            }
        }
        Gtk::TreeModel::Row row = *(_store->append());
        row[_mColumns._colSelector]   = selector;
        row[_mColumns._colExpand] = colExpand;
        row[_mColumns._colType] = colType;
        row[_mColumns._colObj]        = objVec;
        row[_mColumns._colProperties] = properties;
        row[_mColumns._colVisible] = true;
        if (colType == SELECTOR) {
            // Add as children, objects that match selector.
            for (auto &obj : objVec) {
                Gtk::TreeModel::Row childrow = *(_store->append(row->children()));
                childrow[_mColumns._colSelector] = "#" + Glib::ustring(obj->getId());
                childrow[_mColumns._colExpand] = false;
                childrow[_mColumns._colType] = OBJECT;
                childrow[_mColumns._colObj] = std::vector<SPObject *>(1, obj);
                childrow[_mColumns._colProperties] = ""; // Unused
                childrow[_mColumns._colVisible] = true; // Unused
            }
        }
    }
    _updating = false;
}

/**
 * @brief StyleDialog::setRepr
 *
 * Set the internal xml object that I'm working on right now.
 */
void StyleDialog::setRepr(Inkscape::XML::Node *repr)
{
    if (repr == _repr)
        return;
    if (_repr) {
        _store->clear();
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
    _repr = repr;
    if (repr) {
        Inkscape::GC::anchor(_repr);
        _repr->addListener(&css_repr_events, this);
        _repr->synthesizeEvents(&css_repr_events, this);
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
    std::map<Glib::ustring, Glib::ustring> ret;

    REMOVE_SPACES(style_string); // We'd use const, but we need to trip spaces
    std::vector<Glib::ustring> props = r_props->split(style_string);

                if (!found) {
                    // Update row
                    objVec.push_back(obj); // Adding to copy so need to update tree
                    row[_mColumns._colObj]      = objVec;
                    row[_mColumns._colSelector] = _getIdList( objVec );
                    row[_mColumns._colExpand] = true;
                    // Add child row
                    Gtk::TreeModel::Row childrow = *(_store->append(row->children()));
                    childrow[_mColumns._colSelector]   = "#" + Glib::ustring(obj->getId());
                    childrow[_mColumns._colType] = OBJECT;
                    childrow[_mColumns._colObj]        = std::vector<SPObject *>(1, obj);
                    childrow[_mColumns._colProperties] = "";  // Unused
                    childrow[_mColumns._colVisible] = true;  // Unused
                }
            }
        }

        else if (selector[0] == '.') {
            // 'class' selector... add value to class attribute of selected objects.

            // Get first class (split on white space or comma)
            std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[,\\s]+", selector);
            Glib::ustring className = tokens[0];
            className.erase(0,1);
            Inkscape::Selection* selection = getDesktop()->getSelection();
            std::vector<SPObject *> sel_obj(selection->objects().begin(), selection->objects().end());
            _insertClass(sel_obj, className);
            std::vector<SPObject *> objVec = _getObjVec(selector);
            ;
            for (auto &obj : sel_obj) {

                Glib::ustring id = (obj->getId() ? obj->getId() : "");
                bool found = false;
                for (auto &obj : objVec) {
                    if (id == obj->getId()) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    // Update row
                    objVec.push_back(obj); // Adding to copy so need to update tree
                    row[_mColumns._colObj] = objVec;
                    row[_mColumns._colExpand] = true;

                    // Update row
                    Gtk::TreeModel::Row childrow = *(_store->append(row->children()));
                    childrow[_mColumns._colSelector] = "#" + Glib::ustring(obj->getId());
                    childrow[_mColumns._colExpand] = false;
                    childrow[_mColumns._colType] = OBJECT;
                    childrow[_mColumns._colObj] = std::vector<SPObject *>(1, obj);
                    childrow[_mColumns._colProperties] = ""; // Unused
                    childrow[_mColumns._colVisible] = true; // Unused
                }
            }
        }

        if (pair.size() > 1) {
            ret[pair[0]] = pair[1];
        }
    }
    return ret;
}

/**
 * @brief StyleDialog::compileStyle
 *
 * Turn a vector map back into a style string.
 *
 */
Glib::ustring StyleDialog::compileStyle(std::map<Glib::ustring, Glib::ustring> props)
{
    auto ret = Glib::ustring("");
    for (auto const pair : props) {
        if (!pair.first.empty() && !pair.second.empty()) {
            ret += pair.first;
            ret += ":";
            ret += pair.second;
            ret += ";";
        }
    }
    return ret;
}


/**
 * @brief StyleDialog::onAttrChanged
 *
 * This is called when the XML has an updated attribute (we only care about style)
 */
void StyleDialog::onAttrChanged(Inkscape::XML::Node *repr, const gchar *name, const gchar *new_value)
{
    if (strcmp(name, "style") != 0)
        return;

    // Clear the list and return if the new_value is empty
    _store->clear();
    if (!new_value || new_value[0] == 0)
        return;

    // Get the object's style attribute and it's calculated properties
    SPDocument *document = this->_desktop->doc();
    SPObject *obj = document->getObjectByRepr(repr);
    // std::vector<SPIBase *> calc_prop = obj->style->properties();

    // Get a dictionary lookup of the style in the attribute
    std::map<Glib::ustring, Glib::ustring> attr_prop = parseStyle(new_value);

    for (auto iter : obj->style->properties()) {
        if (iter->style_src != SP_STYLE_SRC_UNSET) {
            if (attr_prop.count(iter->name)) {
                Gtk::TreeModel::Row row = *(_store->append());
                // Delete is available to attribute properties only in attr mode.
                row[_cssColumns.deleteButton] = iter->style_src == SP_STYLE_SRC_ATTRIBUTE;
                row[_cssColumns.label] = iter->name;
                row[_cssColumns._styleAttrVal] = attr_prop[iter->name];
                row[_cssColumns.deleteButton] = true;
/*             } else if (iter->style) {
                Gtk::TreeModel::Row row = *(_store->append());
                // Delete is available to attribute properties only in attr mode.
                row[_cssColumns.deleteButton] = iter->style_src == SP_STYLE_SRC_ATTRIBUTE;
                row[_cssColumns.label] = iter->name;
                row[_cssColumns._styleAttrVal] = iter->get_value();
                row[_cssColumns.label_color] = Gdk::RGBA("gray");
                row[_cssColumns.attr_color] = Gdk::RGBA("gray");
                row[_cssColumns.deleteButton] = false;
            */
            }
        }
    }
}

/*
 * Sets the CSSDialog status bar, depending on which attr is selected.
 */
void StyleDialog::css_reset_context(gint css)
{
    g_debug("StyleDialog::_selectObjects: %d, %d", eventX, eventY);

    getDesktop()->selection->clear();
    Gtk::TreeViewColumn *col = _treeView.get_column(1);
    Gtk::TreeModel::Path path;
    int x2 = 0;
    int y2 = 0;
    // To do: We should be able to do this via passing in row.
    if (_treeView.get_path_at_pos(eventX, eventY, path, col, x2, y2)) {
        if (col == _treeView.get_column(1)) {
            Gtk::TreeModel::iterator iter = _store->get_iter(path);
            if (iter) {
                Gtk::TreeModel::Row row = *iter;
                Gtk::TreeModel::Children children = row.children();
                if (children.empty() && !_stylemode) {
                    del->set_sensitive(true);
                }
                std::vector<SPObject *> objVec = row[_mColumns._colObj];

                for (auto obj : objVec) {
                    getDesktop()->selection->add(obj);
                }
            }
        }
    }
}

/**
 * @brief StyleDialog::setStyleProperty
 *
 * Set or delete a single property in the style attribute.
 */
bool StyleDialog::setStyleProperty(Glib::ustring name, Glib::ustring value)
{
    g_debug("StyleDialog::_addSelector: Entrance");

    // Store list of selected elements on desktop (not to be confused with selector).
    Inkscape::Selection* selection = getDesktop()->getSelection();
    std::vector<SPObject *> objVec( selection->objects().begin(),
                                    selection->objects().end() );

    // ==== Create popup dialog ====
    Gtk::Dialog *textDialogPtr =  new Gtk::Dialog();
    textDialogPtr->add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    textDialogPtr->add_button(_("Add"),    Gtk::RESPONSE_OK);

    Gtk::Entry *textEditPtr = manage ( new Gtk::Entry() );
    textEditPtr->signal_activate().connect(
        sigc::bind<Gtk::Dialog *>(sigc::mem_fun(*this, &StyleDialog::_closeDialog), textDialogPtr));
    textDialogPtr->get_content_area()->pack_start(*textEditPtr, Gtk::PACK_SHRINK);

    Gtk::Label *textLabelPtr = manage ( new Gtk::Label(
      _("Invalid entry: Not an id (#), class (.), or element CSS selector.")
    ) );
    textDialogPtr->get_content_area()->pack_start(*textLabelPtr, Gtk::PACK_SHRINK);

    /**
     * By default, the entrybox contains 'Class1' as text. However, if object(s)
     * is(are) selected and user clicks '+' at the bottom of dialog, the
     * entrybox will have the id(s) of the selected objects as text.
     */
    if (getDesktop()->getSelection()->isEmpty()) {
        textEditPtr->set_text(".Class1");
    } else {
        textEditPtr->set_text(_getIdList(objVec));
    }

    Gtk::Requisition sreq1, sreq2;
    textDialogPtr->get_preferred_size(sreq1, sreq2);
    int minWidth = 200;
    int minHeight = 100;
    minWidth  = (sreq2.width  > minWidth  ? sreq2.width  : minWidth );
    minHeight = (sreq2.height > minHeight ? sreq2.height : minHeight);
    textDialogPtr->set_size_request(minWidth, minHeight);
    textEditPtr->show();
    textLabelPtr->hide();
    textDialogPtr->show();


    // ==== Get response ====
    int result = -1;
    bool invalid = true;
    Glib::ustring selectorValue;
    bool handled = true;
    while (invalid) {
        result = textDialogPtr->run();
        if (result != Gtk::RESPONSE_OK) { // Cancel, close dialog, etc.
            textDialogPtr->hide();
            delete textDialogPtr;
            return;
        }
        /**
         * @brief selectorName
         * This string stores selector name. The text from entrybox is saved as name
         * for selector. If the entrybox is empty, the text (thus selectorName) is
         * set to ".Class1"
         */
        selectorValue = textEditPtr->get_text();
        Glib::ustring firstWord = selectorValue.substr(0, selectorValue.find_first_of(" >+~"));
        if (firstWord != selectorValue) {
            handled = false;
        }
        if (!_stylemode) {
            del->set_sensitive(true);
        }
        if (selectorValue[0] == '.' || selectorValue[0] == '#' || selectorValue[0] == '*' ||
            SPAttributeRelSVG::isSVGElement(selectorValue)) {
            invalid = false;
        } else {
            textLabelPtr->show();
        }
    } else if (properties.count(name)) {
        // Delete value
        properties.erase(name);
        updated = true;
    }

    if (updated) {
        auto new_styles = this->compileStyle(properties);
        this->_repr->setAttribute("style", new_styles, false);
        // this->setUndo(_("Delete style property"));
    }
    return updated;
}

/**
 * @brief StyleDialog::onPropertyDelete
 *
 * This function is a slot to signal_activated for '-' button panel.
 */
void StyleDialog::onPropertyDelete(Glib::ustring path)
{
    g_debug("StyleDialog::_delSelector");

    Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
    _treeView.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        if (!row.children().empty()) {
            return;
        }
        _updating = true;
        _store->erase(iter);
        _updating = false;
        _writeStyleElement();
        if (!_stylemode) {
            del->set_sensitive(false);
        }
    }
}

/**
 * @brief StyleDialog::onPropertyCreate
 * This function is a slot to signal_clicked for '+' button panel.
 */
bool StyleDialog::onPropertyCreate(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_RELEASE && event->button == 1 && this->_repr) {
        Gtk::TreeIter iter = _store->append();
        Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
        _treeView.set_cursor(path, *_propCol, true);
        grab_focus();
        return true;
    }
    return false;
}

/**
 * @brief CssDialog::onKeyPressed
 * @param event_description
 * @return
 * Send an undo message and mark this point for undo
 */
void CssDialog::setUndo(Glib::ustring const &event_description)
{
    SPDocument *document = this->_desktop->doc();
    DocumentUndo::done(document, SP_VERB_DIALOG_XML_EDITOR, event_description);
}

/**
 * @brief CssDialog::nameEdited
 * @param event
 * This function detects single or double click on a selector in any row. Clicking
 * on a selector selects the matching objects on the desktop. A double click will
 * in addition open the CSS dialog.
 */
void StyleDialog::_buttonEventsSelectObjs(GdkEventButton* event )
{
    g_debug("StyleDialog::_buttonEventsSelectObjs");
    _treeView.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    _updating = true;
    if (!_stylemode) {
        del->set_sensitive(true);
    }
    if (event->type == GDK_BUTTON_RELEASE && event->button == 1) {
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        _selectObjects(x, y);
    }
    _updating = false;
}


/**
 * @brief StyleDialog::_selectRow
 * This function selects the row in treeview corresponding to an object selected
 * in the drawing. If more than one row matches, the first is chosen.
 */
void CssDialog::nameEdited (const Glib::ustring& path, const Glib::ustring& name)
{
    g_debug("StyleDialog::_selectRow: updating: %s", (_updating ? "true" : "false"));
    if (!_stylemode) {
        del->set_sensitive(false);
    }
    if (_updating || !getDesktop()) return; // Avoid updating if we have set row via dialog.
    if (SP_ACTIVE_DESKTOP != getDesktop()) {
        std::cerr << "StyleDialog::_selectRow: SP_ACTIVE_DESKTOP != getDesktop()" << std::endl;
        return;
    }
    _treeView.get_selection()->unselect_all();
    Gtk::TreeModel::Children children = _store->children();
    Inkscape::Selection* selection = getDesktop()->getSelection();
    SPObject *obj = nullptr;
    if (!selection->isEmpty()) {
        obj = selection->objects().back();
    }
    for (auto row : children) {
        std::vector<SPObject *> objVec = row[_mColumns._colObj];
        if (obj) {
            for (auto & i : objVec) {
                if (obj->getId() == i->getId()) {
                    if (!_stylemode) {
                        _treeView.get_selection()->select(row);
                    }
                    row[_mColumns._colVisible] = true;
                } else if(_stylemode) {
                    row[_mColumns._colVisible] = false;
                }
            }
        }
        if (!name.empty()) {
            row[_cssColumns.label] = name;
        }
        this->setUndo(_("Rename CSS attribute"));
    }
    if (_stylemode) {
        _modelfilter->refilter();
    }
/*     if (_stylemode) {
        _store->foreach_iter(sigc::bind<std::vector<Gtk::TreeModel::Row> >(sigc::mem_fun(*this, &StyleDialog::_showStyleSelectors), toshow));
    } */
}
/* bool StyleDialog::_showStyleSelectors(const Gtk::TreeModel::iterator& iter, std::vector<Gtk::TreeModel::Row> toshow)
{
    Gtk::TreeModel::Row row = *iter;
    if (std::find(toshow.begin(), toshow.end(), row)!= toshow.end()) {
        std::cout << "fasdsfaasfasfasfasfasf" << std::endl;
    } else {

    }
    _store->erase(row);
    std::cout << "1111111111111111111" << std::endl;
    return true;
} */

/**
 * @brief CssDialog::valueEdited
 * @param event
 * @return
 * Called when the value is edited in the TreeView editable column
 */
void CssDialog::valueEdited (const Glib::ustring& path, const Glib::ustring& value)
{
    Gtk::TreeModel::Row row = *_store->get_iter(path);
    if(row && this->_repr) {
        Glib::ustring name = row[_cssColumns.label];
        if(name.empty()) return;
        setStyleProperty(name, value);
        if(!value.empty()) {
            row[_cssColumns._styleAttrVal] = value;
        }

        this->setUndo(_("Change attribute value"));
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
