/*
 * Authors:
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm.h>
#include "live_effects/parameter/message.h"
#include "live_effects/effect.h"
#include <glibmm/i18n.h>

namespace Inkscape {

namespace LivePathEffect {

MessageParam::MessageParam( const Glib::ustring& label, const Glib::ustring& tip,
                      const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                      Effect* effect, const gchar * default_message )
    : Parameter(label, tip, key, wr, effect),
      message(g_strdup(default_message)),
      defmessage(g_strdup(default_message))
{

}

void
MessageParam::param_set_default()
{
    param_setValue(defmessage);
}

void 
MessageParam::param_update_default(const gchar * default_message)
{
    defmessage = g_strdup(default_message);
}

bool
MessageParam::param_readSVGValue(const gchar * strvalue)
{
    param_setValue(strvalue);
    return true;
}

gchar *
MessageParam::param_getSVGValue() const
{
    return message;
}

gchar *
MessageParam::param_getDefaultSVGValue() const
{
    return defmessage;
}

Gtk::Widget *
MessageParam::param_newWidget()
{
    Gtk::Frame * frame = new Gtk::Frame (param_label);
    Gtk::Widget * widg_frame = frame->get_label_widget();
    widg_frame->set_margin_right(5);
    widg_frame->set_margin_left(5);
    Gtk::Label * label = new Gtk::Label (message, Gtk::ALIGN_END);
    label->set_use_underline (true);
    label->set_use_markup();
    label->set_line_wrap(true);
    Gtk::Widget * widg_label = dynamic_cast<Gtk::Widget *> (label);
    widg_label->set_margin_top(8);
    widg_label->set_margin_bottom(10);
    widg_label->set_margin_right(6);
    widg_label->set_margin_left(6);

    frame->add(*widg_label);
    return dynamic_cast<Gtk::Widget *> (frame);
}

void
MessageParam::param_setValue(const gchar * strvalue)
{
    if (strcmp(strvalue, message) != 0) {
        param_effect->upd_params = true;
    }
    message = g_strdup(strvalue);
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
