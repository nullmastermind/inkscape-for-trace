// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Fill and Stroke dialog - implementation.
 *
 * Based on the old sp_object_properties_dialog.
 */
/* Authors:
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   Gustav Broberg <broberg@kth.se>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2004--2007 Authors
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "desktop-style.h"
#include "document.h"
#include "fill-and-stroke.h"
#include "filter-chemistry.h"
#include "inkscape.h"
#include "preferences.h"
#include "verbs.h"

#include "svg/css-ostringstream.h"

#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/widget/notebook-page.h"

#include "widgets/fill-style.h"
#include "widgets/paint-selector.h"
#include "widgets/stroke-style.h"


namespace Inkscape {
namespace UI {
namespace Dialog {

FillAndStroke::FillAndStroke()
    : UI::Widget::Panel("/dialogs/fillstroke", SP_VERB_DIALOG_FILL_STROKE)
    , _page_fill(Gtk::manage(new UI::Widget::NotebookPage(1, 1, true, true)))
    , _page_stroke_paint(Gtk::manage(new UI::Widget::NotebookPage(1, 1, true, true)))
    , _page_stroke_style(Gtk::manage(new UI::Widget::NotebookPage(1, 1, true, true)))
    , _composite_settings(SP_VERB_DIALOG_FILL_STROKE, "fillstroke",
                          UI::Widget::SimpleFilterModifier::ISOLATION |
                          UI::Widget::SimpleFilterModifier::BLEND |
                          UI::Widget::SimpleFilterModifier::BLUR |
                          UI::Widget::SimpleFilterModifier::OPACITY)
    , deskTrack()
    , targetDesktop(nullptr)
    , fillWdgt(nullptr)
    , strokeWdgt(nullptr)
    , desktopChangeConn()
{
    Gtk::Box *contents = _getContents();
    contents->set_spacing(2);
    contents->pack_start(_notebook, true, true);

    _notebook.append_page(*_page_fill, _createPageTabLabel(_("_Fill"), INKSCAPE_ICON("object-fill")));
    _notebook.append_page(*_page_stroke_paint, _createPageTabLabel(_("Stroke _paint"), INKSCAPE_ICON("object-stroke")));
    _notebook.append_page(*_page_stroke_style, _createPageTabLabel(_("Stroke st_yle"), INKSCAPE_ICON("object-stroke-style")));
    _notebook.set_vexpand(true);

    _notebook.signal_switch_page().connect(sigc::mem_fun(this, &FillAndStroke::_onSwitchPage));

    _layoutPageFill();
    _layoutPageStrokePaint();
    _layoutPageStrokeStyle();

    contents->pack_end(_composite_settings, Gtk::PACK_SHRINK);

    show_all_children();

    _composite_settings.setSubject(&_subject);

    // Connect this up last
    desktopChangeConn = deskTrack.connectDesktopChanged( sigc::mem_fun(*this, &FillAndStroke::setTargetDesktop) );
    deskTrack.connect(GTK_WIDGET(gobj()));
}

FillAndStroke::~FillAndStroke()
{
    _composite_settings.setSubject(nullptr);

    desktopChangeConn.disconnect();
    deskTrack.disconnect();
}

void FillAndStroke::setDesktop(SPDesktop *desktop)
{
    Panel::setDesktop(desktop);
    deskTrack.setBase(desktop);
}

void FillAndStroke::setTargetDesktop(SPDesktop *desktop)
{
    if (targetDesktop != desktop) {
        targetDesktop = desktop;
        if (fillWdgt) {
            sp_fill_style_widget_set_desktop(fillWdgt, desktop);
        }
        if (strokeWdgt) {
            sp_fill_style_widget_set_desktop(strokeWdgt, desktop);
        }
        if (strokeStyleWdgt) {
            sp_stroke_style_widget_set_desktop(strokeStyleWdgt, desktop);
        }
        _composite_settings.setSubject(&_subject);
    }
}

void FillAndStroke::_onSwitchPage(Gtk::Widget * /*page*/, guint pagenum)
{
    _savePagePref(pagenum);
}

void
FillAndStroke::_savePagePref(guint page_num)
{
    // remember the current page
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/dialogs/fillstroke/page", page_num);
}

void
FillAndStroke::_layoutPageFill()
{
    fillWdgt = Gtk::manage(sp_fill_style_widget_new());
    _page_fill->table().attach(*fillWdgt, 0, 0, 1, 1);
}

void
FillAndStroke::_layoutPageStrokePaint()
{
    strokeWdgt = Gtk::manage(sp_stroke_style_paint_widget_new());
    _page_stroke_paint->table().attach(*strokeWdgt, 0, 0, 1, 1);
}

void
FillAndStroke::_layoutPageStrokeStyle()
{
    strokeStyleWdgt = sp_stroke_style_line_widget_new();
    strokeStyleWdgt->set_hexpand();
    strokeStyleWdgt->set_halign(Gtk::ALIGN_START);

    _page_stroke_style->table().attach(*strokeStyleWdgt, 0, 0, 1, 1);
}

void
FillAndStroke::showPageFill()
{
    present();
    _notebook.set_current_page(0);
    _savePagePref(0);

}

void
FillAndStroke::showPageStrokePaint()
{
    present();
    _notebook.set_current_page(1);
    _savePagePref(1);
}

void
FillAndStroke::showPageStrokeStyle()
{
    present();
    _notebook.set_current_page(2);
    _savePagePref(2);

}

Gtk::HBox&
FillAndStroke::_createPageTabLabel(const Glib::ustring& label, const char *label_image)
{
    Gtk::HBox *_tab_label_box = Gtk::manage(new Gtk::HBox(false, 4));

    auto img = Gtk::manage(sp_get_icon_image(label_image, Gtk::ICON_SIZE_MENU));
    _tab_label_box->pack_start(*img);

    Gtk::Label *_tab_label = Gtk::manage(new Gtk::Label(label, true));
    _tab_label_box->pack_start(*_tab_label);
    _tab_label_box->show_all();

    return *_tab_label_box;
}

} // namespace Dialog
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
