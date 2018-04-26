/*
 * Author:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Felipe Corrêa da Silva Sanches, Tavmong Bah
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_FONT_VARIATIONS_H
#define INKSCAPE_UI_WIDGET_FONT_VARIATIONS_H

#include <gtkmm/grid.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>

#include "libnrtype/OpenTypeUtil.h"

namespace Inkscape {
namespace UI {
namespace Widget {


/**
 * A widget for a single axis.
 */
class FontVariationAxis : public Gtk::Grid
{
public:
    FontVariationAxis(Glib::ustring name, OTVarAxis& axis);
    Glib::ustring get_name() { return name; }
    Gtk::Label* get_label() { return label; }
    double get_value() { return scale->get_value(); }
    int get_precision() { return precision; }

private:

    Glib::ustring name;
    Gtk::Label* label;
    Gtk::Scale* scale;
    int precision;
    sigc::connection _changed_connection;


};

/**
 * A widget for selecting font variations (OpenType Variations).
 */
class FontVariations : public Gtk::Grid
{

public:

    /**
     * Constructor
     */
    FontVariations();

protected:

public:

    /**
     * Update GUI based on query results.
     */
    void update( const SPStyle& query, bool different_features, Glib::ustring& font_spec );

    /**
     * Fill SPCSSAttr based on settings of buttons.
     */
    void fill_css( SPCSSAttr* css );

    /**
     * Get CSS String
     */
    Glib::ustring get_css_string();

private:
    std::vector<FontVariationAxis*> axes;
    Glib::RefPtr<Gtk::SizeGroup> size_group;
};

 
} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_FONT_VARIATIONS_H

/* 
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
