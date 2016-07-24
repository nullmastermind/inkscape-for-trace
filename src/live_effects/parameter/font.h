#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_FONT_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_FONT_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Authors:
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include <glib.h>
#include <gtkmm.h>
#include "live_effects/parameter/parameter.h"

namespace Inkscape {

namespace LivePathEffect {

class FontParam : public Parameter {
public:
    FontParam( const Glib::ustring& label,
               const Glib::ustring& tip,
               const Glib::ustring& key,
               Inkscape::UI::Widget::Registry* wr,
               Effect* effect,
               const Glib::ustring default_value = "");
    virtual ~FontParam() {}

    virtual Gtk::Widget * param_newWidget();
    virtual bool param_readSVGValue(const gchar * strvalue);
    double param_readFontSize(const gchar * strvalue);
    void param_update_default(const Glib::ustring defvalue);
    Glib::ustring param_readFontSpec(const gchar * strvalue);
    virtual gchar * param_getSVGValue() const;

    void param_setValue(const Glib::ustring newvalue);
    
    virtual void param_set_default();

    const Glib::ustring get_value() const { return defvalue; };

private:
    FontParam(const FontParam&);
    FontParam& operator=(const FontParam&);
    Glib::ustring value;
    Glib::ustring defvalue;

};

/*
 * This parameter does not display a widget in the LPE dialog; LPEs can use it to display on-canvas
 * text that should not be settable by the user. Note that since no widget is provided, the
 * parameter must be initialized differently than usual (only with a pointer to the parent effect;
 * no label, no tooltip, etc.).
 */
class FontParamInternal : public FontParam {
public:
    FontParamInternal(Effect* effect) :
        FontParam("", "", "", NULL, effect) {}

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
