// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Symbols dialog
 */
/* Authors:
 *   Tavmjong Bah, Martin Owens
 *
 * Copyright (C) 2012 Tavmjong Bah
 *               2013 Martin Owens
 *               2017 Jabiertxo Arraiza
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_SYMBOLS_H
#define INKSCAPE_UI_DIALOG_SYMBOLS_H

#include <vector>

#include <gtkmm.h>

#include "display/drawing.h"
#include "include/gtkmm_version.h"
#include "ui/dialog/desktop-tracker.h"
#include "ui/widget/panel.h"

class SPObject;
class SPSymbol;
class SPUse;

namespace Inkscape {
namespace UI {
namespace Dialog {

class SymbolColumns; // For Gtk::ListStore

/**
 * A dialog that displays selectable symbols and allows users to drag or paste
 * those symbols from the dialog into the document.
 *
 * Symbol documents are loaded from the preferences paths and displayed in a
 * drop-down list to the user. The user then selects which of the symbols
 * documents they want to get symbols from. The first document in the list is
 * always the current document.
 *
 * This then updates an icon-view with all the symbols available. Selecting one
 * puts it onto the clipboard. Dragging it or pasting it onto the canvas copies
 * the symbol from the symbol document, into the current document and places a
 * new <use element at the correct location on the canvas.
 *
 * Selected groups on the canvas can be added to the current document's symbols
 * table, and symbols can be removed from the current document. This allows
 * new symbols documents to be constructed and if saved in the prefs folder will
 * make those symbols available for all future documents.
 */

const int SYMBOL_ICON_SIZES[] = {16, 24, 32, 48, 64};

class SymbolsDialog : public UI::Widget::Panel {

public:
    SymbolsDialog( gchar const* prefsPath = "/dialogs/symbols" );
    ~SymbolsDialog() override;

    static SymbolsDialog& getInstance();

private:
    SymbolsDialog(SymbolsDialog const &) = delete; // no copy
    SymbolsDialog &operator=(SymbolsDialog const &) = delete; // no assign

    static SymbolColumns *getColumns();

    Glib::ustring CURRENTDOC;
    Glib::ustring ALLDOCS;

    void packless();
    void packmore();
    void zoomin();
    void zoomout();
    void rebuild();
    void insertSymbol();
    void revertSymbol();
    void defsModified(SPObject *object, guint flags);
    void selectionChanged(Inkscape::Selection *selection);
    void documentReplaced(SPDesktop *desktop, SPDocument *document);
    SPDocument* selectedSymbols();
    Glib::ustring selectedSymbolId();
    Glib::ustring selectedSymbolDocTitle();
    void iconChanged();
    void iconDragDataGet(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time);
    void getSymbolsTitle();
    Glib::ustring documentTitle(SPDocument* doc);
    std::pair<Glib::ustring, SPDocument*> getSymbolsSet(Glib::ustring title);
    void addSymbol( SPObject* symbol, Glib::ustring doc_title);
    SPDocument* symbolsPreviewDoc();
    void symbolsInDocRecursive (SPObject *r, std::map<Glib::ustring, std::pair<Glib::ustring, SPSymbol*> > &l, Glib::ustring doc_title);
    std::map<Glib::ustring, std::pair<Glib::ustring, SPSymbol*> > symbolsInDoc( SPDocument* document, Glib::ustring doc_title);
    void useInDoc(SPObject *r, std::vector<SPUse*> &l);
    std::vector<SPUse*> useInDoc( SPDocument* document);
    void beforeSearch(GdkEventKey* evt);
    void unsensitive(GdkEventKey* evt);
    void searchsymbols();
    void addSymbols();
    void addSymbolsInDoc(SPDocument* document);
    void showOverlay();
    void hideOverlay();
    void clearSearch();
    bool callbackSymbols();
    bool callbackAllSymbols();
    void enableWidgets(bool enable);
    Glib::ustring ellipsize(Glib::ustring data, size_t limit);
    gchar const* styleFromUse( gchar const* id, SPDocument* document);
    Glib::RefPtr<Gdk::Pixbuf> drawSymbol(SPObject *symbol);
    Glib::RefPtr<Gdk::Pixbuf> getOverlay(gint width, gint height);
    /* Keep track of all symbol template documents */
    std::map<Glib::ustring, SPDocument*> symbol_sets;
    std::map<Glib::ustring, std::pair<Glib::ustring, SPSymbol*> > l;
    // Index into sizes which is selected
    int pack_size;
    // Scale factor
    int scale_factor;
    bool sensitive;
    double previous_height;
    double previous_width;
    bool all_docs_processed;
    size_t number_docs;
    size_t number_symbols;
    size_t counter_symbols;
    bool icons_found;
    Glib::RefPtr<Gtk::ListStore> store;
    Glib::ustring search_str;
    Gtk::ComboBoxText* symbol_set;
    Gtk::ProgressBar* progress_bar;
    Gtk::HBox* progress;
    Gtk::SearchEntry* search;
    Gtk::IconView* icon_view;
    Gtk::Button* add_symbol;
    Gtk::Button* remove_symbol;
    Gtk::Button* zoom_in;
    Gtk::Button* zoom_out;
    Gtk::Button* more;
    Gtk::Button* fewer;
    Gtk::HBox* tools;
    Gtk::Overlay* overlay;
    Gtk::Image* overlay_icon;
    Gtk::Image* overlay_opacity;
    Gtk::Label* overlay_title;
    Gtk::Label* overlay_desc;
    Gtk::ScrolledWindow *scroller;
    Gtk::ToggleButton* fit_symbol;
    Gtk::IconSize iconsize;
    void setTargetDesktop(SPDesktop *desktop);
    SPDesktop*  current_desktop;
    DesktopTracker desk_track;
    SPDocument* current_document;
    SPDocument* preview_document; /* Document to render single symbol */
    
    sigc::connection idleconn;
    
    /* For rendering the template drawing */
    unsigned key;
    Inkscape::Drawing renderDrawing;

    std::vector<sigc::connection> instanceConns;
};

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape


#endif // INKSCAPE_UI_DIALOG_SYMBOLS_H

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
