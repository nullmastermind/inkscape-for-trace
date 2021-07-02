// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/markup.h>
#include <glibmm/regex.h>

#include <gtkmm/cellrenderertext.h>

#include <libnrtype/font-instance.h>

#include "font-lister.h"
#include "FontFactory.h"

#include "desktop.h"
#include "desktop-style.h"
#include "document.h"
#include "inkscape.h"
#include "preferences.h"

#include "object/sp-object.h"

// Following are needed to limit the source of updating font data to text and containers.
#include "object/sp-root.h"
#include "object/sp-object-group.h"
#include "object/sp-anchor.h"
#include "object/sp-text.h"
#include "object/sp-tspan.h"
#include "object/sp-textpath.h"
#include "object/sp-tref.h"
#include "object/sp-flowtext.h"
#include "object/sp-flowdiv.h"

#include "xml/repr.h"


//#define DEBUG_FONT

// CSS dictates that font family names are case insensitive.
// This should really implement full Unicode case unfolding.
bool familyNamesAreEqual(const Glib::ustring &a, const Glib::ustring &b)
{
    return (a.casefold().compare(b.casefold()) == 0);
}

static const char* sp_font_family_get_name(PangoFontFamily* family)
{
    const char* name = pango_font_family_get_name(family);
    if (strncmp(name, "Sans", 4) == 0 && strlen(name) == 4)
        return "sans-serif";
    if (strncmp(name, "Serif", 5) == 0 && strlen(name) == 5)
        return "serif";
    if (strncmp(name, "Monospace", 9) == 0 && strlen(name) == 9)
        return "monospace";
    return name;
}

namespace Inkscape {

FontLister::FontLister()
    : current_family_row (0)
    , current_family ("sans-serif")
    , current_style ("Normal")
    , block (false)
{
    font_list_store = Gtk::ListStore::create(FontList);
    font_list_store->freeze_notify();
    
    /* Create default styles for use when font-family is unknown on system. */
    default_styles = g_list_append(nullptr, new StyleNames("Normal"));
    default_styles = g_list_append(default_styles, new StyleNames("Italic"));
    default_styles = g_list_append(default_styles, new StyleNames("Bold"));
    default_styles = g_list_append(default_styles, new StyleNames("Bold Italic"));

    // Get sorted font families from Pango
    std::vector<PangoFontFamily *> familyVector;
    font_factory::Default()->GetUIFamilies(familyVector);

    // Traverse through the family names and set up the list store
    for (auto & i : familyVector) {
        const char* displayName = sp_font_family_get_name(i);
        
        if (displayName == nullptr || *displayName == '\0') {
            continue;
        }
        
        Glib::ustring familyName = displayName;
        if (!familyName.empty()) {
            Gtk::TreeModel::iterator treeModelIter = font_list_store->append();
            (*treeModelIter)[FontList.family] = familyName;

            // we don't set this now (too slow) but the style will be cached if the user 
            // ever decides to use this font
            (*treeModelIter)[FontList.styles] = NULL;
            // store the pango representation for generating the style
            (*treeModelIter)[FontList.pango_family] = i;
            (*treeModelIter)[FontList.onSystem] = true;
        }
    }

    font_list_store->thaw_notify();

    style_list_store = Gtk::ListStore::create(FontStyleList);

    // Initialize style store with defaults
    style_list_store->freeze_notify();
    style_list_store->clear();
    for (GList *l = default_styles; l; l = l->next) {
        Gtk::TreeModel::iterator treeModelIter = style_list_store->append();
        (*treeModelIter)[FontStyleList.cssStyle] = ((StyleNames *)l->data)->CssName;
        (*treeModelIter)[FontStyleList.displayStyle] = ((StyleNames *)l->data)->DisplayName;
    }
    style_list_store->thaw_notify();
}

FontLister::~FontLister()
{
    // Delete default_styles
    for (GList *l = default_styles; l; l = l->next) {
        delete ((StyleNames *)l->data);
    }

    // Delete other styles
    Gtk::TreeModel::iterator iter = font_list_store->get_iter("0");
    while (iter != font_list_store->children().end()) {
        Gtk::TreeModel::Row row = *iter;
        GList *styles = row[FontList.styles];
        for (GList *l = styles; l; l = l->next) {
            delete ((StyleNames *)l->data);
        }
        ++iter;
    }
}

FontLister *FontLister::get_instance()
{
    static Inkscape::FontLister *instance = new Inkscape::FontLister();
    return instance;
}

// To do: remove model (not needed for C++ version).
// Ensures the style list for a particular family has been created.
void FontLister::ensureRowStyles(Glib::RefPtr<Gtk::TreeModel> model, Gtk::TreeModel::iterator const iter)
{
    Gtk::TreeModel::Row row = *iter;
    if (!row[FontList.styles]) {
        if (row[FontList.pango_family]) {
            row[FontList.styles] = font_factory::Default()->GetUIStyles(row[FontList.pango_family]);
        } else {
            row[FontList.styles] = default_styles;
        }
    }
}

Glib::ustring FontLister::get_font_family_markup(Gtk::TreeIter const &iter)
{
    Gtk::TreeModel::Row row = *iter;

    Glib::ustring family = row[FontList.family];
    bool onSystem        = row[FontList.onSystem];

    Glib::ustring family_escaped = Glib::Markup::escape_text( family );
    Glib::ustring markup;

    if (!onSystem) {
        markup = "<span foreground='darkblue'>";

        // See if font-family is on system (separately for each family in font stack).
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s*,\\s*", family);

        for (auto token: tokens) {
            bool found = false;
            Gtk::TreeModel::Children children = get_font_list()->children();
            for (auto iter2: children) {
                Gtk::TreeModel::Row row2 = *iter2;
                Glib::ustring family2 = row2[FontList.family];
                bool onSystem2        = row2[FontList.onSystem];
                if (onSystem2 && familyNamesAreEqual(token, family2)) {
                    found = true;
                    break;
                }
            }

            if (found) {
                markup += Glib::Markup::escape_text (token);
                markup += ", ";
            } else {
                markup += "<span strikethrough=\"true\" strikethrough_color=\"red\">";
                markup += Glib::Markup::escape_text (token);
                markup += "</span>";
                markup += ", ";
            }
        }

        // Remove extra comma and space from end.
        if (markup.size() >= 2) {
            markup.resize(markup.size() - 2);
        }
        markup += "</span>";

    } else {
        markup = family_escaped;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int show_sample = prefs->getInt("/tools/text/show_sample_in_list", 1);
    if (show_sample) {

        Glib::ustring sample = prefs->getString("/tools/text/font_sample");

        markup += "  <span foreground='gray' font_family='";
        markup += family_escaped;
        markup += "'>";
        markup += sample;
        markup += "</span>";
    }

    // std::cout << "Markup: " << markup << std::endl;
    return markup;
}

// Example of how to use "foreach_iter"
// bool
// FontLister::print_document_font( const Gtk::TreeModel::iterator &iter ) {
//   Gtk::TreeModel::Row row = *iter;
//   if( !row[FontList.onSystem] ) {
// 	   std::cout << " Not on system: " << row[FontList.family] << std::endl;
// 	   return false;
//   }
//   return true;
// }
// font_list_store->foreach_iter( sigc::mem_fun(*this, &FontLister::print_document_font ));

/* Used to insert a font that was not in the document and not on the system into the font list. */
void FontLister::insert_font_family(Glib::ustring new_family)
{
    GList *styles = default_styles;

    /* In case this is a fallback list, check if first font-family on system. */
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", new_family);
    if (!tokens.empty() && !tokens[0].empty()) {
        Gtk::TreeModel::iterator iter2 = font_list_store->get_iter("0");
        while (iter2 != font_list_store->children().end()) {
            Gtk::TreeModel::Row row = *iter2;
            if (row[FontList.onSystem] && familyNamesAreEqual(tokens[0], row[FontList.family])) {
                if (!row[FontList.styles]) {
                    row[FontList.styles] = font_factory::Default()->GetUIStyles(row[FontList.pango_family]);
                }
                styles = row[FontList.styles];
                break;
            }
            ++iter2;
        }
    }

    Gtk::TreeModel::iterator treeModelIter = font_list_store->prepend();
    (*treeModelIter)[FontList.family] = new_family;
    (*treeModelIter)[FontList.styles] = styles;
    (*treeModelIter)[FontList.onSystem] = false;
    (*treeModelIter)[FontList.pango_family] = NULL;

    current_family = new_family;
    current_family_row = 0;
    current_style = "Normal";

    emit_update();
}

void FontLister::update_font_list(SPDocument *document)
{
    SPObject *root = document->getRoot();
    if (!root) {
        return;
    }

    font_list_store->freeze_notify();

    /* Find if current row is in document or system part of list */
    gboolean row_is_system = false;
    if (current_family_row > -1) {
        Gtk::TreePath path;
        path.push_back(current_family_row);
        Gtk::TreeModel::iterator iter = font_list_store->get_iter(path);
        if (iter) {
            row_is_system = (*iter)[FontList.onSystem];
            // std::cout << "  In:  row: " << current_family_row << "  " << (*iter)[FontList.family] << std::endl;
        }
    }

    /* Clear all old document font-family entries */
    Gtk::TreeModel::iterator iter = font_list_store->get_iter("0");
    while (iter != font_list_store->children().end()) {
        Gtk::TreeModel::Row row = *iter;
        if (!row[FontList.onSystem]) {
            // std::cout << " Not on system: " << row[FontList.family] << std::endl;
            iter = font_list_store->erase(iter);
        } else {
            // std::cout << " First on system: " << row[FontList.family] << std::endl;
            break;
        }
    }

    /* Get "font-family"s and styles used in document. */
    std::map<Glib::ustring, std::set<Glib::ustring>> font_data;
    update_font_data_recursive(*root, font_data);

    /* Insert separator */
    if (!font_data.empty()) {
        Gtk::TreeModel::iterator treeModelIter = font_list_store->prepend();
        (*treeModelIter)[FontList.family] = "#";
        (*treeModelIter)[FontList.onSystem] = false;
    }

    /* Insert font-family's in document. */
    for (auto i: font_data) {

        GList *styles = default_styles;

        /* See if font-family (or first in fallback list) is on system. If so, get styles. */
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", i.first);
        if (!tokens.empty() && !tokens[0].empty()) {

            Gtk::TreeModel::iterator iter2 = font_list_store->get_iter("0");
            while (iter2 != font_list_store->children().end()) {
                Gtk::TreeModel::Row row = *iter2;
                if (row[FontList.onSystem] && familyNamesAreEqual(tokens[0], row[FontList.family])) {
                    // Found font on system, set style list to system font style list.
                    if (!row[FontList.styles]) {
                        row[FontList.styles] = font_factory::Default()->GetUIStyles(row[FontList.pango_family]);
                    }

                    // Add new styles (from 'font-variation-settings', these are not include in GetUIStyles()).
                    for (auto j: i.second) {
                        // std::cout << "  Inserting: " << j << std::endl;

                        bool exists = false;
                        for(GList *temp = row[FontList.styles]; temp; temp = temp->next) {
                            if( ((StyleNames*)temp->data)->CssName.compare( j ) == 0 ) {
                                exists = true;
                                break;
                            }
                        }

                        if (!exists) {
                            row[FontList.styles] = g_list_append(row[FontList.styles], new StyleNames(j,j));
                        }
                    }

                    styles = row[FontList.styles];
                    break;
                }
                ++iter2;
            }
        }

        Gtk::TreeModel::iterator treeModelIter = font_list_store->prepend();
        (*treeModelIter)[FontList.family] = reinterpret_cast<const char *>(g_strdup((i.first).c_str()));
        (*treeModelIter)[FontList.styles] = styles;
        (*treeModelIter)[FontList.onSystem] = false;    // false if document font
        (*treeModelIter)[FontList.pango_family] = NULL; // CHECK ME (set to pango_family if on system?)

    }

    /* Now we do a song and dance to find the correct row as the row corresponding
     * to the current_family may have changed. We can't simply search for the
     * family name in the list since it can occur twice, once in the document
     * font family part and once in the system font family part. Above we determined
     * which part it is in.
     */
    if (current_family_row > -1) {
        int start = 0;
        if (row_is_system)
            start = font_data.size();
        int length = font_list_store->children().size();
        for (int i = 0; i < length; ++i) {
            int row = i + start;
            if (row >= length)
                row -= length;
            Gtk::TreePath path;
            path.push_back(row);
            Gtk::TreeModel::iterator iter = font_list_store->get_iter(path);
            if (iter) {
                if (familyNamesAreEqual(current_family, (*iter)[FontList.family])) {
                    current_family_row = row;
                    break;
                }
            }
        }
    }
    // std::cout << "  Out: row: " << current_family_row << "  " << current_family << std::endl;

    font_list_store->thaw_notify();
    emit_update();
}

void FontLister::update_font_data_recursive(SPObject& r, std::map<Glib::ustring, std::set<Glib::ustring>> &font_data)
{
    // Text nodes (i.e. the content of <text> or <tspan>) do not have their own style.
    if (r.getRepr()->type() == Inkscape::XML::NodeType::TEXT_NODE) {
        return;
    }

    PangoFontDescription* descr = ink_font_description_from_style( r.style );
    const gchar* font_family_char = pango_font_description_get_family(descr);
    if (font_family_char) {
        Glib::ustring font_family(font_family_char);
        pango_font_description_unset_fields( descr, PANGO_FONT_MASK_FAMILY);

        gchar* font_style_char = pango_font_description_to_string(descr);
        Glib::ustring font_style(font_style_char);
        g_free(font_style_char);

        if (!font_family.empty() && !font_style.empty()) {
            font_data[font_family].insert(font_style);
        }
    } else {
        // We're starting from root and looking at all elements... we should probably white list text/containers.
        std::cerr << "FontLister::update_font_data_recursive: descr without font family! " << (r.getId()?r.getId():"null") << std::endl;
    }
    pango_font_description_free(descr);

    if (SP_IS_GROUP(&r)    ||
        SP_IS_ANCHOR(&r)   ||
        SP_IS_ROOT(&r)     ||
        SP_IS_TEXT(&r)     ||
        SP_IS_TSPAN(&r)    ||
        SP_IS_TEXTPATH(&r) ||
        SP_IS_TREF(&r)     ||
        SP_IS_FLOWTEXT(&r) ||
        SP_IS_FLOWDIV(&r)  ||
        SP_IS_FLOWPARA(&r) ||
        SP_IS_FLOWLINE(&r)) {
        for (auto& child: r.children) {
            update_font_data_recursive(child, font_data);
        }
    }
}

void FontLister::emit_update()
{
    if (block) return;

    block = true;
    update_signal.emit ();
    block = false;
}


Glib::ustring FontLister::canonize_fontspec(Glib::ustring fontspec)
{

    // Pass fontspec to and back from Pango to get a the fontspec in
    // canonical form.  -inkscape-font-specification relies on the
    // Pango constructed fontspec not changing form. If it does,
    // this is the place to fix it.
    PangoFontDescription *descr = pango_font_description_from_string(fontspec.c_str());
    gchar *canonized = pango_font_description_to_string(descr);
    Glib::ustring Canonized = canonized;
    g_free(canonized);
    pango_font_description_free(descr);

    // Pango canonized strings remove space after comma between family names. Put it back.
    // But don't add a space inside a 'font-variation-settings' declaration (this breaks Pango).
    size_t i = 0;
    while ((i = Canonized.find_first_of(",@", i)) != std::string::npos ) {
        if (Canonized[i] == '@') // Found start of 'font-variation-settings'.
            break;
        Canonized.replace(i, 1, ", ");
        i += 2;
    }

    return Canonized;
}

Glib::ustring FontLister::system_fontspec(Glib::ustring fontspec)
{
    // Find what Pango thinks is the closest match.
    Glib::ustring out = fontspec;

    PangoFontDescription *descr = pango_font_description_from_string(fontspec.c_str());
    font_instance *res = (font_factory::Default())->Face(descr);
    if (res && res->pFont) {
        PangoFontDescription *nFaceDesc = pango_font_describe(res->pFont);
        out = sp_font_description_get_family(nFaceDesc);
    }
    pango_font_description_free(descr);

    return out;
}

std::pair<Glib::ustring, Glib::ustring> FontLister::ui_from_fontspec(Glib::ustring fontspec)
{
    PangoFontDescription *descr = pango_font_description_from_string(fontspec.c_str());
    const gchar *family = pango_font_description_get_family(descr);
    if (!family)
        family = "sans-serif";
    Glib::ustring Family = family;

    // PANGO BUG...
    //   A font spec of Delicious, 500 Italic should result in a family of 'Delicious'
    //   and a style of 'Medium Italic'. It results instead with: a family of
    //   'Delicious, 500' with a style of 'Medium Italic'. We chop of any weight numbers
    //   at the end of the family:  match ",[1-9]00^".
    Glib::RefPtr<Glib::Regex> weight = Glib::Regex::create(",[1-9]00$");
    Family = weight->replace(Family, 0, "", Glib::REGEX_MATCH_PARTIAL);

    // Pango canonized strings remove space after comma between family names. Put it back.
    size_t i = 0;
    while ((i = Family.find(",", i)) != std::string::npos) {
        Family.replace(i, 1, ", ");
        i += 2;
    }

    pango_font_description_unset_fields(descr, PANGO_FONT_MASK_FAMILY);
    gchar *style = pango_font_description_to_string(descr);
    Glib::ustring Style = style;
    pango_font_description_free(descr);
    g_free(style);

    return std::make_pair(Family, Style);
}

std::pair<Glib::ustring, Glib::ustring> FontLister::selection_update()
{
#ifdef DEBUG_FONT
    std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "FontLister::selection_update: entrance" << std::endl;
#endif
    // Get fontspec from a selection, preferences, or thin air.
    Glib::ustring fontspec;
    SPStyle query(SP_ACTIVE_DOCUMENT);

    // Directly from stored font specification.
    int result =
        sp_desktop_query_style(SP_ACTIVE_DESKTOP, &query, QUERY_STYLE_PROPERTY_FONT_SPECIFICATION);

    //std::cout << "  Attempting selected style" << std::endl;
    if (result != QUERY_STYLE_NOTHING && query.font_specification.set) {
        fontspec = query.font_specification.value();
        //std::cout << "   fontspec from query   :" << fontspec << ":" << std::endl;
    }

    // From style
    if (fontspec.empty()) {
        //std::cout << "  Attempting desktop style" << std::endl;
        int rfamily = sp_desktop_query_style(SP_ACTIVE_DESKTOP, &query, QUERY_STYLE_PROPERTY_FONTFAMILY);
        int rstyle = sp_desktop_query_style(SP_ACTIVE_DESKTOP, &query, QUERY_STYLE_PROPERTY_FONTSTYLE);

        // Must have text in selection
        if (rfamily != QUERY_STYLE_NOTHING && rstyle != QUERY_STYLE_NOTHING) {
            fontspec = fontspec_from_style(&query);
        }
        //std::cout << "   fontspec from style   :" << fontspec << ":" << std::endl;
    }

    // From preferences
    if (fontspec.empty()) {
        //std::cout << "  Attempting preferences" << std::endl;
        query.readFromPrefs("/tools/text");
        fontspec = fontspec_from_style(&query);
        //std::cout << "   fontspec from prefs   :" << fontspec << ":" << std::endl;
    }

    // From thin air
    if (fontspec.empty()) {
        //std::cout << "  Attempting thin air" << std::endl;
        fontspec = current_family + ", " + current_style;
        //std::cout << "   fontspec from thin air   :" << fontspec << ":" << std::endl;
    }

    std::pair<Glib::ustring, Glib::ustring> ui = ui_from_fontspec(fontspec);
    set_font_family(ui.first);
    set_font_style(ui.second);

#ifdef DEBUG_FONT
    std::cout << "   family_row:           :" << current_family_row << ":" << std::endl;
    std::cout << "   family:               :" << current_family << ":" << std::endl;
    std::cout << "   style:                :" << current_style << ":" << std::endl;
    std::cout << "FontLister::selection_update: exit" << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
#endif

    emit_update();

    return std::make_pair(current_family, current_style);
}


// Set fontspec. If check is false, best style match will not be done.
void FontLister::set_fontspec(Glib::ustring new_fontspec, bool /*check*/)
{
    std::pair<Glib::ustring, Glib::ustring> ui = ui_from_fontspec(new_fontspec);
    Glib::ustring new_family = ui.first;
    Glib::ustring new_style = ui.second;

#ifdef DEBUG_FONT
    std::cout << "FontLister::set_fontspec: family: " << new_family
              << "   style:" << new_style << std::endl;
#endif

    set_font_family(new_family, false, false);
    set_font_style(new_style, false);

    emit_update();
}


// TODO: use to determine font-selector best style
// TODO: create new function new_font_family(Gtk::TreeModel::iterator iter)
std::pair<Glib::ustring, Glib::ustring> FontLister::new_font_family(Glib::ustring new_family, bool /*check_style*/)
{
#ifdef DEBUG_FONT
    std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "FontLister::new_font_family: " << new_family << std::endl;
#endif

    // No need to do anything if new family is same as old family.
    if (familyNamesAreEqual(new_family, current_family)) {
#ifdef DEBUG_FONT
        std::cout << "FontLister::new_font_family: exit: no change in family." << std::endl;
        std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
#endif
        return std::make_pair(current_family, current_style);
    }

    // We need to do two things:
    // 1. Update style list for new family.
    // 2. Select best valid style match to old style.

    // For finding style list, use list of first family in font-family list.
    GList *styles = nullptr;
    Gtk::TreeModel::iterator iter = font_list_store->get_iter("0");
    while (iter != font_list_store->children().end()) {

        Gtk::TreeModel::Row row = *iter;

        if (familyNamesAreEqual(new_family, row[FontList.family])) {
            if (!row[FontList.styles]) {
                row[FontList.styles] = font_factory::Default()->GetUIStyles(row[FontList.pango_family]);
            }
            styles = row[FontList.styles];
            break;
        }
        ++iter;
    }

    // Newly typed in font-family may not yet be in list... use default list.
    // TODO: if font-family is list, check if first family in list is on system
    // and set style accordingly.
    if (styles == nullptr) {
        styles = default_styles;
    }

    // Update style list.
    style_list_store->freeze_notify();
    style_list_store->clear();

    for (GList *l = styles; l; l = l->next) {
        Gtk::TreeModel::iterator treeModelIter = style_list_store->append();
        (*treeModelIter)[FontStyleList.cssStyle] = ((StyleNames *)l->data)->CssName;
        (*treeModelIter)[FontStyleList.displayStyle] = ((StyleNames *)l->data)->DisplayName;
    }

    style_list_store->thaw_notify();

    // Find best match to the style from the old font-family to the
    // styles available with the new font.
    // TODO: Maybe check if an exact match exists before using Pango.
    Glib::ustring best_style = get_best_style_match(new_family, current_style);

#ifdef DEBUG_FONT
    std::cout << "FontLister::new_font_family: exit: " << new_family << " " << best_style << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
#endif
    return std::make_pair(new_family, best_style);
}

std::pair<Glib::ustring, Glib::ustring> FontLister::set_font_family(Glib::ustring new_family, bool check_style,
                                                                    bool emit)
{

#ifdef DEBUG_FONT
    std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "FontLister::set_font_family: " << new_family << std::endl;
#endif

    std::pair<Glib::ustring, Glib::ustring> ui = new_font_family(new_family, check_style);
    current_family = ui.first;
    current_style = ui.second;

#ifdef DEBUG_FONT
    std::cout << "   family_row:           :" << current_family_row << ":" << std::endl;
    std::cout << "   family:               :" << current_family << ":" << std::endl;
    std::cout << "   style:                :" << current_style << ":" << std::endl;
    std::cout << "FontLister::set_font_family: end" << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
#endif
    if (emit) {
        emit_update();
    }
    return ui;
}


std::pair<Glib::ustring, Glib::ustring> FontLister::set_font_family(int row, bool check_style, bool emit)
{

#ifdef DEBUG_FONT
    std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "FontLister::set_font_family( row ): " << row << std::endl;
#endif

    current_family_row = row;
    Gtk::TreePath path;
    path.push_back(row);
    Glib::ustring new_family = current_family;
    Gtk::TreeModel::iterator iter = font_list_store->get_iter(path);
    if (iter) {
        new_family = (*iter)[FontList.family];
    }

    std::pair<Glib::ustring, Glib::ustring> ui = set_font_family(new_family, check_style, emit);

#ifdef DEBUG_FONT
    std::cout << "FontLister::set_font_family( row ): end" << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
#endif
    return ui;
}


void FontLister::set_font_style(Glib::ustring new_style, bool emit)
{

// TODO: Validate input using Pango. If Pango doesn't recognize a style it will
// attach the "invalid" style to the font-family.

#ifdef DEBUG_FONT
    std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "FontLister:set_font_style: " << new_style << std::endl;
#endif
    current_style = new_style;

#ifdef DEBUG_FONT
    std::cout << "   family:                " << current_family << std::endl;
    std::cout << "   style:                 " << current_style << std::endl;
    std::cout << "FontLister::set_font_style: end" << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
#endif
    if (emit) {
        emit_update();
    }
}


// We do this ourselves as we can't rely on FontFactory.
void FontLister::fill_css(SPCSSAttr *css, Glib::ustring fontspec)
{
    if (fontspec.empty()) {
        fontspec = get_fontspec();
    }

    std::pair<Glib::ustring, Glib::ustring> ui = ui_from_fontspec(fontspec);

    Glib::ustring family = ui.first;


    // Font spec is single quoted... for the moment
    Glib::ustring fontspec_quoted(fontspec);
    css_quote(fontspec_quoted);
    sp_repr_css_set_property(css, "-inkscape-font-specification", fontspec_quoted.c_str());

    // Font families needs to be properly quoted in CSS (used unquoted in font-lister)
    css_font_family_quote(family);
    sp_repr_css_set_property(css, "font-family", family.c_str());

    PangoFontDescription *desc = pango_font_description_from_string(fontspec.c_str());
    PangoWeight weight = pango_font_description_get_weight(desc);
    switch (weight) {
        case PANGO_WEIGHT_THIN:
            sp_repr_css_set_property(css, "font-weight", "100");
            break;
        case PANGO_WEIGHT_ULTRALIGHT:
            sp_repr_css_set_property(css, "font-weight", "200");
            break;
        case PANGO_WEIGHT_LIGHT:
            sp_repr_css_set_property(css, "font-weight", "300");
            break;
#if PANGO_VERSION_CHECK(1,36,6)
        case PANGO_WEIGHT_SEMILIGHT:
            sp_repr_css_set_property(css, "font-weight", "350");
            break;
#endif
        case PANGO_WEIGHT_BOOK:
            sp_repr_css_set_property(css, "font-weight", "380");
            break;
        case PANGO_WEIGHT_NORMAL:
            sp_repr_css_set_property(css, "font-weight", "normal");
            break;
        case PANGO_WEIGHT_MEDIUM:
            sp_repr_css_set_property(css, "font-weight", "500");
            break;
        case PANGO_WEIGHT_SEMIBOLD:
            sp_repr_css_set_property(css, "font-weight", "600");
            break;
        case PANGO_WEIGHT_BOLD:
            sp_repr_css_set_property(css, "font-weight", "bold");
            break;
        case PANGO_WEIGHT_ULTRABOLD:
            sp_repr_css_set_property(css, "font-weight", "800");
            break;
        case PANGO_WEIGHT_HEAVY:
            sp_repr_css_set_property(css, "font-weight", "900");
            break;
        case PANGO_WEIGHT_ULTRAHEAVY:
            sp_repr_css_set_property(css, "font-weight", "1000");
            break;
    }

    PangoStyle style = pango_font_description_get_style(desc);
    switch (style) {
        case PANGO_STYLE_NORMAL:
            sp_repr_css_set_property(css, "font-style", "normal");
            break;
        case PANGO_STYLE_OBLIQUE:
            sp_repr_css_set_property(css, "font-style", "oblique");
            break;
        case PANGO_STYLE_ITALIC:
            sp_repr_css_set_property(css, "font-style", "italic");
            break;
    }

    PangoStretch stretch = pango_font_description_get_stretch(desc);
    switch (stretch) {
        case PANGO_STRETCH_ULTRA_CONDENSED:
            sp_repr_css_set_property(css, "font-stretch", "ultra-condensed");
            break;
        case PANGO_STRETCH_EXTRA_CONDENSED:
            sp_repr_css_set_property(css, "font-stretch", "extra-condensed");
            break;
        case PANGO_STRETCH_CONDENSED:
            sp_repr_css_set_property(css, "font-stretch", "condensed");
            break;
        case PANGO_STRETCH_SEMI_CONDENSED:
            sp_repr_css_set_property(css, "font-stretch", "semi-condensed");
            break;
        case PANGO_STRETCH_NORMAL:
            sp_repr_css_set_property(css, "font-stretch", "normal");
            break;
        case PANGO_STRETCH_SEMI_EXPANDED:
            sp_repr_css_set_property(css, "font-stretch", "semi-expanded");
            break;
        case PANGO_STRETCH_EXPANDED:
            sp_repr_css_set_property(css, "font-stretch", "expanded");
            break;
        case PANGO_STRETCH_EXTRA_EXPANDED:
            sp_repr_css_set_property(css, "font-stretch", "extra-expanded");
            break;
        case PANGO_STRETCH_ULTRA_EXPANDED:
            sp_repr_css_set_property(css, "font-stretch", "ultra-expanded");
            break;
    }

    PangoVariant variant = pango_font_description_get_variant(desc);
    switch (variant) {
        case PANGO_VARIANT_NORMAL:
            sp_repr_css_set_property(css, "font-variant", "normal");
            break;
        case PANGO_VARIANT_SMALL_CAPS:
            sp_repr_css_set_property(css, "font-variant", "small-caps");
            break;
    }

#if PANGO_VERSION_CHECK(1,41,1)
    // Convert Pango variations string to CSS format
    const char* str = pango_font_description_get_variations(desc);

    std::string variations;

    if (str) {

        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", str);

        Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("(\\w{4})=([-+]?\\d*\\.?\\d+([eE][-+]?\\d+)?)");
        Glib::MatchInfo matchInfo;
        for (auto token: tokens) {
            regex->match(token, matchInfo);
            if (matchInfo.matches()) {
                variations += "'";
                variations += matchInfo.fetch(1).raw();
                variations += "' ";
                variations += matchInfo.fetch(2).raw();
                variations += ", ";
            }
        }
        if (variations.length() >= 2) { // Remove last comma/space
            variations.pop_back();
            variations.pop_back();
        }
    }

    if (!variations.empty()) {
        sp_repr_css_set_property(css, "font-variation-settings", variations.c_str());
    } else {
        sp_repr_css_unset_property(css,  "font-variation-settings" );
    }
#endif
    pango_font_description_free(desc);
}


Glib::ustring FontLister::fontspec_from_style(SPStyle *style)
{

    PangoFontDescription* descr = ink_font_description_from_style( style );
    Glib::ustring fontspec = pango_font_description_to_string( descr );
    pango_font_description_free(descr);

    //std::cout << "FontLister:fontspec_from_style: " << fontspec << std::endl;

    return fontspec;
}


Gtk::TreeModel::Row FontLister::get_row_for_font(Glib::ustring family)
{

    Gtk::TreeModel::iterator iter = font_list_store->get_iter("0");
    while (iter != font_list_store->children().end()) {

        Gtk::TreeModel::Row row = *iter;

        if (familyNamesAreEqual(family, row[FontList.family])) {
            return row;
        }

        ++iter;
    }

    throw FAMILY_NOT_FOUND;
}

Gtk::TreePath FontLister::get_path_for_font(Glib::ustring family)
{
    return font_list_store->get_path(get_row_for_font(family));
}

bool FontLister::is_path_for_font(Gtk::TreePath path, Glib::ustring family)
{
    Gtk::TreeModel::iterator iter = font_list_store->get_iter(path);
    if (iter) {
        return familyNamesAreEqual(family, (*iter)[FontList.family]);
    }

    return false;
}

Gtk::TreeModel::Row FontLister::get_row_for_style(Glib::ustring style)
{

    Gtk::TreeModel::iterator iter = style_list_store->get_iter("0");
    while (iter != style_list_store->children().end()) {

        Gtk::TreeModel::Row row = *iter;

        if (familyNamesAreEqual(style, row[FontStyleList.cssStyle])) {
            return row;
        }

        ++iter;
    }

    throw STYLE_NOT_FOUND;
}

static gint compute_distance(const PangoFontDescription *a, const PangoFontDescription *b)
{

    // Weight: multiples of 100
    gint distance = abs(pango_font_description_get_weight(a) -
                        pango_font_description_get_weight(b));

    distance += 10000 * abs(pango_font_description_get_stretch(a) -
                            pango_font_description_get_stretch(b));

    PangoStyle style_a = pango_font_description_get_style(a);
    PangoStyle style_b = pango_font_description_get_style(b);
    if (style_a != style_b) {
        if ((style_a == PANGO_STYLE_OBLIQUE && style_b == PANGO_STYLE_ITALIC) ||
            (style_b == PANGO_STYLE_OBLIQUE && style_a == PANGO_STYLE_ITALIC)) {
            distance += 1000; // Oblique and italic are almost the same
        } else {
            distance += 100000; // Normal vs oblique/italic, not so similar
        }
    }

    // Normal vs small-caps
    distance += 1000000 * abs(pango_font_description_get_variant(a) -
                              pango_font_description_get_variant(b));
    return distance;
}

// This is inspired by pango_font_description_better_match, but that routine
// always returns false if variant or stretch are different. This means, for
// example, that PT Sans Narrow with style Bold Condensed is never matched
// to another font-family with Bold style.
gboolean font_description_better_match(PangoFontDescription *target, PangoFontDescription *old_desc, PangoFontDescription *new_desc)
{
    if (old_desc == nullptr)
        return true;
    if (new_desc == nullptr)
        return false;

    int old_distance = compute_distance(target, old_desc);
    int new_distance = compute_distance(target, new_desc);
    //std::cout << "font_description_better_match: old: " << old_distance << std::endl;
    //std::cout << "                               new: " << new_distance << std::endl;

    return (new_distance < old_distance);
}

// void
// font_description_dump( PangoFontDescription* target ) {
//   std::cout << "  Font:      " << pango_font_description_to_string( target ) << std::endl;
//   std::cout << "    style:   " << pango_font_description_get_style(   target ) << std::endl;
//   std::cout << "    weight:  " << pango_font_description_get_weight(  target ) << std::endl;
//   std::cout << "    variant: " << pango_font_description_get_variant( target ) << std::endl;
//   std::cout << "    stretch: " << pango_font_description_get_stretch( target ) << std::endl;
//   std::cout << "    gravity: " << pango_font_description_get_gravity( target ) << std::endl;
// }

/* Returns style string */
// TODO: Remove or turn into function to be used by new_font_family.
Glib::ustring FontLister::get_best_style_match(Glib::ustring family, Glib::ustring target_style)
{

#ifdef DEBUG_FONT
    std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "FontLister::get_best_style_match: " << family << " : " << target_style << std::endl;
#endif

    Glib::ustring fontspec = family + ", " + target_style;

    Gtk::TreeModel::Row row;
    try
    {
        row = get_row_for_font(family);
    }
    catch (...)
    {
        std::cerr << "FontLister::get_best_style_match(): can't find family: " << family << std::endl;
        return (target_style);
    }

    PangoFontDescription *target = pango_font_description_from_string(fontspec.c_str());
    PangoFontDescription *best = nullptr;

    //font_description_dump( target );

    GList *styles = default_styles;
    if (row[FontList.onSystem] && !row[FontList.styles]) {
        row[FontList.styles] = font_factory::Default()->GetUIStyles(row[FontList.pango_family]);
        styles = row[FontList.styles];
    }

    for (GList *l = styles; l; l = l->next) {
        Glib::ustring fontspec = family + ", " + ((StyleNames *)l->data)->CssName;
        PangoFontDescription *candidate = pango_font_description_from_string(fontspec.c_str());
        //font_description_dump( candidate );
        //std::cout << "           " << font_description_better_match( target, best, candidate ) << std::endl;
        if (font_description_better_match(target, best, candidate)) {
            pango_font_description_free(best);
            best = candidate;
            //std::cout << "  ... better: " << std::endl;
        } else {
            pango_font_description_free(candidate);
            //std::cout << "  ... not better: " << std::endl;
        }
    }

    Glib::ustring best_style = target_style;
    if (best) {
        pango_font_description_unset_fields(best, PANGO_FONT_MASK_FAMILY);
        best_style = pango_font_description_to_string(best);
    }

    if (target)
        pango_font_description_free(target);
    if (best)
        pango_font_description_free(best);


#ifdef DEBUG_FONT
    std::cout << "  Returning: " << best_style << std::endl;
    std::cout << "FontLister::get_best_style_match: exit" << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
#endif
    return best_style;
}

const Glib::RefPtr<Gtk::ListStore> FontLister::get_font_list() const
{
    return font_list_store;
}

const Glib::RefPtr<Gtk::ListStore> FontLister::get_style_list() const
{
    return style_list_store;
}

} // namespace Inkscape

// Helper functions

// Separator function (if true, a separator will be drawn)
bool font_lister_separator_func(const Glib::RefPtr<Gtk::TreeModel>& model,
                                const Gtk::TreeModel::iterator& iter) {

    // Of what use is 'model', can we avoid using font_lister?
    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring entry = row[font_lister->FontList.family];
    return entry == "#";
}

// Needed until Text toolbar updated
gboolean font_lister_separator_func2(GtkTreeModel *model, GtkTreeIter *iter, gpointer /*data*/)
{
    gchar *text = nullptr;
    gtk_tree_model_get(model, iter, 0, &text, -1); // Column 0: FontList.family
    bool result = (text && strcmp(text, "#") == 0);
    g_free(text);
    return result;
}

// Draw system fonts in dark blue, missing fonts with red strikeout.
// Used by both FontSelector and Text toolbar.
void font_lister_cell_data_func (Gtk::CellRenderer *renderer, Gtk::TreeIter const &iter)
{
    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();
    Glib::ustring markup = font_lister->get_font_family_markup(iter);
    renderer->set_property("markup", markup);
}

// Needed until Text toolbar updated
void font_lister_cell_data_func2(GtkCellLayout * /*cell_layout*/,
                                GtkCellRenderer *cell,
                                GtkTreeModel *model,
                                GtkTreeIter *iter,
                                gpointer data)
{
    gchar *family;
    gboolean onSystem = false;
    gtk_tree_model_get(model, iter, 0, &family, 2, &onSystem, -1);
    gchar* family_escaped = g_markup_escape_text(family, -1);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool dark = prefs->getBool("/theme/darkTheme", false);
    Glib::ustring markup;

    if (!onSystem) {
        markup = "";
        if (dark) {
            markup += "<span foreground='powderblue'>";
        } else {
            markup += "<span foreground='darkblue'>";
        }
        /* See if font-family on system */
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s*,\\s*", family);
        for (auto token : tokens) {

            GtkTreeIter iter;
            gboolean valid;
            gboolean onSystem = true;
            gboolean found = false;
            for (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
                 valid;
                 valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter)) {

                gchar *token_family = nullptr;
                gtk_tree_model_get(model, &iter, 0, &token_family, 2, &onSystem, -1);
                if (onSystem && familyNamesAreEqual(token, token_family)) {
                    found = true;
                    g_free(token_family);
                    break;
                }
                g_free(token_family);
            }
            if (found) {
                markup += g_markup_escape_text(token.c_str(), -1);
                markup += ", ";
            } else {
                if (dark) {
                    markup += "<span strikethrough='true' strikethrough_color='salmon'>";
                } else {
                    markup += "<span strikethrough='true' strikethrough_color='red'>";
                }
                markup += g_markup_escape_text(token.c_str(), -1);
                markup += "</span>";
                markup += ", ";
            }
        }
        // Remove extra comma and space from end.
        if (markup.size() >= 2) {
            markup.resize(markup.size() - 2);
        }
        markup += "</span>";
        // std::cout << markup << std::endl;
    } else {
        markup = family_escaped;
    }

    int show_sample = prefs->getInt("/tools/text/show_sample_in_list", 1);
    if (show_sample) {

        Glib::ustring sample = prefs->getString("/tools/text/font_sample");
        gchar* sample_escaped = g_markup_escape_text(sample.data(), -1);
        if (data) {
            markup += " <span alpha='55%";
            markup += "' font_family='";
            markup += family_escaped;
        } else {
            markup += " <span alpha='1";
        }
        markup += "'>";
        markup += sample_escaped;
        markup += "</span>";
        g_free(sample_escaped);
    }

    g_object_set(G_OBJECT(cell), "markup", markup.c_str(), nullptr);
    g_free(family);
    g_free(family_escaped);
}

// Draw Face name with face style.
void font_lister_style_cell_data_func (Gtk::CellRenderer *renderer, Gtk::TreeIter const &iter)
{
    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();
    Gtk::TreeModel::Row row = *iter;

    Glib::ustring family = font_lister->get_font_family();
    Glib::ustring style  = row[font_lister->FontStyleList.cssStyle];

    Glib::ustring style_escaped  = Glib::Markup::escape_text( style );
    Glib::ustring font_desc = family + ", " + style;
    Glib::ustring markup;

    markup = "<span font='" + font_desc + "'>" + style_escaped + "</span>";
    std::cout << "  markup: " << markup << std::endl;

    renderer->set_property("markup", markup);
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
