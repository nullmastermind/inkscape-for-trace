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
#include "ui/dialog/lpe-fillet-chamfer-properties.h"
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
    : ArrayParam<std::pair<unsigned int,Geom::Satellite> >(label, tip, key, wr, effect, 0)
{
    knot_shape = SP_KNOT_SHAPE_DIAMOND;
    knot_mode = SP_KNOT_MODE_XOR;
    knot_color = 0x00ff0000;
    helper_size = 0;
    use_distance = false;
    _effectType = FILLET_CHAMFER;
    last_pointwise = NULL;
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
    param_set_and_write_new_value(last_pointwise->getSatellites());
}

void SatellitePairArrayParam::set_document_unit(Glib::ustring value_document_unit)
{
    documentUnit = value_document_unit;
}

void SatellitePairArrayParam::set_use_distance(bool use_knot_distance )
{
    use_distance = use_knot_distance;
}

void SatellitePairArrayParam::set_unit(const gchar *abbr)
{
    unit = abbr;
}

void SatellitePairArrayParam::set_effect_type(EffectType et)
{
    _effectType = et;
}

void SatellitePairArrayParam::set_helper_size(int hs)
{
    helper_size = hs;
    updateCanvasIndicators();
}

void SatellitePairArrayParam::updateCanvasIndicators(bool mirror)
{
    if(!last_pointwise){
        return;
    }
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = last_pointwise->getPwd2();
    if( mirror == true){
        hp.clear();
    }
    for (unsigned int i = 0; i < _vector.size(); ++i) {
        if(!_vector[i].second.getActive() || _vector[i].second.getHidden()){
            continue;
        }
        if((!_vector[i].second.getHasMirror() && mirror == true) || _vector[i].second.getAmount() == 0){
            continue;
        }
        double pos = 0;
        if(pwd2.size() <= (unsigned)_vector[i].first){
            break;
        }
        Geom::D2<Geom::SBasis> d2 = pwd2[_vector[i].first];
        bool overflow = false;
        double size_out = _vector[i].second.getSize(pwd2[_vector[i].first]);
        double lenght_out = Geom::length(pwd2[_vector[i].first], Geom::EPSILON);
        double lenght_in = 0;
        boost::optional<unsigned int> d2_prev_index = last_pointwise->getPrevious(_vector[i].first);
        if(d2_prev_index){
            lenght_in = Geom::length(pwd2[*d2_prev_index], Geom::EPSILON);
        }
        if(mirror == true){
            if(d2_prev_index){
                pos = _vector[i].second.getOpositeTime(size_out,pwd2[*d2_prev_index]);
                if(lenght_out < size_out){
                    overflow = true;
                }
            }
        } else {
            pos = _vector[i].second.getTime(d2);
            if(lenght_in < size_out){
                overflow = true;
            }
        }
        if (pos <= 0 || pos >= 1) {
            continue;
        }
        Geom::Point ptA = d2.valueAt(pos);
        Geom::Point derivA = unit_vector(derivative(d2).valueAt(pos));
        Geom::Rotate rot(Geom::Rotate::from_degrees(-90));
        derivA = derivA * rot;
        Geom::Point C = ptA - derivA * helper_size;
        Geom::Point D = ptA + derivA * helper_size;
        Geom::Ray ray1(C, D);
        char const * svgd = "M 1,0.25 0.5,0 1,-0.25 M 1,0.5 0,0 1,-0.5";
        Geom::PathVector pathv = sp_svg_read_pathv(svgd);
        Geom::Affine aff = Geom::Affine();
        aff *= Geom::Scale(helper_size);
        if(mirror == true){
            aff *= Geom::Rotate(ray1.angle() - deg_to_rad(90));
        } else {
            aff *= Geom::Rotate(ray1.angle() - deg_to_rad(270));
        }
        pathv *= aff;
        pathv += d2.valueAt(pos);
        hp.push_back(pathv[0]);
        hp.push_back(pathv[1]);
        if(overflow){
            double diameter = helper_size;
            if(helper_size == 0){
                diameter = 15;
                char const * svgd;
                svgd = "M 0.7,0.35 A 0.35,0.35 0 0 1 0.35,0.7 0.35,0.35 0 0 1 0,0.35 0.35,0.35 0 0 1 0.35,0 0.35,0.35 0 0 1 0.7,0.35 Z";
                Geom::PathVector pathv = sp_svg_read_pathv(svgd);
                pathv *= Geom::Scale (diameter);
                pathv += ptA-Geom::Point(diameter * 0.35,diameter * 0.35);
                hp.push_back(pathv[0]);
            } else {
                char const * svgd;
                svgd = "M 0 -1.32 A 1.32 1.32 0 0 0 -1.32 0 A 1.32 1.32 0 0 0 0 1.32 A 1.32 1.32 0 0 0 1.18 0.59 L 0 0 L 1.18 -0.59 A 1.32 1.32 0 0 0 0 -1.32 z";
                Geom::PathVector pathv = sp_svg_read_pathv(svgd);
                aff = Geom::Affine();
                aff *= Geom::Scale(helper_size/2.0);
                if(mirror == true){
                    aff *= Geom::Rotate(ray1.angle() - deg_to_rad(90));
                } else {
                    aff *= Geom::Rotate(ray1.angle() - deg_to_rad(270));
                }
                pathv *= aff;
                pathv += d2.valueAt(pos);
                hp.push_back(pathv[0]);
            }
        }
    }
    if( mirror == true){
        updateCanvasIndicators(false);
    }
}
void SatellitePairArrayParam::updateCanvasIndicators()
{
    updateCanvasIndicators(true);
}

void SatellitePairArrayParam::addCanvasIndicators(
    SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(hp);
}

void SatellitePairArrayParam::recalculate_knots()
{
    if(last_pointwise){
        _vector = last_pointwise->getSatellites();
        write_to_SVG();
    }
}

void SatellitePairArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item,
        bool mirror)
{
    recalculate_knots();
    for (unsigned int i = 0; i < _vector.size(); ++i) {
        int iPlus = i;
        if( mirror == true){
            iPlus = i + _vector.size();
        }
        if(!_vector[i].second.getActive() || _vector[i].second.getHidden()){
            continue;
        }
        if(!_vector[i].second.getHasMirror() && mirror == true){
            continue;
        }
        using namespace Geom;
        SatelliteType type = _vector[i].second.getSatelliteType();
        //IF is for filletChamfer effect...
        if(_effectType == FILLET_CHAMFER){
            const gchar *tip;
            if (type == C){
                 tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IC) {
                tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IF) {
                tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else {
                tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            }
            FilletChamferKnotHolderEntity *e =
                new FilletChamferKnotHolderEntity(this, iPlus);
            e->create(desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _(tip),
                      knot_shape, knot_mode, knot_color);
            knotholder->add(e);
        }
    }
    if( mirror == true){
        addKnotHolderEntities(knotholder, desktop, item, false);
    }
}

void SatellitePairArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item)
{
        addKnotHolderEntities(knotholder, desktop, item, true);
}

FilletChamferKnotHolderEntity::FilletChamferKnotHolderEntity(SatellitePairArrayParam *p, unsigned int index) 
  : _pparam(p), 
    _index(index)
{ 
}



void FilletChamferKnotHolderEntity::knot_set(Point const &p,
                                              Point const &/*origin*/,
                                              guint state)
{
    Geom::Point s = snap_knot_position(p, state);
    int index = _index;
    if( _index >= _pparam->_vector.size()){
        index = _index-_pparam->_vector.size();
    }
    if (!valid_index(index)) {
        return;
    }

    if( !_pparam->last_pointwise ){
        return;
    }

    std::pair<int,Geom::Satellite> satellite = _pparam->_vector.at(index);
    if(!satellite.second.getActive() || satellite.second.getHidden()){
        return;
    }
    Geom::Pointwise* pointwise = _pparam->last_pointwise;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = pointwise->getPwd2();
    if(_pparam->_vector.size() <= _index){
        boost::optional<unsigned int> d2_prev_index = pointwise->getPrevious(satellite.first);
        if(d2_prev_index){
            Geom::D2<Geom::SBasis> d2_in = pwd2[*d2_prev_index];
            double mirrorTime = Geom::nearest_point(s, d2_in);
            double timeStart = 0;
            std::vector<unsigned int> satIndexes = pointwise->findPeviousSatellites(satellite.first,1);
            if(satIndexes.size()>0){
                timeStart =  pointwise->getSatellites()[satIndexes[0]].second.getTime(d2_in);
            }
            if(timeStart > mirrorTime){
                mirrorTime = timeStart;
            }
            double size = satellite.second.toSize(mirrorTime, d2_in);
            double amount = Geom::length(d2_in, Geom::EPSILON) - size;
            if(satellite.second.getIsTime()){
                amount = satellite.second.toTime(amount,pwd2[satellite.first]);
            }
            satellite.second.setAmount(amount);
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
FilletChamferKnotHolderEntity::knot_get() const
{
    Geom::Point tmpPoint;
    int index = _index;
    if( _index >= _pparam->_vector.size()){
        index = _index-_pparam->_vector.size();
    }
    if (!valid_index(index)) {
        return Point(infinity(), infinity());
    }
    std::pair<int,Geom::Satellite> satellite = _pparam->_vector.at(index);
    if(!_pparam->last_pointwise){
        return Point(infinity(), infinity());
    }
    if(!satellite.second.getActive() || satellite.second.getHidden()){
        return Point(infinity(), infinity());
    }
    Geom::Pointwise* pointwise = _pparam->last_pointwise;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = pointwise->getPwd2();
    if( _index >= _pparam->_vector.size()){
        tmpPoint = satellite.second.getPosition(pwd2[satellite.first]);
        boost::optional<unsigned int> d2_prev_index = pointwise->getPrevious(satellite.first);
        if(d2_prev_index){
            Geom::D2<Geom::SBasis> d2_in =  pwd2[*d2_prev_index];
            double s = satellite.second.getSize(pwd2[satellite.first]);
            double t = satellite.second.getOpositeTime(s,d2_in);
            if(t > 1){
                t = 1;
            }
            if(t < 0){
                t = 0;
            }
            double timeStart = 0;
            std::vector<unsigned int> satIndexes = pointwise->findPeviousSatellites(satellite.first,1);
            if(satIndexes.size()>0){
                timeStart =  pointwise->getSatellites()[satIndexes[0]].second.getTime(d2_in);
            }
            if(timeStart > t){
                t = timeStart;
            }
            tmpPoint = (d2_in).valueAt(t);
        }
    } else {
        tmpPoint = satellite.second.getPosition(pwd2[satellite.first]);
    }
    Geom::Point const canvas_point = tmpPoint;
    return canvas_point;
}

void FilletChamferKnotHolderEntity::knot_click(guint state)
{
    if( !_pparam->last_pointwise){
        return;
    }

    int index = _index;
    if( _index >= _pparam->_vector.size()){
        index = _index-_pparam->_vector.size();
    }
    if (state & GDK_CONTROL_MASK) {
        if (state & GDK_MOD1_MASK) {
            _pparam->_vector.at(index).second.setAmount(0.0);
            _pparam->param_set_and_write_new_value(_pparam->_vector);
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
        }else{
            using namespace Geom;
            SatelliteType type = _pparam->_vector.at(index).second.getSatelliteType();
            switch(type){
                case F:
                    type = IF;
                    break;
                case IF:
                    type =  C;
                    break;
                case C:
                    type =  IC;
                    break;
                default:
                    type = F;
                    break;
            }
            _pparam->_vector.at(index).second.setSatelliteType(type);
            _pparam->param_set_and_write_new_value(_pparam->_vector);
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
            const gchar *tip;
            if (type == C){
                 tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IC) {
                tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IF) {
                tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else {
                tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            }
            this->knot->tip = g_strdup(tip);
            this->knot->show();
        }
    } else if (state & GDK_SHIFT_MASK) {
        double amount = _pparam->_vector.at(index).second.getAmount(); 
        if(!_pparam->use_distance && !_pparam->_vector.at(index).second.getIsTime()){
             amount = _pparam->last_pointwise->len_to_rad(amount, _pparam->_vector.at(index));
        }
        bool aprox = false;
        D2<SBasis> d2_out = _pparam->last_pointwise->getPwd2()[index];
        boost::optional<unsigned int> d2_prev_index = _pparam->last_pointwise->getPrevious(_pparam->_vector.at(index).first);
        if(d2_prev_index){
            Geom::D2<Geom::SBasis> d2_in =  _pparam->last_pointwise->getPwd2()[*d2_prev_index];
            aprox = ((d2_in)[0].degreesOfFreedom() != 2 || d2_out[0].degreesOfFreedom() != 2) && !_pparam->use_distance?true:false;
        }
        Inkscape::UI::Dialogs::FilletChamferPropertiesDialog::showDialog(
            this->desktop, amount , this, _pparam->unit, _pparam->use_distance, aprox, _pparam->documentUnit,_pparam->_vector.at(index).second);
    
    }
}

void FilletChamferKnotHolderEntity::knot_set_offset(Geom::Satellite satellite)
{
    if( !_pparam->last_pointwise){
        return;
    }
    int index = _index;
    if( _index >= _pparam->_vector.size()){
        index = _index-_pparam->_vector.size();
    }
    double amount = satellite.getAmount();
    double maxAmount = amount;
    if(!_pparam->use_distance && !satellite.getIsTime()){
        amount = _pparam->last_pointwise->rad_to_len(amount, _pparam->_vector.at(index));
        if(maxAmount > 0 && amount == 0){
            amount = _pparam->_vector.at(index).second.getAmount();
        }
    }
    satellite.setAmount(amount);
    _pparam->_vector.at(index).second = satellite;
    this->parent_holder->knot_ungrabbed_handler(this->knot, 0);
    _pparam->param_set_and_write_new_value(_pparam->_vector);
    SPLPEItem * splpeitem = dynamic_cast<SPLPEItem *>(item);
    if(splpeitem){
        sp_lpe_item_update_patheffect(splpeitem, false, false);
    }
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
