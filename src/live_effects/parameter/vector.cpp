// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/registered-widget.h"
#include <glibmm/i18n.h>

#include "live_effects/parameter/vector.h"

#include "knotholder.h"
#include "svg/svg.h"
#include "svg/stringstream.h"

#include "live_effects/effect.h"
#include "verbs.h"

namespace Inkscape {

namespace LivePathEffect {

VectorParam::VectorParam( const Glib::ustring& label, const Glib::ustring& tip,
                        const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                        Effect* effect, Geom::Point default_vector)
    : Parameter(label, tip, key, wr, effect),
      defvalue(default_vector),
      origin(0.,0.),
      vector(default_vector)
{
    vec_knot_shape = SP_KNOT_SHAPE_DIAMOND;
    vec_knot_mode  = SP_KNOT_MODE_XOR;
    vec_knot_color = 0xffffb500;
    ori_knot_shape = SP_KNOT_SHAPE_CIRCLE;
    ori_knot_mode  = SP_KNOT_MODE_XOR;
    ori_knot_color = 0xffffb500;
}

VectorParam::~VectorParam()
= default;

void
VectorParam::param_set_default()
{
    setOrigin(Geom::Point(0.,0.));
    setVector(defvalue);
}

void
VectorParam::param_update_default(Geom::Point default_point)
{
    defvalue = default_point;
}

void
VectorParam::param_update_default(const gchar * default_point)
{
    gchar ** strarray = g_strsplit(default_point, ",", 2);
    double newx, newy;
    unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
    success += sp_svg_number_read_d(strarray[1], &newy);
    g_strfreev (strarray);
    if (success == 2) {
        param_update_default( Geom::Point(newx, newy) );
    }
}

bool
VectorParam::param_readSVGValue(const gchar * strvalue)
{
    gchar ** strarray = g_strsplit(strvalue, ",", 4);
    if (!strarray) {
        return false;
    }
    double val[4];
    unsigned int i = 0;
    while (i < 4 && strarray[i]) {
        if (sp_svg_number_read_d(strarray[i], &val[i]) != 0) {
            i++;
        } else {
            break;
        }
    }
    g_strfreev (strarray);
    if (i == 4) {
        setOrigin( Geom::Point(val[0], val[1]) );
        setVector( Geom::Point(val[2], val[3]) );
        return true;
    }
    return false;
}

Glib::ustring
VectorParam::param_getSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << origin << " , " << vector;
    return os.str();
}

Glib::ustring
VectorParam::param_getDefaultSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << defvalue;
    return os.str();
}

Gtk::Widget *
VectorParam::param_newWidget()
{
    Inkscape::UI::Widget::RegisteredVector * pointwdg = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredVector( param_label,
                                                    param_tooltip,
                                                    param_key,
                                                    *param_wr,
                                                    param_effect->getRepr(),
                                                    param_effect->getSPDoc() ) );
    pointwdg->setPolarCoords();
    pointwdg->setValue( vector, origin );
    pointwdg->clearProgrammatically();
    pointwdg->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Change vector parameter"));

    Gtk::HBox * hbox = Gtk::manage( new Gtk::HBox() );
    static_cast<Gtk::HBox*>(hbox)->pack_start(*pointwdg, true, true);
    static_cast<Gtk::HBox*>(hbox)->show_all_children();

    return dynamic_cast<Gtk::Widget *> (hbox);
}

void
VectorParam::set_and_write_new_values(Geom::Point const &new_origin, Geom::Point const &new_vector)
{
    setValues(new_origin, new_vector);
    param_write_to_repr(param_getSVGValue().c_str());
}

void
VectorParam::param_transform_multiply(Geom::Affine const& postmul, bool /*set*/)
{
    set_and_write_new_values( origin * postmul, vector * postmul.withoutTranslation() );
}


void
VectorParam::set_vector_oncanvas_looks(SPKnotShapeType shape, SPKnotModeType mode, guint32 color)
{
    vec_knot_shape = shape;
    vec_knot_mode  = mode;
    vec_knot_color = color;
}

void
VectorParam::set_origin_oncanvas_looks(SPKnotShapeType shape, SPKnotModeType mode, guint32 color)
{
    ori_knot_shape = shape;
    ori_knot_mode  = mode;
    ori_knot_color = color;
}

void
VectorParam::set_oncanvas_color(guint32 color)
{
    vec_knot_color = color;
    ori_knot_color = color;
}

class VectorParamKnotHolderEntity_Origin : public KnotHolderEntity {
public:
    VectorParamKnotHolderEntity_Origin(VectorParam *p) : param(p) { }
    ~VectorParamKnotHolderEntity_Origin() override = default;

    void knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, guint state) override {
        Geom::Point const s = snap_knot_position(p, state);
        param->setOrigin(s);
        param->set_and_write_new_values(param->origin, param->vector);
        sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
    };
    Geom::Point knot_get() const override {
        return param->origin;
    };
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override
    {
        param->param_effect->refresh_widgets = true;
        param->write_to_SVG();
    };
    void knot_click(guint /*state*/) override{
        g_print ("This is the origin handle associated to parameter '%s'\n", param->param_key.c_str());
    };

private:
    VectorParam *param;
};

class VectorParamKnotHolderEntity_Vector : public KnotHolderEntity {
public:
    VectorParamKnotHolderEntity_Vector(VectorParam *p) : param(p) { }
    ~VectorParamKnotHolderEntity_Vector() override = default;

    void knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, guint /*state*/) override {
        Geom::Point const s = p - param->origin;
        /// @todo implement angle snapping when holding CTRL
        param->setVector(s);
        param->set_and_write_new_values(param->origin, param->vector);
        sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
    };
    Geom::Point knot_get() const override {
        return param->origin + param->vector;
    };
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override
    {
        param->param_effect->refresh_widgets = true;
        param->write_to_SVG();
    };
    void knot_click(guint /*state*/) override{
        g_print ("This is the vector handle associated to parameter '%s'\n", param->param_key.c_str());
    };

private:
    VectorParam *param;
};

void
VectorParam::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    VectorParamKnotHolderEntity_Origin *origin_e = new VectorParamKnotHolderEntity_Origin(this);
    origin_e->create(nullptr, item, knotholder, Inkscape::CTRL_TYPE_LPE, handleTip(), ori_knot_shape, ori_knot_mode,
                     ori_knot_color);
    knotholder->add(origin_e);

    VectorParamKnotHolderEntity_Vector *vector_e = new VectorParamKnotHolderEntity_Vector(this);
    vector_e->create(nullptr, item, knotholder, Inkscape::CTRL_TYPE_LPE, handleTip(), vec_knot_shape, vec_knot_mode,
                     vec_knot_color);
    knotholder->add(vector_e);
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
