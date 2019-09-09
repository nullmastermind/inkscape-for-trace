// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Text editing dialog.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   Abhishek Sharma
 *   John Smith
 *   Tavmjong Bah
 *
 * Copyright (C) 1999-2013 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "text-edit.h"

#include <glibmm/i18n.h>
#include <glibmm/markup.h>

#ifdef WITH_GTKSPELL
extern "C" {
# include <gtkspell/gtkspell.h>
}
#endif

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "style.h"
#include "text-editing.h"
#include "verbs.h"

#include <libnrtype/FontFactory.h>
#include <libnrtype/font-instance.h>
#include <libnrtype/font-lister.h>

#include "object/sp-flowtext.h"
#include "object/sp-text.h"
#include "object/sp-textpath.h"

#include "svg/css-ostringstream.h"
#include "ui/icon-names.h"
#include "ui/toolbar/text-toolbar.h"
#include "ui/widget/font-selector.h"

#include "util/units.h"


namespace Inkscape {
namespace UI {
namespace Dialog {

TextEdit::TextEdit()
    : UI::Widget::Panel("/dialogs/textandfont", SP_VERB_DIALOG_TEXT),
      font_label(_("_Font"), true),
      text_label(_("_Text"), true),
      feat_label(_("_Features"), true),
      setasdefault_button(_("Set as _default")),
      close_button(_("_Close"), true),
      apply_button(_("_Apply"), true),
      desktop(nullptr),
      deskTrack(),
      selectChangedConn(),
      subselChangedConn(),
      selectModifiedConn(),
      blocked(false),
      /*
           TRANSLATORS: Test string used in text and font dialog (when no
           * text has been entered) to get a preview of the font.  Choose
           * some representative characters that users of your locale will be
           * interested in.*/
      samplephrase(_("AaBbCcIiPpQq12369$\342\202\254\302\242?.;/()"))
{

    /* Font tab -------------------------------- */

    /* Font selector */
    // Do nothing.

    /* Font preview */
    preview_label.set_ellipsize (Pango::ELLIPSIZE_END);
    preview_label.set_justify (Gtk::JUSTIFY_CENTER);
    preview_label.set_line_wrap (false);

    font_vbox.set_border_width(4);
    font_vbox.pack_start(font_selector, true, true);
    font_vbox.pack_start(preview_label, false, false, 4);

    /* Features tab ---------------------------- */

    /* Features preview */
    preview_label2.set_ellipsize (Pango::ELLIPSIZE_END);
    preview_label2.set_justify (Gtk::JUSTIFY_CENTER);
    preview_label2.set_line_wrap (false);

    feat_vbox.set_border_width(4);
    feat_vbox.pack_start(font_features, true, true);
    feat_vbox.pack_start(preview_label2, false, false, 4);

    /* Text tab -------------------------------- */
    scroller.set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
    scroller.set_shadow_type(Gtk::SHADOW_IN);

    text_buffer = gtk_text_buffer_new (nullptr);
    text_view = gtk_text_view_new_with_buffer (text_buffer);
    gtk_text_view_set_wrap_mode ((GtkTextView *) text_view, GTK_WRAP_WORD);

#ifdef WITH_GTKSPELL
    /*
       TODO: Use computed xml:lang attribute of relevant element, if present, to specify the
       language (either as 2nd arg of gtkspell_new_attach, or with explicit
       gtkspell_set_language call in; see advanced.c example in gtkspell docs).
       onReadSelection looks like a suitable place.
    */
    GtkSpellChecker * speller = gtk_spell_checker_new();

    if (! gtk_spell_checker_attach(speller, GTK_TEXT_VIEW(text_view))) {
        g_print("gtkspell error:\n");
    }
#endif

    gtk_widget_set_size_request (text_view, -1, 64);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), TRUE);
    scroller.add(*Gtk::manage(Glib::wrap(text_view)));
    text_vbox.pack_start(scroller, true, true, 0);

    /* Notebook -----------------------------------*/
    notebook.set_name( "TextEdit Notebook" );
    notebook.append_page(font_vbox, font_label);
    notebook.append_page(feat_vbox, feat_label);
    notebook.append_page(text_vbox, text_label);

    /* Buttons (below notebook) ------------------ */
    setasdefault_button.set_use_underline(true);
    apply_button.set_can_default();
    button_row.pack_start(setasdefault_button, false, false, 0);
    button_row.pack_end(close_button, false, false, VB_MARGIN);
    button_row.pack_end(apply_button, false, false, VB_MARGIN);

    Gtk::Box *contents = _getContents();
    contents->set_name("TextEdit Dialog Box");
    contents->set_spacing(4);
    contents->pack_start(notebook, true, true);
    contents->pack_start(button_row, false, false, VB_MARGIN);

    /* Signal handlers */
    g_signal_connect ( G_OBJECT (text_buffer), "changed", G_CALLBACK (onTextChange), this );
    setasdefault_button.signal_clicked().connect(sigc::mem_fun(*this, &TextEdit::onSetDefault));
    apply_button.signal_clicked().connect(sigc::mem_fun(*this, &TextEdit::onApply));
    close_button.signal_clicked().connect(sigc::bind(_signal_response.make_slot(), GTK_RESPONSE_CLOSE));
    fontChangedConn = font_selector.connectChanged (sigc::mem_fun(*this, &TextEdit::onFontChange));
    fontFeaturesChangedConn = font_features.connectChanged(sigc::mem_fun(*this, &TextEdit::onChange));
    notebook.signal_switch_page().connect(sigc::mem_fun(*this, &TextEdit::onFontFeatures));
    desktopChangeConn = deskTrack.connectDesktopChanged( sigc::mem_fun(*this, &TextEdit::setTargetDesktop) );
    deskTrack.connect(GTK_WIDGET(gobj()));

    font_selector.set_name ("TextEdit");

    show_all_children();
}

TextEdit::~TextEdit()
{
    selectModifiedConn.disconnect();
    subselChangedConn.disconnect();
    selectChangedConn.disconnect();
    desktopChangeConn.disconnect();
    deskTrack.disconnect();
    fontChangedConn.disconnect();
    fontFeaturesChangedConn.disconnect();
}

void TextEdit::onSelectionModified(guint flags )
{
    gboolean style, content;

    style =  ((flags & ( SP_OBJECT_CHILD_MODIFIED_FLAG |
                    SP_OBJECT_STYLE_MODIFIED_FLAG  )) != 0 );

    content = ((flags & ( SP_OBJECT_CHILD_MODIFIED_FLAG |
                    SP_TEXT_CONTENT_MODIFIED_FLAG  )) != 0 );

    onReadSelection (style, content);
}

void TextEdit::onReadSelection ( gboolean dostyle, gboolean /*docontent*/ )
{
    if (blocked)
        return;

    if (!desktop || SP_ACTIVE_DESKTOP != desktop)
    {
        return;
    }

    blocked = true;

    SPItem *text = getSelectedTextItem ();

    Glib::ustring phrase = samplephrase;

    if (text)
    {
        guint items = getSelectedTextCount ();
        if (items == 1) {
            gtk_widget_set_sensitive (text_view, TRUE);
        } else {
            gtk_widget_set_sensitive (text_view, FALSE);
        }
        apply_button.set_sensitive ( false );
        setasdefault_button.set_sensitive ( true );

        gchar *str;
        str = sp_te_get_string_multiline (text);
        if (str) {
            if (items == 1) {
                gtk_text_buffer_set_text (text_buffer, str, strlen (str));
                gtk_text_buffer_set_modified (text_buffer, FALSE);
            }
            phrase = str;

        } else {
            gtk_text_buffer_set_text (text_buffer, "", 0);
        }

        text->getRepr(); // was being called but result ignored. Check this.
    } else {
        gtk_widget_set_sensitive (text_view, FALSE);
        apply_button.set_sensitive ( false );
        setasdefault_button.set_sensitive ( false );
    }

    if (dostyle) {

        // create temporary style
        SPStyle query(SP_ACTIVE_DOCUMENT);

        // Query style from desktop into it. This returns a result flag and fills query with the
        // style of subselection, if any, or selection

        int result_numbers = sp_desktop_query_style (SP_ACTIVE_DESKTOP, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);

        // If querying returned nothing, read the style from the text tool prefs (default style for new texts).
        if (result_numbers == QUERY_STYLE_NOTHING) {
            query.readFromPrefs("/tools/text");
        }

        Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();

        // Update family/style based on selection.
        font_lister->selection_update();
        Glib::ustring fontspec = font_lister->get_fontspec();

        // Update Font Face.
        font_selector.update_font ();

        // Update Size.
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
        double size = sp_style_css_size_px_to_units(query.font_size.computed, unit); 
        font_selector.update_size (size);
        selected_fontsize = size;
        // Update font features (variant) widget
        //int result_features =
        sp_desktop_query_style (SP_ACTIVE_DESKTOP, &query, QUERY_STYLE_PROPERTY_FONTVARIANTS);
        int result_features =
            sp_desktop_query_style (SP_ACTIVE_DESKTOP, &query, QUERY_STYLE_PROPERTY_FONTFEATURESETTINGS);
        font_features.update( &query, result_features == QUERY_STYLE_MULTIPLE_DIFFERENT, fontspec );
        Glib::ustring features = font_features.get_markup();

        // Update Preview
        setPreviewText (fontspec, features, phrase);
    }

    blocked = false;
}


void TextEdit::setPreviewText (Glib::ustring font_spec, Glib::ustring font_features, Glib::ustring phrase)
{
    if (font_spec.empty()) {
        preview_label.set_markup("");
        preview_label2.set_markup("");
        return;
    }

    Glib::ustring font_spec_escaped = Glib::Markup::escape_text( font_spec );
    Glib::ustring phrase_escaped = Glib::Markup::escape_text( phrase );

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
    double pt_size =
        Inkscape::Util::Quantity::convert(
            sp_style_css_size_units_to_px(font_selector.get_fontsize(), unit), "px", "pt");
    pt_size = std::min(pt_size, 100.0);

    // Pango font size is in 1024ths of a point
    Glib::ustring size = std::to_string( int(pt_size * PANGO_SCALE) );
    Glib::ustring markup = "<span font=\'" + font_spec_escaped +
        "\' size=\'" + size + "\'";
    if (!font_features.empty()) {
        markup += " font_features=\'" + font_features + "\'";
    }
    markup += ">" + phrase_escaped + "</span>";

    preview_label.set_markup (markup);
    preview_label2.set_markup (markup);
}


SPItem *TextEdit::getSelectedTextItem ()
{
    if (!SP_ACTIVE_DESKTOP)
        return nullptr;

    auto tmp= SP_ACTIVE_DESKTOP->getSelection()->items();
	for(auto i=tmp.begin();i!=tmp.end();++i)
    {
        if (SP_IS_TEXT(*i) || SP_IS_FLOWTEXT(*i))
            return *i;
    }

    return nullptr;
}


unsigned TextEdit::getSelectedTextCount ()
{
    if (!SP_ACTIVE_DESKTOP)
        return 0;

    unsigned int items = 0;

    auto tmp= SP_ACTIVE_DESKTOP->getSelection()->items();
	for(auto i=tmp.begin();i!=tmp.end();++i)
    {
        if (SP_IS_TEXT(*i) || SP_IS_FLOWTEXT(*i))
            ++items;
    }

    return items;
}

void TextEdit::onSelectionChange()
{
    onReadSelection (TRUE, TRUE);
}

void TextEdit::updateObjectText ( SPItem *text )
{
        GtkTextIter start, end;

        // write text
        if (gtk_text_buffer_get_modified (text_buffer)) {
            gtk_text_buffer_get_bounds (text_buffer, &start, &end);
            gchar *str = gtk_text_buffer_get_text (text_buffer, &start, &end, TRUE);
            sp_te_set_repr_text_multiline (text, str);
            g_free (str);
            gtk_text_buffer_set_modified (text_buffer, FALSE);
        }
}

SPCSSAttr *TextEdit::fillTextStyle ()
{
        SPCSSAttr *css = sp_repr_css_attr_new ();

        Glib::ustring fontspec = font_selector.get_fontspec();

        if( !fontspec.empty() ) {

            Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
            fontlister->fill_css( css, fontspec );

            // TODO, possibly move this to FontLister::set_css to be shared.
            Inkscape::CSSOStringStream os;
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
            if (prefs->getBool("/options/font/textOutputPx", true)) {
                os << sp_style_css_size_units_to_px(font_selector.get_fontsize(), unit)
                   << sp_style_get_css_unit_string(SP_CSS_UNIT_PX);
            } else {
                os << font_selector.get_fontsize() << sp_style_get_css_unit_string(unit);
            }
            sp_repr_css_set_property (css, "font-size", os.str().c_str());
        }

        // Font features
        font_features.fill_css( css );

        return css;
}

void TextEdit::onSetDefault()
{
    SPCSSAttr *css = fillTextStyle ();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    blocked = true;
    prefs->mergeStyle("/tools/text/style", css);
    blocked = false;

    sp_repr_css_attr_unref (css);

    setasdefault_button.set_sensitive ( false );
}

void TextEdit::onApply()
{
    blocked = true;

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    unsigned items = 0;
    auto item_list = desktop->getSelection()->items();
    SPCSSAttr *css = fillTextStyle ();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    for(auto i=item_list.begin();i!=item_list.end();++i){
        // apply style to the reprs of all text objects in the selection
        if (SP_IS_TEXT (*i) || (SP_IS_FLOWTEXT (*i)) ) {
            ++items;
        }
    }
    if (items == 1) {
        double factor = font_selector.get_fontsize() / selected_fontsize;
        prefs->setDouble("/options/font/scaleLineHeightFromFontSIze", factor);
    }
    sp_desktop_set_style(desktop, css, true);

    if (items == 0) {
        // no text objects; apply style to prefs for new objects
        prefs->mergeStyle("/tools/text/style", css);
        setasdefault_button.set_sensitive ( false );

    } else if (items == 1) {
        // exactly one text object; now set its text, too
        SPItem *item = SP_ACTIVE_DESKTOP->getSelection()->singleItem();
        if (SP_IS_TEXT (item) || SP_IS_FLOWTEXT(item)) {
            updateObjectText (item);
            SPStyle *item_style = item->style;
            if (SP_IS_TEXT(item) && item_style->inline_size.value == 0) {
                css = sp_css_attr_from_style(item_style, SP_STYLE_FLAG_IFSET);
                sp_repr_css_unset_property(css, "inline-size");
                item->changeCSS(css, "style");
            }
        }
    }

    // Update FontLister
    Glib::ustring fontspec = font_selector.get_fontspec();
    if( !fontspec.empty() ) {
        Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
        fontlister->set_fontspec( fontspec, false );
    }

    // complete the transaction
    DocumentUndo::done(SP_ACTIVE_DESKTOP->getDocument(), SP_VERB_CONTEXT_TEXT,
                       _("Set text style"));
    apply_button.set_sensitive ( false );

    sp_repr_css_attr_unref (css);

    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();
    font_lister->update_font_list(SP_ACTIVE_DESKTOP->getDocument());

    blocked = false;
}

void TextEdit::onFontFeatures(Gtk::Widget * widgt, int pos)
{
    if (pos == 1) {
        Glib::ustring fontspec = font_selector.get_fontspec();
        if (!fontspec.empty()) {
            font_instance *res = font_factory::Default()->FaceFromFontSpecification(fontspec.c_str());
            if (res && !res->fulloaded) {
                res->InitTheFace(true);
                font_features.update_opentype(fontspec);
            }
        }
    }
}

void TextEdit::onChange()
{
    if (blocked) {
        return;
    }

    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    gchar *str = gtk_text_buffer_get_text(text_buffer, &start, &end, TRUE);

    Glib::ustring fontspec = font_selector.get_fontspec();
    Glib::ustring features = font_features.get_markup();
    const gchar *phrase = str && *str ? str : samplephrase.c_str();
    setPreviewText(fontspec, features, phrase);
    g_free (str);

    SPItem *text = getSelectedTextItem();
    if (text) {
        apply_button.set_sensitive ( true );
    }

    setasdefault_button.set_sensitive ( true);
}

void TextEdit::onTextChange (GtkTextBuffer *text_buffer, TextEdit *self)
{
    self->onChange();
}

void TextEdit::onFontChange(Glib::ustring fontspec)
{
    font_features.update_opentype ( fontspec );
    onChange();
}

void TextEdit::setDesktop(SPDesktop *desktop)
{
    Panel::setDesktop(desktop);
    deskTrack.setBase(desktop);
}

void TextEdit::setTargetDesktop(SPDesktop *desktop)
{
    if (this->desktop != desktop) {
        if (this->desktop) {
            selectModifiedConn.disconnect();
            subselChangedConn.disconnect();
            selectChangedConn.disconnect();
        }
        this->desktop = desktop;
        if (desktop && desktop->selection) {
            selectChangedConn = desktop->selection->connectChanged(sigc::hide(sigc::mem_fun(*this, &TextEdit::onSelectionChange)));
            subselChangedConn = desktop->connectToolSubselectionChanged(sigc::hide(sigc::mem_fun(*this, &TextEdit::onSelectionChange)));
            selectModifiedConn = desktop->selection->connectModified(sigc::hide<0>(sigc::mem_fun(*this, &TextEdit::onSelectionModified)));
        }
        //widget_setup();
        onReadSelection (TRUE, TRUE);
    }
}

} //namespace Dialog
} //namespace UI
} //namespace Inkscape

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
