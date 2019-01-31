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
    add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |  Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK );
    Glib::ustring gladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-add.ui");
    try {
        _builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }
    _builder->get_widget("LPEDialogSelector", _LPEDialogSelector);
    _builder->get_widget("LPESelectorFlowBox", _LPESelectorFlowBox);
    _builder->get_widget("LPESelectorEffectInfo", _LPESelectorEffectInfo);
    _builder->get_widget("LPEFilter", _LPEFilter);
    _builder->get_widget("LPEInfo", _LPEInfo);
    _LPEFilter->signal_search_changed().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_search));
    Glib::ustring effectgladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-add-effect.ui");
    for (int i = 0; i < static_cast<int>(converter._length); ++i) {
        Glib::RefPtr<Gtk::Builder> builder_effect;
        try {
            builder_effect = Gtk::Builder::create_from_file(effectgladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for filter effect dialog");
            return;
        }
        const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *data = &converter.data(i);
        Gtk::Label *LPEName;
        builder_effect->get_widget("LPEName", LPEName);
        LPEName->set_text(converter.get_label(data->id).c_str());
        Gtk::Label *LPEDescription;
        builder_effect->get_widget("LPEDescription", LPEDescription);
        LPEDescription->set_text(converter.get_description(data->id));
        Gtk::Image *LPEIcon;
        builder_effect->get_widget("LPEIcon", LPEIcon);
        LPEIcon->set_from_icon_name(converter.get_icon(data->id), Gtk::BuiltinIconSize(Gtk::ICON_SIZE_DIALOG));
        Gtk::EventBox *LPESelectorEffectInfoLauncher;
        builder_effect->get_widget("LPESelectorEffectInfoLauncher", LPESelectorEffectInfoLauncher);
        LPESelectorEffectInfoLauncher->signal_button_press_event().connect(sigc::bind<Glib::RefPtr<Gtk::Builder> >(sigc::mem_fun(*this, &LivePathEffectAdd::pop_description), builder_effect));
        Gtk::Box *LPESelectorEffect;
        builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
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

bool LivePathEffectAdd::pop_description(GdkEventButton* evt, Glib::RefPtr<Gtk::Builder> builder_effect)
{
    Gtk::EventBox *LPESelectorEffectInfoLauncher;
    builder_effect->get_widget("LPESelectorEffectInfoLauncher", LPESelectorEffectInfoLauncher);
    _LPESelectorEffectInfo->set_relative_to(*LPESelectorEffectInfoLauncher->get_children()[0]);
    
    Gtk::Label *LPEName;
    builder_effect->get_widget("LPEName", LPEName);
    Gtk::Label *LPEDescription;
    builder_effect->get_widget("LPEDescription", LPEDescription);
    Gtk::Image *LPEIcon;
    builder_effect->get_widget("LPEIcon", LPEIcon);     

    Gtk::Image *LPESelectorEffectInfoIcon;
    _builder->get_widget("LPESelectorEffectInfoIcon", LPESelectorEffectInfoIcon);
    LPESelectorEffectInfoIcon->set_from_icon_name(LPEIcon->get_icon_name(), Gtk::IconSize(60));
        
    Gtk::Label *LPESelectorEffectInfoName;
    _builder->get_widget("LPESelectorEffectInfoName", LPESelectorEffectInfoName);
    LPESelectorEffectInfoName->set_text(LPEName->get_text());
    
    Gtk::Label *LPESelectorEffectInfoDescription;
    _builder->get_widget("LPESelectorEffectInfoDescription", LPESelectorEffectInfoDescription);
    LPESelectorEffectInfoDescription->set_text(LPEDescription->get_text());
    
    _LPESelectorEffectInfo->show();
    
    return true;
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

void LivePathEffectAdd::on_search()
{
    _visiblelpe = 0;
    _LPESelectorFlowBox->set_filter_func(sigc::mem_fun(*this, &LivePathEffectAdd::on_filter));
    if (_visiblelpe == 0) {
        _LPEInfo->set_text(_("Your search do a empty result, please try again"));
        _LPEInfo->set_visible(true);
        _LPEInfo->get_style_context()->add_class("lpeinfowarn");
    } else {
        _LPEInfo->set_visible(false);
        _LPEInfo->get_style_context()->remove_class("lpeinfowarn");
    }
}

void LivePathEffectAdd::onAdd()
{
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
