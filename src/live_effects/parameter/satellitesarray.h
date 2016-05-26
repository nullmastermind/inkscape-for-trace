#ifndef INKSCAPE_LIVEPATHEFFECT_SATELLITES_ARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_SATELLITES_ARRAY_H

/*
 * Inkscape::LivePathEffectParameters
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * To Nathan Hurst for his review and help on refactor
 * and finaly to Liam P. White for his big help on coding, that save me a lot of
 * hours
 *
 *
 * This parameter act as bridge from pathVectorSatellites class to serialize it as a LPE
 * parameter
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/array.h"
#include "live_effects/effect-enum.h"
#include "helper/geom-pathvectorsatellites.h"
#include "knot-holder-entity.h"
#include <glib.h>

namespace Inkscape {

namespace LivePathEffect {

class FilletChamferKnotHolderEntity;

class SatellitesArrayParam : public ArrayParam<std::vector<Satellite> > {
public:
    SatellitesArrayParam(const Glib::ustring &label, const Glib::ustring &tip,
                        const Glib::ustring &key,
                        Inkscape::UI::Widget::Registry *wr, Effect *effect);

    virtual Gtk::Widget *param_newWidget()
    {
        return NULL;
    }
    virtual void setHelperSize(int hs);
    virtual void addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop,
                                       SPItem *item);
    virtual void addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop,
                                       SPItem *item, bool mirror);
    virtual void addCanvasIndicators(SPLPEItem const *lpeitem,
                                     std::vector<Geom::PathVector> &hp_vec);
    virtual void updateCanvasIndicators();
    virtual void updateCanvasIndicators(bool mirror);
    virtual bool providesKnotHolderEntities() const
    {
        return true;
    }
    void param_transform_multiply(Geom::Affine const &postmul, bool /*set*/);
    void setUseDistance(bool use_knot_distance);
    void setEffectType(EffectType et);
    void setPathVectorSatellites(PathVectorSatellites pathVectorSatellites);
    bool validData(size_t index, size_t subindex);
    void set_oncanvas_looks(SPKnotShapeType shape, SPKnotModeType mode, guint32 color);

    friend class FilletChamferKnotHolderEntity;
    friend class LPEFilletChamfer;

protected:
    KnotHolder *knoth;

private:
    SatellitesArrayParam(const SatellitesArrayParam &);
    SatellitesArrayParam &operator=(const SatellitesArrayParam &);

    SPKnotShapeType _knot_shape;
    SPKnotModeType _knot_mode;
    guint32 _knot_color;
    Geom::PathVector _hp;
    int _helper_size;
    bool _use_distance;
    EffectType _effectType;
    PathVectorSatellites _last_pathVectorSatellites;

};

class FilletChamferKnotHolderEntity : public KnotHolderEntity {
public:
    FilletChamferKnotHolderEntity(SatellitesArrayParam *p, size_t index, size_t subindex);
    virtual ~FilletChamferKnotHolderEntity()
    {
        _pparam->knoth = NULL;
    }

    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin,
                          guint state);
    virtual Geom::Point knot_get() const;
    virtual void knot_click(guint state);
    void knot_set_offset(Satellite);
    /** Checks whether the index falls within the size of the parameter's vector
     */
    bool valid_index(size_t index) const
    {
        return (_pparam->_vector.size() > index);
    };

private:
    SatellitesArrayParam *_pparam;
    size_t _index;
    size_t _subindex;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif
