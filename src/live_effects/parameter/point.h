// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_POINT_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_POINT_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <2geom/point.h>
#include "ui/widget/registered-widget.h"
#include "live_effects/parameter/parameter.h"

#include "knot-holder-entity.h"

namespace Inkscape {

namespace LivePathEffect {

class PointParamKnotHolderEntity;

class PointParam : public Geom::Point, public Parameter {
public:
    PointParam( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect,
                const gchar *handle_tip = nullptr,// tip for automatically associated on-canvas handle
                Geom::Point default_value = Geom::Point(0,0), 
                bool live_update = true );
    ~PointParam() override;

    Gtk::Widget * param_newWidget() override;

    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    inline const gchar *handleTip() const { return handle_tip ? handle_tip : param_tooltip.c_str(); }
    void param_setValue(Geom::Point newpoint, bool write = false);
    void param_set_default() override;
    void param_hide_knot(bool hide);
    Geom::Point param_get_default() const;
    void param_set_liveupdate(bool live_update);
    void param_update_default(Geom::Point default_point);

    void param_update_default(const gchar * default_point) override;
    void param_transform_multiply(Geom::Affine const & /*postmul*/, bool set) override;

    void set_oncanvas_looks(SPKnotShapeType shape, SPKnotModeType mode, guint32 color);

    bool providesKnotHolderEntities() const override { return true; }
    void addKnotHolderEntities(KnotHolder *knotholder, SPItem *item) override;
    friend class PointParamKnotHolderEntity;
private:
    PointParam(const PointParam&) = delete;
    PointParam& operator=(const PointParam&) = delete;
    bool on_button_release(GdkEventButton* button_event);
    Geom::Point defvalue;
    bool liveupdate;
    KnotHolderEntity * _knot_entity;
    SPKnotShapeType knot_shape;
    SPKnotModeType knot_mode;
    guint32 knot_color;
    gchar *handle_tip;
};


} //namespace LivePathEffect

} //namespace Inkscape

#endif
