// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "document-undo.h"
#include "message-context.h"
#include "message-stack.h"
#include "style.h"
#include "ui/icon-loader.h"
#include "ui/widget/iconrenderer.h"

#include "xml/node-event-vector.h"
#include "xml/attribute-record.h"

#include <gdk/gdkkeysyms.h>
#include <glibmm/i18n.h>

/**
 * Return true if `node` is a text or comment node
 */
static bool is_text_or_comment_node(Inkscape::XML::Node const &node)
{
    switch (node.type()) {
      case Inkscape::XML::NodeType::TEXT_NODE:
      case Inkscape::XML::NodeType::COMMENT_NODE:
            return true;
        default:
            return false;
    }
}

static void on_attr_changed (Inkscape::XML::Node * repr,
                         const gchar * name,
                         const gchar * /*old_value*/,
                         const gchar * new_value,
                         bool /*is_interactive*/,
                         gpointer data)
{
    ATTR_DIALOG(data)->onAttrChanged(repr, name, new_value);
}

static void on_content_changed (Inkscape::XML::Node * repr,
                                gchar const * oldcontent,
                                gchar const * newcontent,
                                gpointer data)
{
    auto self = ATTR_DIALOG(data);
    auto buffer = self->_content_tv->get_buffer();
    if (!buffer->get_modified()) {
        const char *c = repr->content();
        buffer->set_text(c ? c : "");
    }
    buffer->set_modified(false);
}

Inkscape::XML::NodeEventVector _repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    on_attr_changed,
    on_content_changed, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Dialog {

static gboolean key_callback(GtkWidget *widget, GdkEventKey *event, AttrDialog *attrdialog);
/**
 * Constructor
 * A treeview whose each row corresponds to an XML attribute of a selected node
 * New attribute can be added by clicking '+' at bottom of the attr pane. '-'
 */
AttrDialog::AttrDialog()
    : DialogBase("/dialogs/attr", SP_VERB_DIALOG_ATTR_XML)
    , _desktop(nullptr)
    , _repr(nullptr)
    , _mainBox(Gtk::ORIENTATION_VERTICAL)
    , status_box(Gtk::ORIENTATION_HORIZONTAL)
{
    set_size_request(20, 15);
    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);

    // For text and comment nodes
    _content_tv = Gtk::manage(new Gtk::TextView());
    _content_tv->show();
    _content_tv->set_wrap_mode(Gtk::WrapMode::WRAP_CHAR);
    _content_tv->set_monospace(true);
    _content_tv->set_border_width(4);
    _content_tv->set_buffer(Gtk::TextBuffer::create());
    _content_tv->get_buffer()->signal_end_user_action().connect([this]() {
        if (_repr) {
            _repr->setContent(_content_tv->get_buffer()->get_text().c_str());
            setUndo(_("Type text"));
        }
    });
    _content_sw = Gtk::manage(new Gtk::ScrolledWindow());
    _content_sw->hide();
    _content_sw->set_no_show_all();
    _content_sw->add(*_content_tv);
    _mainBox.pack_start(*_content_sw);

    // For element nodes
    _treeView.set_headers_visible(true);
    _treeView.set_hover_selection(true);
    _treeView.set_activate_on_single_click(true);
    _treeView.set_can_focus(false);
    _scrolledWindow.add(_treeView);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _store = Gtk::ListStore::create(_attrColumns);
    _treeView.set_model(_store);

    Inkscape::UI::Widget::IconRenderer * addRenderer = manage(new Inkscape::UI::Widget::IconRenderer());
    addRenderer->add_icon("edit-delete");

    _treeView.append_column("", *addRenderer);
    Gtk::TreeViewColumn *col = _treeView.get_column(0);
    if (col) {
        auto add_icon = Gtk::manage(sp_get_icon_image("list-add", Gtk::ICON_SIZE_SMALL_TOOLBAR));
        col->set_clickable(true);
        col->set_widget(*add_icon);
        add_icon->set_tooltip_text(_("Add a new attribute"));
        add_icon->show();
        auto button = add_icon->get_parent()->get_parent()->get_parent();
        // Assign the button event so that create happens BEFORE delete. If this code
        // isn't in this exact way, the onAttrDelete is called when the header lines are pressed.
        button->signal_button_release_event().connect(sigc::mem_fun(*this, &AttrDialog::onAttrCreate), false);
    }
    addRenderer->signal_activated().connect(sigc::mem_fun(*this, &AttrDialog::onAttrDelete));
    _treeView.signal_key_press_event().connect(sigc::mem_fun(*this, &AttrDialog::onKeyPressed));
    _treeView.set_search_column(-1);

    _nameRenderer = Gtk::manage(new Gtk::CellRendererText());
    _nameRenderer->property_editable() = true;
    _nameRenderer->property_placeholder_text().set_value(_("Attribute Name"));
    _nameRenderer->signal_edited().connect(sigc::mem_fun(*this, &AttrDialog::nameEdited));
    _nameRenderer->signal_editing_started().connect(sigc::mem_fun(*this, &AttrDialog::startNameEdit));
    _treeView.append_column(_("Name"), *_nameRenderer);
    _nameCol = _treeView.get_column(1);
    if (_nameCol) {
        _nameCol->set_resizable(true);
        _nameCol->add_attribute(_nameRenderer->property_text(), _attrColumns._attributeName);
    }
    status.set_halign(Gtk::ALIGN_START);
    status.set_valign(Gtk::ALIGN_CENTER);
    status.set_size_request(1, -1);
    status.set_markup("");
    status.set_line_wrap(true);
    status.get_style_context()->add_class("inksmall");
    status_box.pack_start(status, TRUE, TRUE, 0);
    pack_end(status_box, false, false, 2);

    _message_stack = std::make_shared<Inkscape::MessageStack>();
    _message_context = std::unique_ptr<Inkscape::MessageContext>(new Inkscape::MessageContext(_message_stack));
    _message_changed_connection =
        _message_stack->connectChanged(sigc::bind(sigc::ptr_fun(_set_status_message), GTK_WIDGET(status.gobj())));

    _valueRenderer = Gtk::manage(new Gtk::CellRendererText());
    _valueRenderer->property_editable() = true;
    _valueRenderer->property_placeholder_text().set_value(_("Attribute Value"));
    _valueRenderer->property_ellipsize().set_value(Pango::ELLIPSIZE_END);
    _valueRenderer->signal_edited().connect(sigc::mem_fun(*this, &AttrDialog::valueEdited));
    _valueRenderer->signal_editing_started().connect(sigc::mem_fun(*this, &AttrDialog::startValueEdit));
    _treeView.append_column(_("Value"), *_valueRenderer);
    _valueCol = _treeView.get_column(2);
    if (_valueCol) {
        _valueCol->add_attribute(_valueRenderer->property_text(), _attrColumns._attributeValueRender);
    }
    _popover = Gtk::manage(new Gtk::Popover());
    Gtk::Box *vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    _textview = Gtk::manage(new Gtk::TextView());
    _textview->set_wrap_mode(Gtk::WrapMode::WRAP_CHAR);
    _textview->set_editable(true);
    _textview->set_monospace(true);
    _textview->set_border_width(6);
    _textview->signal_map().connect(sigc::mem_fun(*this, &AttrDialog::textViewMap));
    Glib::RefPtr<Gtk::TextBuffer> textbuffer = Gtk::TextBuffer::create();
    textbuffer->set_text("");
    _textview->set_buffer(textbuffer);
    _scrolled_text_view.add(*_textview);
    _scrolled_text_view.set_max_content_height(450);
    _scrolled_text_view.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _scrolled_text_view.set_propagate_natural_width(true);
    Gtk::Label *helpreturn = Gtk::manage(new Gtk::Label(_("Shift+Return for a new line")));
    helpreturn->get_style_context()->add_class("inksmall");
    Gtk::Button *apply = Gtk::manage(new Gtk::Button());
    Gtk::Image *icon = Gtk::manage(sp_get_icon_image("on-outline", 26));
    apply->set_relief(Gtk::RELIEF_NONE);
    icon->show();
    apply->add(*icon);
    apply->signal_clicked().connect(sigc::mem_fun(*this, &AttrDialog::valueEditedPop));
    Gtk::Button *cancel = Gtk::manage(new Gtk::Button());
    icon = Gtk::manage(sp_get_icon_image("off-outline", 26));
    cancel->set_relief(Gtk::RELIEF_NONE);
    icon->show();
    cancel->add(*icon);
    cancel->signal_clicked().connect(sigc::mem_fun(*this, &AttrDialog::valueCanceledPop));
    hbox->pack_end(*apply, Gtk::PACK_SHRINK, 3);
    hbox->pack_end(*cancel, Gtk::PACK_SHRINK, 3);
    hbox->pack_end(*helpreturn, Gtk::PACK_SHRINK, 3);
    vbox->pack_start(_scrolled_text_view, Gtk::PACK_EXPAND_WIDGET, 3);
    vbox->pack_start(*hbox, Gtk::PACK_EXPAND_WIDGET, 3);
    _popover->add(*vbox);
    _popover->show();
    _popover->set_relative_to(_treeView);
    _popover->set_position(Gtk::PositionType::POS_BOTTOM);
    _popover->signal_closed().connect(sigc::mem_fun(*this, &AttrDialog::popClosed));
    _popover->get_style_context()->add_class("attrpop");
    attr_reset_context(0);
    pack_start(_mainBox, Gtk::PACK_EXPAND_WIDGET);
    // I couldent get the signal go well not using C way signals
    g_signal_connect(GTK_WIDGET(_popover->gobj()), "key-press-event", G_CALLBACK(key_callback), this);
    _popover->hide();
    _updating = false;
}

void AttrDialog::textViewMap()
{
    auto vscroll = _scrolled_text_view.get_vadjustment();
    int height = vscroll->get_upper() + 12; // padding 6+6
    if (height < 450) {
        _scrolled_text_view.set_min_content_height(height);
        vscroll->set_value(vscroll->get_lower());
    } else {
        _scrolled_text_view.set_min_content_height(450);
    }
}

gboolean sp_show_pop_map(gpointer data)
{
    AttrDialog *attrdialog = reinterpret_cast<AttrDialog *>(data);
    attrdialog->textViewMap();
    return FALSE;
}

static gboolean key_callback(GtkWidget *widget, GdkEventKey *event, AttrDialog *attrdialog)
{
    switch (event->keyval) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter: {
            if (attrdialog->_popover->is_visible()) {
                if (!(event->state & GDK_SHIFT_MASK)) {
                    attrdialog->valueEditedPop();
                    attrdialog->_popover->hide();
                    return true;
                } else {
                    g_timeout_add(50, &sp_show_pop_map, attrdialog);
                }
            }
        } break;
    }
    return false;
}

/**
 * Prepare value string suitable for display in a Gtk::CellRendererText
 *
 * Value is truncated at the first new line character (if any) and a visual indicator and ellipsis is added.
 * Overall length is limited as well to prevent performance degradation for very long values.
 *
 * @param value Raw attribute value as UTF-8 encoded string
 * @return Single-line string with fixed maximum length
 */
static Glib::ustring prepare_rendervalue(const char *value)
{
    constexpr int MAX_LENGTH = 500; // maximum length of string before it's truncated for performance reasons
                                    // ~400 characters fit horizontally on a WQHD display, so 500 should be plenty

    Glib::ustring renderval;

    // truncate to MAX_LENGTH
    if (g_utf8_strlen(value, -1) > MAX_LENGTH) {
        renderval = Glib::ustring(value, MAX_LENGTH) + "…";
    } else {
        renderval = value;
    }

    // truncate at first newline (if present) and add a visual indicator
    auto ind = renderval.find('\n');
    if (ind != Glib::ustring::npos) {
        renderval.replace(ind, Glib::ustring::npos, " ⏎ …");
    }

    return renderval;
}


/**
 * @brief AttrDialog::~AttrDialog
 * Class destructor
 */
AttrDialog::~AttrDialog()
{
    _message_changed_connection.disconnect();
    _message_context = nullptr;
    _message_stack = nullptr;
}

void AttrDialog::startNameEdit(Gtk::CellEditable *cell, const Glib::ustring &path)
{
    Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(cell);
    entry->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &AttrDialog::onNameKeyPressed), entry));
}


gboolean sp_show_attr_pop(gpointer data)
{
    AttrDialog *attrdialog = reinterpret_cast<AttrDialog *>(data);
    attrdialog->_popover->show_all();

    return FALSE;
}

gboolean sp_close_entry(gpointer data)
{
    Gtk::CellEditable *cell = reinterpret_cast<Gtk::CellEditable *>(data);
    if (cell) {
        cell->property_editing_canceled() = true;
        cell->remove_widget();
    }
    return FALSE;
}

void AttrDialog::startValueEdit(Gtk::CellEditable *cell, const Glib::ustring &path)
{
    Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(cell);
    int width = 0;
    int height = 0;
    int colwidth = _valueCol->get_width();
    _textview->set_size_request(510, -1);
    _popover->set_size_request(520, -1);
    valuepath = path;
    entry->get_layout()->get_pixel_size(width, height);
    Gtk::TreeIter iter = *_store->get_iter(path);
    Gtk::TreeModel::Row row = *iter;
    if (row && this->_repr) {
        Glib::ustring name = row[_attrColumns._attributeName];
        if (row[_attrColumns._attributeValue] != row[_attrColumns._attributeValueRender] || colwidth - 10 < width) {
            valueediting = entry->get_text();
            Gdk::Rectangle rect;
            _treeView.get_cell_area((Gtk::TreeModel::Path)iter, *_valueCol, rect);
            if (_popover->get_position() == Gtk::PositionType::POS_BOTTOM) {
                rect.set_y(rect.get_y() + 20);
            }
            _popover->set_pointing_to(rect);
            Glib::RefPtr<Gtk::TextBuffer> textbuffer = Gtk::TextBuffer::create();
            textbuffer->set_text(row[_attrColumns._attributeValue]);
            _textview->set_buffer(textbuffer);
            g_timeout_add(50, &sp_close_entry, cell);
            g_timeout_add(50, &sp_show_attr_pop, this);
        } else {
            entry->signal_key_press_event().connect(
                sigc::bind(sigc::mem_fun(*this, &AttrDialog::onValueKeyPressed), entry));
        }
    }
}

void AttrDialog::popClosed()
{
    Glib::RefPtr<Gtk::TextBuffer> textbuffer = Gtk::TextBuffer::create();
    textbuffer->set_text("");
    _textview->set_buffer(textbuffer);
    _scrolled_text_view.set_min_content_height(20);
}

/**
 * @brief AttrDialog::update
 * @param desktop
 * This function sets the 'desktop' for the CSS pane.
 */
void AttrDialog::update()
{
    if (!_app) {
        std::cerr << "AttrDialog::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();

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
        _repr = nullptr;
    }
    _repr = repr;
    if (repr) {
        Inkscape::GC::anchor(_repr);
        _repr->addListener(&_repr_events, this);
        _repr->synthesizeEvents(&_repr_events, this);

        // show either attributes or content
        bool show_content = is_text_or_comment_node(*_repr);
        _scrolledWindow.set_visible(!show_content);
        _content_sw->set_visible(show_content);
    }
}

void AttrDialog::setUndo(Glib::ustring const &event_description)
{
    SPDocument *document = this->_desktop->doc();
    DocumentUndo::done(document, SP_VERB_DIALOG_XML_EDITOR, event_description);
}

void AttrDialog::_set_status_message(Inkscape::MessageType /*type*/, const gchar *message, GtkWidget *widget)
{
    if (widget) {
        gtk_label_set_markup(GTK_LABEL(widget), message ? message : "");
    }
}

/**
 * Sets the AttrDialog status bar, depending on which attr is selected.
 */
void AttrDialog::attr_reset_context(gint attr)
{
    if (attr == 0) {
        _message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> attribute to edit."));
    } else {
        const gchar *name = g_quark_to_string(attr);
        _message_context->setF(
            Inkscape::NORMAL_MESSAGE,
            _("Attribute <b>%s</b> selected. Press <b>Ctrl+Enter</b> when done editing to commit changes."), name);
    }
}

/**
 * @brief AttrDialog::onAttrChanged
 * This is called when the XML has an updated attribute
 */
void AttrDialog::onAttrChanged(Inkscape::XML::Node *repr, const gchar * name, const gchar * new_value)
{
    if (_updating) {
        return;
    }
    Glib::ustring renderval;
    if (new_value) {
        renderval = prepare_rendervalue(new_value);
    }
    for(auto iter: this->_store->children())
    {
        Gtk::TreeModel::Row row = *iter;
        Glib::ustring col_name = row[_attrColumns._attributeName];
        if(name == col_name) {
            if(new_value) {
                row[_attrColumns._attributeValue] = new_value;
                row[_attrColumns._attributeValueRender] = renderval;
                new_value = nullptr; // Don't make a new one
            } else {
                _store->erase(iter);
            }
            break;
        }
    }
    if (new_value) {
        Gtk::TreeModel::Row row = *(_store->prepend());
        row[_attrColumns._attributeName] = name;
        row[_attrColumns._attributeValue] = new_value;
        row[_attrColumns._attributeValueRender] = renderval;
    }
}

/**
 * @brief AttrDialog::onAttrCreate
 * This function is a slot to signal_clicked for '+' button panel.
 */
bool AttrDialog::onAttrCreate(GdkEventButton *event)
{
    if(event->type == GDK_BUTTON_RELEASE && event->button == 1 && this->_repr) {
        Gtk::TreeIter iter = _store->prepend();
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
 * @return true
 * Delete the attribute from the xml
 */
void AttrDialog::onAttrDelete(Glib::ustring path)
{
    Gtk::TreeModel::Row row = *_store->get_iter(path);
    if (row) {
        Glib::ustring name = row[_attrColumns._attributeName];
        {
            this->_store->erase(row);
            this->_repr->removeAttribute(name);
            this->setUndo(_("Delete attribute"));
        }
    }
}

/**
 * @brief AttrDialog::onKeyPressed
 * @param event
 * @return true
 * Delete or create elements based on key presses
 */
bool AttrDialog::onKeyPressed(GdkEventKey *event)
{
    bool ret = false;
    if(this->_repr) {
        auto selection = this->_treeView.get_selection();
        Gtk::TreeModel::Row row = *(selection->get_selected());
        Gtk::TreeIter iter = *(selection->get_selected());
        switch (event->keyval)
        {
            case GDK_KEY_Delete:
            case GDK_KEY_KP_Delete: {
                // Create new attribute (repeat code, fold into above event!)
                Glib::ustring name = row[_attrColumns._attributeName];
                {
                    this->_store->erase(row);
                    this->_repr->removeAttribute(name);
                    this->setUndo(_("Delete attribute"));
                }
                ret = true;
            } break;
            case GDK_KEY_plus:
            case GDK_KEY_Insert:
              {
                // Create new attribute (repeat code, fold into above event!)
                Gtk::TreeIter iter = this->_store->prepend();
                Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
                this->_treeView.set_cursor(path, *this->_nameCol, true);
                grab_focus();
                ret = true;
            } break;
            case GDK_KEY_Return:
            case GDK_KEY_KP_Enter: {
                if (_popover->is_visible()) {
                    if (!(event->state & GDK_SHIFT_MASK)) {
                        valueEditedPop();
                        _popover->hide();
                        ret = true;
                    }
                }
            } break;
        }
    }
    return ret;
}

bool AttrDialog::onNameKeyPressed(GdkEventKey *event, Gtk::Entry *entry)
{
    g_debug("StyleDialog::_onNameKeyPressed");
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


bool AttrDialog::onValueKeyPressed(GdkEventKey *event, Gtk::Entry *entry)
{
    g_debug("StyleDialog::_onValueKeyPressed");
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

gboolean sp_attrdialog_store_move_to_next(gpointer data)
{
    AttrDialog *attrdialog = reinterpret_cast<AttrDialog *>(data);
    auto selection = attrdialog->_treeView.get_selection();
    Gtk::TreeIter iter = *(selection->get_selected());
    Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
    Gtk::TreeViewColumn *focus_column;
    attrdialog->_treeView.get_cursor(path, focus_column);
    if (path == attrdialog->_modelpath && focus_column == attrdialog->_treeView.get_column(1)) {
        attrdialog->_treeView.set_cursor(attrdialog->_modelpath, *attrdialog->_valueCol, true);
    }
    return FALSE;
}

/**
 *
 *
 * @brief AttrDialog::nameEdited
 * @param event
 * @return
 * Called when the name is edited in the TreeView editable column
 */
void AttrDialog::nameEdited (const Glib::ustring& path, const Glib::ustring& name)
{
    Gtk::TreeIter iter = *_store->get_iter(path);
    _modelpath = (Gtk::TreeModel::Path)iter;
    Gtk::TreeModel::Row row = *iter;
    if(row && this->_repr) {
        Glib::ustring old_name = row[_attrColumns._attributeName];
        if (old_name == name) {
            g_timeout_add(50, &sp_attrdialog_store_move_to_next, this);
            grab_focus();
            return;
        }
        // Do not allow empty name (this would delete the attribute)
        if (name.empty()) {
            return;
        }
        // Do not allow duplicate names
        const auto children = _store->children();
        for (const auto &child : children) {
            if (name == child[_attrColumns._attributeName]) {
                return;
            }
        }
        if(std::any_of(name.begin(), name.end(), isspace)) {
            return;
        }
        // Copy old value and remove old name
        Glib::ustring value;
        if (!old_name.empty()) {
            value = row[_attrColumns._attributeValue];
            _updating = true;
            _repr->removeAttribute(old_name);
            _updating = false;
        }

        // Do the actual renaming and set new value
        row[_attrColumns._attributeName] = name;
        grab_focus();
        _updating = true;
        _repr->setAttributeOrRemoveIfEmpty(name, value); // use char * overload (allows empty attribute values)
        _updating = false;
        g_timeout_add(50, &sp_attrdialog_store_move_to_next, this);
        this->setUndo(_("Rename attribute"));
    }
}

void AttrDialog::valueEditedPop()
{
    Glib::ustring value = _textview->get_buffer()->get_text();
    valueEdited(valuepath, value);
    valueediting = "";
    _popover->hide();
}

void AttrDialog::valueCanceledPop()
{
    if (!valueediting.empty()) {
        Glib::RefPtr<Gtk::TextBuffer> textbuffer = Gtk::TextBuffer::create();
        textbuffer->set_text(valueediting);
        _textview->set_buffer(textbuffer);
    }
    _popover->hide();
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
    if(row && this->_repr) {
        Glib::ustring name = row[_attrColumns._attributeName];
        Glib::ustring old_value = row[_attrColumns._attributeValue];
        if (old_value == value) {
            return;
        }
        if(name.empty()) return;
        {
            _repr->setAttributeOrRemoveIfEmpty(name, value);
        }
        if(!value.empty()) {
            row[_attrColumns._attributeValue] = value;
            Glib::ustring renderval = prepare_rendervalue(value.c_str());
            row[_attrColumns._attributeValueRender] = renderval;
        }
        Inkscape::Selection *selection = _desktop->getSelection();
        SPObject *obj = nullptr;
        if (selection->objects().size() == 1) {
            obj = selection->objects().back();

            obj->style->readFromObject(obj);
            obj->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
        this->setUndo(_("Change attribute value"));
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
