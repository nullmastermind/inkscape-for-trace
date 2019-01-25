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

#include "live_effects/effect.h"
#include "livepatheffect-add.h"
#include <glibmm/i18n.h>
#include "io/resource.h"
#include "desktop.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

LivePathEffectAdd::LivePathEffectAdd() :
    _add_button(_("_Add"), true),
    _close_button(_("_Cancel"), true),
    converter(Inkscape::LivePathEffect::LPETypeConverter)
{
    const std::string req_widgets[] = {"LPESelector", "LPESelectorFlowBox"};
    Glib::ustring gladefile = get_filename(Inkscape::IO::Resource::UIS, "lpe-selector.glade");
    try {
        _builder = Gtk::Builder::create_from_file(gladefile);
    } catch(const Glib::Error& ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }

    Gtk::Object* test;
    for(std::string w:req_widgets) {
        _builder->get_widget(w,test);
        if(!test){
            g_warning("Required widget %s does not exist", w.c_str());
            return;
        }
    }
    _builder->get_widget("LPESelector", _LPESelector);
    auto mainVBox = get_content_area();
    mainVBox->pack_start(*_LPESelector, true, true);
    /**
     * Initialize Effect list
     */
    _builder->get_widget("LPESelectorFlowBox", _LPESelectorFlowBox);
    _builder->get_widget("LPEFilter", _LPEFilter);
    _builder->get_widget("LPEInfo", _LPEInfo);
    _LPEFilter->signal_search_changed().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_search));
    const std::string le_widgets[] = {"LPESelectorItem", "LPEName","LPEDescription"};
    Glib::ustring le_gladefile = get_filename(Inkscape::IO::Resource::UIS, "lpe-selector-item.glade");
    for(int i = 0; i < static_cast<int>(converter._length); ++i) {
        
        try {
            _builder = Gtk::Builder::create_from_file(le_gladefile);
        } catch(const Glib::Error& ex) {
            g_warning("Glade file loading failed for filter effect dialog");
            return;
        }

        Gtk::Object* test;
        for(std::string w:le_widgets) {
            _builder->get_widget(w,test);
            if(!test){
                g_warning("Required widget %s does not exist", w.c_str());
                return;
            }
        }
        const LivePathEffect::EnumEffectData<LivePathEffect::EffectType>* data = &converter.data(i);
        Gtk::Label * LPEName;
        _builder->get_widget("LPEName", LPEName);
        Glib::ustring newid = "LPEName_" + Glib::ustring::format(i);
        (*LPEName).set_name(newid);
        (*LPEName).set_text(converter.get_label(data->id).c_str());
        Gtk::Label * LPEDescription;
        _builder->get_widget("LPEDescription", LPEDescription);
        newid = "LPEDescription_" + Glib::ustring::format(i);
        (*LPEDescription).set_name(newid);
        (*LPEDescription).set_text(converter.get_description(data->id));
        Gtk::Image * LPEIcon;
        _builder->get_widget("LPEIcon", LPEIcon);
        newid = "LPEIcon_" + Glib::ustring::format(i);
        (*LPEIcon).set_name(newid);
        (*LPEIcon).set_from_icon_name(converter.get_icon(data->id),Gtk::BuiltinIconSize(Gtk::ICON_SIZE_DIALOG));
        Gtk::Box * LPESelectorItem;
        _builder->get_widget("LPESelectorItem", LPESelectorItem);
        newid = "LPESelectorItem" + Glib::ustring::format(i);
        (*LPESelectorItem).set_name(newid);
        _LPESelectorFlowBox->insert(*LPESelectorItem, i);
    }
    _visiblelpe = _LPESelectorFlowBox->get_children().size();
    _LPESelectorFlowBox->signal_child_activated().connect(sigc::mem_fun(*this, &LivePathEffectAdd::on_activate));
    set_title(_("Live Efects Selector"));
    show_all_children();
    _LPEInfo->set_visible(false);
}

void LivePathEffectAdd::on_activate(Gtk::FlowBoxChild *child){
    for (auto i:_LPESelectorFlowBox->get_children()) {
        Gtk::FlowBoxChild * leitem = dynamic_cast<Gtk::FlowBoxChild *>(i);
        leitem->get_style_context()->remove_class("lpeactive");
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(leitem->get_child());
        if (box) {
            std::vector<Gtk::Widget*> contents = box->get_children();
            Gtk::Box *actions = dynamic_cast<Gtk::Box *>(contents[3]);
            if (actions) {
                actions->set_visible(false);
            }
        }
    }
    child->get_style_context()->add_class("lpeactive");
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
        std::vector<Gtk::Widget*> contents = box->get_children();
        Gtk::Label *lpename = dynamic_cast<Gtk::Label *>(contents[1]);
        if (lpename) {
            size_t s = lpename->get_text().uppercase().find(_LPEFilter->get_text().uppercase(),0);
            if(s != -1) {
                _visiblelpe++;
                return true;
            }
        }
        Gtk::Label *lpedesc = dynamic_cast<Gtk::Label *>(contents[2]);
        if (lpedesc) {
            size_t s = lpedesc->get_text().uppercase().find(_LPEFilter->get_text().uppercase(),0);
            if(s != -1) {
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

void LivePathEffectAdd::onClose()
{
    hide();
}

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
    dial.set_modal(true);
    //dial.set_decorated(false);
    dial.set_name("lpedialogselector");
    //desktop->setWindowTransient (dial.gobj());
    dial.property_destroy_with_parent() = true;
    dial.run();
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
