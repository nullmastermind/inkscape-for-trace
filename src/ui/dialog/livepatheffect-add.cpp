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
#include "preferences.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

bool sp_has_fav(Glib::ustring effect){
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring favlist = prefs->getString("/dialogs/livepatheffect/favs");
    size_t pos = favlist.find(effect);
	if (pos != std::string::npos){
        return true;
    }
    return false;
}

void sp_add_fav(Glib::ustring effect){
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring favlist = prefs->getString("/dialogs/livepatheffect/favs");
    if (!sp_has_fav(effect)) {
        prefs->setString("/dialogs/livepatheffect/favs", favlist + effect + ";");
    }
}

void sp_remove_fav(Glib::ustring effect){
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring favlist = prefs->getString("/dialogs/livepatheffect/favs");
    effect += ";";
    size_t pos = favlist.find(effect);
	if (pos != std::string::npos)
	{
      	favlist.erase(pos, effect.length());
        prefs->setString("/dialogs/livepatheffect/favs", favlist);
	}
}

bool 
LivePathEffectAdd::mouseover(GdkEventCrossing* evt, GtkWidget *wdg)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor  *cursor  = gdk_cursor_new_for_display(display, GDK_HAND2);
    GdkWindow *window = gtk_widget_get_window(wdg);
    gdk_window_set_cursor(window, cursor);
    g_object_unref(cursor);
    return true;
}

bool
LivePathEffectAdd::mouseout(GdkEventCrossing* evt, GtkWidget *wdg)
{
    GdkWindow *window = gtk_widget_get_window(wdg);
    gdk_window_set_cursor(window, nullptr);
    return true;
}

LivePathEffectAdd::LivePathEffectAdd()
    : converter(Inkscape::LivePathEffect::LPETypeConverter)
{
    Glib::ustring gladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-add.ui");
    try {
        _builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }
    _showfavs = false;
    _builder->get_widget("LPEDialogSelector", _LPEDialogSelector);
    _builder->get_widget("LPESelectorFlowBox", _LPESelectorFlowBox);
    _builder->get_widget("LPESelectorEffectInfoPop", _LPESelectorEffectInfoPop);
    _builder->get_widget("LPEFilter", _LPEFilter);
    _builder->get_widget("LPEInfo", _LPEInfo);
    _builder->get_widget("LPEExperimentals", _LPEExperimentals);
    _builder->get_widget("LPEScrolled", _LPEScrolled);
    _builder->get_widget("LPESelectorEffectEventFavShow", _LPESelectorEffectEventFavShow);
    _LPEFilter->signal_search_changed().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_search));
    _LPESelectorFlowBox->signal_child_activated().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_activate));
    _LPEDialogSelector->add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |  Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK );    
    Glib::ustring effectgladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-livepatheffect-add-effect.ui");
    for (int i = 0; i < static_cast<int>(converter._length); ++i) {
        Glib::RefPtr<Gtk::Builder> builder_effect;
        try {
            builder_effect = Gtk::Builder::create_from_file(effectgladefile);
        } catch (const Glib::Error &ex) {
            g_warning("Glade file loading failed for filter effect dialog");
            return;
        }
        Gtk::EventBox *LPESelectorEffect;
        builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
        const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *data = &converter.data(i);
        Gtk::Label *LPEName;
        builder_effect->get_widget("LPEName", LPEName);
        LPEName->set_text(converter.get_label(data->id).c_str());
        Gtk::Label *LPEDescription;
        builder_effect->get_widget("LPEDescription", LPEDescription);
        LPEDescription->set_text(converter.get_description(data->id));
        Gtk::ToggleButton *LPEExperimental;
        builder_effect->get_widget("LPEExperimental", LPEExperimental);
        bool active = converter.get_experimental(data->id)?true:false;
        LPEExperimental->set_active(active);
        Gtk::Image *LPEIcon;
        builder_effect->get_widget("LPEIcon", LPEIcon);
        LPEIcon->set_from_icon_name(converter.get_icon(data->id), Gtk::BuiltinIconSize(Gtk::ICON_SIZE_DIALOG));
        Gtk::EventBox *LPESelectorEffectEventInfo;
        builder_effect->get_widget("LPESelectorEffectEventInfo", LPESelectorEffectEventInfo);
        LPESelectorEffectEventInfo->signal_button_press_event().connect(sigc::bind<Glib::RefPtr<Gtk::Builder> >(sigc::mem_fun(*this, &LivePathEffectAdd::pop_description), builder_effect));
        Gtk::EventBox *LPESelectorEffectEventFav;
        builder_effect->get_widget("LPESelectorEffectEventFav", LPESelectorEffectEventFav);
        if (sp_has_fav(LPEName->get_text())){
            Gtk::Image *fav = dynamic_cast<Gtk::Image *>(LPESelectorEffectEventFav->get_child());
            fav->set_from_icon_name("draw-star", Gtk::IconSize(25));
        }
        Gtk::EventBox *LPESelectorEffectEventFavTop;
        builder_effect->get_widget("LPESelectorEffectEventFavTop", LPESelectorEffectEventFavTop);
        LPESelectorEffectEventFav->signal_button_press_event().connect(sigc::bind<Glib::RefPtr<Gtk::Builder> >(sigc::mem_fun(*this, &LivePathEffectAdd::fav_toggler), builder_effect));
        LPESelectorEffectEventFavTop->signal_button_press_event().connect(sigc::bind<Glib::RefPtr<Gtk::Builder> >(sigc::mem_fun(*this, &LivePathEffectAdd::fav_toggler), builder_effect));
        Gtk::ButtonBox *LPESelectorButtonBox;
        builder_effect->get_widget("LPESelectorButtonBox", LPESelectorButtonBox);
        LPESelectorButtonBox->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(LPESelectorEffect->gobj())));
        LPESelectorButtonBox->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(LPESelectorEffect->gobj())));
        LPESelectorEffect->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(LPESelectorEffect->gobj())));
        LPESelectorEffect->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(LPESelectorEffect->gobj())));
        
        _LPESelectorFlowBox->insert(*LPESelectorEffect, i);
    }
    _visiblelpe = _LPESelectorFlowBox->get_children().size();
    _LPEInfo->set_visible(false);
    _LPESelectorEffectEventFavShow->signal_enter_notify_event().connect(sigc::bind<GtkWidget *>(sigc::mem_fun(*this, &LivePathEffectAdd::mouseover), GTK_WIDGET(_LPESelectorEffectEventFavShow->gobj())));
    _LPESelectorEffectEventFavShow->signal_leave_notify_event().connect(sigc::bind<GtkWidget *>(sigc::mem_fun(*this, &LivePathEffectAdd::mouseout), GTK_WIDGET(_LPESelectorEffectEventFavShow->gobj())));
    _LPESelectorEffectEventFavShow->signal_button_press_event().connect(sigc::mem_fun(*this, &LivePathEffectAdd::show_fav_toggler));
    _LPESelectorFlowBox->set_sort_func(sigc::mem_fun(*this, &LivePathEffectAdd::on_sort));
    _LPESelectorFlowBox->set_filter_func(sigc::mem_fun(*this, &LivePathEffectAdd::on_filter));
    _LPEExperimentals->property_active().signal_changed().connect(sigc::mem_fun(*this, &LivePathEffectAdd::reload_effect_list));
    _LPEDialogSelector->show_all_children();
}

void LivePathEffectAdd::on_activate(Gtk::FlowBoxChild *child)
{
    for (auto i : _LPESelectorFlowBox->get_children()) {
        Gtk::FlowBoxChild *leitem = dynamic_cast<Gtk::FlowBoxChild *>(i);
        leitem->get_style_context()->remove_class("lpeactive");
        leitem->get_style_context()->remove_class("colorinverse");
        leitem->get_style_context()->remove_class("backgroundinverse");
        Gtk::EventBox *eventbox = dynamic_cast<Gtk::EventBox *>(leitem->get_child());
        if (eventbox) {
            Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
            if (box) {
                std::vector<Gtk::Widget *> contents = box->get_children();
                Gtk::Box *actions = dynamic_cast<Gtk::Box *>(contents[4]);
                if (actions) {
                    actions->set_visible(false);
                }
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
    LPESelectorEffectInfoIcon->set_from_icon_name(LPEIcon->get_icon_name(), Gtk::IconSize(60));
        
    Gtk::Label *LPESelectorEffectInfoName;
    _builder->get_widget("LPESelectorEffectInfoName", LPESelectorEffectInfoName);
    LPESelectorEffectInfoName->set_text(LPEName->get_text());
    
    Gtk::Label *LPESelectorEffectInfoDescription;
    _builder->get_widget("LPESelectorEffectInfoDescription", LPESelectorEffectInfoDescription);
    LPESelectorEffectInfoDescription->set_text(LPEDescription->get_text());
    
    _LPESelectorEffectInfoPop->show();
    
    return true;
}

bool LivePathEffectAdd::fav_toggler(GdkEventButton* evt, Glib::RefPtr<Gtk::Builder> builder_effect)
{
    Gtk::EventBox *LPESelectorEffect;
    builder_effect->get_widget("LPESelectorEffect", LPESelectorEffect);
    Gtk::Label *LPEName;
    builder_effect->get_widget("LPEName", LPEName);
    Gtk::Image *LPESelectorEffectFav;
    builder_effect->get_widget("LPESelectorEffectFav", LPESelectorEffectFav);
    Gtk::EventBox *LPESelectorEffectEventFavTop;
    builder_effect->get_widget("LPESelectorEffectEventFavTop", LPESelectorEffectEventFavTop);
    if (LPESelectorEffectFav && LPESelectorEffectEventFavTop) {
        if (sp_has_fav(LPEName->get_text())){
            LPESelectorEffectEventFavTop->set_visible(false);
            LPESelectorEffectEventFavTop->hide();
            LPESelectorEffectFav->set_from_icon_name("draw-star-outline", Gtk::IconSize(25));
            sp_remove_fav(LPEName->get_text());
            LPESelectorEffect->get_parent()->get_style_context()->remove_class("lpefav");
        } else {
            LPESelectorEffectEventFavTop->set_visible(true);
            LPESelectorEffectEventFavTop->show();
            LPESelectorEffectFav->set_from_icon_name("draw-star", Gtk::IconSize(25));
            sp_add_fav(LPEName->get_text());
            LPESelectorEffect->get_parent()->get_style_context()->add_class("lpefav");
            Glib::RefPtr< Gtk::Adjustment > vadjust = _LPEScrolled->get_vadjustment();
            vadjust->set_value(vadjust->get_lower());    
        }
    }
    return true;
}

bool LivePathEffectAdd::show_fav_toggler(GdkEventButton* evt)
{
    _showfavs = !_showfavs;
    Gtk::Image *favimage = dynamic_cast<Gtk::Image *>(_LPESelectorEffectEventFavShow->get_child());
    if (favimage) {
        if (_showfavs) {
            favimage->set_from_icon_name("draw-star", Gtk::IconSize(favimage->get_pixel_size()));
        } else {
            favimage->set_from_icon_name("draw-star-outline", Gtk::IconSize(favimage->get_pixel_size()));
        }
    }
    _LPESelectorFlowBox->invalidate_filter();
    return true;
}

bool LivePathEffectAdd::on_filter(Gtk::FlowBoxChild *child)
{
    child->set_valign(Gtk::ALIGN_START);
    Gtk::EventBox *eventbox = dynamic_cast<Gtk::EventBox *>(child->get_child());
    if (eventbox) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
        if (box) {
            std::vector<Gtk::Widget *> contents = box->get_children();
            Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
            if (!sp_has_fav(lpename->get_text()) && _showfavs){
                return false;
            }
            Gtk::ToggleButton *experimental = dynamic_cast<Gtk::ToggleButton *>(contents[3]);
            if (experimental) {
                if (experimental->get_active() && _LPEExperimentals->get_active()) {
                    return false;
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
            Gtk::Label *lpedesc = dynamic_cast<Gtk::Label *>(contents[2]);
            if (lpedesc) {
                size_t s = lpedesc->get_text().uppercase().find(_LPEFilter->get_text().uppercase(), 0);
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
    /* if(_LPEExperimentals->get_active()) {
        _LPEExperimentals->get_style_context()->add_class("active");
    } else {
        _LPEExperimentals->get_style_context()->remove_class("active");
    } */
    _LPESelectorFlowBox->invalidate_filter();
}

void LivePathEffectAdd::on_search()
{
    _visiblelpe = 0;
    _LPESelectorFlowBox->invalidate_filter();
    if (_visiblelpe == 0) {
        _LPEInfo->set_text(_("Your search do a empty result, please try again"));
        _LPEInfo->set_visible(true);
        _LPEInfo->get_style_context()->add_class("lpeinfowarn");
    } else {
        _LPEInfo->set_visible(false);
        _LPEInfo->get_style_context()->remove_class("lpeinfowarn");
    }
}

int LivePathEffectAdd::on_sort(Gtk::FlowBoxChild *child1, Gtk::FlowBoxChild *child2)
{
    Glib::ustring name1 = "";
    Glib::ustring name2 = "";
    Gtk::EventBox *eventbox = dynamic_cast<Gtk::EventBox *>(child1->get_child());
    if (eventbox) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
        if (box) {
            std::vector<Gtk::Widget *> contents = box->get_children();
            Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
            name1 = lpename->get_text();
            Gtk::Overlay *overlay = dynamic_cast<Gtk::Overlay *>(contents[0]);
            if (overlay) {
                std::vector<Gtk::Widget *> contents_overlay = overlay->get_children();
                Gtk::EventBox *LPESelectorEffectEventFavTop = dynamic_cast<Gtk::EventBox *>(contents_overlay[1]);
                if (LPESelectorEffectEventFavTop) {
                    if (sp_has_fav(name1)){
                        LPESelectorEffectEventFavTop->set_visible(true);
                        LPESelectorEffectEventFavTop->show();
                        child1->get_style_context()->add_class("lpefav");
                    } else {
                        LPESelectorEffectEventFavTop->set_visible(false);
                        LPESelectorEffectEventFavTop->hide();
                        child1->get_style_context()->remove_class("lpefav");
                    }
                }
            }
        }
    }
    eventbox = dynamic_cast<Gtk::EventBox *>(child2->get_child());
    if (eventbox) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(eventbox->get_child());
        if (box) {
            std::vector<Gtk::Widget *> contents = box->get_children();
            Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
            name2 = lpename->get_text();
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
    if (effect[0] == name1) {//&& !sp_has_fav(name2)) {
        return -1;
    }
    return 1;
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
    dial._LPESelectorFlowBox->invalidate_filter();
    dial._LPESelectorFlowBox->invalidate_sort();
    Glib::RefPtr< Gtk::Adjustment > vadjust = dial._LPEScrolled->get_vadjustment();
    vadjust->set_value(vadjust->get_lower());
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
