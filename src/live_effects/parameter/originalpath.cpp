// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/box.h>
#include "live_effects/parameter/originalpath.h"

#include <glibmm/i18n.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>

#include "display/curve.h"
#include "live_effects/effect.h"

#include "object/uri.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"

#include "inkscape.h"
#include "desktop.h"
#include "selection.h"
#include "ui/icon-names.h"

namespace Inkscape {

namespace LivePathEffect {

OriginalPathParam::OriginalPathParam( const Glib::ustring& label, const Glib::ustring& tip,
                      const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                      Effect* effect)
    : PathParam(label, tip, key, wr, effect, "")
{
    oncanvas_editable = false;
    _from_original_d = false;
}

OriginalPathParam::~OriginalPathParam()
= default;

Gtk::Widget *
OriginalPathParam::param_newWidget()
{
    Gtk::Box *_widget = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

    { // Label
        Gtk::Label *pLabel = Gtk::manage(new Gtk::Label(param_label));
        _widget->pack_start(*pLabel, true, true);
        pLabel->set_tooltip_text(param_tooltip);
    }

    { // Paste path to link button
        Gtk::Image *pIcon = Gtk::manage(new Gtk::Image());
        pIcon->set_from_icon_name("edit-clone", Gtk::ICON_SIZE_BUTTON);
        Gtk::Button *pButton = Gtk::manage(new Gtk::Button());
        pButton->set_relief(Gtk::RELIEF_NONE);
        pIcon->show();
        pButton->add(*pIcon);
        pButton->show();
        pButton->signal_clicked().connect(sigc::mem_fun(*this, &OriginalPathParam::on_link_button_click));
        _widget->pack_start(*pButton, true, true);
        pButton->set_tooltip_text(_("Link to path in clipboard"));
    }

    { // Select original button
        Gtk::Image *pIcon = Gtk::manage(new Gtk::Image());
        pIcon->set_from_icon_name("edit-select-original", Gtk::ICON_SIZE_BUTTON);
        Gtk::Button *pButton = Gtk::manage(new Gtk::Button());
        pButton->set_relief(Gtk::RELIEF_NONE);
        pIcon->show();
        pButton->add(*pIcon);
        pButton->show();
        pButton->signal_clicked().connect(sigc::mem_fun(*this, &OriginalPathParam::on_select_original_button_click));
        _widget->pack_start(*pButton, true, true);
        pButton->set_tooltip_text(_("Select original"));
    }

    _widget->show_all_children();

    return dynamic_cast<Gtk::Widget *> (_widget);
}

void
OriginalPathParam::on_select_original_button_click()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPItem *original = ref.getObject();
    if (desktop == nullptr || original == nullptr) {
        return;
    }
    Inkscape::Selection *selection = desktop->getSelection();
    selection->clear();
    selection->set(original);
    SP_OBJECT(param_effect->getLPEObj())->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

} /* namespace LivePathEffect */

} /* namespace Inkscape */

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
