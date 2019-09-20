// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Symbols dialog.
 */
/* Authors:
 * Copyright (C) 2012 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <iostream>
#include <algorithm>
#include <locale>
#include <sstream>
#include <fstream>
#include <regex>

#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <glibmm/regex.h>
#include <glibmm/stringutils.h>

#include "desktop.h"
#include "document.h"
#include "inkscape.h"
#include "path-prefix.h"
#include "selection.h"
#include "symbols.h"
#include "verbs.h"

#include "display/cairo-utils.h"
#include "helper/action.h"
#include "include/gtkmm_version.h"
#include "io/resource.h"
#include "io/sys.h"
#include "object/sp-defs.h"
#include "object/sp-root.h"
#include "object/sp-symbol.h"
#include "object/sp-use.h"
#include "ui/cache/svg_preview_cache.h"
#include "ui/clipboard.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"

#ifdef WITH_LIBVISIO
  #include <libvisio/libvisio.h>

  // TODO: Drop this check when librevenge is widespread.
  #if WITH_LIBVISIO01
    #include <librevenge-stream/librevenge-stream.h>

    using librevenge::RVNGFileStream;
    using librevenge::RVNGString;
    using librevenge::RVNGStringVector;
    using librevenge::RVNGPropertyList;
    using librevenge::RVNGSVGDrawingGenerator;
  #else
    #include <libwpd-stream/libwpd-stream.h>

    typedef WPXFileStream             RVNGFileStream;
    typedef libvisio::VSDStringVector RVNGStringVector;
  #endif
#endif


namespace Inkscape {
namespace UI {

namespace Dialog {

// See: http://developer.gnome.org/gtkmm/stable/classGtk_1_1TreeModelColumnRecord.html
class SymbolColumns : public Gtk::TreeModel::ColumnRecord
{
public:

  Gtk::TreeModelColumn<Glib::ustring>                symbol_id;
  Gtk::TreeModelColumn<Glib::ustring>                symbol_title;
  Gtk::TreeModelColumn<Glib::ustring>                symbol_doc_title;
  Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >  symbol_image;
  

  SymbolColumns() {
    add(symbol_id);
    add(symbol_title);
    add(symbol_doc_title);
    add(symbol_image);
  }
};

SymbolColumns* SymbolsDialog::getColumns()
{
  SymbolColumns* columns = new SymbolColumns();
  return columns;
}

/**
 * Constructor
 */
SymbolsDialog::SymbolsDialog( gchar const* prefsPath ) :
  UI::Widget::Panel(prefsPath, SP_VERB_DIALOG_SYMBOLS),
  store(Gtk::ListStore::create(*getColumns())),
  all_docs_processed(false),
  icon_view(nullptr),
  current_desktop(nullptr),
  desk_track(),
  current_document(nullptr),
  preview_document(nullptr),
  instanceConns(),
  CURRENTDOC(_("Current document")),
  ALLDOCS(_("All symbol sets"))
{

    /********************    Table    *************************/
  auto table = new Gtk::Grid();

  table->set_margin_start(3);
  table->set_margin_end(3);
  table->set_margin_top(4);
  // panel is a locked Gtk::VBox
  _getContents()->pack_start(*Gtk::manage(table), Gtk::PACK_EXPAND_WIDGET);
  guint row = 0;

  /******************** Symbol Sets *************************/
  Gtk::Label* label_set = new Gtk::Label(Glib::ustring(_("Symbol set")) + ": ");
  table->attach(*Gtk::manage(label_set),0,row,1,1);
  symbol_set = new Gtk::ComboBoxText();  // Fill in later
  symbol_set->append(CURRENTDOC);
  symbol_set->append(ALLDOCS);
  symbol_set->set_active_text(CURRENTDOC);
  symbol_set->set_hexpand();
  
  table->attach(*Gtk::manage(symbol_set),1,row,1,1);
  sigc::connection connSet = symbol_set->signal_changed().connect(
          sigc::mem_fun(*this, &SymbolsDialog::rebuild));
  instanceConns.push_back(connSet);

  ++row;
  
  /********************    Separator    *************************/
  

  Gtk::Separator* separator = Gtk::manage(new Gtk::Separator());  // Search
  separator->set_margin_top(10);
  separator->set_margin_bottom(10);
  table->attach(*Gtk::manage(separator),0,row,2,1);

  ++row;

  /********************    Search    *************************/
  

  search = Gtk::manage(new Gtk::SearchEntry());  // Search
  search->set_tooltip_text(_("Return to start search."));
  search->signal_key_press_event().connect_notify(  sigc::mem_fun(*this, &SymbolsDialog::beforeSearch));
  search->signal_key_release_event().connect_notify(sigc::mem_fun(*this, &SymbolsDialog::unsensitive));

  search->set_margin_bottom(6);
  search->signal_search_changed().connect(sigc::mem_fun(*this, &SymbolsDialog::clearSearch));
  table->attach(*Gtk::manage(search),0,row,2,1);
  search_str = "";

  ++row;


  /********************* Icon View **************************/
  SymbolColumns* columns = getColumns();

  icon_view = new Gtk::IconView(static_cast<Glib::RefPtr<Gtk::TreeModel> >(store));
  //icon_view->set_text_column(  columns->symbol_id  );
  icon_view->set_tooltip_column( 1 );
  icon_view->set_pixbuf_column( columns->symbol_image );
  // Giving the iconview a small minimum size will help users understand
  // What the dialog does.
  icon_view->set_size_request( 100, 250 );

  std::vector< Gtk::TargetEntry > targets;
  targets.emplace_back( "application/x-inkscape-paste");

  icon_view->enable_model_drag_source (targets, Gdk::BUTTON1_MASK, Gdk::ACTION_COPY);
  icon_view->signal_drag_data_get().connect(
          sigc::mem_fun(*this, &SymbolsDialog::iconDragDataGet));

  sigc::connection connIconChanged;
  connIconChanged = icon_view->signal_selection_changed().connect(
          sigc::mem_fun(*this, &SymbolsDialog::iconChanged));
  instanceConns.push_back(connIconChanged);

  scroller = new Gtk::ScrolledWindow();
  scroller->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
  scroller->add(*Gtk::manage(icon_view));
  scroller->set_hexpand();
  scroller->set_vexpand();

  overlay = new Gtk::Overlay();
  overlay->set_hexpand();
  overlay->set_vexpand();
  overlay->add(* scroller);
  overlay->get_style_context()->add_class("forcebright");
  scroller->set_size_request(100, 250);
  table->attach(*Gtk::manage(overlay), 0, row, 2, 1);

  /*************************Overlays******************************/
  overlay_opacity = new Gtk::Image();
  overlay_opacity->set_halign(Gtk::ALIGN_START);
  overlay_opacity->set_valign(Gtk::ALIGN_START);
  overlay_opacity->get_style_context()->add_class("rawstyle");

  // No results
  overlay_icon = sp_get_icon_image("searching", Gtk::ICON_SIZE_DIALOG);
  overlay_icon->set_pixel_size(110);
  overlay_icon->set_halign(Gtk::ALIGN_CENTER);
  overlay_icon->set_valign(Gtk::ALIGN_START);

  overlay_icon->set_margin_top(45);

  overlay_title = new Gtk::Label();
  overlay_title->set_halign(Gtk::ALIGN_CENTER );
  overlay_title->set_valign(Gtk::ALIGN_START );
  overlay_title->set_justify(Gtk::JUSTIFY_CENTER);
  overlay_title->set_margin_top(155);

  overlay_desc = new Gtk::Label();
  overlay_desc->set_halign(Gtk::ALIGN_CENTER);
  overlay_desc->set_valign(Gtk::ALIGN_START);
  overlay_desc->set_margin_top(180);
  overlay_desc->set_justify(Gtk::JUSTIFY_CENTER);

  overlay->add_overlay(*overlay_opacity);
  overlay->add_overlay(*overlay_icon);
  overlay->add_overlay(*overlay_title);
  overlay->add_overlay(*overlay_desc);

  previous_height = 0;
  previous_width = 0;
  ++row;

  /******************** Progress *******************************/
  progress = new Gtk::HBox();
  progress_bar = Gtk::manage(new Gtk::ProgressBar()); 
  table->attach(*Gtk::manage(progress),0,row, 2, 1);
  progress->pack_start(* progress_bar, Gtk::PACK_EXPAND_WIDGET);
  progress->set_margin_top(15);
  progress->set_margin_bottom(15);
  progress->set_margin_start(20);
  progress->set_margin_end(20);

  ++row;

  /******************** Tools *******************************/
  tools = new Gtk::HBox();

  //tools->set_layout( Gtk::BUTTONBOX_END );
  scroller->set_hexpand();
  table->attach(*Gtk::manage(tools),0,row,2,1);

  auto add_symbol_image = Gtk::manage(sp_get_icon_image("symbol-add", Gtk::ICON_SIZE_SMALL_TOOLBAR));

  add_symbol = Gtk::manage(new Gtk::Button());
  add_symbol->add(*add_symbol_image);
  add_symbol->set_tooltip_text(_("Add Symbol from the current document."));
  add_symbol->set_relief( Gtk::RELIEF_NONE );
  add_symbol->set_focus_on_click( false );
  add_symbol->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::insertSymbol));
  tools->pack_start(* add_symbol, Gtk::PACK_SHRINK);

  auto remove_symbolImage = Gtk::manage(sp_get_icon_image("symbol-remove", Gtk::ICON_SIZE_SMALL_TOOLBAR));

  remove_symbol = Gtk::manage(new Gtk::Button());
  remove_symbol->add(*remove_symbolImage);
  remove_symbol->set_tooltip_text(_("Remove Symbol from the current document."));
  remove_symbol->set_relief( Gtk::RELIEF_NONE );
  remove_symbol->set_focus_on_click( false );
  remove_symbol->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::revertSymbol));
  tools->pack_start(* remove_symbol, Gtk::PACK_SHRINK);

  Gtk::Label* spacer = Gtk::manage(new Gtk::Label(""));
  tools->pack_start(* Gtk::manage(spacer));

  // Pack size (controls display area)
  pack_size = 2; // Default 32px

  auto packMoreImage = Gtk::manage(sp_get_icon_image("pack-more", Gtk::ICON_SIZE_SMALL_TOOLBAR));

  more = Gtk::manage(new Gtk::Button());
  more->add(*packMoreImage);
  more->set_tooltip_text(_("Display more icons in row."));
  more->set_relief( Gtk::RELIEF_NONE );
  more->set_focus_on_click( false );
  more->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::packmore));
  tools->pack_start(* more, Gtk::PACK_SHRINK);

  auto packLessImage = Gtk::manage(sp_get_icon_image("pack-less", Gtk::ICON_SIZE_SMALL_TOOLBAR));

  fewer = Gtk::manage(new Gtk::Button());
  fewer->add(*packLessImage);
  fewer->set_tooltip_text(_("Display fewer icons in row."));
  fewer->set_relief( Gtk::RELIEF_NONE );
  fewer->set_focus_on_click( false );
  fewer->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::packless));
  tools->pack_start(* fewer, Gtk::PACK_SHRINK);

  // Toggle scale to fit on/off
  auto fit_symbolImage = Gtk::manage(sp_get_icon_image("symbol-fit", Gtk::ICON_SIZE_SMALL_TOOLBAR));

  fit_symbol = Gtk::manage(new Gtk::ToggleButton());
  fit_symbol->add(*fit_symbolImage);
  fit_symbol->set_tooltip_text(_("Toggle 'fit' symbols in icon space."));
  fit_symbol->set_relief( Gtk::RELIEF_NONE );
  fit_symbol->set_focus_on_click( false );
  fit_symbol->set_active( true );
  fit_symbol->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::rebuild));
  tools->pack_start(* fit_symbol, Gtk::PACK_SHRINK);

  // Render size (scales symbols within display area)
  scale_factor = 0; // Default 1:1 * pack_size/pack_size default
  auto zoom_outImage = Gtk::manage(sp_get_icon_image("symbol-smaller", Gtk::ICON_SIZE_SMALL_TOOLBAR));

  zoom_out = Gtk::manage(new Gtk::Button());
  zoom_out->add(*zoom_outImage);
  zoom_out->set_tooltip_text(_("Make symbols smaller by zooming out."));
  zoom_out->set_relief( Gtk::RELIEF_NONE );
  zoom_out->set_focus_on_click( false );
  zoom_out->set_sensitive( false );
  zoom_out->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::zoomout));
  tools->pack_start(* zoom_out, Gtk::PACK_SHRINK);

  auto zoom_inImage = Gtk::manage(sp_get_icon_image("symbol-bigger", Gtk::ICON_SIZE_SMALL_TOOLBAR));

  zoom_in = Gtk::manage(new Gtk::Button());
  zoom_in->add(*zoom_inImage);
  zoom_in->set_tooltip_text(_("Make symbols bigger by zooming in."));
  zoom_in->set_relief( Gtk::RELIEF_NONE );
  zoom_in->set_focus_on_click( false );
  zoom_in->set_sensitive( false );
  zoom_in->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::zoomin));
  tools->pack_start(* zoom_in, Gtk::PACK_SHRINK);

  ++row;

  sensitive = true;

  current_desktop  = SP_ACTIVE_DESKTOP;
  current_document = current_desktop->getDocument();
  preview_document = symbolsPreviewDoc(); /* Template to render symbols in */
  preview_document->ensureUpToDate(); /* Necessary? */
  key = SPItem::display_key_new(1);
  renderDrawing.setRoot(preview_document->getRoot()->invoke_show(renderDrawing, key, SP_ITEM_SHOW_DISPLAY ));

  // This might need to be a global variable so setTargetDesktop can modify it
  SPDefs *defs = current_document->getDefs();
  sigc::connection defsModifiedConn = defs->connectModified(sigc::mem_fun(*this, &SymbolsDialog::defsModified));
  instanceConns.push_back(defsModifiedConn);

  sigc::connection selectionChangedConn = current_desktop->selection->connectChanged(
          sigc::mem_fun(*this, &SymbolsDialog::selectionChanged));
  instanceConns.push_back(selectionChangedConn);

  sigc::connection documentReplacedConn = current_desktop->connectDocumentReplaced(
          sigc::mem_fun(*this, &SymbolsDialog::documentReplaced));
  instanceConns.push_back(documentReplacedConn);
  getSymbolsTitle();
  icons_found = false;
  
  addSymbolsInDoc(current_document); /* Defaults to current document */
  sigc::connection desktopChangeConn =
    desk_track.connectDesktopChanged( sigc::mem_fun(*this, &SymbolsDialog::setTargetDesktop) );
  instanceConns.push_back( desktopChangeConn );
  desk_track.connect(GTK_WIDGET(gobj()));
}

SymbolsDialog::~SymbolsDialog()
{
  for (auto & instanceConn : instanceConns) {
      instanceConn.disconnect();
  }
  idleconn.disconnect();
  instanceConns.clear();
  desk_track.disconnect();
}

SymbolsDialog& SymbolsDialog::getInstance()
{
  return *new SymbolsDialog();
}

void SymbolsDialog::packless() {
  if(pack_size < 4) {
      pack_size++;
      rebuild();
  }
}

void SymbolsDialog::packmore() {
  if(pack_size > 0) {
      pack_size--;
      rebuild();
  }
}

void SymbolsDialog::zoomin() {
  if(scale_factor < 4) {
      scale_factor++;
      rebuild();
  }
}

void SymbolsDialog::zoomout() {
  if(scale_factor > -8) {
      scale_factor--;
      rebuild();
  }
}

void SymbolsDialog::rebuild() {

  if (!sensitive) {
    return;
  }

  if( fit_symbol->get_active() ) {
    zoom_in->set_sensitive( false );
    zoom_out->set_sensitive( false );
  } else {
    zoom_in->set_sensitive( true);
    zoom_out->set_sensitive( true );
  }
  store->clear();
  SPDocument* symbol_document = selectedSymbols();
  icons_found = false;
  //We are not in search all docs
  if (search->get_text() != _("Searching...") && search->get_text() != _("Loading all symbols...")) {
      Glib::ustring current = Glib::Markup::escape_text(symbol_set->get_active_text());
      if (current == ALLDOCS && search->get_text() != "") {
          searchsymbols();
          return;
      }
  }
  if (symbol_document) {
    addSymbolsInDoc(symbol_document);
  } else {
    showOverlay();
  }
}
void SymbolsDialog::showOverlay() {
  Glib::ustring current = Glib::Markup::escape_text(symbol_set->get_active_text());
  if (current == ALLDOCS && !l.size()) 
  {
    overlay_icon->hide();
    if (!all_docs_processed ) {
        overlay_icon->show();
        overlay_title->set_markup(Glib::ustring("<span size=\"large\">") +
                                  Glib::ustring(_("Search in all symbol sets...")) + Glib::ustring("</span>"));
        overlay_desc->set_markup(Glib::ustring("<span size=\"small\">") +
                                 Glib::ustring(_("First search can be slow.")) + Glib::ustring("</span>"));
    } else if (!icons_found && !search_str.empty()) {
        overlay_title->set_markup(Glib::ustring("<span size=\"large\">") + Glib::ustring(_("No results found")) +
                                  Glib::ustring("</span>"));
        overlay_desc->set_markup(Glib::ustring("<span size=\"small\">") +
                                 Glib::ustring(_("Try a different search term.")) + Glib::ustring("</span>"));
    } else {
        overlay_icon->show();
        overlay_title->set_markup(Glib::ustring("<spansize=\"large\">") +
                                  Glib::ustring(_("Search in all symbol sets...")) + Glib::ustring("</span>"));
        overlay_desc->set_markup(Glib::ustring("<span size=\"small\">") + Glib::ustring("</span>"));
    }
  } else if (!number_symbols && (current != CURRENTDOC || !search_str.empty())) {
      overlay_title->set_markup(Glib::ustring("<span size=\"large\">") + Glib::ustring(_("No results found")) +
                                Glib::ustring("</span>"));
      overlay_desc->set_markup(Glib::ustring("<span size=\"small\">") +
                               Glib::ustring(_("Try a different search term,\nor switch to a different symbol set.")) +
                               Glib::ustring("</span>"));
  } else if (!number_symbols && current == CURRENTDOC) {
      overlay_title->set_markup(Glib::ustring("<span size=\"large\">") + Glib::ustring(_("No symbols found")) +
                                Glib::ustring("</span>"));
      overlay_desc->set_markup(
          Glib::ustring("<span size=\"small\">") +
          Glib::ustring(_("No symbols in current document.\nChoose a different symbol set\nor add a new symbol.")) +
          Glib::ustring("</span>"));
  } else if (!icons_found && !search_str.empty()) {
      overlay_title->set_markup(Glib::ustring("<span size=\"large\">") + Glib::ustring(_("No results found")) +
                                Glib::ustring("</span>"));
      overlay_desc->set_markup(Glib::ustring("<span size=\"small\">") +
                               Glib::ustring(_("Try a different search term,\nor switch to a different symbol set.")) +
                               Glib::ustring("</span>"));
  }
  gint width = scroller->get_allocated_width();
  gint height = scroller->get_allocated_height();
  if (previous_height != height || previous_width != width) {
      previous_height = height;
      previous_width = width;
      overlay_opacity->set_size_request(width, height);
      overlay_opacity->set(getOverlay(width, height));
  }
  overlay_opacity->hide();
  overlay_icon->show();
  overlay_title->show();
  overlay_desc->show();
  if (l.size()) {
    overlay_opacity->show();
    overlay_icon->hide();
    overlay_title->hide();
    overlay_desc->hide();
  }
}

void SymbolsDialog::hideOverlay() {
    overlay_opacity->hide();
    overlay_icon->hide();
    overlay_title->hide();
    overlay_desc->hide();
}

void SymbolsDialog::insertSymbol() {
    Inkscape::Verb *verb = Inkscape::Verb::get( SP_VERB_EDIT_SYMBOL );
    SPAction *action = verb->get_action(Inkscape::ActionContext( (Inkscape::UI::View::View *) current_desktop) );
    sp_action_perform (action, nullptr);
}

void SymbolsDialog::revertSymbol() {
    Inkscape::Verb *verb = Inkscape::Verb::get( SP_VERB_EDIT_UNSYMBOL );
    SPAction *action = verb->get_action(Inkscape::ActionContext( (Inkscape::UI::View::View *) current_desktop ) );
    sp_action_perform (action, nullptr);
}

void SymbolsDialog::iconDragDataGet(const Glib::RefPtr<Gdk::DragContext>& /*context*/, Gtk::SelectionData& data, guint /*info*/, guint /*time*/)
{
  auto iconArray = icon_view->get_selected_items();

  if( iconArray.empty() ) {
    //std::cout << "  iconArray empty: huh? " << std::endl;
  } else {
    Gtk::TreeModel::Path const & path = *iconArray.begin();
    Gtk::ListStore::iterator row = store->get_iter(path);
    Glib::ustring symbol_id = (*row)[getColumns()->symbol_id];
    GdkAtom dataAtom = gdk_atom_intern( "application/x-inkscape-paste", FALSE );
    gtk_selection_data_set( data.gobj(), dataAtom, 9, (guchar*)symbol_id.c_str(), symbol_id.length() );
  }

}

void SymbolsDialog::defsModified(SPObject * /*object*/, guint /*flags*/)
{
  Glib::ustring doc_title = symbol_set->get_active_text();
  if (doc_title != ALLDOCS && !symbol_sets[doc_title] ) {
    rebuild();
  }
}

void SymbolsDialog::selectionChanged(Inkscape::Selection *selection) {
  Glib::ustring symbol_id = selectedSymbolId();
  Glib::ustring doc_title = selectedSymbolDocTitle();
  if (!doc_title.empty()) {
    SPDocument* symbol_document = symbol_sets[doc_title];
    if (!symbol_document) {
      //we are in global search so get the original symbol document by title
      symbol_document = selectedSymbols();
    }
    if (symbol_document) {
      SPObject* symbol = symbol_document->getObjectById(symbol_id);
      if(symbol && !selection->includes(symbol)) {
          icon_view->unselect_all();
      }
    }
  }
}

void SymbolsDialog::documentReplaced(SPDesktop *desktop, SPDocument *document)
{
  current_desktop  = desktop;
  current_document = document;
  rebuild();
}

SPDocument* SymbolsDialog::selectedSymbols() {
  /* OK, we know symbol name... now we need to copy it to clipboard, bon chance! */
  Glib::ustring doc_title = symbol_set->get_active_text();
  if (doc_title == ALLDOCS) {
    return nullptr;
  }
  SPDocument* symbol_document = symbol_sets[doc_title];
  if( !symbol_document ) {
    symbol_document = getSymbolsSet(doc_title).second;
    // Symbol must be from Current Document (this method of checking should be language independent).
    if( !symbol_document ) {
      // Symbol must be from Current Document (this method of
      // checking should be language independent).
      symbol_document = current_document;
      add_symbol->set_sensitive( true );
      remove_symbol->set_sensitive( true );
    } else {
      add_symbol->set_sensitive( false );
      remove_symbol->set_sensitive( false );
    }
  }
  return symbol_document;
}

Glib::ustring SymbolsDialog::selectedSymbolId() {

  auto iconArray = icon_view->get_selected_items();

  if( !iconArray.empty() ) {
    Gtk::TreeModel::Path const & path = *iconArray.begin();
    Gtk::ListStore::iterator row = store->get_iter(path);
    return (*row)[getColumns()->symbol_id];
  }
  return Glib::ustring("");
}

Glib::ustring SymbolsDialog::selectedSymbolDocTitle() {

  auto iconArray = icon_view->get_selected_items();

  if( !iconArray.empty() ) {
    Gtk::TreeModel::Path const & path = *iconArray.begin();
    Gtk::ListStore::iterator row = store->get_iter(path);
    return (*row)[getColumns()->symbol_doc_title];
  }
  return Glib::ustring("");
}

Glib::ustring SymbolsDialog::documentTitle(SPDocument* symbol_doc) {
  if (symbol_doc) {
    SPRoot * root = symbol_doc->getRoot();
    gchar * title = root->title();
    if (title) {
      return ellipsize(Glib::ustring(title), 33);
    }
    g_free(title);
  }
  Glib::ustring current = symbol_set->get_active_text();
  if (current == CURRENTDOC) {
    return current;
  }
  return _("Untitled document");
}

void SymbolsDialog::iconChanged() {

  Glib::ustring symbol_id = selectedSymbolId();
  SPDocument* symbol_document = selectedSymbols();
  if (!symbol_document) {
    //we are in global search so get the original symbol document by title
      Glib::ustring doc_title = selectedSymbolDocTitle();
      if (!doc_title.empty()) {
        symbol_document = symbol_sets[doc_title];
      }
  }
  if (symbol_document) {
    SPObject* symbol = symbol_document->getObjectById(symbol_id);

    if( symbol ) {
      if( symbol_document == current_document ) {
        // Select the symbol on the canvas so it can be manipulated
        current_desktop->selection->set( symbol, false );
      }
      // Find style for use in <use>
      // First look for default style stored in <symbol>
      gchar const* style = symbol->getAttribute("inkscape:symbol-style");
      if( !style ) {
        // If no default style in <symbol>, look in documents.
        if( symbol_document == current_document ) {
          style = styleFromUse( symbol_id.c_str(), current_document );
        } else {
          style = symbol_document->getReprRoot()->attribute("style");
        }
      }

      ClipboardManager *cm = ClipboardManager::get();
      cm->copySymbol(symbol->getRepr(), style, symbol_document == current_document);
    }
  }
}

#ifdef WITH_LIBVISIO

#if WITH_LIBVISIO01
// Extend libvisio's native RVNGSVGDrawingGenerator with support for extracting stencil names (to be used as ID/title)
class REVENGE_API RVNGSVGDrawingGenerator_WithTitle : public RVNGSVGDrawingGenerator {
  public:
    RVNGSVGDrawingGenerator_WithTitle(RVNGStringVector &output, RVNGStringVector &titles, const RVNGString &nmSpace)
      : RVNGSVGDrawingGenerator(output, nmSpace)
      , _titles(titles)
    {}

    void startPage(const RVNGPropertyList &propList) override
    {
      RVNGSVGDrawingGenerator::startPage(propList);
      if (propList["draw:name"]) {
          _titles.append(propList["draw:name"]->getStr());
      } else {
          _titles.append("");
      }
    }

  private:
    RVNGStringVector &_titles;
};
#endif

// Read Visio stencil files
SPDocument* read_vss(Glib::ustring filename, Glib::ustring name ) {
  gchar *fullname;
  #ifdef _WIN32
    // RVNGFileStream uses fopen() internally which unfortunately only uses ANSI encoding on Windows
    // therefore attempt to convert uri to the system codepage
    // even if this is not possible the alternate short (8.3) file name will be used if available
    fullname = g_win32_locale_filename_from_utf8(filename.c_str());
  #else
    fullname = strdup(filename.c_str());
  #endif

  RVNGFileStream input(fullname);
  g_free(fullname);

  if (!libvisio::VisioDocument::isSupported(&input)) {
    return nullptr;
  }
  RVNGStringVector output;
  RVNGStringVector titles;
#if WITH_LIBVISIO01
  RVNGSVGDrawingGenerator_WithTitle generator(output, titles, "svg");

  if (!libvisio::VisioDocument::parseStencils(&input, &generator)) {
#else
  if (!libvisio::VisioDocument::generateSVGStencils(&input, output)) {
#endif
    return nullptr;
  }
  if (output.empty()) {
    return nullptr;
  }

  // prepare a valid title for the symbol file
  Glib::ustring title = Glib::Markup::escape_text(name);
  // prepare a valid id prefix for symbols libvisio doesn't give us a name for
  Glib::RefPtr<Glib::Regex> regex1 = Glib::Regex::create("[^a-zA-Z0-9_-]");
  Glib::ustring id = regex1->replace(name, 0, "_", Glib::REGEX_MATCH_PARTIAL);

  Glib::ustring tmpSVGOutput;
  tmpSVGOutput += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
  tmpSVGOutput += "<svg\n";
  tmpSVGOutput += "  xmlns=\"http://www.w3.org/2000/svg\"\n";
  tmpSVGOutput += "  xmlns:svg=\"http://www.w3.org/2000/svg\"\n";
  tmpSVGOutput += "  xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n";
  tmpSVGOutput += "  version=\"1.1\"\n";
  tmpSVGOutput += "  style=\"fill:none;stroke:#000000;stroke-width:2\">\n";
  tmpSVGOutput += "  <title>";
  tmpSVGOutput += title;
  tmpSVGOutput += "</title>\n";
  tmpSVGOutput += "  <defs>\n";

  // Each "symbol" is in its own SVG file, we wrap with <symbol> and merge into one file.
  for (unsigned i=0; i<output.size(); ++i) {

    std::stringstream ss;
    if (titles.size() == output.size() && titles[i] != "") {
      // TODO: Do we need to check for duplicated titles?
      ss << regex1->replace(titles[i].cstr(), 0, "_", Glib::REGEX_MATCH_PARTIAL);
    } else {
      ss << id << "_" << i;
    }

    tmpSVGOutput += "    <symbol id=\"" + ss.str() + "\">\n";

#if WITH_LIBVISIO01
    if (titles.size() == output.size() && titles[i] != "") {
      tmpSVGOutput += "      <title>" + Glib::ustring(RVNGString::escapeXML(titles[i].cstr()).cstr()) + "</title>\n";
    }
#endif

    std::istringstream iss( output[i].cstr() );
    std::string line;
    while( std::getline( iss, line ) ) {
      if( line.find( "svg:svg" ) == std::string::npos ) {
        tmpSVGOutput += "      " + line + "\n";
      }
    }

    tmpSVGOutput += "    </symbol>\n";
  }

  tmpSVGOutput += "  </defs>\n";
  tmpSVGOutput += "</svg>\n";
  return SPDocument::createNewDocFromMem( tmpSVGOutput.c_str(), strlen( tmpSVGOutput.c_str()), false );

}
#endif

/* Hunts preference directories for symbol files */
void SymbolsDialog::getSymbolsTitle() {

    using namespace Inkscape::IO::Resource;
    Glib::ustring title;
    number_docs = 0;
    std::regex matchtitle (".*?<title.*?>(.*?)<(/| /)"); 
    for(auto &filename: get_filenames(SYMBOLS, {".svg", ".vss"})) {
        if(Glib::str_has_suffix(filename, ".vss")) {
          std::size_t found = filename.find_last_of("/\\");
          filename = filename.substr(found+1);
          title = filename.erase(filename.rfind('.'));
          if(title.empty()) {
            title = _("Unnamed Symbols");
          }
          symbol_sets[title]= nullptr;
          ++number_docs;
        } else {
          std::ifstream infile(filename);
          std::string line;
          while (std::getline(infile, line)) {
              std::string title_res = std::regex_replace (line, matchtitle,"$1",std::regex_constants::format_no_copy);
              if (!title_res.empty()) {
                  title_res = g_dpgettext2(nullptr, "Symbol", title_res.c_str());
                  symbol_sets[ellipsize(Glib::ustring(title_res), 33)]= nullptr;
                  ++number_docs;
                  break;
              }
              std::string::size_type position_exit = line.find ("<defs");
              if (position_exit != std::string::npos) {
                  std::size_t found = filename.find_last_of("/\\");
                  filename = filename.substr(found+1);
                  title = filename.erase(filename.rfind('.'));
                  if(title.empty()) {
                    title = _("Unnamed Symbols");
                  }
                  symbol_sets[title]= nullptr;
                  ++number_docs;
                  break;
              }
          } 
        }
    }
    for(auto const &symbol_document_map : symbol_sets) {
      symbol_set->append(symbol_document_map.first);
    }
}

/* Hunts preference directories for symbol files */
std::pair<Glib::ustring, SPDocument*>
SymbolsDialog::getSymbolsSet(Glib::ustring title) 
{
    SPDocument* symbol_doc = nullptr;
    Glib::ustring current = symbol_set->get_active_text();
    if (current == CURRENTDOC) {
      return std::make_pair(CURRENTDOC, symbol_doc);
    }
    if (symbol_sets[title]) {
      sensitive = false;
      symbol_set->remove_all();
      symbol_set->append(CURRENTDOC);
      symbol_set->append(ALLDOCS);
      for(auto const &symbol_document_map : symbol_sets) {
        if (CURRENTDOC != symbol_document_map.first) {
          symbol_set->append(symbol_document_map.first);
        }
      }
      symbol_set->set_active_text(title);
      sensitive = true;
      return std::make_pair(title, symbol_sets[title]);
    }
    using namespace Inkscape::IO::Resource;
    Glib::ustring new_title;

    std::regex matchtitle (".*?<title.*?>(.*?)<(/| /)"); 
    for(auto &filename: get_filenames(SYMBOLS, {".svg", ".vss"})) {
        if(Glib::str_has_suffix(filename, ".vss")) {
#ifdef WITH_LIBVISIO
          std::size_t pos = filename.find_last_of("/\\");
          Glib::ustring filename_short = "";
          if (pos != std::string::npos) {
            filename_short = filename.substr(pos+1);
          }
          if (filename_short == title + ".vss") {
              new_title = title;
              symbol_doc = read_vss(Glib::ustring(filename), title);
          }
#endif
        } else {
            std::ifstream infile(filename);
            std::string line;
            while (std::getline(infile, line)) {
                std::string title_res = std::regex_replace (line, matchtitle,"$1",std::regex_constants::format_no_copy);
                if (!title_res.empty()) {
                  title_res = g_dpgettext2(nullptr, "Symbol", title_res.c_str());
                  new_title = ellipsize(Glib::ustring(title_res), 33);
                }
                std::size_t pos = filename.find_last_of("/\\");
                Glib::ustring filename_short = "";
                if (pos != std::string::npos) {
                  filename_short = filename.substr(pos+1);
                }
                if (title == new_title || filename_short == title + ".svg") {
                    new_title = title;
                    if(Glib::str_has_suffix(filename, ".svg")) {
                        symbol_doc = SPDocument::createNewDoc(filename.c_str(), FALSE);
                    }
                }
                if (symbol_doc) {
                    break;
                }
                std::string::size_type position_exit = line.find ("<defs");
                if (position_exit != std::string::npos) {
                    break;
                }
            }
        }
        if (symbol_doc) {
            break;
        }
    }
    if(symbol_doc) {
      symbol_sets.erase(title);
      symbol_sets[new_title] = symbol_doc;
      sensitive = false;
      symbol_set->remove_all();
      symbol_set->append(CURRENTDOC);
      symbol_set->append(ALLDOCS);
      for(auto const &symbol_document_map : symbol_sets) {
        if (CURRENTDOC != symbol_document_map.first) {
          symbol_set->append(symbol_document_map.first);
        }
      }
      symbol_set->set_active_text(new_title);
      sensitive = true;
    }
    return std::make_pair(new_title, symbol_doc);
}

void SymbolsDialog::symbolsInDocRecursive (SPObject *r, std::map<Glib::ustring, std::pair<Glib::ustring, SPSymbol*> > &l, Glib::ustring doc_title)
{
  if(!r) return;

  // Stop multiple counting of same symbol
  if ( dynamic_cast<SPUse *>(r) ) {
    return;
  }

  if ( dynamic_cast<SPSymbol *>(r)) {
    Glib::ustring id = r->getAttribute("id");
    gchar * title = r->title();
    if(title) {
      l[doc_title + title + id] = std::make_pair(doc_title,dynamic_cast<SPSymbol *>(r));
    } else {
      l[Glib::ustring(_("notitle_")) + id] = std::make_pair(doc_title,dynamic_cast<SPSymbol *>(r));
    }
    g_free(title);
  }
  for (auto& child: r->children) {
    symbolsInDocRecursive(&child, l, doc_title);
  }
}

std::map<Glib::ustring, std::pair<Glib::ustring, SPSymbol*> > 
SymbolsDialog::symbolsInDoc( SPDocument* symbol_document, Glib::ustring doc_title)
{

  std::map<Glib::ustring, std::pair<Glib::ustring, SPSymbol*> > l;
  if (symbol_document) {
    symbolsInDocRecursive (symbol_document->getRoot(), l , doc_title);
  }
  return l;
}

void SymbolsDialog::useInDoc (SPObject *r, std::vector<SPUse*> &l)
{

  if ( dynamic_cast<SPUse *>(r) ) {
    l.push_back(dynamic_cast<SPUse *>(r));
  }

  for (auto& child: r->children) {
    useInDoc( &child, l );
  }
}

std::vector<SPUse*> SymbolsDialog::useInDoc( SPDocument* useDocument) {
  std::vector<SPUse*> l;
  useInDoc (useDocument->getRoot(), l);
  return l;
}

// Returns style from first <use> element found that references id.
// This is a last ditch effort to find a style.
gchar const* SymbolsDialog::styleFromUse( gchar const* id, SPDocument* document) {

  gchar const* style = nullptr;
  std::vector<SPUse*> l = useInDoc( document );
  for( auto use:l ) {
    if ( use ) {
      gchar const *href = use->getRepr()->attribute("xlink:href");
      if( href ) {
        Glib::ustring href2(href);
        Glib::ustring id2(id);
        id2 = "#" + id2;
        if( !href2.compare(id2) ) {
          style = use->getRepr()->attribute("style");
          break;
        }
      }
    }
  }
  return style;
}

void SymbolsDialog::clearSearch() 
{
  if(search->get_text().empty() && sensitive) {
    enableWidgets(false);
    search_str = "";
    store->clear();
    SPDocument* symbol_document = selectedSymbols();
    if (symbol_document) {
      //We are not in search all docs
      icons_found = false;
      addSymbolsInDoc(symbol_document);
    } else {
      showOverlay();
      enableWidgets(true);
    }
  }
}

void SymbolsDialog::enableWidgets(bool enable) 
{
  symbol_set->set_sensitive(enable);
  search->set_sensitive(enable);
  tools ->set_sensitive(enable);
}

void SymbolsDialog::beforeSearch(GdkEventKey* evt) 
{
  sensitive = false;
  search_str = search->get_text().lowercase();
  if (evt->keyval != GDK_KEY_Return) {
    return;
  }
  searchsymbols();
}

void SymbolsDialog::searchsymbols()
{
    progress_bar->set_fraction(0.0);
    enableWidgets(false);
    SPDocument *symbol_document = selectedSymbols();
    if (symbol_document) {
        // We are not in search all docs
        search->set_text(_("Searching..."));
        store->clear();
        icons_found = false;
        addSymbolsInDoc(symbol_document);
    } else {
        idleconn.disconnect();
        idleconn = Glib::signal_idle().connect(sigc::mem_fun(*this, &SymbolsDialog::callbackAllSymbols));
        search->set_text(_("Loading all symbols..."));
    }
}

void SymbolsDialog::unsensitive(GdkEventKey* evt) 
{
  sensitive = true;
}

bool SymbolsDialog::callbackSymbols(){
  if (l.size()) {
    showOverlay();
    for (auto symbol_data = l.begin(); symbol_data != l.end();) {
      Glib::ustring doc_title = symbol_data->second.first;
      SPSymbol * symbol = symbol_data->second.second;
      counter_symbols ++;
      gchar *symbol_title_char = symbol->title();
      gchar *symbol_desc_char = symbol->description();
      bool found = false;
      if (symbol_title_char) {
        Glib::ustring symbol_title = Glib::ustring(symbol_title_char).lowercase();
        auto pos = symbol_title.rfind(search_str);
        if (pos != std::string::npos) {
          found = true;
        }
        if (!found && symbol_desc_char) {
          Glib::ustring symbol_desc = Glib::ustring(symbol_desc_char).lowercase();
          auto pos = symbol_desc.rfind(search_str);
          if (pos != std::string::npos) {
            found = true;
          }
        }
      }
      if (symbol && (search_str.empty() || found)) {
        addSymbol( symbol, doc_title);
        icons_found = true;
      }

      progress_bar->set_fraction(((100.0/number_symbols) * counter_symbols)/100.0);
      symbol_data = l.erase(l.begin());
      //to get more items and best performance
      int modulus = number_symbols > 200 ? 50 : (number_symbols/4);
      g_free(symbol_title_char);
      g_free(symbol_desc_char);
      if (modulus && counter_symbols % modulus == 0 && !l.empty()) { 
        return true;
      }
    }
    if (!icons_found && !search_str.empty()) {
      showOverlay();
    } else {
      hideOverlay();
    }
    progress_bar->set_fraction(0);
    sensitive = false;
    search->set_text(search_str);
    sensitive = true;
    enableWidgets(true);
    return false;
  }
  return true;
}

bool SymbolsDialog::callbackAllSymbols(){
  Glib::ustring current = symbol_set->get_active_text();
  if (current == ALLDOCS && search->get_text() == _("Loading all symbols...")) {
    size_t counter = 0;
    std::map<Glib::ustring, SPDocument*> symbol_sets_tmp = symbol_sets;
    for(auto const &symbol_document_map : symbol_sets_tmp) {
      ++counter;
      SPDocument* symbol_document = symbol_document_map.second;
      if (symbol_document) {
        continue;
      }
      symbol_document = getSymbolsSet(symbol_document_map.first).second;
      symbol_set->set_active_text(ALLDOCS);
      if (!symbol_document) {
        continue;
      }
      progress_bar->set_fraction(((100.0/number_docs) * counter)/100.0);
      return true;
    }
    symbol_sets_tmp.clear();
    hideOverlay();
    all_docs_processed = true;
    addSymbols();
    progress_bar->set_fraction(0);
    search->set_text("Searching...");
    return false;
  }
  return true;
}

Glib::ustring SymbolsDialog::ellipsize(Glib::ustring data, size_t limit) {
    if (data.length() > limit) {
      data = data.substr(0, limit-3);
      return data + "...";
    }
    return data;
}

void SymbolsDialog::addSymbolsInDoc(SPDocument* symbol_document) {
  
  if (!symbol_document) {
    return; //Search all
  }
  Glib::ustring doc_title = documentTitle(symbol_document);
  progress_bar->set_fraction(0.0);
  counter_symbols = 0;
  l = symbolsInDoc(symbol_document, doc_title);
  number_symbols = l.size();
  if (!number_symbols) {
    sensitive = false;
    search->set_text(search_str);
    sensitive = true;
    enableWidgets(true);
    idleconn.disconnect();
    showOverlay();
  } else {
    idleconn.disconnect();
    idleconn = Glib::signal_idle().connect( sigc::mem_fun(*this, &SymbolsDialog::callbackSymbols));
  }
}

void SymbolsDialog::addSymbols() {
  store->clear();
  icons_found = false;
  for(auto const &symbol_document_map : symbol_sets) {
    SPDocument* symbol_document = symbol_document_map.second;
    if (!symbol_document) {
      continue;
    }
    Glib::ustring doc_title = documentTitle(symbol_document);
    std::map<Glib::ustring, std::pair<Glib::ustring, SPSymbol*> > l_tmp = symbolsInDoc(symbol_document, doc_title);
    for(auto &p : l_tmp ) {
      l[p.first] = p.second;
    }
    l_tmp.clear();
  }
  counter_symbols = 0;
  progress_bar->set_fraction(0.0);
  number_symbols = l.size();
  if (!number_symbols) {
    showOverlay();
    idleconn.disconnect();
    sensitive = false;
    search->set_text(search_str);
    sensitive = true;
    enableWidgets(true);
  } else {
    idleconn.disconnect();
    idleconn = Glib::signal_idle().connect( sigc::mem_fun(*this, &SymbolsDialog::callbackSymbols));
  }
}

void SymbolsDialog::addSymbol( SPObject* symbol, Glib::ustring doc_title)
{
  gchar const *id = symbol->getRepr()->attribute("id");
  
  if (doc_title.empty()) {
    doc_title = CURRENTDOC;
  } else {
      doc_title = g_dpgettext2(nullptr, "Symbol", doc_title.c_str());
  }
  
  Glib::ustring symbol_title;
  gchar *title = symbol->title(); // From title element
  if (title) {
    symbol_title = Glib::ustring::compose("%1 (%2)", g_dpgettext2(nullptr, "Symbol", title), doc_title.c_str());
  } else {
    symbol_title = Glib::ustring::compose("%1 %2 (%3)", _("Symbol without title"), Glib::ustring(id), doc_title);
  }
  g_free(title);
  
  Glib::RefPtr<Gdk::Pixbuf> pixbuf = drawSymbol( symbol );
  if( pixbuf ) {
    Gtk::ListStore::iterator row = store->append();
    SymbolColumns* columns = getColumns();
    (*row)[columns->symbol_id]        = Glib::ustring( id );
    (*row)[columns->symbol_title]     = Glib::Markup::escape_text(symbol_title);
    (*row)[columns->symbol_doc_title] = Glib::Markup::escape_text(doc_title);
    (*row)[columns->symbol_image]     = pixbuf;
    delete columns;
  }
}

/*
 * Returns image of symbol.
 *
 * Symbols normally are not visible. They must be referenced by a
 * <use> element.  A temporary document is created with a dummy
 * <symbol> element and a <use> element that references the symbol
 * element. Each real symbol is swapped in for the dummy symbol and
 * the temporary document is rendered.
 */
Glib::RefPtr<Gdk::Pixbuf>
SymbolsDialog::drawSymbol(SPObject *symbol)
{
  // Create a copy repr of the symbol with id="the_symbol"
  Inkscape::XML::Document *xml_doc = preview_document->getReprDoc();
  Inkscape::XML::Node *repr = symbol->getRepr()->duplicate(xml_doc);
  repr->setAttribute("id", "the_symbol");

  // Replace old "the_symbol" in preview_document by new.
  Inkscape::XML::Node *root = preview_document->getReprRoot();
  SPObject *symbol_old = preview_document->getObjectById("the_symbol");
  if (symbol_old) {
      symbol_old->deleteObject(false);
  }

  // First look for default style stored in <symbol>
  gchar const* style = repr->attribute("inkscape:symbol-style");
  if( !style ) {
    // If no default style in <symbol>, look in documents.
    if( symbol->document == current_document ) {
      gchar const *id = symbol->getRepr()->attribute("id");
      style = styleFromUse( id, symbol->document );
    } else {
      style = symbol->document->getReprRoot()->attribute("style");
    }
  }
  // Last ditch effort to provide some default styling
  if( !style ) style = "fill:#bbbbbb;stroke:#808080";

  // This is for display in Symbols dialog only
  if( style ) repr->setAttribute( "style", style );

  // BUG: Symbols don't work if defined outside of <defs>. Causes Inkscape
  // crash when trying to read in such a file.
  root->appendChild(repr);
  //defsrepr->appendChild(repr);
  Inkscape::GC::release(repr);

  // Uncomment this to get the preview_document documents saved (useful for debugging)
  // FILE *fp = fopen (g_strconcat(id, ".svg", NULL), "w");
  // sp_repr_save_stream(preview_document->getReprDoc(), fp);
  // fclose (fp);

  // Make sure preview_document is up-to-date.
  preview_document->getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
  preview_document->ensureUpToDate();

  // Make sure we have symbol in preview_document
  SPObject *object_temp = preview_document->getObjectById( "the_use" );
  preview_document->getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
  preview_document->ensureUpToDate();

  SPItem *item = dynamic_cast<SPItem *>(object_temp);
  g_assert(item != nullptr);
  unsigned psize = SYMBOL_ICON_SIZES[pack_size];

  Glib::RefPtr<Gdk::Pixbuf> pixbuf(nullptr);
  // We could use cache here, but it doesn't really work with the structure
  // of this user interface and we've already cached the pixbuf in the gtklist

  // Find object's bbox in document.
  // Note symbols can have own viewport... ignore for now.
  //Geom::OptRect dbox = item->geometricBounds();
  Geom::OptRect dbox = item->documentVisualBounds();

  if (dbox) {
    /* Scale symbols to fit */
    double scale = 1.0;
    double width  = dbox->width();
    double height = dbox->height();

    if( width == 0.0 ) width = 1.0;
    if( height == 0.0 ) height = 1.0;

    if( fit_symbol->get_active() )
      scale = psize / ceil(std::max(width, height)); 
    else
      scale = pow( 2.0, scale_factor/2.0 ) * psize / 32.0;

    pixbuf = Glib::wrap(render_pixbuf(renderDrawing, scale, *dbox, psize));
  }

  return pixbuf;
}

/*
 * Return empty doc to render symbols in.
 * Symbols are by default not rendered so a <use> element is
 * provided.
 */
SPDocument* SymbolsDialog::symbolsPreviewDoc()
{
  // BUG: <symbol> must be inside <defs>
  gchar const *buffer =
"<svg xmlns=\"http://www.w3.org/2000/svg\""
"     xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\""
"     xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\""
"     xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
"  <defs id=\"defs\">"
"    <symbol id=\"the_symbol\"/>"
"  </defs>"
"  <use id=\"the_use\" xlink:href=\"#the_symbol\"/>"
"</svg>";
  return SPDocument::createNewDocFromMem( buffer, strlen(buffer), FALSE );
}

/*
 * Update image widgets
 */
Glib::RefPtr<Gdk::Pixbuf> 
SymbolsDialog::getOverlay(gint width, gint height)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);
  cairo_set_source_rgba(cr, 1, 1, 1, 0.75);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);
  GdkPixbuf* pixbuf = ink_pixbuf_create_from_cairo_surface(surface);
  cairo_destroy (cr);
  return Glib::wrap(pixbuf);
}

void SymbolsDialog::setTargetDesktop(SPDesktop *desktop)
{
  if (this->current_desktop != desktop) {
    this->current_desktop = desktop;
    if( !symbol_sets[symbol_set->get_active_text()] ) {
      // Symbol set is from Current document, update
      rebuild();
    }
  }
}

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :
