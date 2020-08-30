// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gradient vector widget
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <gtkmm/treeview.h>
#include <vector>

#include "document-undo.h"
#include "document.h"
#include "gradient-chemistry.h"
#include "id-clash.h"
#include "inkscape.h"
#include "preferences.h"
#include "verbs.h"

#include "object/sp-defs.h"
#include "style.h"

#include "helper/action.h"
#include "ui/icon-loader.h"

#include "ui/icon-names.h"

#include "ui/widget/gradient-vector-selector.h"

namespace Inkscape {
namespace UI {
namespace Widget {

void GradientSelector::style_button(Gtk::Button *btn, char const *iconName)
{
    GtkWidget *child = sp_get_icon_image(iconName, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(child);
    btn->add(*manage(Glib::wrap(child)));
    btn->set_relief(Gtk::RELIEF_NONE);
}

GradientSelector::GradientSelector()
    : _blocked(false)
    , _mode(MODE_LINEAR)
    , _gradientUnits(SP_GRADIENT_UNITS_USERSPACEONUSE)
    , _gradientSpread(SP_GRADIENT_SPREAD_PAD)
{
    set_orientation(Gtk::ORIENTATION_VERTICAL);

    /* Vectors */
    _vectors = Gtk::manage(new GradientVectorSelector(nullptr, nullptr));
    _store = _vectors->get_store();
    _columns = _vectors->get_columns();

    _treeview = Gtk::manage(new Gtk::TreeView());
    _treeview->set_model(_store);
    _treeview->set_headers_clickable(true);
    _treeview->set_search_column(1);
    _treeview->set_vexpand();
    _icon_renderer = Gtk::manage(new Gtk::CellRendererPixbuf());
    _text_renderer = Gtk::manage(new Gtk::CellRendererText());

    _treeview->append_column(_("Gradient"), *_icon_renderer);
    auto icon_column = _treeview->get_column(0);
    icon_column->add_attribute(_icon_renderer->property_pixbuf(), _columns->pixbuf);
    icon_column->set_sort_column(_columns->color);
    icon_column->set_clickable(true);

    _treeview->append_column(_("Name"), *_text_renderer);
    auto name_column = _treeview->get_column(1);
    _text_renderer->property_editable() = true;
    name_column->add_attribute(_text_renderer->property_text(), _columns->name);
    name_column->set_min_width(180);
    name_column->set_clickable(true);
    name_column->set_resizable(true);

    _treeview->append_column("#", _columns->refcount);
    auto count_column = _treeview->get_column(2);
    count_column->set_clickable(true);
    count_column->set_resizable(true);

    _treeview->signal_key_press_event().connect(sigc::mem_fun(this, &GradientSelector::onKeyPressEvent), false);

    _treeview->show();

    icon_column->signal_clicked().connect(sigc::mem_fun(this, &GradientSelector::onTreeColorColClick));
    name_column->signal_clicked().connect(sigc::mem_fun(this, &GradientSelector::onTreeNameColClick));
    count_column->signal_clicked().connect(sigc::mem_fun(this, &GradientSelector::onTreeCountColClick));

    auto tree_select_connection = _treeview->get_selection()->signal_changed().connect(sigc::mem_fun(this, &GradientSelector::onTreeSelection));
    _vectors->set_tree_select_connection(tree_select_connection);
    _text_renderer->signal_edited().connect(sigc::mem_fun(this, &GradientSelector::onGradientRename));

    _scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
    _scrolled_window->add(*_treeview);
    _scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _scrolled_window->set_shadow_type(Gtk::SHADOW_IN);
    _scrolled_window->set_size_request(0, 180);
    _scrolled_window->set_hexpand();
    _scrolled_window->show();

    pack_start(*_scrolled_window, true, true, 4);


    /* Create box for buttons */
    auto hb = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
    hb->set_homogeneous(false);
    pack_start(*hb, false, false, 0);

    _add = Gtk::manage(new Gtk::Button());
    style_button(_add, INKSCAPE_ICON("list-add"));

    _nonsolid.push_back(_add);
    hb->pack_start(*_add, false, false, 0);

    _add->signal_clicked().connect(sigc::mem_fun(this, &GradientSelector::add_vector_clicked));
    _add->set_sensitive(false);
    _add->set_relief(Gtk::RELIEF_NONE);
    _add->set_tooltip_text(_("Create a duplicate gradient"));

    _edit = Gtk::manage(new Gtk::Button());
    style_button(_edit, INKSCAPE_ICON("edit"));

    _nonsolid.push_back(_edit);
    hb->pack_start(*_edit, false, false, 0);
    _edit->signal_clicked().connect(sigc::mem_fun(this, &GradientSelector::edit_vector_clicked));
    _edit->set_sensitive(false);
    _edit->set_relief(Gtk::RELIEF_NONE);
    _edit->set_tooltip_text(_("Edit gradient"));

    _del = Gtk::manage(new Gtk::Button());
    style_button(_del, INKSCAPE_ICON("list-remove"));

    _swatch_widgets.push_back(_del);
    hb->pack_start(*_del, false, false, 0);
    _del->signal_clicked().connect(sigc::mem_fun(this, &GradientSelector::delete_vector_clicked));
    _del->set_sensitive(false);
    _del->set_relief(Gtk::RELIEF_NONE);
    _del->set_tooltip_text(_("Delete swatch"));

    hb->show_all();
}

void GradientSelector::setSpread(SPGradientSpread spread)
{
    _gradientSpread = spread;
    // gtk_combo_box_set_active (GTK_COMBO_BOX(this->spread), gradientSpread);
}

void GradientSelector::setMode(SelectorMode mode)
{
    if (mode != _mode) {
        _mode = mode;
        if (mode == MODE_SWATCH) {
            for (auto &it : _nonsolid) {
                it->hide();
            }
            for (auto &swatch_widget : _swatch_widgets) {
                swatch_widget->show_all();
            }

            auto icon_column = _treeview->get_column(0);
            icon_column->set_title(_("Swatch"));

            _vectors->setSwatched();
        } else {
            for (auto &it : _nonsolid) {
                it->show_all();
            }
            for (auto &swatch_widget : _swatch_widgets) {
                swatch_widget->hide();
            }
            auto icon_column = _treeview->get_column(0);
            icon_column->set_title(_("Gradient"));
        }
    }
}

void GradientSelector::setUnits(SPGradientUnits units) { _gradientUnits = units; }

SPGradientUnits GradientSelector::getUnits() { return _gradientUnits; }

SPGradientSpread GradientSelector::getSpread() { return _gradientSpread; }

void GradientSelector::onGradientRename(const Glib::ustring &path_string, const Glib::ustring &new_text)
{
    Gtk::TreePath path(path_string);
    auto iter = _store->get_iter(path);

    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        if (row) {
            SPObject *obj = row[_columns->data];
            if (obj) {
                row[_columns->name] = gr_prepare_label(obj);
                if (!new_text.empty() && new_text != row[_columns->name]) {
                    rename_id(obj, new_text);
                    Inkscape::DocumentUndo::done(obj->document, SP_VERB_CONTEXT_GRADIENT, _("Rename gradient"));
                }
            }
        }
    }
}

void GradientSelector::onTreeColorColClick()
{
    auto column = _treeview->get_column(0);
    column->set_sort_column(_columns->color);
}

void GradientSelector::onTreeNameColClick()
{
    auto column = _treeview->get_column(1);
    column->set_sort_column(_columns->name);
}


void GradientSelector::onTreeCountColClick()
{
    auto column = _treeview->get_column(2);
    column->set_sort_column(_columns->refcount);
}

void GradientSelector::moveSelection(int amount, bool down, bool toEnd)
{
    auto select = _treeview->get_selection();
    auto iter = select->get_selected();

    if (amount < 0) {
        down = !down;
        amount = -amount;
    }

    auto canary = iter;
    if (down) {
        ++canary;
    } else {
        --canary;
    }
    while (canary && (toEnd || amount > 0)) {
        --amount;
        if (down) {
            ++canary;
            ++iter;
        } else {
            --canary;
            --iter;
        }
    }

    select->select(iter);
    _treeview->scroll_to_row(_store->get_path(iter), 0.5);
}

bool GradientSelector::onKeyPressEvent(GdkEventKey *event)
{
    bool consume = false;
    auto display = Gdk::Display::get_default();
    auto keymap = display->get_keymap();
    guint key = 0;
    gdk_keymap_translate_keyboard_state(keymap, event->hardware_keycode, static_cast<GdkModifierType>(event->state), 0,
                                        &key, 0, 0, 0);

    switch (key) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up: {
            moveSelection(-1);
            consume = true;
            break;
        }
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down: {
            moveSelection(1);
            consume = true;
            break;
        }
        case GDK_KEY_Page_Up:
        case GDK_KEY_KP_Page_Up: {
            moveSelection(-5);
            consume = true;
            break;
        }

        case GDK_KEY_Page_Down:
        case GDK_KEY_KP_Page_Down: {
            moveSelection(5);
            consume = true;
            break;
        }

        case GDK_KEY_End:
        case GDK_KEY_KP_End: {
            moveSelection(0, true, true);
            consume = true;
            break;
        }

        case GDK_KEY_Home:
        case GDK_KEY_KP_Home: {
            moveSelection(0, false, true);
            consume = true;
            break;
        }
    }
    return consume;
}

void GradientSelector::onTreeSelection()
{
    if (!_treeview) {
        return;
    }

    if (_blocked) {
        return;
    }

    if (!_treeview->has_focus()) {
        /* Workaround for GTK bug on Windows/OS X
         * When the treeview initially doesn't have focus and is clicked
         * sometimes get_selection()->signal_changed() has the wrong selection
         */
        _treeview->grab_focus();
    }

    const auto sel = _treeview->get_selection();
    if (!sel) {
        return;
    }

    SPGradient *obj = nullptr;
    /* Single selection */
    auto iter = sel->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        obj = row[_columns->data];
    }

    if (obj) {
        vector_set(SP_GRADIENT(obj));
    }
}

bool GradientSelector::_checkForSelected(const Gtk::TreePath &path, const Gtk::TreeIter &iter, SPGradient *vector)
{
    bool found = false;

    Gtk::TreeModel::Row row = *iter;
    if (vector == row[_columns->data]) {
        _treeview->scroll_to_row(path, 0.5);
        auto select = _treeview->get_selection();
        bool wasBlocked = _blocked;
        _blocked = true;
        select->select(iter);
        _blocked = wasBlocked;
        found = true;
    }

    return found;
}

void GradientSelector::selectGradientInTree(SPGradient *vector)
{
    _store->foreach (sigc::bind<SPGradient *>(sigc::mem_fun(*this, &GradientSelector::_checkForSelected), vector));
}

void GradientSelector::setVector(SPDocument *doc, SPGradient *vector)
{
    g_return_if_fail(!vector || SP_IS_GRADIENT(vector));
    g_return_if_fail(!vector || (vector->document == doc));

    if (vector && !vector->hasStops()) {
        return;
    }

    _vectors->set_gradient(doc, vector);

    selectGradientInTree(vector);

    if (vector) {
        if ((_mode == MODE_SWATCH) && vector->isSwatch()) {
            if (vector->isSolid()) {
                for (auto &it : _nonsolid) {
                    it->hide();
                }
            } else {
                for (auto &it : _nonsolid) {
                    it->show_all();
                }
            }
        } else if (_mode != MODE_SWATCH) {

            for (auto &swatch_widget : _swatch_widgets) {
                swatch_widget->hide();
            }
            for (auto &it : _nonsolid) {
                it->show_all();
            }
        }

        if (_edit) {
            _edit->set_sensitive(true);
        }
        if (_add) {
            _add->set_sensitive(true);
        }
        if (_del) {
            _del->set_sensitive(true);
        }
    } else {
        if (_edit) {
            _edit->set_sensitive(false);
        }
        if (_add) {
            _add->set_sensitive(doc != nullptr);
        }
        if (_del) {
            _del->set_sensitive(false);
        }
    }
}

SPGradient *GradientSelector::getVector()
{
    return _vectors->get_gradient();
}


void GradientSelector::vector_set(SPGradient *gr)
{
    if (!_blocked) {
        _blocked = true;
        gr = sp_gradient_ensure_vector_normalized(gr);
        setVector((gr) ? gr->document : nullptr, gr);
        _signal_changed.emit(gr);
        _blocked = false;
    }
}


void GradientSelector::delete_vector_clicked()
{
    const auto selection = _treeview->get_selection();
    if (!selection) {
        return;
    }

    SPGradient *obj = nullptr;
    /* Single selection */
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        obj = row[_columns->data];
    }

    if (obj) {
        std::string id = obj->getId();
        sp_gradient_unset_swatch(SP_ACTIVE_DESKTOP, id);
    }
}

void GradientSelector::edit_vector_clicked()
{
    // Invoke the gradient tool
    auto verb = Inkscape::Verb::get(SP_VERB_CONTEXT_GRADIENT);
    if (verb) {
        auto action = verb->get_action(Inkscape::ActionContext((Inkscape::UI::View::View *)SP_ACTIVE_DESKTOP));
        if (action) {
            sp_action_perform(action, nullptr);
        }
    }
}

void GradientSelector::add_vector_clicked()
{
    auto doc = _vectors->get_document();

    if (!doc)
        return;

    auto gr = _vectors->get_gradient();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    Inkscape::XML::Node *repr = nullptr;

    if (gr) {
        repr = gr->getRepr()->duplicate(xml_doc);
        // Rename the new gradients id to be similar to the cloned gradients
        Glib::ustring old_id = gr->getId();
        rename_id(gr, old_id);
        doc->getDefs()->getRepr()->addChild(repr, nullptr);
    } else {
        repr = xml_doc->createElement("svg:linearGradient");
        Inkscape::XML::Node *stop = xml_doc->createElement("svg:stop");
        stop->setAttribute("offset", "0");
        stop->setAttribute("style", "stop-color:#000;stop-opacity:1;");
        repr->appendChild(stop);
        Inkscape::GC::release(stop);
        stop = xml_doc->createElement("svg:stop");
        stop->setAttribute("offset", "1");
        stop->setAttribute("style", "stop-color:#fff;stop-opacity:1;");
        repr->appendChild(stop);
        Inkscape::GC::release(stop);
        doc->getDefs()->getRepr()->addChild(repr, nullptr);
        gr = SP_GRADIENT(doc->getObjectByRepr(repr));
    }

    _vectors->set_gradient(doc, gr);

    selectGradientInTree(gr);

    Inkscape::GC::release(repr);
}

} // namespace Widget
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
