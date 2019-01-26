// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Dialog for adding a live path effect.
 *
 * Author:
 *
 * Copyright (C) 2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "desktop.h"
#include "io/resource.h"
#include "live_effects/effect.h"
#include "livepatheffect-add.h"
#include <glibmm/i18n.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

LivePathEffectAdd::LivePathEffectAdd()
    : converter(Inkscape::LivePathEffect::LPETypeConverter)
{
    const std::string req_widgets[] = { "LPEDialogSelector", "LPESelector", "LPESelectorFlowBox" };
    Glib::ustring gladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-add.ui");
    try {
        _builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }

    Gtk::Object *test;
    for (std::string w : req_widgets) {
        _builder->get_widget(w, test);
        if (!test) {
            g_warning("Required widget %s does not exist", w.c_str());
            return;
        }
    }
    _builder->get_widget("LPEDialogSelector", _LPEDialogSelector);
    /**
     * Initialize Effect list
     */
    _builder->get_widget("LPESelectorFlowBox", _LPESelectorFlowBox);
    _builder->get_widget("LPEFilter", _LPEFilter);
    _builder->get_widget("LPEInfo", _LPEInfo);
    _LPEFilter->signal_search_changed().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_search));
    const std::string le_widgets[] = { "LPESelectorEffect", "LPEName", "LPEDescription" };
    Glib::ustring le_gladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-add-effect.ui");
    for (int i = 0; i < static_cast<int>(converter._length); ++i) {

        try {
            _builder = Gtk::Builder::create_from_file(le_gladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for filter effect dialog");
            return;
        }

        Gtk::Object *test;
        for (std::string w : le_widgets) {
            _builder->get_widget(w, test);
            if (!test) {
                g_warning("Required widget %s does not exist", w.c_str());
                return;
            }
        }
        const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *data = &converter.data(i);
        Gtk::Label *LPEName;
        _builder->get_widget("LPEName", LPEName);
        Glib::ustring newid = "LPEName_" + Glib::ustring::format(i);
        (*LPEName).set_name(newid);
        (*LPEName).set_text(converter.get_label(data->id).c_str());
        Gtk::Label *LPEDescription;
        _builder->get_widget("LPEDescription", LPEDescription);
        newid = "LPEDescription_" + Glib::ustring::format(i);
        (*LPEDescription).set_name(newid);
        (*LPEDescription).set_text(converter.get_description(data->id));
        Gtk::Image *LPEIcon;
        _builder->get_widget("LPEIcon", LPEIcon);
        newid = "LPEIcon_" + Glib::ustring::format(i);
        (*LPEIcon).set_name(newid);
        (*LPEIcon).set_from_icon_name(converter.get_icon(data->id), Gtk::BuiltinIconSize(Gtk::ICON_SIZE_DIALOG));
        Gtk::Box *LPESelectorEffect;
        _builder->get_widget("LPESelectorEffect", LPESelectorEffect);
        newid = "LPESelectorEffect" + Glib::ustring::format(i);
        (*LPESelectorEffect).set_name(newid);
        _LPESelectorFlowBox->insert(*LPESelectorEffect, i);
    }
    _visiblelpe = _LPESelectorFlowBox->get_children().size();
    _LPESelectorFlowBox->signal_child_activated().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_activate));
    _LPEDialogSelector->set_title(_("Live Efects Selector"));
    _LPEDialogSelector->show_all_children();
    _LPEInfo->set_visible(false);
}

void LivePathEffectAdd::on_activate(Gtk::FlowBoxChild *child)
{
    for (auto i : _LPESelectorFlowBox->get_children()) {
        Gtk::FlowBoxChild *leitem = dynamic_cast<Gtk::FlowBoxChild *>(i);
        leitem->get_style_context()->remove_class("lpeactive");
        leitem->get_style_context()->remove_class("colorinverse");
        leitem->get_style_context()->remove_class("backgroundinverse");
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(leitem->get_child());
        if (box) {
            std::vector<Gtk::Widget *> contents = box->get_children();
            Gtk::Box *actions = dynamic_cast<Gtk::Box *>(contents[3]);
            if (actions) {
                actions->set_visible(false);
            }
        }
    }
    child->get_style_context()->add_class("lpeactive");
    child->get_style_context()->add_class("colorinverse");
    child->get_style_context()->add_class("backgroundinverse");
    child->show_all_children();
}

bool LivePathEffectAdd::on_filter(Gtk::FlowBoxChild *child)
{
    if (_LPEFilter->get_text().length() < 4) {
        _visiblelpe = _LPESelectorFlowBox->get_children().size();
        return true;
    }
    Gtk::Box *box = dynamic_cast<Gtk::Box *>(child->get_child());
    if (box) {
        std::vector<Gtk::Widget *> contents = box->get_children();
        Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
        if (lpename) {
            size_t s = lpename->get_text().uppercase().find(_LPEFilter->get_text().uppercase(), 0);
            if (s != -1) {
                _visiblelpe++;
                return true;
            }
        }
        Gtk::Label *lpedesc = dynamic_cast<Gtk::Label *>(contents[2]);
        if (lpedesc) {
            size_t s = lpedesc->get_text().uppercase().find(_LPEFilter->get_text().uppercase(), 0);
            if (s != -1) {
                _visiblelpe++;
                return true;
            }
        }
    }
    return false;
}

    mainVBox->pack_start(scrolled_window, true, true);
    add_action_widget(close_button, Gtk::RESPONSE_CLOSE);
    add_action_widget(add_button, Gtk::RESPONSE_APPLY);

    
    /**
     * Signal handlers
     */
    effectlist_treeview.signal_button_press_event().connect_notify( sigc::mem_fun(*this, &LivePathEffectAdd::onButtonEvent) );
    effectlist_treeview.signal_key_press_event().connect_notify(sigc::mem_fun(*this, &LivePathEffectAdd::onKeyEvent));
    close_button.signal_clicked().connect(sigc::mem_fun(*this, &LivePathEffectAdd::onClose));
    add_button.signal_clicked().connect(sigc::mem_fun(*this, &LivePathEffectAdd::onAdd));
    signal_delete_event().connect( sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &LivePathEffectAdd::onClose)), true ) );

    add_button.grab_default();

    show_all_children();
}

void LivePathEffectAdd::onAdd()
{
    applied = true;
    onClose();
}

void LivePathEffectAdd::onClose() { _LPEDialogSelector->hide(); }

void LivePathEffectAdd::onKeyEvent(GdkEventKey* evt)
{
    if (evt->keyval == GDK_KEY_Return) {
         onAdd();
    }
    if (evt->keyval == GDK_KEY_Escape) {
         onClose();
    }
}

void LivePathEffectAdd::onButtonEvent(GdkEventButton* evt)
{
    // Double click on tree is same as clicking the add button
    if (evt->type == GDK_2BUTTON_PRESS) {
        onAdd();
    }
}

const Util::EnumData<LivePathEffect::EffectType>*
LivePathEffectAdd::getActiveData()
{
    Gtk::TreeModel::iterator iter = instance().effectlist_treeview.get_selection()->get_selected();
    if ( iter ) {
        Gtk::TreeModel::Row row = *iter;
        return row[instance()._columns.data];
    }

    return nullptr;
}


void LivePathEffectAdd::show(SPDesktop *desktop)
{
    LivePathEffectAdd &dial = instance();
    dial._LPEDialogSelector->run();
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
