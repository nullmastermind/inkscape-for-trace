/*
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "font-selector.h"
#include "widgets/icon.h"
#include <glibmm/i18n.h>

namespace Inkscape {
namespace UI {
namespace Widget {

FontSelector::FontSelector(Glib::ustring const &label, Glib::ustring const &tooltip,
              Glib::ustring const &suffix,
              Glib::ustring const &icon,
              bool mnemonic)
             :_widget(new Gtk::HBox()), expanded(false)
{
    Gtk::VBox * vbox_expander = Gtk::manage( new Gtk::VBox() );
    GtkWidget *fontsel = sp_font_selector_new();
    gtk_widget_set_size_request (fontsel, 290, 150);
    fsel = SP_FONT_SELECTOR(fontsel);
    Gtk::Widget*  pIcon = Gtk::manage( sp_icon_get_icon( "on", Inkscape::ICON_SIZE_BUTTON) );
    Gtk::Button * pButton = Gtk::manage(new Gtk::Button());
    pButton->set_relief(Gtk::RELIEF_NONE);
    pIcon->show();
    pButton->add(*pIcon);
    pButton->show();
    pButton->signal_clicked().connect(sigc::mem_fun(*this, &FontSelector::onFontSelectorSave));
    pButton->set_tooltip_text(_("Save the changes to font selector"));
    vbox_expander->pack_start(*Gtk::manage(Glib::wrap(fontsel)), true, true);
    vbox_expander->pack_start(*pButton, true, true);
    expander = Gtk::manage(new Gtk::Expander(Glib::ustring(_("Font Selector:"))));
    expander->add(*vbox_expander);
    expander->set_expanded(expanded);
    expander->set_spacing(5);
    expander->set_use_markup(true);
    onExpanderChanged();
    _widget->set_tooltip_text(tooltip);
    expander->property_expanded().signal_changed().connect(sigc::mem_fun(*this, &FontSelector::onExpanderChanged) );
    pack_start(*expander, true, true);
    pack_start(*Gtk::manage(_widget), true, true);
}

void
FontSelector::onExpanderChanged()
{
    expanded = expander->get_expanded();
    if(expanded) {
        expander->set_label (Glib::ustring(_("Font Selector:")));
    } else {
        expander->set_label (Glib::ustring(_("Font Selector: <b>hided</b>")));
    }
}

Glib::ustring FontSelector::getFontSpec() const
{
    return _fontspec;
}

void FontSelector::setFontSpec(Glib::ustring fontspec)
{
    _fontspec = fontspec;
}

double FontSelector::getFontSize() const
{
    return _fontsize;
}

void FontSelector::setFontSize(double fontsize)
{
    _fontsize = fontsize;
}

void FontSelector::setValue (Glib::ustring fontspec, double fontsize)
{
    if (_fontsize != fontsize || _fontspec != fontspec) {
        setFontSize(fontsize);
        setFontSpec(fontspec);
        sp_font_selector_set_fontspec(fsel, fontspec, fontsize);
    }
}

void
FontSelector::onFontSelectorSave()
{
    if (_fontspec != sp_font_selector_get_fontspec(fsel) || _fontsize != sp_font_selector_get_size(fsel)) {
        setFontSpec(sp_font_selector_get_fontspec(fsel));
        setFontSize(sp_font_selector_get_size(fsel));
        signal_fontselupd.emit();
        onExpanderChanged();
    }
}


} // namespace Widget
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
