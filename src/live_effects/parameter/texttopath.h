#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_TEXT_TO_PATH_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_TEXT_TO_PATH_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Authors:
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>

#include "display/canvas-bpath.h"
#include "live_effects/parameter/parameter.h"

struct SPCanvasText;

namespace Inkscape {

namespace LivePathEffect {

class TextToPathParam : public Parameter {
public:
    TextToPathParam( const Glib::ustring& label,
               const Glib::ustring& tip,
               const Glib::ustring& key,
               Inkscape::UI::Widget::Registry* wr,
               Effect* effect,
               const Glib::ustring default_value = "");
    virtual ~TextToPathParam() {}

    virtual Gtk::Widget * param_newWidget();

    virtual bool param_readSVGValue(const gchar * strvalue);
    virtual gchar * param_getSVGValue() const;

    void param_setValue(const Glib::ustring newvalue);
    virtual void param_set_default();
    void setPos(Geom::Point pos);
    void setPosAndAnchor(const Geom::Piecewise<Geom::D2<Geom::SBasis> > &pwd2,
			 const double t, const double length, bool use_curvature = false);
    void setAnchor(double x_value, double y_value);

    const Glib::ustring get_value() const { return defvalue; };

private:
    TextToPathParam(const TextToPathParam&);
    TextToPathParam& operator=(const TextToPathParam&);
    double anchor_x;
    double anchor_y;

    Glib::ustring value;
    Glib::ustring defvalue;

    SPCanvasText *canvas_text;
};

/*
 * This parameter does not display a widget in the LPE dialog; LPEs can use it to display on-canvas
 * text that should not be settable by the user. Note that since no widget is provided, the
 * parameter must be initialized differently than usual (only with a pointer to the parent effect;
 * no label, no tooltip, etc.).
 */
class TextToPathParamInternal : public TextToPathParam {
public:
    TextToPathParamInternal(Effect* effect) :
        TextToPathParam("", "", "", NULL, effect) {}

    virtual Gtk::Widget * param_newWidget() { return NULL; }
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif

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
