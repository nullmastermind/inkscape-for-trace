// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Carl Hetherington <inkscape@carlh.net>
 *   Derek P. Moore <derekm@hackunix.org>
 *   Bryce Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2004 Carl Hetherington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "random.h"
#include "ui/icon-loader.h"
#include <glibmm/i18n.h>

#include <gtkmm/button.h>
#include <gtkmm/image.h>

namespace Inkscape {
namespace UI {
namespace Widget {

Random::Random(Glib::ustring const &label, Glib::ustring const &tooltip,
               Glib::ustring const &suffix,
               Glib::ustring const &icon,
               bool mnemonic)
    : Scalar(label, tooltip, suffix, icon, mnemonic)
{
    startseed = 0;
    addReseedButton();
}

Random::Random(Glib::ustring const &label, Glib::ustring const &tooltip,
               unsigned digits,
               Glib::ustring const &suffix,
               Glib::ustring const &icon,
               bool mnemonic)
    : Scalar(label, tooltip, digits, suffix, icon, mnemonic)
{
    startseed = 0;
    addReseedButton();
}

Random::Random(Glib::ustring const &label, Glib::ustring const &tooltip,
               Glib::RefPtr<Gtk::Adjustment> &adjust,
               unsigned digits,
               Glib::ustring const &suffix,
               Glib::ustring const &icon,
               bool mnemonic)
    : Scalar(label, tooltip, adjust, digits, suffix, icon, mnemonic)
{
    startseed = 0;
    addReseedButton();
}

long Random::getStartSeed() const
{
    return startseed;
}

void Random::setStartSeed(long newseed)
{
    startseed = newseed;
}

void Random::addReseedButton()
{
    Gtk::Image *pIcon = Gtk::manage(sp_get_icon_image("randomize", Gtk::ICON_SIZE_BUTTON));
    Gtk::Button * pButton = Gtk::manage(new Gtk::Button());
    pButton->set_relief(Gtk::RELIEF_NONE);
    pIcon->show();
    pButton->add(*pIcon);
    pButton->show();
    pButton->signal_clicked().connect(sigc::mem_fun(*this, &Random::onReseedButtonClick));
    pButton->set_tooltip_text(_("Reseed the random number generator; this creates a different sequence of random numbers."));

    pack_start(*pButton, Gtk::PACK_SHRINK, 0);
}

void
Random::onReseedButtonClick()
{
    startseed = g_random_int();
    signal_reseeded.emit();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
