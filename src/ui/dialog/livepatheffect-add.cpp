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

#include "livepatheffect-add.h"

#include <cmath>
#include <glibmm/i18n.h>

#include "desktop.h"
#include "io/resource.h"
#include "live_effects/effect.h"
#include "object/sp-clippath.h"
#include "object/sp-item-group.h"
#include "object/sp-mask.h"
#include "object/sp-path.h"
#include "object/sp-shape.h"
#include "preferences.h"
#include "ui/widget/canvas.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

bool sp_has_fav(Glib::ustring effect)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring favlist = prefs->getString("/dialogs/livepatheffect/favs");
    size_t pos = favlist.find(effect);
    if (pos != std::string::npos) {
        return true;
    }
    return false;
}

void sp_add_fav(Glib::ustring effect)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring favlist = prefs->getString("/dialogs/livepatheffect/favs");
    if (!sp_has_fav(effect)) {
        prefs->setString("/dialogs/livepatheffect/favs", favlist + effect + ";");
    }
}

void sp_remove_fav(Glib::ustring effect)
{
    if (sp_has_fav(effect)) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        Glib::ustring favlist = prefs->getString("/dialogs/livepatheffect/favs");
        effect += ";";
        size_t pos = favlist.find(effect);
        if (pos != std::string::npos) {
            favlist.erase(pos, effect.length());
            prefs->setString("/dialogs/livepatheffect/favs", favlist);
        }
    }
}

void sp_add_top_window_classes_callback(Gtk::Widget *widg)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop) {
        Gtk::Widget *canvas = desktop->canvas;
        Gtk::Window *toplevel_window = dynamic_cast<Gtk::Window *>(canvas->get_toplevel());
        if (toplevel_window) {
            Gtk::Window *current_window = dynamic_cast<Gtk::Window *>(widg);
            if (!current_window) {
                current_window = dynamic_cast<Gtk::Window *>(widg->get_toplevel());
            }
            if (current_window) {
                if (toplevel_window->get_style_context()->has_class("dark")) {
                    current_window->get_style_context()->add_class("dark");
                    current_window->get_style_context()->remove_class("bright");
                } else {
                    current_window->get_style_context()->add_class("bright");
                    current_window->get_style_context()->remove_class("dark");
                }
                if (toplevel_window->get_style_context()->has_class("symbolic")) {
                    current_window->get_style_context()->add_class("symbolic");
                    current_window->get_style_context()->remove_class("regular");
                } else {
                    current_window->get_style_context()->remove_class("symbolic");
                    current_window->get_style_context()->add_class("regular");
                }
            }
        }
    }
}

void sp_add_top_window_classes(Gtk::Widget *widg)
{
    if (!widg) {
        return;
    }
    if (!widg->get_realized()) {
        widg->signal_realize().connect(sigc::bind(sigc::ptr_fun(&sp_add_top_window_classes_callback), widg));
    } else {
        sp_add_top_window_classes_callback(widg);
    }
}

bool LivePathEffectAdd::mouseover(GdkEventCrossing *evt, GtkWidget *wdg)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_HAND2);
    GdkWindow *window = gtk_widget_get_window(wdg);
    gdk_window_set_cursor(window, cursor);
    g_object_unref(cursor);
    return true;
}

bool LivePathEffectAdd::mouseout(GdkEventCrossing *evt, GtkWidget *wdg)
{
    GdkWindow *window = gtk_widget_get_window(wdg);
    gdk_window_set_cursor(window, nullptr);
    hide_pop_description(evt);
    return true;
}

LivePathEffectAdd::LivePathEffectAdd()
    : converter(Inkscape::LivePathEffect::LPETypeConverter)
    , _applied(false)
    , _showfavs(false)
{
    auto gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-add.glade");
    try {
        _builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }
    _builder->get_widget("LPEDialogSelector", _LPEDialogSelector);
    _builder->get_widget("LPESelectorFlowBox", _LPESelectorFlowBox);
    _builder->get_widget("LPESelectorEffectInfoPop", _LPESelectorEffectInfoPop);
    _builder->get_widget("LPEFilter", _LPEFilter);
    _builder->get_widget("LPEInfo", _LPEInfo);
    _builder->get_widget("LPEExperimental", _LPEExperimental);
    _builder->get_widget("LPEScrolled", _LPEScrolled);
    _builder->get_widget("LPESelectorEffectEventFavShow", _LPESelectorEffectEventFavShow);
    _builder->get_widget("LPESelectorEffectInfoEventBox", _LPESelectorEffectInfoEventBox);
    _builder->get_widget("LPESelectorEffectRadioPackLess", _LPESelectorEffectRadioPackLess);
    _builder->get_widget("LPESelectorEffectRadioPackMore", _LPESelectorEffectRadioPackMore);
    _builder->get_widget("LPESelectorEffectRadioList", _LPESelectorEffectRadioList);

    _LPEFilter->signal_search_changed().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_search));
    _LPEDialogSelector->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                                   Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::KEY_PRESS_MASK);
    _LPESelectorFlowBox->signal_set_focus_child().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_focus));

    gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-effect.glade");
    for (int i = 0; i < static_cast<int>(converter._length); ++i) {
        Glib::RefPtr<Gtk::Builder> builder_effect;
        try {
            builder_effect = Gtk::Builder::create_from_file(gladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for filter effect dialog");
            return;
        }
        const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *data = &converter.data(i);
        Gtk::EventBox *LPESelectorEffect;
        builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
        LPESelectorEffect->signal_button_press_event().connect(
            sigc::bind<Glib::RefPtr<Gtk::Builder>, const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *>(
                sigc::mem_fun(*this, &LivePathEffectAdd::apply), builder_effect, &converter.data(i)));
        Gtk::EventBox *LPESelectorEffectEventExpander;
        builder_effect->get_widget("LPESelectorEffectEventExpander", LPESelectorEffectEventExpander);
        LPESelectorEffectEventExpander->signal_button_press_event().connect(
            sigc::bind<Glib::RefPtr<Gtk::Builder>>(sigc::mem_fun(*this, &LivePathEffectAdd::expand), builder_effect));
        LPESelectorEffectEventExpander->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(LPESelectorEffect->gobj())));
        LPESelectorEffectEventExpander->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(LPESelectorEffect->gobj())));
        Gtk::Label *LPEName;
        builder_effect->get_widget("LPEName", LPEName);
        const Glib::ustring label = _(converter.get_label(data->id).c_str());
        const Glib::ustring untranslated_label = converter.get_untranslated_label(data->id);
        if (untranslated_label == label) {
            LPEName->set_text(label);
        } else {
            LPEName->set_markup((label + "\n<span size='x-small'>" + untranslated_label + "</span>").c_str());
        }
        Gtk::Label *LPEDescription;
        builder_effect->get_widget("LPEDescription", LPEDescription);
        const Glib::ustring description = _(converter.get_description(data->id).c_str());
        LPEDescription->set_text(description);
        Gtk::ToggleButton *LPEExperimentalToggle;
        builder_effect->get_widget("LPEExperimentalToggle", LPEExperimentalToggle);
        bool active = converter.get_experimental(data->id) ? true : false;
        LPEExperimentalToggle->set_active(active);
        Gtk::Image *LPEIcon;
        builder_effect->get_widget("LPEIcon", LPEIcon);
        LPEIcon->set_from_icon_name(converter.get_icon(data->id), Gtk::IconSize(Gtk::ICON_SIZE_DIALOG));
        Gtk::EventBox *LPESelectorEffectEventInfo;
        builder_effect->get_widget("LPESelectorEffectEventInfo", LPESelectorEffectEventInfo);
        LPESelectorEffectEventInfo->signal_enter_notify_event().connect(sigc::bind<Glib::RefPtr<Gtk::Builder>>(
            sigc::mem_fun(*this, &LivePathEffectAdd::pop_description), builder_effect));
        Gtk::EventBox *LPESelectorEffectEventFav;
        builder_effect->get_widget("LPESelectorEffectEventFav", LPESelectorEffectEventFav);
        Gtk::Image *fav = dynamic_cast<Gtk::Image *>(LPESelectorEffectEventFav->get_child());
        if (sp_has_fav(LPEName->get_text())) {
            fav->set_from_icon_name("draw-star", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
        } else {
            fav->set_from_icon_name("draw-star-outline", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
        }
        Gtk::EventBox *LPESelectorEffectEventFavTop;
        builder_effect->get_widget("LPESelectorEffectEventFavTop", LPESelectorEffectEventFavTop);
        LPESelectorEffectEventFav->signal_button_press_event().connect(sigc::bind<Glib::RefPtr<Gtk::Builder>>(
            sigc::mem_fun(*this, &LivePathEffectAdd::fav_toggler), builder_effect));
        LPESelectorEffectEventFavTop->signal_button_press_event().connect(sigc::bind<Glib::RefPtr<Gtk::Builder>>(
            sigc::mem_fun(*this, &LivePathEffectAdd::fav_toggler), builder_effect));
        Gtk::Image *favtop = dynamic_cast<Gtk::Image *>(LPESelectorEffectEventFavTop->get_child());
        if (sp_has_fav(LPEName->get_text())) {
            favtop->set_from_icon_name("draw-star", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
        } else {
            favtop->set_from_icon_name("draw-star-outline", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
        }
        Gtk::EventBox *LPESelectorEffectEventApply;
        builder_effect->get_widget("LPESelectorEffectEventApply", LPESelectorEffectEventApply);
        LPESelectorEffectEventApply->signal_button_press_event().connect(
            sigc::bind<Glib::RefPtr<Gtk::Builder>, const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *>(
                sigc::mem_fun(*this, &LivePathEffectAdd::apply), builder_effect, &converter.data(i)));
        LPESelectorEffectEventApply->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(LPESelectorEffectEventApply->gobj())));
        LPESelectorEffectEventApply->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(LPESelectorEffectEventApply->gobj())));
        Gtk::ButtonBox *LPESelectorButtonBox;
        builder_effect->get_widget("LPESelectorButtonBox", LPESelectorButtonBox);
        LPESelectorButtonBox->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(LPESelectorEffect->gobj())));
        LPESelectorButtonBox->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(LPESelectorEffect->gobj())));
        LPESelectorEffect->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(LPESelectorEffect->gobj())));
        LPESelectorEffect->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(
            sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(LPESelectorEffect->gobj())));
        _LPESelectorFlowBox->insert(*LPESelectorEffect, i);
        LPESelectorEffect->get_parent()->signal_key_press_event().connect(
            sigc::bind<Glib::RefPtr<Gtk::Builder>, const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *>(
                sigc::mem_fun(*this, &LivePathEffectAdd::on_press_enter), builder_effect, &converter.data(i)));
        LPESelectorEffect->get_parent()->get_style_context()->add_class(
            ("LPEIndex" + Glib::ustring::format(i)).c_str());
    }
    _LPESelectorFlowBox->set_activate_on_single_click(false);
    _visiblelpe = _LPESelectorFlowBox->get_children().size();
    _LPEInfo->set_visible(false);
    _LPESelectorEffectRadioPackLess->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &LivePathEffectAdd::viewChanged), 0));
    _LPESelectorEffectRadioPackMore->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &LivePathEffectAdd::viewChanged), 1));
    _LPESelectorEffectRadioList->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &LivePathEffectAdd::viewChanged), 2));
    _LPESelectorEffectEventFavShow->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(
        sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(_LPESelectorEffectEventFavShow->gobj())));
    _LPESelectorEffectEventFavShow->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(
        sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(_LPESelectorEffectEventFavShow->gobj())));
    _LPESelectorEffectEventFavShow->signal_button_press_event().connect(
        sigc::mem_fun(*this, &LivePathEffectAdd::show_fav_toggler));
    _LPESelectorEffectInfoEventBox->signal_leave_notify_event().connect(
        sigc::mem_fun(*this, &LivePathEffectAdd::hide_pop_description));
    _LPESelectorEffectInfoEventBox->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(
        sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(_LPESelectorEffectInfoEventBox->gobj())));
    _LPESelectorEffectInfoEventBox->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(
        sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(_LPESelectorEffectInfoEventBox->gobj())));
    _LPEExperimental->property_active().signal_changed().connect(
        sigc::mem_fun(*this, &LivePathEffectAdd::reload_effect_list));
    Gtk::Window *window = SP_ACTIVE_DESKTOP->getToplevel();
    int width;
    int height;
    window->get_size(width, height);
    _LPEDialogSelector->resize(std::min(width - 300, 1440), std::min(height - 300, 900));
    _LPEDialogSelector->set_transient_for(*window);
    _LPESelectorFlowBox->set_focus_vadjustment(_LPEScrolled->get_vadjustment());
    _LPEDialogSelector->show_all_children();
    _lasteffect = nullptr;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint mode = prefs->getInt("/dialogs/livepatheffect/dialogmode", 0);
    switch (mode) {
        case 0:
            _LPESelectorEffectRadioPackLess->set_active();
            viewChanged(0);
            break;
        case 1:
            _LPESelectorEffectRadioPackMore->set_active();
            viewChanged(1);
            break;
        default:
            _LPESelectorEffectRadioList->set_active();
            viewChanged(2);
    }
    Gtk::Widget *widg = dynamic_cast<Gtk::Widget *>(_LPEDialogSelector);
    INKSCAPE.signal_change_theme.connect(sigc::bind(sigc::ptr_fun(sp_add_top_window_classes), widg));
    sp_add_top_window_classes(widg);
}
const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *LivePathEffectAdd::getActiveData()
{
    return instance()._to_add;
}

void LivePathEffectAdd::viewChanged(gint mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool changed = false;
    if (mode == 2 && !_LPEDialogSelector->get_style_context()->has_class("LPEList")) {
        _LPEDialogSelector->get_style_context()->add_class("LPEList");
        _LPEDialogSelector->get_style_context()->remove_class("LPEPackLess");
        _LPEDialogSelector->get_style_context()->remove_class("LPEPackMore");
        _LPESelectorFlowBox->set_max_children_per_line(1);
        changed = true;
    } else if (mode == 1 && !_LPEDialogSelector->get_style_context()->has_class("LPEPackMore")) {
        _LPEDialogSelector->get_style_context()->remove_class("LPEList");
        _LPEDialogSelector->get_style_context()->remove_class("LPEPackLess");
        _LPEDialogSelector->get_style_context()->add_class("LPEPackMore");
        _LPESelectorFlowBox->set_max_children_per_line(30);
        changed = true;
    } else if (mode == 0 && !_LPEDialogSelector->get_style_context()->has_class("LPEPackLess")) {
        _LPEDialogSelector->get_style_context()->remove_class("LPEList");
        _LPEDialogSelector->get_style_context()->add_class("LPEPackLess");
        _LPEDialogSelector->get_style_context()->remove_class("LPEPackMore");
        _LPESelectorFlowBox->set_max_children_per_line(30);
        changed = true;
    }
    prefs->setInt("/dialogs/livepatheffect/dialogmode", mode);
    if (changed) {
        _LPESelectorFlowBox->unset_sort_func();
        _LPESelectorFlowBox->set_sort_func(sigc::mem_fun(this, &LivePathEffectAdd::on_sort));
        std::vector<Gtk::FlowBoxChild *> selected = _LPESelectorFlowBox->get_selected_children();
        if (selected.size() == 1) {
            _LPESelectorFlowBox->get_selected_children()[0]->grab_focus();
        }
    }
}

void LivePathEffectAdd::on_focus(Gtk::Widget *widget)
{
    Gtk::FlowBoxChild *child = dynamic_cast<Gtk::FlowBoxChild *>(widget);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint mode = prefs->getInt("/dialogs/livepatheffect/dialogmode", 0);
    if (child && mode != 2) {
        for (auto i : _LPESelectorFlowBox->get_children()) {
            Gtk::FlowBoxChild *leitem = dynamic_cast<Gtk::FlowBoxChild *>(i);
            Gtk::EventBox *eventbox = dynamic_cast<Gtk::EventBox *>(leitem->get_child());
            if (eventbox) {
                Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
                if (box) {
                    std::vector<Gtk::Widget *> contents = box->get_children();
                    Gtk::Box *actions = dynamic_cast<Gtk::Box *>(contents[5]);
                    if (actions) {
                        actions->set_visible(false);
                    }
                    Gtk::EventBox *expander = dynamic_cast<Gtk::EventBox *>(contents[4]);
                    if (expander) {
                        expander->set_visible(true);
                    }
                }
            }
        }
        Gtk::EventBox *eventbox = dynamic_cast<Gtk::EventBox *>(child->get_child());
        if (eventbox) {
            Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
            if (box) {
                std::vector<Gtk::Widget *> contents = box->get_children();
                Gtk::EventBox *expander = dynamic_cast<Gtk::EventBox *>(contents[4]);
                if (expander) {
                    expander->set_visible(false);
                }
            }
        }

    child->show_all_children();
    _LPESelectorFlowBox->select_child(*child);
    }
}

bool LivePathEffectAdd::pop_description(GdkEventCrossing *evt, Glib::RefPtr<Gtk::Builder> builder_effect)
{
    Gtk::Image *LPESelectorEffectInfo;
    builder_effect->get_widget("LPESelectorEffectInfo", LPESelectorEffectInfo);
    _LPESelectorEffectInfoPop->set_relative_to(*LPESelectorEffectInfo);

    Gtk::Label *LPEName;
    builder_effect->get_widget("LPEName", LPEName);
    Gtk::Label *LPEDescription;
    builder_effect->get_widget("LPEDescription", LPEDescription);
    Gtk::Image *LPEIcon;
    builder_effect->get_widget("LPEIcon", LPEIcon);

    Gtk::Image *LPESelectorEffectInfoIcon;
    _builder->get_widget("LPESelectorEffectInfoIcon", LPESelectorEffectInfoIcon);
    LPESelectorEffectInfoIcon->set_from_icon_name(LPEIcon->get_icon_name(), Gtk::IconSize(Gtk::ICON_SIZE_DIALOG));

    Gtk::Label *LPESelectorEffectInfoName;
    _builder->get_widget("LPESelectorEffectInfoName", LPESelectorEffectInfoName);
    LPESelectorEffectInfoName->set_text(LPEName->get_text());

    Gtk::Label *LPESelectorEffectInfoDescription;
    _builder->get_widget("LPESelectorEffectInfoDescription", LPESelectorEffectInfoDescription);
    LPESelectorEffectInfoDescription->set_text(LPEDescription->get_text());

    _LPESelectorEffectInfoPop->show();

    return true;
}

bool LivePathEffectAdd::hide_pop_description(GdkEventCrossing *evt)
{
    _LPESelectorEffectInfoPop->hide();
    return true;
}

bool LivePathEffectAdd::fav_toggler(GdkEventButton *evt, Glib::RefPtr<Gtk::Builder> builder_effect)
{
    Gtk::EventBox *LPESelectorEffect;
    builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
    Gtk::Label *LPEName;
    builder_effect->get_widget("LPEName", LPEName);
    Gtk::Image *LPESelectorEffectFav;
    builder_effect->get_widget("LPESelectorEffectFav", LPESelectorEffectFav);
    Gtk::Image *LPESelectorEffectFavTop;
    builder_effect->get_widget("LPESelectorEffectFavTop", LPESelectorEffectFavTop);
    Gtk::EventBox *LPESelectorEffectEventFavTop;
    builder_effect->get_widget("LPESelectorEffectEventFavTop", LPESelectorEffectEventFavTop);
    if (LPESelectorEffectFav && LPESelectorEffectEventFavTop) {
        if (sp_has_fav(LPEName->get_text())) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            gint mode = prefs->getInt("/dialogs/livepatheffect/dialogmode", 0);
            if (mode == 2) {
                LPESelectorEffectEventFavTop->set_visible(true);
                LPESelectorEffectEventFavTop->show();
            } else {
                LPESelectorEffectEventFavTop->set_visible(false);
                LPESelectorEffectEventFavTop->hide();
            }
            LPESelectorEffectFavTop->set_from_icon_name("draw-star-outline",
                                                        Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
            LPESelectorEffectFav->set_from_icon_name("draw-star-outline", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
            sp_remove_fav(LPEName->get_text());
            LPESelectorEffect->get_parent()->get_style_context()->remove_class("lpefav");
            LPESelectorEffect->get_parent()->get_style_context()->add_class("lpenormal");
            LPESelectorEffect->get_parent()->get_style_context()->add_class("lpe");
            if (_showfavs) {
                reload_effect_list();
            }
        } else {
            LPESelectorEffectEventFavTop->set_visible(true);
            LPESelectorEffectEventFavTop->show();
            LPESelectorEffectFavTop->set_from_icon_name("draw-star", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
            LPESelectorEffectFav->set_from_icon_name("draw-star", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
            sp_add_fav(LPEName->get_text());
            LPESelectorEffect->get_parent()->get_style_context()->add_class("lpefav");
            LPESelectorEffect->get_parent()->get_style_context()->remove_class("lpenormal");
            LPESelectorEffect->get_parent()->get_style_context()->add_class("lpe");
        }
    }
    return true;
}

bool LivePathEffectAdd::show_fav_toggler(GdkEventButton *evt)
{
    _showfavs = !_showfavs;
    Gtk::Image *favimage = dynamic_cast<Gtk::Image *>(_LPESelectorEffectEventFavShow->get_child());
    if (favimage) {
        if (_showfavs) {
            favimage->set_from_icon_name("draw-star", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
        } else {
            favimage->set_from_icon_name("draw-star-outline", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
        }
    }
    reload_effect_list();
    return true;
}

bool LivePathEffectAdd::apply(GdkEventButton *evt, Glib::RefPtr<Gtk::Builder> builder_effect,
                              const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *to_add)
{
    _to_add = to_add;
    Gtk::EventBox *LPESelectorEffect;
    builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
    Gtk::FlowBoxChild *flowboxchild =  dynamic_cast<Gtk::FlowBoxChild *>(LPESelectorEffect->get_parent());
    _LPESelectorFlowBox->select_child(*flowboxchild);
    if (flowboxchild && flowboxchild->get_style_context()->has_class("lpedisabled")) {
        return true;
    }
    _applied = true;
    _lasteffect = flowboxchild;
    _LPEDialogSelector->response(Gtk::RESPONSE_APPLY);
    _LPEDialogSelector->hide();
    return true;
}

bool LivePathEffectAdd::on_press_enter(GdkEventKey *key, Glib::RefPtr<Gtk::Builder> builder_effect,
                                       const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *to_add)
{
    if (key->keyval == 65293 || key->keyval == 65421) {
        _to_add = to_add;
        Gtk::EventBox *LPESelectorEffect;
        builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
        Gtk::FlowBoxChild *flowboxchild =  dynamic_cast<Gtk::FlowBoxChild *>(LPESelectorEffect->get_parent());
        if (flowboxchild && flowboxchild->get_style_context()->has_class("lpedisabled")) {
            return true;
        }
        _applied = true;
        _lasteffect = flowboxchild;
        _LPEDialogSelector->response(Gtk::RESPONSE_APPLY);
        _LPEDialogSelector->hide();
        return true;
    }
    return false;
}

bool LivePathEffectAdd::expand(GdkEventButton *evt, Glib::RefPtr<Gtk::Builder> builder_effect)
{
    Gtk::EventBox *LPESelectorEffect;
    builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
    Gtk::FlowBoxChild *child = dynamic_cast<Gtk::FlowBoxChild *>(LPESelectorEffect->get_parent());
    if (child) {
        child->grab_focus();
    }
    return true;
}



bool LivePathEffectAdd::on_filter(Gtk::FlowBoxChild *child)
{
    std::vector<Glib::ustring> classes = child->get_style_context()->list_classes();
    int pos = 0;
    for (auto childclass : classes) {
        size_t s = childclass.find("LPEIndex", 0);
        if (s != -1) {
            childclass = childclass.erase(0, 8);
            pos = std::stoi(childclass.raw());
        }
    }
    const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *data = &converter.data(pos);
    bool disable = false;
    if (_item_type == "group" && !converter.get_on_group(data->id)) {
        disable = true;
    } else if (_item_type == "shape" && !converter.get_on_shape(data->id)) {
        disable = true;
    } else if (_item_type == "path" && !converter.get_on_path(data->id)) {
        disable = true;
    }

    if (!_has_clip && data->id == Inkscape::LivePathEffect::POWERCLIP) {
        disable = true;
    }
    if (!_has_mask && data->id == Inkscape::LivePathEffect::POWERMASK) {
        disable = true;
    }

    if (disable) {
        child->get_style_context()->add_class("lpedisabled");
    } else {
        child->get_style_context()->remove_class("lpedisabled");
    }
    child->set_valign(Gtk::ALIGN_START);
    Gtk::EventBox *eventbox = dynamic_cast<Gtk::EventBox *>(child->get_child());
    if (eventbox) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
        if (box) {
            std::vector<Gtk::Widget *> contents = box->get_children();
            Gtk::Overlay *overlay = dynamic_cast<Gtk::Overlay *>(contents[0]);
            std::vector<Gtk::Widget *> content_overlay = overlay->get_children();

            Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
            if (!sp_has_fav(lpename->get_text()) && _showfavs) {
                return false;
            }
            Gtk::ToggleButton *experimental = dynamic_cast<Gtk::ToggleButton *>(contents[3]);
            if (experimental) {
                if (experimental->get_active() && !_LPEExperimental->get_active()) {
                    return false;
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
            if (_LPEFilter->get_text().length() < 1) {
                _visiblelpe++;
                return true;
            }
            if (lpename) {
                size_t s = lpename->get_text().uppercase().find(_LPEFilter->get_text().uppercase(), 0);
                if (s != -1) {
                    _visiblelpe++;
                    return true;
                }
            }
        }
    }
    return false;
}

void LivePathEffectAdd::reload_effect_list()
{
    /* if(_LPEExperimental->get_active()) {
        _LPEExperimental->get_style_context()->add_class("active");
    } else {
        _LPEExperimental->get_style_context()->remove_class("active");
    } */
    _visiblelpe = 0;
    _LPESelectorFlowBox->invalidate_filter();
    if (_showfavs) {
        if (_visiblelpe == 0) {
            _LPEInfo->set_text(_("You don't have any favorites yet. Click on the favorites star again to see all LPEs."));
            _LPEInfo->set_visible(true);
            _LPEInfo->get_style_context()->add_class("lpeinfowarn");
        } else {
            _LPEInfo->set_text(_("These are your favorite effects"));
            _LPEInfo->set_visible(true);
            _LPEInfo->get_style_context()->add_class("lpeinfowarn");
        }
    } else {
        _LPEInfo->set_text(_("Nothing found! Please try again with different search terms."));
        _LPEInfo->set_visible(false);
        _LPEInfo->get_style_context()->remove_class("lpeinfowarn");
    }
}

void LivePathEffectAdd::on_search()
{
    _visiblelpe = 0;
    _LPESelectorFlowBox->invalidate_filter();
    if (_showfavs) {
        if (_visiblelpe == 0) {
            _LPEInfo->set_text(_("Nothing found! Please try again with different search terms."));
            _LPEInfo->set_visible(true);
            _LPEInfo->get_style_context()->add_class("lpeinfowarn");
        } else {
            _LPEInfo->set_visible(true);
            _LPEInfo->get_style_context()->add_class("lpeinfowarn");
        }
    } else {
        if (_visiblelpe == 0) {
            _LPEInfo->set_text(_("Nothing found! Please try again with different search terms."));
            _LPEInfo->set_visible(true);
            _LPEInfo->get_style_context()->add_class("lpeinfowarn");
        } else {
            _LPEInfo->set_visible(false);
            _LPEInfo->get_style_context()->remove_class("lpeinfowarn");
        }
    }
}

int LivePathEffectAdd::on_sort(Gtk::FlowBoxChild *child1, Gtk::FlowBoxChild *child2)
{
    Glib::ustring name1 = "";
    Glib::ustring name2 = "";
    Gtk::EventBox *eventbox = dynamic_cast<Gtk::EventBox *>(child1->get_child());
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint mode = prefs->getInt("/dialogs/livepatheffect/dialogmode", 0);
    if (mode == 2) {
        eventbox->set_halign(Gtk::ALIGN_START);
    } else {
        eventbox->set_halign(Gtk::ALIGN_CENTER);
    }
    if (eventbox) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
        if (mode == 2) {
            box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        } else {
            box->set_orientation(Gtk::ORIENTATION_VERTICAL);
        }
        if (box) {
            std::vector<Gtk::Widget *> contents = box->get_children();
            Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
            name1 = lpename->get_text();
            if (lpename) {
                if (mode == 2) {
                    lpename->set_justify(Gtk::JUSTIFY_LEFT);
                    lpename->set_halign(Gtk::ALIGN_START);
                    lpename->set_valign(Gtk::ALIGN_CENTER);
                    lpename->set_width_chars(-1);
                    lpename->set_max_width_chars(-1);
                } else {
                    lpename->set_justify(Gtk::JUSTIFY_CENTER);
                    lpename->set_halign(Gtk::ALIGN_CENTER);
                    lpename->set_valign(Gtk::ALIGN_CENTER);
                    lpename->set_width_chars(14);
                    lpename->set_max_width_chars(23);
                }
            }
            Gtk::EventBox *lpemore = dynamic_cast<Gtk::EventBox *>(contents[4]);
            if (lpemore) {
                if (mode == 2) {
                    lpemore->hide();
                } else {
                    if (child1->is_selected()) {
                        lpemore->hide();
                    } else {
                        lpemore->show();
                    }
                }
            }
            Gtk::ButtonBox *lpebuttonbox = dynamic_cast<Gtk::ButtonBox *>(contents[5]);
            if (lpebuttonbox) {
                if (mode == 2) {
                    lpebuttonbox->hide();
                } else {
                    if (child1->is_selected()) {
                        lpebuttonbox->show();
                    } else {
                        lpebuttonbox->hide();
                    }
                }
            }
            Gtk::Label *lpedesc = dynamic_cast<Gtk::Label *>(contents[2]);
            if (lpedesc) {
                if (mode == 2) {
                    lpedesc->show();
                    lpedesc->set_justify(Gtk::JUSTIFY_LEFT);
                    lpedesc->set_halign(Gtk::ALIGN_START);
                    lpedesc->set_valign(Gtk::ALIGN_CENTER);
                    lpedesc->set_ellipsize(Pango::ELLIPSIZE_END);
                } else {
                    lpedesc->hide();
                    lpedesc->set_justify(Gtk::JUSTIFY_CENTER);
                    lpedesc->set_halign(Gtk::ALIGN_CENTER);
                    lpedesc->set_valign(Gtk::ALIGN_CENTER);
                    lpedesc->set_ellipsize(Pango::ELLIPSIZE_NONE);
                }
            }
            Gtk::Overlay *overlay = dynamic_cast<Gtk::Overlay *>(contents[0]);
            if (overlay) {
                std::vector<Gtk::Widget *> contents_overlay = overlay->get_children();
                Gtk::Image *icon = dynamic_cast<Gtk::Image *>(contents_overlay[0]);
                if (icon) {
                    if (mode == 2) {
                        icon->set_pixel_size(40);
                        icon->set_margin_right(25);
                        overlay->set_margin_right(5);
                    } else {
                        icon->set_pixel_size(60);
                        icon->set_margin_right(0);
                        overlay->set_margin_right(0);
                    }
                }
                Gtk::EventBox *LPESelectorEffectEventFavTop = dynamic_cast<Gtk::EventBox *>(contents_overlay[1]);
                if (LPESelectorEffectEventFavTop) {
                    Gtk::Image *fav = dynamic_cast<Gtk::Image *>(LPESelectorEffectEventFavTop->get_child());
                    if (sp_has_fav(name1)) {
                        fav->set_from_icon_name("draw-star", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
                        LPESelectorEffectEventFavTop->set_visible(true);
                        LPESelectorEffectEventFavTop->show();
                        child1->get_style_context()->add_class("lpefav");
                        child1->get_style_context()->remove_class("lpenormal");
                    } else if (!sp_has_fav(name1)) {
                        fav->set_from_icon_name("draw-star-outline", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
                        LPESelectorEffectEventFavTop->set_visible(false);
                        LPESelectorEffectEventFavTop->hide();
                        child1->get_style_context()->remove_class("lpefav");
                        child1->get_style_context()->add_class("lpenormal");
                    }
                    if (mode == 2) {
                        LPESelectorEffectEventFavTop->set_visible(true);
                        LPESelectorEffectEventFavTop->show();
                        LPESelectorEffectEventFavTop->set_halign(Gtk::ALIGN_END);
                        LPESelectorEffectEventFavTop->set_valign(Gtk::ALIGN_CENTER);
                    } else {
                        LPESelectorEffectEventFavTop->set_halign(Gtk::ALIGN_END);
                        LPESelectorEffectEventFavTop->set_valign(Gtk::ALIGN_START);
                    }
                    child1->get_style_context()->add_class("lpe");
                }
            }
        }
    }
    eventbox = dynamic_cast<Gtk::EventBox *>(child2->get_child());
    if (mode == 2) {
        eventbox->set_halign(Gtk::ALIGN_START);
    } else {
        eventbox->set_halign(Gtk::ALIGN_CENTER);
    }
    if (eventbox) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
        if (mode == 2) {
            box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        } else {
            box->set_orientation(Gtk::ORIENTATION_VERTICAL);
        }
        if (box) {
            std::vector<Gtk::Widget *> contents = box->get_children();
            Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
            name2 = lpename->get_text();
            if (lpename) {
                if (mode == 2) {
                    lpename->set_justify(Gtk::JUSTIFY_LEFT);
                    lpename->set_halign(Gtk::ALIGN_START);
                    lpename->set_valign(Gtk::ALIGN_CENTER);
                    lpename->set_width_chars(-1);
                    lpename->set_max_width_chars(-1);
                } else {
                    lpename->set_justify(Gtk::JUSTIFY_CENTER);
                    lpename->set_halign(Gtk::ALIGN_CENTER);
                    lpename->set_valign(Gtk::ALIGN_CENTER);
                    lpename->set_width_chars(14);
                    lpename->set_max_width_chars(23);
                }
            }
            Gtk::EventBox *lpemore = dynamic_cast<Gtk::EventBox *>(contents[4]);
            if (lpemore) {
                if (mode == 2) {
                    lpemore->hide();
                } else {
                    if (child2->is_selected()) {
                        lpemore->hide();
                    } else {
                        lpemore->show();
                    }
                }
            }
            Gtk::ButtonBox *lpebuttonbox = dynamic_cast<Gtk::ButtonBox *>(contents[5]);
            if (lpebuttonbox) {
                if (mode == 2) {
                    lpebuttonbox->hide();
                } else {
                    if (child2->is_selected()) {
                        lpebuttonbox->show();
                    } else {
                        lpebuttonbox->hide();
                    }
                }
            }
            Gtk::Label *lpedesc = dynamic_cast<Gtk::Label *>(contents[2]);
            if (lpedesc) {
                if (mode == 2) {
                    lpedesc->show();
                    lpedesc->set_justify(Gtk::JUSTIFY_LEFT);
                    lpedesc->set_halign(Gtk::ALIGN_START);
                    lpedesc->set_valign(Gtk::ALIGN_CENTER);
                    lpedesc->set_ellipsize(Pango::ELLIPSIZE_END);
                } else {
                    lpedesc->hide();
                    lpedesc->set_justify(Gtk::JUSTIFY_CENTER);
                    lpedesc->set_halign(Gtk::ALIGN_CENTER);
                    lpedesc->set_valign(Gtk::ALIGN_CENTER);
                    lpedesc->set_ellipsize(Pango::ELLIPSIZE_NONE);
                }
            }
            Gtk::Overlay *overlay = dynamic_cast<Gtk::Overlay *>(contents[0]);
            if (overlay) {
                std::vector<Gtk::Widget *> contents_overlay = overlay->get_children();
                Gtk::Image *icon = dynamic_cast<Gtk::Image *>(contents_overlay[0]);
                if (icon) {
                    if (mode == 2) {
                        icon->set_pixel_size(33);
                        icon->set_margin_right(40);
                    } else {
                        icon->set_pixel_size(60);
                        icon->set_margin_right(0);
                    }
                }
                Gtk::EventBox *LPESelectorEffectEventFavTop = dynamic_cast<Gtk::EventBox *>(contents_overlay[1]);
                Gtk::Image *fav = dynamic_cast<Gtk::Image *>(LPESelectorEffectEventFavTop->get_child());
                if (LPESelectorEffectEventFavTop) {
                    if (sp_has_fav(name2)) {
                        fav->set_from_icon_name("draw-star", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
                        LPESelectorEffectEventFavTop->set_visible(true);
                        LPESelectorEffectEventFavTop->show();
                        child2->get_style_context()->add_class("lpefav");
                        child2->get_style_context()->remove_class("lpenormal");
                    } else if (!sp_has_fav(name2)) {
                        fav->set_from_icon_name("draw-star-outline", Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR));
                        LPESelectorEffectEventFavTop->set_visible(false);
                        LPESelectorEffectEventFavTop->hide();
                        child2->get_style_context()->remove_class("lpefav");
                        child2->get_style_context()->add_class("lpenormal");
                    }
                    if (mode == 2) {
                        LPESelectorEffectEventFavTop->set_visible(true);
                        LPESelectorEffectEventFavTop->show();
                        LPESelectorEffectEventFavTop->set_halign(Gtk::ALIGN_END);
                        LPESelectorEffectEventFavTop->set_valign(Gtk::ALIGN_CENTER);
                    } else {
                        LPESelectorEffectEventFavTop->set_halign(Gtk::ALIGN_END);
                        LPESelectorEffectEventFavTop->set_valign(Gtk::ALIGN_START);
                    }
                    child2->get_style_context()->add_class("lpe");
                }
            }
        }
    }
    std::vector<Glib::ustring> effect;
    effect.push_back(name1);
    effect.push_back(name2);
    sort(effect.begin(), effect.end());
    /*     if (sp_has_fav(name1) && sp_has_fav(name2)) {
            return effect[0] == name1?-1:1;
        }
        if (sp_has_fav(name1)) {
            return -1;
        } */
    if (effect[0] == name1) { //&& !sp_has_fav(name2)) {
        return -1;
    }
    return 1;
}


void LivePathEffectAdd::onClose() { _LPEDialogSelector->hide(); }

void LivePathEffectAdd::onKeyEvent(GdkEventKey *evt)
{
    if (evt->keyval == GDK_KEY_Escape) {
        onClose();
    }
}

void LivePathEffectAdd::show(SPDesktop *desktop)
{
    LivePathEffectAdd &dial = instance();
    Inkscape::Selection *sel = desktop->getSelection();
    if (sel && !sel->isEmpty()) {
        SPItem *item = sel->singleItem();
        if (item) {
            SPShape *shape = dynamic_cast<SPShape *>(item);
            SPPath *path = dynamic_cast<SPPath *>(item);
            SPGroup *group = dynamic_cast<SPGroup *>(item);
            dial._has_clip = (item->getClipObject() != nullptr);
            dial._has_mask = (item->getMaskObject() != nullptr);
            dial._item_type = "";
            if (group) {
                dial._item_type = "group";
            } else if (path) {
                dial._item_type = "path";
            } else if (shape) {
                dial._item_type = "shape";
            } else {
                dial._LPEDialogSelector->hide();
                return;
            }
        }
    }
    dial._applied = false;
    dial._LPESelectorFlowBox->unset_sort_func();
    dial._LPESelectorFlowBox->unset_filter_func();
    dial._LPESelectorFlowBox->set_filter_func(sigc::mem_fun(dial, &LivePathEffectAdd::on_filter));
    dial._LPESelectorFlowBox->set_sort_func(sigc::mem_fun(dial, &LivePathEffectAdd::on_sort));
    Glib::RefPtr<Gtk::Adjustment> vadjust = dial._LPEScrolled->get_vadjustment();
    vadjust->set_value(vadjust->get_lower());
    dial._LPEDialogSelector->show();
    int searchlen = dial._LPEFilter->get_text().length();
    if (searchlen > 0) {
        dial._LPEFilter->select_region (0, searchlen);
        dial._LPESelectorFlowBox->unselect_all();
    } else if (dial._lasteffect) {
        dial._lasteffect->grab_focus();
    }
    dial._LPEDialogSelector->run();
    dial._LPEDialogSelector->hide();
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
