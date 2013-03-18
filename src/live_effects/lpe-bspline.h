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

public:
    LPEBSpline(LivePathEffectObject *lpeobject);
    virtual ~LPEBSpline();

    virtual LPEPathFlashType pathFlashType() const { return SUPPRESS_FLASH; }

    virtual void doOnApply(SPLPEItem const* lpeitem);

    virtual void doEffect(SPCurve * curve);

    virtual void doBSplineFromWidget(SPCurve * curve, double value, bool noCusp);

    virtual Gtk::Widget * newWidget();

    int steps;

protected:

    Gtk::Widget* scal;

    Gtk::Widget* noCusp;

    Gtk::Widget* reset;

    Gtk::Widget* stepsHandles;

    virtual void registerScal(Glib::ustring title, Glib::ustring tip){scal = LPEBSpline::newScal(title,tip);};

    virtual void registerNoCusp(Glib::ustring title){noCusp = LPEBSpline::newNoCusp(title);};

    virtual void registerReset(Glib::ustring title){reset = LPEBSpline::newReset(title);};

    virtual void registerStepsHandles(Glib::ustring title, Glib::ustring tip){stepsHandles = LPEBSpline::newStepsHandles(title,tip);};

    virtual Gtk::Widget* newScal(Glib::ustring title, Glib::ustring tip);

    virtual Gtk::Widget* newNoCusp(Glib::ustring title);

    virtual Gtk::Widget* newReset(Glib::ustring title);
    
    virtual Gtk::Widget* newStepsHandles(Glib::ustring title, Glib::ustring tip);

    virtual void updateAllHandles();

    virtual void resetHandles();

    virtual void updateSteps();

    virtual void updateStepsValue(int stepsValue){steps=stepsValue;};

private:
    LPEBSpline(const LPEBSpline&);
    LPEBSpline& operator=(const LPEBSpline&);
    
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
