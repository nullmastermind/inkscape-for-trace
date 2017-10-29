/**
 * @file
 * Symbols dialog.
 */
/* Authors:
 * Copyright (C) 2012 Tavmjong Bah
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <iostream>
#include <algorithm>
#include <locale>
#include <sstream>

#include <glibmm/regex.h>
#include <glibmm/stringutils.h>
#include <glibmm/markup.h>

#include "path-prefix.h"
#include "io/sys.h"
#include "io/resource.h"

#include "ui/cache/svg_preview_cache.h"
#include "ui/clipboard.h"
#include "ui/icon-names.h"

#include "symbols.h"

#include "selection.h"
#include "desktop.h"

#include "document.h"
#include "inkscape.h"
#include "sp-root.h"
#include "sp-use.h"
#include "sp-defs.h"
#include "sp-symbol.h"

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

#include "verbs.h"
#include "helper/action.h"
#include <glibmm/i18n.h>

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
  UI::Widget::Panel("", prefsPath, SP_VERB_DIALOG_SYMBOLS),
  store(Gtk::ListStore::create(*getColumns())),
  icon_view(0),
  current_desktop(0),
  desk_track(),
  current_document(0),
  preview_document(0),
  instanceConns()
{
  /********************    Table    *************************/
  table = new Gtk::Grid();
  table->set_margin_left(3);
  table->set_margin_right(3);
  table->set_margin_top(4);
  // panel is a cloked Gtk::VBox
  _getContents()->pack_start(* Gtk::manage(table), Gtk::PACK_EXPAND_WIDGET);
  guint row = 0;

  /******************** Symbol Sets *************************/
  Gtk::Label* label_set = new Gtk::Label(_("Symbol set: "));
  table->attach(*Gtk::manage(label_set),0,row,1,1);
  symbol_set = new Gtk::ComboBoxText();  // Fill in later
  symbol_set->append(_("Current Document"));
  symbol_set->append(_("All symbols sets"));
  symbol_set->set_active_text(_("Current Document"));
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
  search->set_margin_left(10);
  search->set_margin_right(10);
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
  icon_view->set_size_request( 100, 200 );

  std::vector< Gtk::TargetEntry > targets;
  targets.push_back(Gtk::TargetEntry( "application/x-inkscape-paste"));

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
  table->attach(*Gtk::manage(overlay),0,row,2,1);

  ++row;

  /******************** Progress *******************************/
  progress = new Gtk::HBox();
  progress_bar = Gtk::manage(new Gtk::ProgressBar()); 
  table->attach(*Gtk::manage(progress),0,row, 2, 1);
  progress->pack_start(* progress_bar, Gtk::PACK_EXPAND_WIDGET);
  progress->set_margin_top(15);
  progress->set_margin_bottom(15);
  progress->set_margin_left(20);
  progress->set_margin_right(20);

  ++row;

  /******************** Tools *******************************/
  tools = new Gtk::HBox();

  //tools->set_layout( Gtk::BUTTONBOX_END );
  scroller->set_hexpand();
  table->attach(*Gtk::manage(tools),0,row,2,1);

  auto add_symbol_image = Gtk::manage(new Gtk::Image());
  add_symbol_image->set_from_icon_name("symbol-add", Gtk::ICON_SIZE_SMALL_TOOLBAR);

  add_symbol = Gtk::manage(new Gtk::Button());
  add_symbol->add(*add_symbol_image);
  add_symbol->set_tooltip_text(_("Add Symbol from the current document."));
  add_symbol->set_relief( Gtk::RELIEF_NONE );
  add_symbol->set_focus_on_click( false );
  add_symbol->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::insertSymbol));
  tools->pack_start(* add_symbol, Gtk::PACK_SHRINK);

  auto remove_symbolImage = Gtk::manage(new Gtk::Image());
  remove_symbolImage->set_from_icon_name("symbol-remove", Gtk::ICON_SIZE_SMALL_TOOLBAR);

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

  auto packMoreImage = Gtk::manage(new Gtk::Image());
  packMoreImage->set_from_icon_name("pack-more", Gtk::ICON_SIZE_SMALL_TOOLBAR);

  more = Gtk::manage(new Gtk::Button());
  more->add(*packMoreImage);
  more->set_tooltip_text(_("Display more icons in row."));
  more->set_relief( Gtk::RELIEF_NONE );
  more->set_focus_on_click( false );
  more->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::packmore));
  tools->pack_start(* more, Gtk::PACK_SHRINK);

  auto packLessImage = Gtk::manage(new Gtk::Image());
  packLessImage->set_from_icon_name("pack-less", Gtk::ICON_SIZE_SMALL_TOOLBAR);

  fewer = Gtk::manage(new Gtk::Button());
  fewer->add(*packLessImage);
  fewer->set_tooltip_text(_("Display fewer icons in row."));
  fewer->set_relief( Gtk::RELIEF_NONE );
  fewer->set_focus_on_click( false );
  fewer->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::packless));
  tools->pack_start(* fewer, Gtk::PACK_SHRINK);

  // Toggle scale to fit on/off
  auto fit_symbolImage = Gtk::manage(new Gtk::Image());
  fit_symbolImage->set_from_icon_name("symbol-fit", Gtk::ICON_SIZE_SMALL_TOOLBAR);

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
  auto zoom_outImage = Gtk::manage(new Gtk::Image());
  zoom_outImage->set_from_icon_name("symbol-smaller", Gtk::ICON_SIZE_SMALL_TOOLBAR);

  zoom_out = Gtk::manage(new Gtk::Button());
  zoom_out->add(*zoom_outImage);
  zoom_out->set_tooltip_text(_("Make symbols smaller by zooming out."));
  zoom_out->set_relief( Gtk::RELIEF_NONE );
  zoom_out->set_focus_on_click( false );
  zoom_out->set_sensitive( false );
  zoom_out->signal_clicked().connect(sigc::mem_fun(*this, &SymbolsDialog::zoomout));
  tools->pack_start(* zoom_out, Gtk::PACK_SHRINK);

  auto zoom_inImage = Gtk::manage(new Gtk::Image());
  zoom_inImage->set_from_icon_name("symbol-bigger", Gtk::ICON_SIZE_SMALL_TOOLBAR);

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
    
  /*************************Overlays******************************/
  //Loading
  overlay_opacity = new Gtk::Image();
  overlay_opacity->set(getOverlay(overlay_opacity, "overlay", 1000));
  overlay_opacity->set_halign(Gtk::ALIGN_START );
  overlay_opacity->set_valign(Gtk::ALIGN_START );
  //No results
  iconsize =  Gtk::IconSize().register_new(Glib::ustring("ICON_SIZE_DIALOG_EXTRA"), 110, 110);
  overlay_icon = new Gtk::Image();
  overlay_icon->set_from_icon_name("none", iconsize);
  overlay_icon = new Gtk::Image(noresults_icon);
  overlay_icon->set_halign(Gtk::ALIGN_CENTER );
  overlay_icon->set_valign(Gtk::ALIGN_START );
  overlay_icon->set_margin_top(45);
  overlay_title = new Gtk::Label();
  overlay_title->set_halign(Gtk::ALIGN_CENTER );
  overlay_title->set_valign(Gtk::ALIGN_START );
  overlay_title->set_justify(Gtk::JUSTIFY_CENTER);
  overlay_title->set_margin_top(155);
  overlay_title->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"x-large\">") + Glib::ustring(_("No results found")) + Glib::ustring("</span>"));
  overlay_desc = new Gtk::Label();
  overlay_desc->set_halign(Gtk::ALIGN_CENTER );
  overlay_desc->set_valign(Gtk::ALIGN_START );
  overlay_desc->set_margin_top(180);
  overlay_desc->set_justify(Gtk::JUSTIFY_CENTER);
  overlay_desc->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"medium\">") + Glib::ustring(_("Try a another search or select other set")) + Glib::ustring("</span>"));
  overlay->add_overlay(* overlay_opacity);
  overlay->add_overlay(* overlay_icon);
  overlay->add_overlay(* overlay_title);
  overlay->add_overlay(* overlay_desc);

  sigc::connection defsModifiedConn = defs->connectModified(sigc::mem_fun(*this, &SymbolsDialog::defsModified));
  instanceConns.push_back(defsModifiedConn);

  sigc::connection selectionChangedConn = current_desktop->selection->connectChanged(
          sigc::mem_fun(*this, &SymbolsDialog::selectionChanged));
  instanceConns.push_back(selectionChangedConn);

  sigc::connection documentReplacedConn = current_desktop->connectDocumentReplaced(
          sigc::mem_fun(*this, &SymbolsDialog::documentReplaced));
  instanceConns.push_back(documentReplacedConn);
  getSymbolsFilename();
  icons_found = false;
  
  Glib::signal_idle().connect( sigc::mem_fun(*this, &SymbolsDialog::callbackSymbols));
  
  addSymbolsInDoc(current_document); /* Defaults to current document */

  sigc::connection desktopChangeConn =
    desk_track.connectDesktopChanged( sigc::mem_fun(*this, &SymbolsDialog::setTargetDesktop) );
  instanceConns.push_back( desktopChangeConn );
  desk_track.connect(GTK_WIDGET(gobj()));
}

SymbolsDialog::~SymbolsDialog()
{
  for (std::vector<sigc::connection>::iterator it =  instanceConns.begin(); it != instanceConns.end(); ++it) {
      it->disconnect();
  }
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
  if (search->get_text() != _("Searching...") &&
    search->get_text() != _("Loading documents...") &&
    search->get_text() != _("Documents done, searchig inside...") ) 
  {
    search_str = "";
    search->set_text("");
  }
  if (symbol_document) {
    addSymbolsInDoc(symbol_document);
  }
}

void SymbolsDialog::insertSymbol() {
    Inkscape::Verb *verb = Inkscape::Verb::get( SP_VERB_EDIT_SYMBOL );
    SPAction *action = verb->get_action(Inkscape::ActionContext( (Inkscape::UI::View::View *) current_desktop) );
    sp_action_perform (action, NULL);
}

void SymbolsDialog::revertSymbol() {
    Inkscape::Verb *verb = Inkscape::Verb::get( SP_VERB_EDIT_UNSYMBOL );
    SPAction *action = verb->get_action(Inkscape::ActionContext( (Inkscape::UI::View::View *) current_desktop ) );
    sp_action_perform (action, NULL);
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
  if (doc_title != _("All symbols sets") && !symbol_sets[symbol_set->get_active_text()] ) {
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
  if (doc_title == _("All symbols sets")) {
    return NULL;
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
  Glib::ustring title = _("Untitled document");
  if (symbol_doc) {
    SPRoot * root = symbol_doc->getRoot();
    if (root->title()) {
      title = ellipsize(Glib::ustring(root->title()));
      return title;
    }
  }
  Glib::ustring current = symbol_set->get_active_text();
  if (current == _("Current Document")) {
    return current;
  }
  return title;
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

    void startPage(const RVNGPropertyList &propList)
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
  #ifdef WIN32
    // RVNGFileStream uses fopen() internally which unfortunately only uses ANSI encoding on Windows
    // therefore attempt to convert uri to the system codepage
    // even if this is not possible the alternate short (8.3) file name will be used if available
    fullname = g_win32_locale_filename_from_utf8(filename.c_str());
  #else
    filename.copy(fullname, filename.length());
  #endif

  RVNGFileStream input(fullname);
  g_free(fullname);

  if (!libvisio::VisioDocument::isSupported(&input)) {
    return NULL;
  }

  RVNGStringVector output;
  RVNGStringVector titles;
#if WITH_LIBVISIO01
  RVNGSVGDrawingGenerator_WithTitle generator(output, titles, "svg");

  if (!libvisio::VisioDocument::parseStencils(&input, &generator)) {
#else
  if (!libvisio::VisioDocument::generateSVGStencils(&input, output)) {
#endif
    return NULL;
  }

  if (output.empty()) {
    return NULL;
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

  return SPDocument::createNewDocFromMem( tmpSVGOutput.c_str(), strlen( tmpSVGOutput.c_str()), 0 );

}
#endif

/* Hunts preference directories for symbol files */
void SymbolsDialog::getSymbolsFilename() {

    using namespace Inkscape::IO::Resource;
    Glib::ustring title;
    number_docs = 0;
    for(auto &filename: get_filenames(SYMBOLS, {".svg", ".vss"})) {
        if(Glib::str_has_suffix(filename, ".svg")) {
            //TODO: find a way to get real title without loading all SPDocument
            std::size_t found = filename.find_last_of("/\\");
            filename = filename.substr(found+1);
            title = filename.erase(filename.rfind('.'));
            if(title.empty()) {
              title = _("Unnamed Symbols");
            }
            symbol_sets[title]= NULL;
            symbol_set->append(title);
            ++number_docs;
        }
#ifdef WITH_LIBVISIO
        if(Glib::str_has_suffix(filename, ".vss")) {
            std::size_t found = filename.find_last_of("/\\");
            filename = filename.substr(found+1);
            title = filename.erase(filename.rfind('.'));
            if(title.empty()) {
              title = _("Unnamed Symbols");
            }
            symbol_sets[title]= NULL;
            symbol_set->append(title);
            ++number_docs;
        }
#endif
    }
}

/* Hunts preference directories for symbol files */
std::pair<Glib::ustring, SPDocument*>
SymbolsDialog::getSymbolsSet(Glib::ustring title) 
{
    if (symbol_sets[title]) {
      return std::make_pair(title, symbol_sets[title]);
    }
    using namespace Inkscape::IO::Resource;
    SPDocument* symbol_doc = NULL;
    Glib::ustring new_title;
    size_t i = 0;
    for(auto &filename: get_filenames(SYMBOLS, {".svg", ".vss"})) {
        std::size_t pos = filename.find_last_of("/\\");
        if (pos != std::string::npos) {
          Glib::ustring filename_short = filename.substr(pos+1);
          if(filename_short == title + ".svg") {
              symbol_doc = SPDocument::createNewDoc(filename.c_str(), FALSE);
              if(symbol_doc) {
                  new_title = documentTitle(symbol_doc);
              }
          }
          if(Glib::str_has_suffix(filename, ".svg")) {
            i++;
          }
#ifdef WITH_LIBVISIO
          if(filename_short == title + ".vss") {
              symbol_doc = read_vss(filename, title);
              if(symbol_doc) {
                  new_title = documentTitle(symbol_doc);
              }
          }
          if(Glib::str_has_suffix(filename, ".vss")) {
            i++;
          }
#endif
          if(symbol_doc) {
              symbol_sets.erase(title);
              symbol_sets[new_title]= symbol_doc;
              sensitive = false;
              symbol_set->remove_text(i+1);
              symbol_set->insert (i+1, new_title);
              symbol_set->set_active(i+1);
              symbol_set->set_active_text(new_title);
              sensitive = true;
              break;
          }
      }
    }
    return std::make_pair(new_title, symbol_doc);
}

void SymbolsDialog::symbolsInDocRecursive (SPObject *r, std::vector<std::pair<Glib::ustring, SPSymbol*> > &l, Glib::ustring doc_title)
{
  if(!r) return;

  // Stop multiple counting of same symbol
  if ( dynamic_cast<SPUse *>(r) ) {
    return;
  }

  if ( dynamic_cast<SPSymbol *>(r) ) {
    l.push_back(std::make_pair(doc_title,dynamic_cast<SPSymbol *>(r)));
  }

  for (auto& child: r->children) {
    symbolsInDocRecursive(&child, l, doc_title);
  }
}

std::vector<std::pair<Glib::ustring, SPSymbol*> > 
SymbolsDialog::symbolsInDoc( SPDocument* symbol_document, Glib::ustring doc_title)
{

  std::vector<std::pair<Glib::ustring, SPSymbol*> > l;
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

  gchar const* style = 0;
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
  progress_bar->set_fraction(0.0);
  enableWidgets(false);
  SPDocument* symbol_document = selectedSymbols();
  if (symbol_document) {
    //We are not in search all docs
    search->set_text(_("Searching..."));
    store->clear();
    icons_found = false;
    addSymbolsInDoc(symbol_document);
  } else {
    search->set_text(_("Loading documents..."));
  }
}

void SymbolsDialog::unsensitive(GdkEventKey* evt) 
{
  sensitive = true;
}

bool SymbolsDialog::callbackSymbols(){
  Glib::ustring current = symbol_set->get_active_text();
  if (current == _("All symbols sets")  &&
      search->get_text() != _("Loading documents...")) 
  {
    if (!all_docs_processed) {
      overlay_opacity->show();
      overlay_icon->set_from_icon_name("none", iconsize);
      overlay_icon->show();
      overlay_title->show();
      overlay_desc->show();
      overlay_title->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"x-large\">") + Glib::ustring(_("Searching in all symbol sets ...")) + Glib::ustring("</span>"));
      overlay_desc->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"medium\">") + Glib::ustring(_("When run for the first time, search will be slow.\nPlease wait ...")) + Glib::ustring("</span>"));
    }
  }
  if (current == _("Current Document") && !icons_found) {
    if (!all_docs_processed) {
      overlay_icon->set_from_icon_name("none", iconsize);
      overlay_opacity->show();
      overlay_icon->show();
      overlay_title->show();
      overlay_desc->show();
      overlay_title->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"x-large\">") + Glib::ustring(_("No results found")) + Glib::ustring("</span>"));
      overlay_desc->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"medium\">") + Glib::ustring(_("You could try a different search term,\nor switch to a different symbol set.")) + Glib::ustring("</span>"));
    }
  }
  if (current == _("All symbols sets") &&
      search->get_text() == _("Loading documents...") ) 
  {
    overlay_opacity->show();
    if (!all_docs_processed) {
      overlay_title->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"x-large\">") + Glib::ustring(_("Loading all symbol sets ...")) + Glib::ustring("</span>"));
      overlay_desc->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"medium\">") + Glib::ustring(_("When run for the first time, search will be slow.\nPlease wait ...")) + Glib::ustring("</span>"));
      overlay_icon->show();
      overlay_title->show();
      overlay_icon->set_from_icon_name("searching", iconsize);
      overlay_desc->show();
    }
    size_t counter = 0;
    for(auto const &symbol_document_map : symbol_sets) {
      ++counter;
      SPDocument* symbol_document = symbol_document_map.second;
      if (symbol_document) {
        continue;
      }
      symbol_document = getSymbolsSet(symbol_document_map.first).second;
      symbol_set->set_active_text(_("All symbols sets"));
      if (!symbol_document) {
        continue;
      }
      progress_bar->set_fraction(((100.0/number_docs) * counter)/100.0);
      return true;
    }
    overlay_icon->hide();
    overlay_title->hide();
    overlay_desc->hide();
    progress_bar->set_fraction(1.0);
    all_docs_processed = true;
    addSymbols();
    search->set_text("Documents done, searchig inside...");
    return true;
  } else if (l.size()) {
    overlay_opacity->show();
    overlay_icon->hide();
    overlay_title->hide();
    overlay_desc->hide();
    for (auto symbol_data = l.begin(); symbol_data != l.end();) {
      Glib::ustring doc_title = symbol_data->first;
      SPSymbol * symbol = symbol_data->second;
      counter_symbols ++;
      gchar const *symbol_title_char = symbol->title();
      gchar const *symbol_desc_char = symbol->description();
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
      if (symbol && (search_str.empty() || found || (search_str.empty() && !symbol_title_char))) {
        addSymbol( symbol, doc_title);
        icons_found = true;
      }

      progress_bar->set_fraction(((100.0/number_symbols) * counter_symbols)/100.0);
      symbol_data = l.erase(l.begin());
      //to get more items and best performance
      int modulus = number_symbols > 200 ? 50 : (number_symbols/4);
      if (modulus && counter_symbols % modulus == 0 && !l.empty()) { 
        return true;
      }
    }
    if (!icons_found && !search_str.empty()) {
      overlay_title->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"x-large\">") + Glib::ustring(_("No results found")) + Glib::ustring("</span>"));
      overlay_desc->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"medium\">") + Glib::ustring(_("You could try a different search term,\nor switch to a different symbol set.")) + Glib::ustring("</span>"));
      overlay_icon->set_from_icon_name("none", iconsize);
      overlay_icon->show();
      overlay_title->show();
      overlay_desc->show();
    } else {
      overlay_opacity->hide();
    }
    sensitive = false;
    search->set_text(search_str);
    sensitive = true;
    enableWidgets(true);
    return true;
  }
  return true;
}

Glib::ustring SymbolsDialog::ellipsize(Glib::ustring data, size_t limit) {
    if (data.length() > limit) {
      return data.substr(0,limit-3) + "...";
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
  std::vector<std::pair<Glib::ustring, SPSymbol*> > container_symbols_tmp = symbolsInDoc(symbol_document, doc_title);
  number_symbols = container_symbols_tmp.size();
  l = container_symbols_tmp;
  container_symbols_tmp.clear();
  if (!number_symbols) {
    overlay_icon->set_from_icon_name("none", iconsize);
    overlay_icon->show();
    overlay_title->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"x-large\">") + Glib::ustring(_("No results found")) + Glib::ustring("</span>"));
    overlay_desc->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"medium\">") + Glib::ustring(_("You could try a different search term,\nor switch to a different symbol set.")) + Glib::ustring("</span>"));
    overlay_title->show();
    overlay_desc->show();
    sensitive = false;
    search->set_text(search_str);
    sensitive = true;
    enableWidgets(true);
  }
}

void SymbolsDialog::addSymbols() {
  store->clear();
  icons_found = false;
  std::vector<std::pair<Glib::ustring, SPSymbol*> > container_symbols;
  for(auto const &symbol_document_map : symbol_sets) {
    SPDocument* symbol_document = symbol_document_map.second;
    if (!symbol_document) {
      continue;
    }
    Glib::ustring doc_title = documentTitle(symbol_document);
    std::vector<std::pair<Glib::ustring, SPSymbol*> > container_symbols_tmp = symbolsInDoc(symbol_document, doc_title);
    container_symbols.insert(container_symbols.end(), std::make_move_iterator(container_symbols_tmp.begin()), std::make_move_iterator(container_symbols_tmp.end()));
    container_symbols_tmp.clear();
  }
  counter_symbols = 0;
  progress_bar->set_fraction(0.0);
  number_symbols = container_symbols.size();
  l = container_symbols;
  container_symbols.clear();
  if (!number_symbols) {
    overlay_icon->set_from_icon_name("none", iconsize);
    overlay_title->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"x-large\">") + Glib::ustring(_("No results found")) + Glib::ustring("</span>"));
    overlay_desc->set_markup(Glib::ustring("<span foreground=\"#333333\" size=\"medium\">") + Glib::ustring(_("You could try a different search term,\nor switch to a different symbol set.")) + Glib::ustring("</span>"));
    overlay_icon->show();
    overlay_title->show();
    overlay_desc->show();
    sensitive = false;
    search->set_text(search_str);
    sensitive = true;
    enableWidgets(true);
  }
}

void SymbolsDialog::addSymbol( SPObject* symbol, Glib::ustring doc_title) {

  SymbolColumns* columns = getColumns();

  gchar const *id    = symbol->getRepr()->attribute("id");
  gchar const *title = symbol->title(); // From title element
  if( !title ) {
    title = id;
  }
  Glib::ustring symbol_title = Glib::Markup::escape_text(Glib::ustring( g_dpgettext2(NULL, "Symbol", title) ));
  if (doc_title.empty()) {
    doc_title = _("Current Document");
  }
  symbol_title = symbol_title + Glib::Markup::escape_text(Glib::ustring(g_dpgettext2(NULL, "Symbol", (Glib::ustring(" (") + doc_title + Glib::ustring(")")).c_str())));
  Glib::RefPtr<Gdk::Pixbuf> pixbuf = drawSymbol( symbol );

  if( pixbuf ) {
    Gtk::ListStore::iterator row = store->append();
    (*row)[columns->symbol_id]    = Glib::ustring( id );
    (*row)[columns->symbol_title] = symbol_title;
    (*row)[columns->symbol_doc_title] = Glib::Markup::escape_text(doc_title);
    (*row)[columns->symbol_image] = pixbuf;
  }

  delete columns;
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
SymbolsDialog::drawSymbol(SPObject *symbol, unsigned force_psize)
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
  g_assert(item != NULL);
  unsigned psize = SYMBOL_ICON_SIZES[pack_size];

  Glib::RefPtr<Gdk::Pixbuf> pixbuf(NULL);
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

    if (force_psize > 0) {
      psize = force_psize;
      scale = psize / ceil(std::max(width, height)); 
    }
     
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
SymbolsDialog::getOverlay(Gtk::Image* image, gchar const * icon_title, unsigned psize)
{
gchar const *buffer =
"<svg xmlns=\"http://www.w3.org/2000/svg\""
"     xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\""
"     xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\""
"     xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
"  <title>Inkscape</title> "
"  <defs id=\"defs\">"
"    <symbol"
"       id=\"overlay\">"
"      <title"
"         id=\"overlay_title\">Overlay</title>"
"      <desc"
"         id=\"overlay_desc\">Overlay Square</desc>"
"      <path"
"         style=\"fill:#ffffff;opacity:0.75;stroke:none\""
"         d=\"M 0,1 H 1 V 2 H 0 Z\""
"         id=\"overlay_shape_1\" />"
"    </symbol>"
"  </defs>"
"</svg>";
  
  SPDocument* doc = SPDocument::createNewDocFromMem( buffer, strlen(buffer), FALSE );
  std::vector<std::pair<Glib::ustring, SPSymbol*> > symbols_data = symbolsInDoc(doc, "Overlay Doc");
  Glib::RefPtr<Gdk::Pixbuf> pixbuf(NULL);
  for(auto data:symbols_data) {
    Glib::ustring doc_title = data.first;
    SPSymbol * symbol = data.second;
    if (!strcmp(symbol->getId(), icon_title)) {
      pixbuf = drawSymbol(symbol, psize);
      return pixbuf;
    }
  }
  return pixbuf;
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
