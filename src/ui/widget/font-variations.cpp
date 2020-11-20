// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Felipe Corrêa da Silva Sanches, Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>
#include <iomanip>

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include <libnrtype/font-instance.h>

#include "font-variations.h"

// For updating from selection
#include "desktop.h"
#include "object/sp-text.h"

namespace Inkscape {
namespace UI {
namespace Widget {

FontVariationAxis::FontVariationAxis (Glib::ustring name, OTVarAxis& axis)
    : name (name)
{

    // std::cout << "FontVariationAxis::FontVariationAxis:: "
    //           << " name: " << name
    //           << " min:  " << axis.minimum
    //           << " def:  " << axis.def
    //           << " max:  " << axis.maximum
    //           << " val:  " << axis.set_val << std::endl;

    label = Gtk::manage( new Gtk::Label( name ) );
    add( *label );

    precision = 2 - int( log10(axis.maximum - axis.minimum));
    if (precision < 0) precision = 0;

    scale = Gtk::manage( new Gtk::Scale() );
    scale->set_range (axis.minimum, axis.maximum);
    scale->set_value (axis.set_val);
    scale->set_digits (precision);
    scale->set_hexpand(true);
    add( *scale );

    def = axis.def; // Default value
}


// ------------------------------------------------------------- //

FontVariations::FontVariations () :
    Gtk::Grid ()
{
    // std::cout << "FontVariations::FontVariations" << std::endl;
    set_orientation( Gtk::ORIENTATION_VERTICAL );
    set_name ("FontVariations");
    size_group = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    show_all_children();
}


// Update GUI based on query.
void
FontVariations::update (const Glib::ustring& font_spec) {

    font_instance* res = font_factory::Default()->FaceFromFontSpecification (font_spec.c_str());

    auto children = get_children();
    for (auto child: children) {
        remove ( *child );
    }
    axes.clear();

    for (auto a: res->openTypeVarAxes) {
        // std::cout << "Creating axis: " << a.first << std::endl;
        FontVariationAxis* axis = Gtk::manage( new FontVariationAxis( a.first, a.second ));
        axes.push_back( axis );
        add( *axis );
        size_group->add_widget( *(axis->get_label()) ); // Keep labels the same width
        axis->get_scale()->signal_value_changed().connect(
            sigc::mem_fun(*this, &FontVariations::on_variations_change)
            );
    }

    show_all_children();
}

void
FontVariations::fill_css( SPCSSAttr *css ) {

    // Eventually will want to favor using 'font-weight', etc. but at the moment these
    // can't handle "fractional" values. See CSS Fonts Module Level 4.
    sp_repr_css_set_property(css, "font-variation-settings", get_css_string().c_str());
}

Glib::ustring
FontVariations::get_css_string() {

    Glib::ustring css_string;

    for (auto axis: axes) {
        Glib::ustring name = axis->get_name();

        // Translate the "named" axes. (Additional names in 'stat' table, may need to handle them.)
        if (name == "Width")  name = "wdth";       // 'font-stretch'
        if (name == "Weight") name = "wght";       // 'font-weight'
        if (name == "OpticalSize") name = "opsz";  // 'font-optical-sizing' Can trigger glyph substitution.
        if (name == "Slant")  name = "slnt";       // 'font-style'
        if (name == "Italic") name = "ital";       // 'font-style' Toggles from Roman to Italic.

        std::stringstream value;
        value << std::fixed << std::setprecision(axis->get_precision()) << axis->get_value();
        css_string += "'" + name + "' " + value.str() + "', ";
    }

    return css_string;
}

Glib::ustring
FontVariations::get_pango_string() {

    Glib::ustring pango_string;

    if (!axes.empty()) {

        pango_string += "@";

        for (auto axis: axes) {
            if (axis->get_value() == axis->get_def()) continue;
            Glib::ustring name = axis->get_name();

            // Translate the "named" axes. (Additional names in 'stat' table, may need to handle them.)
            if (name == "Width")  name = "wdth";       // 'font-stretch'
            if (name == "Weight") name = "wght";       // 'font-weight'
            if (name == "OpticalSize") name = "opsz";  // 'font-optical-sizing' Can trigger glyph substitution.
            if (name == "Slant")  name = "slnt";       // 'font-style'
            if (name == "Italic") name = "ital";       // 'font-style' Toggles from Roman to Italic.

            std::stringstream value;
            value << std::fixed << std::setprecision(axis->get_precision()) << axis->get_value();
            pango_string += name + "=" + value.str() + ",";
        }

        pango_string.erase (pango_string.size() - 1); // Erase last ',' or '@'
    }

    return pango_string;
}

void
FontVariations::on_variations_change() {
    // std::cout << "FontVariations::on_variations_change: " << get_css_string() << std::endl;;
    signal_changed.emit ();
}

bool FontVariations::variations_present() const {
    return !axes.empty();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
