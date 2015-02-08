#ifndef INKSCAPE_LIVEPATHEFFECT_POINTWISE_H
#define INKSCAPE_LIVEPATHEFFECT_POINTWISE_H

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

#include <glib.h>
#include "knot-holder-entity.h"

namespace Inkscape {

namespace LivePathEffect {

class PointwiseParamKnotHolderEntity;

class PointwiseParam : public ArrayParam<Geom::Pointwise> {
public:
    PointwiseParam(const Glib::ustring &label,
                                 const Glib::ustring &tip,
                                 const Glib::ustring &key,
                                 Inkscape::UI::Widget::Registry *wr,
                                 Effect *effect);
    virtual ~PointwiseParam();

    virtual Gtk::Widget * param_newWidget() {
        return NULL;
    }

    void set_oncanvas_looks(SPKnotShapeType shape, SPKnotModeType mode,
                            guint32 color);
    virtual bool providesKnotHolderEntities() const {
        return true;
    }
    friend class PointwiseParamKnotHolderEntity;

protected:

    StorageType readsvg(const gchar * str);

private:
    PointwiseParam(const PointwiseParam &);
    PointwiseParam &operator=(const PointwiseParam &);

    SPKnotShapeType knot_shape;
    SPKnotModeType knot_mode;
    guint32 knot_color;
};

class PointwiseParamKnotHolderEntity : public KnotHolderEntity {
public:
    PointwiseParamKnotHolderEntity(PointwiseParam *p,
            unsigned int index);
    virtual ~PointwiseParamKnotHolderEntity() {}

    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin,
                          guint state);
    virtual Geom::Point knot_get() const;

    /*Checks whether the index falls within the size of the parameter's vector*/
    bool valid_index(unsigned int index) const {
        return (_pparam->_vector.size() > index);
    }
    ;

private:
    PointwiseParam *_pparam;
    unsigned int _index;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif
