#ifndef INKSCAPE_LPE_BSPLINE_H
#define INKSCAPE_LPE_BSPLINE_H

/*
 * Inkscape::LPEBSpline
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "ui/widget/scalar.h"
#include <gtkmm/checkbutton.h>

namespace Inkscape {
namespace LivePathEffect {



class LPEBSpline : public Effect {
public:
    LPEBSpline(LivePathEffectObject *lpeobject);
    virtual ~LPEBSpline();

    virtual LPEPathFlashType pathFlashType() const { return SUPPRESS_FLASH; }

    virtual void doOnApply(SPLPEItem const* lpeitem);

    virtual void doEffect(SPCurve * curve);

    virtual void updateAllHandles();

    virtual void newScalar(Glib::ustring title, Glib::ustring tip);

    virtual void newCheckButton(Glib::ustring title, Glib::ustring tip);


    virtual void doBSplineFromWidget(SPCurve * curve, double value, bool noCusp);

    virtual Gtk::Widget * newWidget();


private:
    Gtk::Widget * scal;
    Gtk::Widget * noCusp;
    LPEBSpline(const LPEBSpline&);
    LPEBSpline& operator=(const LPEBSpline&);
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
