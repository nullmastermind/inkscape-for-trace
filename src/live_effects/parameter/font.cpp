/*
 * Authors:
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "ui/widget/registered-widget.h"
#include "live_effects/parameter/font.h"
#include "live_effects/effect.h"
#include "ui/widget/font-selector.h"
#include "svg/svg.h"
#include "svg/stringstream.h"
#include "verbs.h"

#include <glibmm/i18n.h>

namespace Inkscape {

namespace LivePathEffect {

FontParam::FontParam( const Glib::ustring& label, const Glib::ustring& tip,
                      const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                      Effect* effect, const Glib::ustring default_value )
    : Parameter(label, tip, key, wr, effect),
      value(default_value),
      defvalue(default_value)
{
}

void
FontParam::param_set_default()
{
    param_setValue(defvalue);
}
void 
FontParam::param_update_default(const Glib::ustring default_value){
    defvalue = default_value;
}

Glib::ustring
FontParam::param_readFontSpec(const gchar * strvalue)
{
    Glib::ustring result;
    gchar ** strarray = g_strsplit(strvalue, " @ ", 2);
    double fontsize;
    if (strarray[1]) {
        unsigned int success = sp_svg_number_read_d(strarray[1], &fontsize);
        if (success == 1) {
            result = (Glib::ustring)strarray[0];
            g_strfreev (strarray);
            return result;
        }
    }
    g_strfreev (strarray);
    return result;
}

double
FontParam::param_readFontSize(const gchar * strvalue)
{
    gchar ** strarray = g_strsplit(strvalue, " @ ", 2);
    double fontsize = 0;
    if (strarray[1]) {
        unsigned int success = sp_svg_number_read_d(strarray[1], &fontsize);
        if (success == 1) {
                g_strfreev (strarray);
                return fontsize;
        }
    }
    g_strfreev (strarray);
    return fontsize;
}

bool
FontParam::param_readSVGValue(const gchar * strvalue)
{
    double fontsize = param_readFontSize(strvalue);
    Glib::ustring fontspec = param_readFontSpec(strvalue);
    Inkscape::SVGOStringStream os;
    os << fontspec << " @ " << fontsize;
    param_setValue((Glib::ustring)os.str());
    return true;
}

gchar *
FontParam::param_getSVGValue() const
{
    return g_strdup(value.c_str());
}

Gtk::Widget *
FontParam::param_newWidget()
{
    Inkscape::UI::Widget::RegisteredFontSelector * fontselectorwdg = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredFontSelector( param_label,
                                                          param_tooltip,
                                                          param_key,
                                                          *param_wr,
                                                          param_effect->getRepr(),
                                                          param_effect->getSPDoc() ) );
    double fontsize = param_readFontSize(param_getSVGValue());
    Glib::ustring fontspec = param_readFontSpec(param_getSVGValue());
    fontselectorwdg->setValue( fontspec, fontsize );
    fontselectorwdg->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Change font selector parameter"));
    param_effect->upd_params = false;
    return dynamic_cast<Gtk::Widget *> (fontselectorwdg);
}

void
FontParam::param_setValue(const Glib::ustring newvalue)
{
    value = newvalue;
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
