#ifndef INKSCAPE_LIVEPATHEFFECT_SATELLITE_PAIR_ARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_SATELLITE_PAIR_ARRAY_H

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
 * This parameter act as bridge from pointwise class to serialize it as a LPE
 * parameter
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/array.h"
#include "live_effects/effect-enum.h"
#include "helper/geom-pointwise.h"
#include "knot-holder-entity.h"
#include <glib.h>

namespace Inkscape {

namespace LivePathEffect {

class FilletChamferKnotHolderEntity;

class SatelliteArrayParam : public ArrayParam<Geom::Satellite> {
public:
    SatelliteArrayParam(const Glib::ustring &label, const Glib::ustring &tip,
                        const Glib::ustring &key,
                        Inkscape::UI::Widget::Registry *wr, Effect *effect);
    virtual ~SatelliteArrayParam();

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
    void setDocumentUnit(Glib::ustring value_document__unit);
    void setUseDistance(bool use_knot_distance);
    void setUnit(const gchar *abbr);
    void setEffectType(EffectType et);
    void setPointwise(Geom::Pointwise *pointwise);
    void set_oncanvas_looks(SPKnotShapeType shape, SPKnotModeType mode,
                            guint32 color);

    friend class FilletChamferKnotHolderEntity;
    friend class LPEFilletChamfer;

protected:
    KnotHolder *knoth;

private:
    SatelliteArrayParam(const SatelliteArrayParam &);
    SatelliteArrayParam &operator=(const SatelliteArrayParam &);

    SPKnotShapeType _knot_shape;
    SPKnotModeType _knot_mode;
    guint32 _knot_color;
    Geom::PathVector _hp;
    int _helper_size;
    bool _use_distance;
    const gchar *_unit;
    Glib::ustring _documentUnit;
    EffectType _effectType;
    Geom::Pointwise *_last_pointwise;

};

class FilletChamferKnotHolderEntity : public KnotHolderEntity {
public:
    FilletChamferKnotHolderEntity(SatelliteArrayParam *p, size_t index);
    virtual ~FilletChamferKnotHolderEntity()
    {
        _pparam->knoth = NULL;
    }

    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin,
                          guint state);
    virtual Geom::Point knot_get() const;
    virtual void knot_click(guint state);
    void knot_set_offset(Geom::Satellite);
    /** Checks whether the index falls within the size of the parameter's vector
     */
    bool valid_index(size_t index) const
    {
        return (_pparam->_vector.size() > index);
    }
    ;

private:
    SatelliteArrayParam *_pparam;
    size_t _index;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif
