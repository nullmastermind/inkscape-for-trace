#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_MESSAGE_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_MESSAGE_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Authors:
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include <glib.h>
#include "live_effects/parameter/parameter.h"

namespace Inkscape {

namespace LivePathEffect {

class MessageParam : public Parameter {
public:
    MessageParam( const Glib::ustring& label,
               const Glib::ustring& tip,
               const Glib::ustring& key,
               Inkscape::UI::Widget::Registry* wr,
               Effect* effect,
               const gchar * default_message = "Default message");
    ~MessageParam() override = default;

    Gtk::Widget * param_newWidget() override;
    bool param_readSVGValue(const gchar * strvalue) override;
    void param_update_default(const gchar * default_value) override;
    gchar * param_getSVGValue() const override;
    gchar * param_getDefaultSVGValue() const override;

    void param_setValue(const gchar * message);
    
    void param_set_default() override;
    void param_set_min_height(int height);
    const gchar *  get_value() const { return message; };

private:
    Gtk::Label * _label;
    int _min_height;
    MessageParam(const MessageParam&) = delete;
    MessageParam& operator=(const MessageParam&) = delete;
    const gchar *  message;
    const gchar *  defmessage;
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
