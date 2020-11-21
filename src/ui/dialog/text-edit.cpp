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

#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>
#include <gtkmm/notebook.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textview.h>

#ifdef WITH_GSPELL
# include <gspell/gspell.h>
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

#include "io/resource.h"
#include "svg/css-ostringstream.h"
#include "ui/icon-names.h"
#include "ui/toolbar/text-toolbar.h"
#include "ui/widget/font-selector.h"

#include "util/units.h"


namespace Inkscape {
namespace UI {
namespace Dialog {

TextEdit::TextEdit()
    : DialogBase("/dialogs/textandfont", SP_VERB_DIALOG_TEXT),
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

    std::string gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "dialog-text-edit.glade");
    Glib::RefPtr<Gtk::Builder> builder;
    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("GtkBuilder file loading failed for save template dialog");
        return;
    }

    Gtk::Box *contents;
    Gtk::Notebook *notebook;
    Gtk::Box *font_box;
    Gtk::Box *feat_box;

    builder->get_widget("contents", contents);
    builder->get_widget("notebook", notebook);
    builder->get_widget("font_box", font_box);
    builder->get_widget("feat_box", feat_box);
    builder->get_widget("preview_label", preview_label);
    builder->get_widget("preview_label2", preview_label2);
    builder->get_widget("text_view", text_view);
    builder->get_widget("setasdefault_button", setasdefault_button);
    builder->get_widget("apply_button", apply_button);

    text_buffer = Glib::RefPtr<Gtk::TextBuffer>::cast_static(builder->get_object("text_buffer"));

    font_box->pack_start(font_selector, true, true);
    font_box->reorder_child(font_selector, 0);
    feat_box->pack_start(font_features, true, true);
    feat_box->reorder_child(font_features, 0);

#ifdef WITH_GSPELL
    /*
       TODO: Use computed xml:lang attribute of relevant element, if present, to specify the
       language (either as 2nd arg of gtkspell_new_attach, or with explicit
       gtkspell_set_language call in; see advanced.c example in gtkspell docs).
       onReadSelection looks like a suitable place.
    */
    GspellTextView *gspell_view = gspell_text_view_get_from_gtk_text_view(text_view->gobj());
    gspell_text_view_basic_setup(gspell_view);
#endif

    add(*contents);

    /* Signal handlers */
    text_buffer->signal_changed().connect(sigc::mem_fun(*this, &TextEdit::onChange));
    setasdefault_button->signal_clicked().connect(sigc::mem_fun(*this, &TextEdit::onSetDefault));
    apply_button->signal_clicked().connect(sigc::mem_fun(*this, &TextEdit::onApply));
    fontChangedConn = font_selector.connectChanged(sigc::mem_fun(*this, &TextEdit::onFontChange));
    fontFeaturesChangedConn = font_features.connectChanged(sigc::mem_fun(*this, &TextEdit::onChange));
    notebook->signal_switch_page().connect(sigc::mem_fun(*this, &TextEdit::onFontFeatures));

    font_selector.set_name("TextEdit");

    show_all_children();
}

TextEdit::~TextEdit()
{
    selectModifiedConn.disconnect();
    subselChangedConn.disconnect();
    selectChangedConn.disconnect();
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

    blocked = true;

    SPItem *text = getSelectedTextItem ();

    Glib::ustring phrase = samplephrase;

    if (text)
    {
        guint items = getSelectedTextCount ();
        bool has_one_item = items == 1;
        text_view->set_sensitive(has_one_item);
        apply_button->set_sensitive(false);
        setasdefault_button->set_sensitive(true);

        Glib::ustring str = sp_te_get_string_multiline(text);
        if (!str.empty()) {
            if (has_one_item) {
                text_buffer->set_text(str);
                text_buffer->set_modified(false);
            }
            phrase = str;

        } else {
            text_buffer->set_text("");
        }

        text->getRepr(); // was being called but result ignored. Check this.
    } else {
        text_view->set_sensitive(false);
        apply_button->set_sensitive(false);
        setasdefault_button->set_sensitive(false);
    }

    if (dostyle && text) {
        auto *desktop = getDesktop();

        // create temporary style
        SPStyle query(desktop->getDocument());

        // Query style from desktop into it. This returns a result flag and fills query with the
        // style of subselection, if any, or selection

        int result_numbers = sp_desktop_query_style (desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);

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
        sp_desktop_query_style (desktop, &query, QUERY_STYLE_PROPERTY_FONTVARIANTS);
        int result_features =
            sp_desktop_query_style (desktop, &query, QUERY_STYLE_PROPERTY_FONTFEATURESETTINGS);
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
        preview_label->set_markup("");
        preview_label2->set_markup("");
        return;
    }

    // Limit number of lines in preview to arbitrary amount to prevent Text and Font dialog
    // from growing taller than a desktop
    const int max_lines = 4;
    // Ignore starting empty lines; they would show up as nothing
    auto start_pos = phrase.find_first_not_of(" \n\r\t");
    if (start_pos == Glib::ustring::npos) {
        start_pos = 0;
    }
    // Now take up to max_lines
    auto end_pos = Glib::ustring::npos;
    auto from = start_pos;
    for (int i = 0; i < max_lines; ++i) {
        end_pos = phrase.find("\n", from);
        if (end_pos == Glib::ustring::npos) { break; }
        from = end_pos + 1;
    }
    Glib::ustring phrase_trimmed = phrase.substr(start_pos, end_pos != Glib::ustring::npos ? end_pos - start_pos : end_pos);

    Glib::ustring font_spec_escaped = Glib::Markup::escape_text( font_spec );
    Glib::ustring phrase_escaped = Glib::Markup::escape_text(phrase_trimmed);

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

    preview_label->set_markup (markup);
    preview_label2->set_markup (markup);
}


SPItem *TextEdit::getSelectedTextItem ()
{
    if (!getDesktop())
        return nullptr;

    auto tmp= getDesktop()->getSelection()->items();
	for(auto i=tmp.begin();i!=tmp.end();++i)
    {
        if (SP_IS_TEXT(*i) || SP_IS_FLOWTEXT(*i))
            return *i;
    }

    return nullptr;
}


unsigned TextEdit::getSelectedTextCount ()
{
    if (!getDesktop())
        return 0;

    unsigned int items = 0;

    auto tmp= getDesktop()->getSelection()->items();
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
    Gtk::TextIter start, end;

    // write text
    if (text_buffer->get_modified()) {
        text_buffer->get_bounds(start, end);
        Glib::ustring str = text_buffer->get_text(start, end);
        sp_te_set_repr_text_multiline (text, str.c_str());
        text_buffer->set_modified(false);
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

    setasdefault_button->set_sensitive ( false );
}

void TextEdit::onApply()
{
    blocked = true;

    SPDesktop *desktop = getDesktop();

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
        setasdefault_button->set_sensitive ( false );

    } else if (items == 1) {
        // exactly one text object; now set its text, too
        SPItem *item = desktop->getSelection()->singleItem();
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
    DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                       _("Set text style"));
    apply_button->set_sensitive ( false );

    sp_repr_css_attr_unref (css);

    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();
    font_lister->update_font_list(desktop->getDocument());

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

    Gtk::TextIter start, end;
    text_buffer->get_bounds(start, end);
    Glib::ustring str = text_buffer->get_text(start, end);

    Glib::ustring fontspec = font_selector.get_fontspec();
    Glib::ustring features = font_features.get_markup();
    const Glib::ustring& phrase = str.empty() ? samplephrase : str;
    setPreviewText(fontspec, features, phrase);

    SPItem *text = getSelectedTextItem();
    if (text) {
        apply_button->set_sensitive ( true );
    }

    setasdefault_button->set_sensitive ( true);
}

void TextEdit::onFontChange(Glib::ustring fontspec)
{
    // Is not necesary update open type features this done when user click on font features tab
    onChange();
}

void TextEdit::update()
{
    if (!_app) {
        std::cerr << "TextEdit::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();

    selectModifiedConn.disconnect();
    subselChangedConn.disconnect();
    selectChangedConn.disconnect();

    {
        if (desktop && desktop->selection) {
            selectChangedConn = desktop->selection->connectChanged(sigc::hide(sigc::mem_fun(*this, &TextEdit::onSelectionChange)));
            subselChangedConn = desktop->connectToolSubselectionChanged(sigc::hide(sigc::mem_fun(*this, &TextEdit::onSelectionChange)));
            selectModifiedConn = desktop->selection->connectModified(sigc::hide<0>(sigc::mem_fun(*this, &TextEdit::onSelectionModified)));
            onReadSelection(TRUE, TRUE);
        }
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
