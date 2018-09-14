/** @file
 * @brief A dialog for XML attributes
 */
/* Authors:
 *   Martin Owens
 *
 * Copyright (C) Martin Owens 2018 <doctormo@gmail.com>
 *
 * Released under GNU GPLv2 or later, read the file 'COPYING' for more information
 */

#include "attrdialog.h"

#include "verbs.h"
#include "selection.h"

#include "helper/icon-loader.h"
#include "ui/widget/addtoicon.h"

#include "xml/node-event-vector.h"
#include "xml/attribute-record.h"

static void on_attr_changed (Inkscape::XML::Node * repr,
                         const gchar * name,
                         const gchar * /*old_value*/,
                         const gchar * new_value,
                         bool /*is_interactive*/,
                         gpointer data)
{
    ATTR_DIALOG(data)->onAttrChanged(repr, name, new_value);
}
Inkscape::XML::NodeEventVector _repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    on_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Dialog {


/**
 * Constructor
 * A treeview whose each row corresponds to an XML attribute of a selected node
 * New attribute can be added by clicking '+' at bottom of the attr pane. '-'
 */
AttrDialog::AttrDialog():
    UI::Widget::Panel("/dialogs/attr", SP_VERB_DIALOG_CSS),
    _desktop(nullptr),
    _repr(nullptr)
{
    set_size_request(20, 15);
    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _treeView.set_headers_visible(true);
    _scrolledWindow.add(_treeView);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _store = Gtk::ListStore::create(_attrColumns);
    _treeView.set_model(_store);

    Inkscape::UI::Widget::AddToIcon * addRenderer = manage(new Inkscape::UI::Widget::AddToIcon());
    addRenderer->property_active() = false;

    int addCol = _treeView.append_column("", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if (col) {
        auto add_icon = Gtk::manage(sp_get_icon_image("list-add", GTK_ICON_SIZE_SMALL_TOOLBAR));
        col->add_attribute(addRenderer->property_active(), _attrColumns._colUnsetAttr);
        col->set_clickable(true);
        col->set_widget(*add_icon);
        add_icon->set_tooltip_text("Add a new attribute");
        add_icon->show();
        // This gets the GtkButton inside the GtkBox, inside the GtkAlignment, inside the GtkImage icon.
        auto button = add_icon->get_parent()->get_parent()->get_parent();
        // Assign the button event so that create happens BEFORE delete. If this code
        // isn't in this exact way, the onAttrDelete is called when the header lines are pressed.
        button->signal_button_release_event().connect(sigc::mem_fun(*this, &AttrDialog::onAttrCreate), false);
    }
    _treeView.signal_button_release_event().connect(sigc::mem_fun(*this, &AttrDialog::onAttrDelete));

    _nameRenderer = Gtk::manage(new Gtk::CellRendererText());
    _nameRenderer->property_editable() = true;
    _nameRenderer->property_placeholder_text().set_value("Attribute Name");
    _nameRenderer->signal_edited().connect(sigc::mem_fun(*this, &AttrDialog::nameEdited));
    int nameColNum = _treeView.append_column("Name", *_nameRenderer) - 1;
    _nameCol = _treeView.get_column(nameColNum);
    if (_nameCol) {
      _nameCol->add_attribute(_nameRenderer->property_text(), _attrColumns._attributeName);
    }

    _valueRenderer = Gtk::manage(new Gtk::CellRendererText());
    _valueRenderer->property_editable() = true;
    _valueRenderer->property_placeholder_text().set_value("Attribute Value");
    _valueRenderer->property_ellipsize().set_value(Pango::ELLIPSIZE_MIDDLE);
    _valueRenderer->signal_edited().connect(sigc::mem_fun(*this, &AttrDialog::valueEdited));
    int valueColNum = _treeView.append_column("Value", *_valueRenderer) - 1;
    _valueCol = _treeView.get_column(valueColNum);
    if (_valueCol) {
      _valueCol->add_attribute(_valueRenderer->property_text(), _attrColumns._attributeValue);
    }

    _getContents()->pack_start(_mainBox, Gtk::PACK_EXPAND_WIDGET);

    setDesktop(getDesktop());
}


/**
 * @brief AttrDialog::~AttrDialog
 * Class destructor
 */
AttrDialog::~AttrDialog()
{
    setDesktop(nullptr);
}


/**
 * @brief AttrDialog::setDesktop
 * @param desktop
 * This function sets the 'desktop' for the CSS pane.
 */
void AttrDialog::setDesktop(SPDesktop* desktop)
{
    _desktop = desktop;
}

/**
 * @brief AttrDialog::setRepr
 * Set the internal xml object that I'm working on right now.
 */
void AttrDialog::setRepr(Inkscape::XML::Node * repr)
{
    if ( repr == _repr ) return;
    if (_repr) {
        _store->clear();
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
    }
    _repr = repr;
    if (repr) {
        Inkscape::GC::anchor(_repr); 
        _repr->addListener(&_repr_events, this);
        _repr->synthesizeEvents(&_repr_events, this);
    }
}

/**
 * @brief AttrDialog::onAttrChanged
 * This is called when the XML has an updated attribute
 */
void AttrDialog::onAttrChanged(Inkscape::XML::Node *repr, const gchar * name, const gchar * new_value)
{
    for(auto iter: this->_store->children())
    {
        Gtk::TreeModel::Row row = *iter;
        Glib::ustring col_name = row[_attrColumns._attributeName];
        if(name == col_name) {
            if(new_value) {
                row[_attrColumns._attributeValue] = new_value;
                new_value = nullptr; // Don't make a new one
            } else {
                _store->erase(iter);
            }
        }
    }
    if(new_value) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_attrColumns._attributeName] = name;
        row[_attrColumns._attributeValue] = new_value;
    }
}

/**
 * @brief AttrDialog::onAttrCreate
 * This function is a slot to signal_clicked for '+' button panel.
 */
bool AttrDialog::onAttrCreate(GdkEventButton *event)
{
    if(event->type == GDK_BUTTON_RELEASE && event->button == 1 && this->_repr) {
        Gtk::TreeIter iter = _store->append();
        Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
        _treeView.set_cursor(path, *_nameCol, true);
        grab_focus();
        return true;
    }
    return false;
}

/**
 * @brief AttrDialog::onAttrDelete
 * @param event
 * @return
 * Delete the attribute from the xml
 */
bool AttrDialog::onAttrDelete(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_RELEASE && event->button == 1 && this->_repr) {
        if(!this->_treeView.has_focus()) {
            // If the treeView doesn't have focus, then it's probably the edit
            // input box or some other widget that propergates to the same click
            return false;
        }
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn *col = nullptr;
        int x2, y2 = 0;
        if (_treeView.get_path_at_pos(x, y, path, col, x2, y2)) {
            Gtk::TreeModel::Row row = *_store->get_iter(path);
            if (col == _treeView.get_column(0) && row) {
                Glib::ustring name = row[_attrColumns._attributeName];
                this->_repr->setAttribute(name.c_str(), nullptr, false);
                // Return true to prevent propergation
                return true;
            }
        }
    }
    return false;
}


/**
 * @brief AttrDialog::nameEdited
 * @param event
 * @return
 * Called when the name is edited in the TreeView editable column
 */
void AttrDialog::nameEdited (const Glib::ustring& path, const Glib::ustring& name)
{

    Gtk::TreeModel::Row row = *_store->get_iter(path);
    if(row) {
        Glib::ustring old_name = row[_attrColumns._attributeName];
        Glib::ustring value = row[_attrColumns._attributeValue];
        if(!old_name.empty()) {
            // Remove named value
            _repr->setAttribute(old_name, nullptr, false);
            _repr->setAttribute(name, value, false);
        } else {
            // Move to editing value, we set the name as a temporary store value
            row[_attrColumns._attributeName] = name;
            Gtk::TreeModel::Path _path = (Gtk::TreeModel::Path)row;
            _treeView.set_cursor(_path, *_valueCol, true);
            grab_focus();
        }
    }
}

/**
 * @brief AttrDialog::valueEdited
 * @param event
 * @return
 * Called when the value is edited in the TreeView editable column
 */
void AttrDialog::valueEdited (const Glib::ustring& path, const Glib::ustring& value)
{

    Gtk::TreeModel::Row row = *_store->get_iter(path);
    if(row) {
        Glib::ustring name = row[_attrColumns._attributeName];
        if(name.empty()) return;
        _repr->setAttribute(name, value, false);
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
