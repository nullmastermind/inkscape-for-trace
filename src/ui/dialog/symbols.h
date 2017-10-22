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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef INKSCAPE_UI_DIALOG_SYMBOLS_H
#define INKSCAPE_UI_DIALOG_SYMBOLS_H
#include <gtkmm.h>

#include "display/drawing.h"
#include "ui/dialog/desktop-tracker.h"
#include "ui/widget/panel.h"
#include "sp-symbol.h"
#include "sp-use.h"
#include <vector>

class SPObject;

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
    virtual ~SymbolsDialog();

    static SymbolsDialog& getInstance();

private:
    SymbolsDialog(SymbolsDialog const &); // no copy
    SymbolsDialog &operator=(SymbolsDialog const &); // no assign

    static SymbolColumns *getColumns();

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
    SPDocument* selectedSymbols(bool ignorecurrent = false);
    Glib::ustring selectedSymbolId();
    Glib::ustring selectedSymbolDocTitle();
    void iconChanged();
    void iconDragDataGet(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time);
    void getSymbolsFilename();
    Glib::ustring documentTitle(SPDocument* doc);
    std::pair<Glib::ustring, SPDocument*> getSymbolsSet(Glib::ustring title);
    void addSymbol( SPObject* symbol, Glib::ustring doc_title);
    SPDocument* symbolsPreviewDoc();
    void symbolsInDocRecursive(SPObject *r, std::vector<SPSymbol*> &l);
    std::vector<SPSymbol*> symbolsInDoc( SPDocument* document);
    void useInDoc(SPObject *r, std::vector<SPUse*> &l);
    std::vector<SPUse*> useInDoc( SPDocument* document);
    void beforeAddSymbols(GdkEventKey* evt);
    void addSymbols();
    bool addSymbolsInDoc(SPDocument* document);
    void clearSearch();
    bool callbackGetSymbolsSets();
    bool callbackGetSymbols();
    
    gchar const* styleFromUse( gchar const* id, SPDocument* document);

    Glib::RefPtr<Gdk::Pixbuf> drawSymbol(SPObject *symbol);

    /* Keep track of all symbol template documents */
    std::map<Glib::ustring, SPDocument*> symbol_sets;

    // Index into sizes which is selected
    int pack_size;
    
    // Scale factor
    int scale_factor;

    bool sensitive;
    
    bool loading;
    
    bool processed;
    
    size_t number_docs;
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
    Gtk::Grid* table;
    Gtk::ScrolledWindow *scroller;
    Gtk::ToggleButton* fit_symbol;

    void setTargetDesktop(SPDesktop *desktop);
    SPDesktop*  current_desktop;
    DesktopTracker desk_track;
    SPDocument* current_document;
    SPDocument* preview_document; /* Document to render single symbol */

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
