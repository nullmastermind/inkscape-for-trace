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
#include "live_effects/parameter/satellitepairarray.h"
#include "sp-lpe-item.h"
// TODO due to internal breakage in glibmm headers,
// this has to be included last.
#include <glibmm/i18n.h>


using namespace Geom;

namespace Inkscape {

namespace LivePathEffect {

SatellitePairArrayParam::SatellitePairArrayParam(
    const Glib::ustring &label, const Glib::ustring &tip,
    const Glib::ustring &key, Inkscape::UI::Widget::Registry *wr,
    Effect *effect)
    : ArrayParam<std::pair<int,Geom::Satellite> >(label, tip, key, wr, effect, 0)
{
    knot_shape = SP_KNOT_SHAPE_DIAMOND;
    knot_mode = SP_KNOT_MODE_XOR;
    knot_color = 0x00ff0000;
}

SatellitePairArrayParam::~SatellitePairArrayParam() {}

void SatellitePairArrayParam::set_oncanvas_looks(SPKnotShapeType shape,
        SPKnotModeType mode,
        guint32 color)
{
    knot_shape = shape;
    knot_mode = mode;
    knot_color = color;
}

void SatellitePairArrayParam::set_pointwise(Geom::Pointwise *pointwise)
{
    last_pointwise = pointwise;
}

void SatellitePairArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item)
{
    for (unsigned int i = 0; i < _vector.size(); ++i) {
        addKnotHolderEntitieMirrored(knotholder, desktop, item, i);
        const gchar *tip;
        tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                "<b>Shift+Click</b> open dialog, "
                "<b>Ctrl+Alt+Click</b> reset");
        SatellitePairArrayParamKnotHolderEntity *e =
            new SatellitePairArrayParamKnotHolderEntity(this, i);
        e->create(desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _(tip),
                  knot_shape, knot_mode, knot_color);
        knotholder->add(e);
    }
}

void SatellitePairArrayParam::addKnotHolderEntitieMirrored(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item, int i)
{
        const gchar *tip;
        tip = _("<b>Mirror</b> ppp");
        SatellitePairArrayParamKnotHolderEntity *e =
            new SatellitePairArrayParamKnotHolderEntity(this, i + _vector.size());
        e->create(desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _(tip),
                  knot_shape, knot_mode, knot_color);
        knotholder->add(e);
}


SatellitePairArrayParamKnotHolderEntity::SatellitePairArrayParamKnotHolderEntity(SatellitePairArrayParam *p, unsigned int index) 
  : _pparam(p), 
    _index(index)
{ 
}



void SatellitePairArrayParamKnotHolderEntity::knot_set(Point const &p,
                                              Point const &/*origin*/,
                                              guint state)
{
    Geom::Point s = snap_knot_position(p, state);
    int index = _index;
    if( _index >= _pparam->_vector.size()){
        index = _index-_pparam->_vector.size();
    }
    std::pair<int,Geom::Satellite> satellite = _pparam->_vector.at(index);
    Geom::Pointwise* pointwise = _pparam->last_pointwise;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = pointwise->getPwd2();
    if(_pparam->_vector.size() <= _index){
        boost::optional<Geom::D2<Geom::SBasis> > d2_in = pointwise->getCurveIn(satellite);
        if(d2_in){
            double mirrorTime = Geom::nearest_point(s, *d2_in);
            double size = satellite.second.toSize(mirrorTime, *d2_in);
            double lenght = Geom::length(*d2_in, Geom::EPSILON) - size;
            double time = satellite.second.toTime(lenght,pwd2[satellite.first]);
            s = pwd2[satellite.first].valueAt(time);
            satellite.second.setPosition(s,pwd2[satellite.first]);
        }
    } else {
        satellite.second.setPosition(s,pwd2[satellite.first]);
    }
     _pparam->_vector.at(index) = satellite;
    SPLPEItem * splpeitem = dynamic_cast<SPLPEItem *>(item);
    if(splpeitem){
        sp_lpe_item_update_patheffect(splpeitem, false, false);
    }
}

Geom::Point 
SatellitePairArrayParamKnotHolderEntity::knot_get() const
{
    Geom::Point tmpPoint;
    int index = _index;
    if( _index >= _pparam->_vector.size()){
        index = _index-_pparam->_vector.size();
    }
    std::pair<int,Geom::Satellite> satellite = _pparam->_vector.at(index);
    Geom::Pointwise* pointwise = _pparam->last_pointwise;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = pointwise->getPwd2();
    if( _index >= _pparam->_vector.size()){
        tmpPoint = satellite.second.getPosition(pwd2[satellite.first]);
        boost::optional<Geom::D2<Geom::SBasis> > d2_in = pointwise->getCurveIn(satellite);
        if(d2_in){
            tmpPoint = (*d2_in).valueAt(satellite.second.getOpositeTime(*d2_in));
        }
    } else {
        tmpPoint = satellite.second.getPosition(pwd2[satellite.first]);
    }
    Geom::Point const canvas_point = tmpPoint;
    return canvas_point;
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
