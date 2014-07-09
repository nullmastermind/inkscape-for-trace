/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Jabiertxo Arraiza Cenoz 2014 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "ui/widget/registered-widget.h"
#include "live_effects/parameter/togglebutton.h"
#include "live_effects/effect.h"
#include "svg/svg.h"
#include "svg/stringstream.h"
#include "widgets/icon.h"
#include "inkscape.h"
#include "verbs.h"
#include "helper-fns.h"
#include <glibmm/i18n.h>

namespace Inkscape {

namespace LivePathEffect {

ToggleButtonParam::ToggleButtonParam( const Glib::ustring& label, const Glib::ustring& tip,
                      const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                      Effect* effect, bool default_value )
    : Parameter(label, tip, key, wr, effect), value(default_value), defvalue(default_value)
{
}

ToggleButtonParam::~ToggleButtonParam()
{
    if (_toggled_connection.connected()) {
        _toggled_connection.disconnect();
    }
}

void
ToggleButtonParam::param_set_default()
{
    param_setValue(defvalue);
}

bool
ToggleButtonParam::param_readSVGValue(const gchar * strvalue)
{
    param_setValue(helperfns_read_bool(strvalue, defvalue));
    return true; // not correct: if value is unacceptable, should return false!
}

gchar *
ToggleButtonParam::param_getSVGValue() const
{
    gchar * str = g_strdup(value ? "true" : "false");
    return str;
}

Gtk::Widget *
ToggleButtonParam::param_newWidget()
{
    if (_toggled_connection.connected()) {
        _toggled_connection.disconnect();
    }

    Inkscape::UI::Widget::RegisteredToggleButton * checkwdg = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredToggleButton( param_label,
                                                         param_tooltip,
                                                         param_key,
                                                         *param_wr,
                                                         false,
                                                         param_effect->getRepr(),
                                                         param_effect->getSPDoc()) );

    checkwdg->setActive(value);
    checkwdg->setProgrammatically = false;
    // TRANSLATORS: "toggle" is a verb here
    checkwdg->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Toggle path effect parameter"));

    _toggled_connection = checkwdg->signal_toggled().connect(sigc::mem_fun(*this, &ToggleButtonParam::toggled));

    return checkwdg;
}

void
ToggleButtonParam::param_setValue(bool newvalue)
{
    value = newvalue;
}

void
ToggleButtonParam::toggled() {
    _signal_toggled.emit();
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
