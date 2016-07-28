/*
 *
 * Copyright (C) 2007 Author
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_FONT_SELECTOR_H
#define INKSCAPE_UI_WIDGET_FONT_SELECTOR_H

#include <gtkmm.h>
#include "widgets/font-selector.h"

struct SPFontSelector;
namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * A labelled text box, with spin buttons and optional
 * icon or suffix, for entering arbitrary number values. It adds an extra
 * number called "startseed", that is not UI edittable, but should be put in SVG.
 * This does NOT generate a random number, but provides merely the saving of 
 * the startseed value.
 */
class FontSelector : public Gtk::HBox
{
public:

    /**
     * Construct a FontSelector Widget.
     *
     * @param label     Label.
     * @param suffix    Suffix, placed after the widget (defaults to "").
     * @param icon      Icon filename, placed before the label (defaults to "").
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the label
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to false).
     */
    FontSelector( Glib::ustring const &label,
           Glib::ustring const &tooltip,
           Glib::ustring const &suffix = "",
           Glib::ustring const &icon = "",
           bool mnemonic = true);

    Glib::ustring getFontSpec() const;
    void onExpanderChanged();
    void setFontSpec(Glib::ustring fontspec);
    double getFontSize() const;
    void setFontSize(double fontsize);
    void setValue (Glib::ustring fontspec, double fontsize);
    sigc::signal <void> signal_fontselupd;

protected:
    Gtk::Widget  *_widget;
    bool expanded;
    Glib::ustring _label;
    Gtk::Expander * expander;
    SPFontSelector *fsel;
    Glib::ustring _fontspec;
    double _fontsize;

private:
    void onFontSelectorSave();
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_RANDOM_H

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
