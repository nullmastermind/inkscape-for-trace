/*
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * and finaly to Liam P. White for his big help on coding, that save me a lot of
 * hours
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "knotholder.h"

// TODO due to internal breakage in glibmm headers,
// this has to be included last.
#include <glibmm/i18n.h>


using namespace Geom;

namespace Inkscape {

namespace LivePathEffect {

PointwiseParam::PointwiseParam(
    const Glib::ustring &label, const Glib::ustring &tip,
    const Glib::ustring &key, Inkscape::UI::Widget::Registry *wr,
    Effect *effect)
    : ArrayParam<Pointwise>(label, tip, key, wr, effect, 0)
{
    knot_shape = SP_KNOT_SHAPE_DIAMOND;
    knot_mode = SP_KNOT_MODE_XOR;
    knot_color = 0x00ff0000;
}

PointwiseParam::~PointwiseParam() {}

Gtk::Widget *PointwiseParam::param_newWidget()
{
    return NULL;
}

void PointwiseParam::set_oncanvas_looks(SPKnotShapeType shape,
        SPKnotModeType mode,
        guint32 color)
{
    knot_shape = shape;
    knot_mode = mode;
    knot_color = color;
}

PointwiseParamKnotHolderEntity::
PointwiseParamKnotHolderEntity(
    PointwiseParam *p)
    : _pparam(p) {}

void PointwiseParamKnotHolderEntity::knot_set(Point const &p,
                                              Point const &/*origin*/,
                                              guint state)
{
    Geom::Point const s = snap_knot_position(p, state);
    _pparam->_vector.at(_index).second.setPosition(s,_pparam->_pointwise.pwd2[_pparam->_vector.at(_index).first]);
    sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
}

Point PointwiseParamKnotHolderEntity::knot_get() const
{
    Geom::Point const canvas_point = _pparam->_vector.at(_index).second.getPosition(_pparam->_pointwise.pwd2[_pparam->_vector.at(_index).first]);
    _pparam->updateCanvasIndicators();
    return canvas_point;
}


void PointwiseParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item)
{
    for (unsigned int i = 0; i < _pointwise.satellites.size(); ++i) {
        const gchar *tip;
        tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                "<b>Shift+Click</b> open dialog, "
                "<b>Ctrl+Alt+Click</b> reset");
        PointwiseParamKnotHolderEntity *e =
            new PointwiseParamKnotHolderEntity(this, i);
        e->create(desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _(tip),
                  knot_shape, knot_mode, knot_color);
        knotholder->add(e);
    }
    updateCanvasIndicators();
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
