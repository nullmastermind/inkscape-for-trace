#ifndef INKSCAPE_LPE_BSPLINE_H
#define INKSCAPE_LPE_BSPLINE_H

/*
 * Inkscape::LPEBSpline
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"


namespace Inkscape {
namespace LivePathEffect {



class LPEBSpline : public Effect {

private:
    double scalWidget;
    bool noCuspWidget;
    LPEBSpline(const LPEBSpline&);
    LPEBSpline& operator=(const LPEBSpline&);

public:
    LPEBSpline(LivePathEffectObject *lpeobject);
    virtual ~LPEBSpline();

    virtual LPEPathFlashType pathFlashType() const { return SUPPRESS_FLASH; }

    virtual void doOnApply(SPLPEItem const* lpeitem);

    virtual void doEffect(SPCurve * curve);

    virtual void updateAllHandles();

    virtual Gtk::Widget* newScalar(Glib::ustring title, Glib::ustring tip);

    virtual Gtk::Widget* newCheckButton(Glib::ustring title);

    virtual void doBSplineFromWidget(SPCurve * curve, double value, bool noCusp);

    virtual double getScal(){return scal;};
    
    virtual double setScal(double setScal){scal = setScal;};
    
    virtual bool getNoCusp(){return noCusp;};
    
    virtual bool setNoCusp(bool setNoCusp){noCusp = setNoCusp;};
    
    virtual Gtk::Widget* newWidget();

};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
