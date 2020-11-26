// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Spellcheck dialog.
 */
/* Authors:
 *   bulia byak <bulia@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2009 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "spellcheck.h"

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "message-stack.h"
#include "selection-chemistry.h"
#include "text-editing.h"
#include "verbs.h"

#include "display/control/canvas-item-rect.h"

#include "object/sp-defs.h"
#include "object/sp-flowtext.h"
#include "object/sp-object.h"
#include "object/sp-root.h"
#include "object/sp-string.h"
#include "object/sp-text.h"
#include "object/sp-tref.h"

#include "ui/dialog/dialog-container.h"
#include "ui/dialog/inkscape-preferences.h" // for PREFS_PAGE_SPELLCHECK
#include "ui/tools-switch.h"
#include "ui/tools/text-tool.h"

#include <glibmm/i18n.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * Get the list of installed dictionaries/languages
 */
std::vector<LanguagePair> SpellCheck::get_available_langs()
{
    std::vector<LanguagePair> langs;

#if WITH_GSPELL
    // TODO: write a gspellmm library.
    // TODO: why is this not const?
    GList *list = const_cast<GList *>(gspell_language_get_available());
    g_list_foreach(list, [](gpointer data, gpointer user_data) {
        GspellLanguage *language = reinterpret_cast<GspellLanguage*>(data);
        std::vector<LanguagePair> *langs = reinterpret_cast<std::vector<LanguagePair>*>(user_data);
        const gchar *name = gspell_language_get_name(language);
        const gchar *code = gspell_language_get_code(language);
        langs->emplace_back(name, code);
    }, &langs);
#endif

    return langs;
}

static void show_spellcheck_preferences_dialog()
{
    Inkscape::Preferences::get()->setInt("/dialogs/preferences/page", PREFS_PAGE_SPELLCHECK);
    SP_ACTIVE_DESKTOP->getContainer()->new_dialog(SP_VERB_DIALOG_PREFERENCES);
}

SpellCheck::SpellCheck()
    : DialogBase("/dialogs/spellcheck/", SP_VERB_DIALOG_SPELLCHECK)
    , _text(nullptr)
    , _layout(nullptr)
    , _stops(0)
    , _adds(0)
    , _working(false)
    , _local_change(false)
    , _prefs(nullptr)
    , accept_button(_("_Accept"), true)
    , ignoreonce_button(_("_Ignore once"), true)
    , ignore_button(_("_Ignore"), true)
    , add_button(_("A_dd"), true)
    , dictionary_label(_("Language"))
    , dictionary_hbox(Gtk::ORIENTATION_HORIZONTAL, 0)
    , stop_button(_("_Stop"), true)
    , start_button(_("_Start"), true)
    , desktop(nullptr)
    , suggestion_hbox(Gtk::ORIENTATION_HORIZONTAL)
    , changebutton_vbox(Gtk::ORIENTATION_VERTICAL)
{
    _prefs = Inkscape::Preferences::get();

    banner_hbox.set_layout(Gtk::BUTTONBOX_START);
    banner_hbox.add(banner_label);

    if (_langs.empty()) {
        _langs = get_available_langs();

        if (_langs.empty()) {
            banner_label.set_markup("<i>No dictionaries installed</i>");
        }
    }

    scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrolled_window.set_shadow_type(Gtk::SHADOW_IN);
    scrolled_window.set_size_request(120, 96);
    scrolled_window.add(tree_view);

    model = Gtk::ListStore::create(tree_columns);
    tree_view.set_model(model);
    tree_view.append_column(_("Suggestions:"), tree_columns.suggestions);

    if (!_langs.empty()) {
        for (const LanguagePair &pair : _langs) {
            dictionary_combo.append(pair.second, pair.first);
        }
        // Set previously set language (or the first item)
        if(!dictionary_combo.set_active_id(_prefs->getString("/dialogs/spellcheck/lang"))) {
            dictionary_combo.set_active(0);
        }
    }

    accept_button.set_tooltip_text(_("Accept the chosen suggestion"));
    ignoreonce_button.set_tooltip_text(_("Ignore this word only once"));
    ignore_button.set_tooltip_text(_("Ignore this word in this session"));
    add_button.set_tooltip_text(_("Add this word to the chosen dictionary"));
    pref_button.set_tooltip_text(_("Preferences"));
    pref_button.set_image_from_icon_name("preferences-system");

    dictionary_hbox.pack_start(dictionary_label, false, false, 6);
    dictionary_hbox.pack_start(dictionary_combo, true, true, 0);
    dictionary_hbox.pack_start(pref_button, false, false, 0);

    changebutton_vbox.set_spacing(4);
    changebutton_vbox.pack_start(accept_button, false, false, 0);
    changebutton_vbox.pack_start(ignoreonce_button, false, false, 0);
    changebutton_vbox.pack_start(ignore_button, false, false, 0);
    changebutton_vbox.pack_start(add_button, false, false, 0);

    suggestion_hbox.pack_start (scrolled_window, true, true, 4);
    suggestion_hbox.pack_end (changebutton_vbox, false, false, 0);

    stop_button.set_tooltip_text(_("Stop the check"));
    start_button.set_tooltip_text(_("Start the check"));

    actionbutton_hbox.set_layout(Gtk::BUTTONBOX_END);
    actionbutton_hbox.set_spacing(4);
    actionbutton_hbox.add(stop_button);
    actionbutton_hbox.add(start_button);

    /*
     * Main dialog
     */
    set_spacing(6);
    pack_start (banner_hbox, false, false, 0);
    pack_start (suggestion_hbox, true, true, 0);
    pack_start (dictionary_hbox, false, false, 0);
    pack_start (action_sep, false, false, 6);
    pack_start (actionbutton_hbox, false, false, 0);

    /*
     * Signal handlers
     */
    accept_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onAccept));
    ignoreonce_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onIgnoreOnce));
    ignore_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onIgnore));
    add_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onAdd));
    start_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onStart));
    stop_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onStop));
    tree_view.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &SpellCheck::onTreeSelectionChange));
    dictionary_combo.signal_changed().connect(sigc::mem_fun(*this, &SpellCheck::onLanguageChanged));
    pref_button.signal_clicked().connect(sigc::ptr_fun(show_spellcheck_preferences_dialog));

    show_all_children ();

    tree_view.set_sensitive(false);
    accept_button.set_sensitive(false);
    ignore_button.set_sensitive(false);
    ignoreonce_button.set_sensitive(false);
    add_button.set_sensitive(false);
    stop_button.set_sensitive(false);
}

SpellCheck::~SpellCheck()
{
    clearRects();
    disconnect();
}

void SpellCheck::update()
{
    if (!_app) {
        std::cerr << "SpellCheck::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();

    if (this->desktop == desktop) {
        return;
    }

    this->desktop = desktop;

    if (desktop) {
        if (_working) {
            // Stop and start on the new desktop
            finished();
            onStart();
        }
    }
}

void SpellCheck::clearRects()
{
    for(auto rect : _rects) {
        rect->hide();
        delete rect;
    }
    _rects.clear();
}

void SpellCheck::disconnect()
{
    if (_release_connection) {
        _release_connection.disconnect();
    }
    if (_modified_connection) {
        _modified_connection.disconnect();
    }
}

void SpellCheck::allTextItems (SPObject *r, std::vector<SPItem *> &l, bool hidden, bool locked)
{
    if (!desktop)
        return; // no desktop to check

    if (SP_IS_DEFS(r))
        return; // we're not interested in items in defs

    if (!strcmp(r->getRepr()->name(), "svg:metadata")) {
        return; // we're not interested in metadata
    }

    for (auto& child: r->children) {
        if (SP_IS_ITEM (&child) && !child.cloned && !desktop->isLayer(SP_ITEM(&child))) {
                if ((hidden || !desktop->itemIsHidden(SP_ITEM(&child))) && (locked || !SP_ITEM(&child)->isLocked())) {
                    if (SP_IS_TEXT(&child) || SP_IS_FLOWTEXT(&child))
                        l.push_back(static_cast<SPItem*>(&child));
                }
        }
        allTextItems (&child, l, hidden, locked);
    }
    return;
}

bool
SpellCheck::textIsValid (SPObject *root, SPItem *text)
{
    std::vector<SPItem*> l;
    allTextItems (root, l, false, true);
    return (std::find(l.begin(), l.end(), text) != l.end());
}

bool SpellCheck::compareTextBboxes (gconstpointer a, gconstpointer b)//returns a<b
{
    SPItem *i1 = SP_ITEM(a);
    SPItem *i2 = SP_ITEM(b);

    Geom::OptRect bbox1 = i1->documentVisualBounds();
    Geom::OptRect bbox2 = i2->documentVisualBounds();
    if (!bbox1 || !bbox2) {
        return false;
    }

    // vector between top left corners
    Geom::Point diff = bbox1->min() - bbox2->min();

    return diff[Geom::Y] == 0 ? (diff[Geom::X] < 0) : (diff[Geom::Y] < 0);
}

// We regenerate and resort the list every time, because user could have changed it while the
// dialog was waiting
SPItem *SpellCheck::getText (SPObject *root)
{
    std::vector<SPItem*> l;
    allTextItems (root, l, false, true);
    std::sort(l.begin(),l.end(),SpellCheck::compareTextBboxes);

    for (auto item:l) {
        if(_seen_objects.insert(item).second)
            return item;
    }
    return nullptr;
}

void
SpellCheck::nextText()
{
    disconnect();

    _text = getText(_root);
    if (_text) {

        _modified_connection = (SP_OBJECT(_text))->connectModified(sigc::mem_fun(*this, &SpellCheck::onObjModified));
        _release_connection = (SP_OBJECT(_text))->connectRelease(sigc::mem_fun(*this, &SpellCheck::onObjReleased));

        _layout = te_get_layout (_text);
        _begin_w = _layout->begin();
    }
    _end_w = _begin_w;
    _word.clear();
}

void SpellCheck::deleteSpeller() {
}

bool SpellCheck::updateSpeller() {
#if WITH_GSPELL
    auto lang = dictionary_combo.get_active_id();
    if (!lang.empty()) {
        const GspellLanguage *language = gspell_language_lookup(lang.c_str());
        _checker = gspell_checker_new(language);
    }

    return _checker != nullptr;
#else
    return false;
#endif
}

void SpellCheck::onStart()
{
    if (!desktop)
        return;

    start_button.set_sensitive(false);

    _stops = 0;
    _adds = 0;
    clearRects();

    if (!updateSpeller())
        return;

    _root = desktop->getDocument()->getRoot();

    // empty the list of objects we've checked
    _seen_objects.clear();

    // grab first text
    nextText();

    _working = true;

    doSpellcheck();
}

void
SpellCheck::finished ()
{
    deleteSpeller();

    clearRects();
    disconnect();

    tree_view.unset_model();
    tree_view.set_sensitive(false);
    accept_button.set_sensitive(false);
    ignore_button.set_sensitive(false);
    ignoreonce_button.set_sensitive(false);
    add_button.set_sensitive(false);
    stop_button.set_sensitive(false);
    start_button.set_sensitive(true);

    {
        gchar *label;
        if (_stops)
            label = g_strdup_printf(_("<b>Finished</b>, <b>%d</b> words added to dictionary"), _adds);
        else
            label = g_strdup_printf("%s", _("<b>Finished</b>, nothing suspicious found"));
        banner_label.set_markup(label);
        g_free(label);
    }

    _seen_objects.clear();

    _root = nullptr;

    _working = false;
}

bool
SpellCheck::nextWord()
{
    if (!_working)
        return false;

    if (!_text) {
        finished();
        return false;
    }
    _word.clear();

    while (_word.size() == 0) {
        _begin_w = _end_w;

        if (!_layout || _begin_w == _layout->end()) {
            nextText();
            return false;
        }

        if (!_layout->isStartOfWord(_begin_w)) {
            _begin_w.nextStartOfWord();
        }

        _end_w = _begin_w;
        _end_w.nextEndOfWord();
        _word = sp_te_get_string_multiline (_text, _begin_w, _end_w);
    }

    // try to link this word with the next if separated by '
    SPObject *char_item = nullptr;
    Glib::ustring::iterator text_iter;
    _layout->getSourceOfCharacter(_end_w, &char_item, &text_iter);
    if (SP_IS_STRING(char_item)) {
        int this_char = *text_iter;
        if (this_char == '\'' || this_char == 0x2019) {
            Inkscape::Text::Layout::iterator end_t = _end_w;
            end_t.nextCharacter();
            _layout->getSourceOfCharacter(end_t, &char_item, &text_iter);
            if (SP_IS_STRING(char_item)) {
                int this_char = *text_iter;
                if (g_ascii_isalpha(this_char)) { // 's
                    _end_w.nextEndOfWord();
                    _word = sp_te_get_string_multiline (_text, _begin_w, _end_w);
                }
            }
        }
    }

    // skip words containing digits
    if (_prefs->getInt(_prefs_path + "ignorenumbers") != 0) {
        bool digits = false;
        for (unsigned int i : _word) {
            if (g_unichar_isdigit(i)) {
               digits = true;
               break;
            }
        }
        if (digits) {
            return false;
        }
    }

    // skip ALL-CAPS words
    if (_prefs->getInt(_prefs_path + "ignoreallcaps") != 0) {
        bool allcaps = true;
        for (unsigned int i : _word) {
            if (!g_unichar_isupper(i)) {
               allcaps = false;
               break;
            }
        }
        if (allcaps) {
            return false;
        }
    }

    int have = 0;

#if WITH_GSPELL
    if (_checker) {
        GError *error = nullptr;
        have += gspell_checker_check_word(_checker, _word.c_str(), -1, &error);
    }
#endif  /* WITH_GSPELL */

    if (have == 0) { // not found in any!
        _stops ++;

        // display it in window
        {
            gchar *label = g_strdup_printf(_("Not in dictionary: <b>%s</b>"), _word.c_str());
            banner_label.set_markup(label);
            g_free(label);
        }

        tree_view.set_sensitive(true);
        ignore_button.set_sensitive(true);
        ignoreonce_button.set_sensitive(true);
        add_button.set_sensitive(true);
        stop_button.set_sensitive(true);

        // draw rect
        std::vector<Geom::Point> points =
            _layout->createSelectionShape(_begin_w, _end_w, _text->i2dt_affine());
        if (points.size() >= 4) { // We may not have a single quad if this is a clipped part of text on path;
                                  // in that case skip drawing the rect
            Geom::Point tl, br;
            tl = br = points.front();
            for (auto & point : points) {
                if (point[Geom::X] < tl[Geom::X])
                    tl[Geom::X] = point[Geom::X];
                if (point[Geom::Y] < tl[Geom::Y])
                    tl[Geom::Y] = point[Geom::Y];
                if (point[Geom::X] > br[Geom::X])
                    br[Geom::X] = point[Geom::X];
                if (point[Geom::Y] > br[Geom::Y])
                    br[Geom::Y] = point[Geom::Y];
            }

            // expand slightly
            Geom::Rect area = Geom::Rect(tl, br);
            double mindim = fabs(tl[Geom::Y] - br[Geom::Y]);
            if (fabs(tl[Geom::X] - br[Geom::X]) < mindim)
                mindim = fabs(tl[Geom::X] - br[Geom::X]);
            area.expandBy(MAX(0.05 * mindim, 1));

            // Create canvas item rect with red stroke. (TODO: a quad could allow non-axis aligned rects.)
            auto rect = new Inkscape::CanvasItemRect(desktop->getCanvasSketch(), area);
            rect->set_stroke(0xff0000ff);
            rect->show();
            _rects.push_back(rect);

            // scroll to make it all visible
            Geom::Point const center = desktop->current_center();
            area.expandBy(0.5 * mindim);
            Geom::Point scrollto;
            double dist = 0;
            for (unsigned corner = 0; corner < 4; corner ++) {
                if (Geom::L2(area.corner(corner) - center) > dist) {
                    dist = Geom::L2(area.corner(corner) - center);
                    scrollto = area.corner(corner);
                }
            }
            desktop->scroll_to_point (scrollto, 1.0);
        }

        // select text; if in Text tool, position cursor to the beginning of word
        // unless it is already in the word
        if (desktop->selection->singleItem() != _text) {
            desktop->selection->set (_text);
        }

        if (dynamic_cast<Inkscape::UI::Tools::TextTool *>(desktop->event_context)) {
            Inkscape::Text::Layout::iterator *cursor =
                sp_text_context_get_cursor_position(SP_TEXT_CONTEXT(desktop->event_context), _text);
            if (!cursor) // some other text is selected there
                desktop->selection->set (_text);
            else if (*cursor <= _begin_w || *cursor >= _end_w)
                sp_text_context_place_cursor (SP_TEXT_CONTEXT(desktop->event_context), _text, _begin_w);
        }

#if WITH_GSPELL

        // get suggestions
        model = Gtk::ListStore::create(tree_columns);
        tree_view.set_model(model);
        unsigned n_sugg = 0;

        if (_checker) {
            GSList *list = gspell_checker_get_suggestions(_checker, _word.c_str(), -1);
            std::vector<std::string> suggs;

            // TODO: use a better API for that, or figure out how to make gspellmm.
            g_slist_foreach(list, [](gpointer data, gpointer user_data) {
                const gchar *suggestion = reinterpret_cast<const gchar*>(data);
                std::vector<std::string> *suggs = reinterpret_cast<std::vector<std::string>*>(user_data);
                suggs->push_back(suggestion);
            }, &suggs);
            g_slist_free_full(list, g_free);

            Gtk::TreeModel::iterator iter;
            for (std::string sugg : suggs) {
                iter = model->append();
                Gtk::TreeModel::Row row = *iter;
                row[tree_columns.suggestions] = sugg;

                // select first suggestion
                if (++n_sugg == 1) {
                    tree_view.get_selection()->select(iter);
                }
            }
        }

        accept_button.set_sensitive(n_sugg > 0);

#endif  /* WITH_GSPELL */

        return true;

    }
    return false;
}



void
SpellCheck::deleteLastRect ()
{
    if (!_rects.empty()) {
        _rects.back()->hide();
        delete _rects.back();
        _rects.pop_back();
    }
}

void SpellCheck::doSpellcheck ()
{
    if (_langs.empty()) {
        return;
    }

    banner_label.set_markup(_("<i>Checking...</i>"));

    while (_working)
        if (nextWord())
            break;
}

void SpellCheck::onTreeSelectionChange()
{
    accept_button.set_sensitive(true);
}

void SpellCheck::onObjModified (SPObject* /* blah */, unsigned int /* bleh */)
{
    if (_local_change) { // this was a change by this dialog, i.e. an Accept, skip it
        _local_change = false;
        return;
    }

    if (_working && _root) {
        // user may have edited the text we're checking; try to do the most sensible thing in this
        // situation

        // just in case, re-get text's layout
        _layout = te_get_layout (_text);

        // re-get the word
        _layout->validateIterator(&_begin_w);
        _end_w = _begin_w;
        _end_w.nextEndOfWord();
        Glib::ustring word_new = sp_te_get_string_multiline (_text, _begin_w, _end_w);
        if (word_new != _word) {
            _end_w = _begin_w;
            deleteLastRect ();
            doSpellcheck (); // recheck this word and go ahead if it's ok
        }
    }
}

void SpellCheck::onObjReleased (SPObject* /* blah */)
{
    if (_working && _root) {
        // the text object was deleted
        deleteLastRect ();
        nextText();
        doSpellcheck (); // get next text and continue
    }
}

void SpellCheck::onAccept ()
{
    // insert chosen suggestion

    Glib::RefPtr<Gtk::TreeSelection> selection = tree_view.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        Glib::ustring sugg = row[tree_columns.suggestions];

        if (sugg.length() > 0) {
            //g_print("chosen: %s\n", sugg);
            _local_change = true;
            sp_te_replace(_text, _begin_w, _end_w, sugg.c_str());
            // find the end of the word anew
            _end_w = _begin_w;
            _end_w.nextEndOfWord();
            DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_TEXT,
                               _("Fix spelling"));
        }
    }

    deleteLastRect();
    doSpellcheck();
}

void
SpellCheck::onIgnore ()
{
#if WITH_GSPELL
    if (_checker) {
        gspell_checker_add_word_to_session(_checker, _word.c_str(), -1);
    }
#endif  /* WITH_GSPELL */

    deleteLastRect();
    doSpellcheck();
}

void
SpellCheck::onIgnoreOnce ()
{
    deleteLastRect();
    doSpellcheck();
}

void
SpellCheck::onAdd ()
{
    _adds++;

#if WITH_GSPELL
    if (_checker) {
        gspell_checker_add_word_to_personal(_checker, _word.c_str(), -1);
    }
#endif  /* WITH_GSPELL */

    deleteLastRect();
    doSpellcheck();
}

void
SpellCheck::onStop ()
{
    finished();
}

void SpellCheck::onLanguageChanged()
{
    // First, save language for next load
    auto lang = dictionary_combo.get_active_id();
    _prefs->setString("/dialogs/spellcheck/lang", lang);

    if (!_working) {
        onStart();
        return;
    }

    if (!updateSpeller()) {
        return;
    }

    // recheck current word
    _end_w = _begin_w;
    deleteLastRect();
    doSpellcheck();
}
}
}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
