// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Paint Servers dialog
 */
/* Authors:
 *   Valentin Ionita
 *
 * Copyright (C) 2019 Valentin Ionita
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include <iostream>
#include <map>
#include <utility>

#include <giomm/listmodel.h>
#include <glibmm/regex.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/iconview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/stockid.h>
#include <gtkmm/switch.h>

#include "document.h"
#include "inkscape.h"
#include "paint-servers.h"
#include "path-prefix.h"
#include "style.h"
#include "verbs.h"

#include "io/resource.h"
#include "object/sp-defs.h"
#include "object/sp-hatch.h"
#include "object/sp-pattern.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "ui/cache/svg_preview_cache.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

static Glib::ustring const wrapper = R"=====(
<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <defs id="Defs"/>
  <rect id="Back" x="0" y="0" width="100px" height="100px" fill="lightgray"/>
  <rect id="Rect" x="0" y="0" width="100px" height="100px" stroke="black"/>
</svg>
)=====";

class PaintServersColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    Gtk::TreeModelColumn<Glib::ustring> id;
    Gtk::TreeModelColumn<Glib::ustring> paint;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> pixbuf;
    Gtk::TreeModelColumn<Glib::ustring> document;

    PaintServersColumns() {
    add(id);
    add(paint);
    add(pixbuf);
    add(document);
    }
};

PaintServersColumns *PaintServersDialog::getColumns() { return new PaintServersColumns(); }

// Constructor
PaintServersDialog::PaintServersDialog(gchar const *prefsPath)
    : Inkscape::UI::Widget::Panel(prefsPath, SP_VERB_DIALOG_PAINT)
    , desktop(SP_ACTIVE_DESKTOP)
    , target_selected(true)
    , ALLDOCS(_("All paint servers"))
    , CURRENTDOC(_("Current document"))
{
    current_store = ALLDOCS;

    store[ALLDOCS] = Gtk::ListStore::create(*getColumns());
    store[CURRENTDOC] = Gtk::ListStore::create(*getColumns());

    // Grid holding the contents
    Gtk::Grid *grid = Gtk::manage(new Gtk::Grid());
    grid->set_margin_start(3);
    grid->set_margin_end(3);
    grid->set_margin_top(3);
    grid->set_row_spacing(3);
    _getContents()->pack_start(*grid, Gtk::PACK_EXPAND_WIDGET);

    // Grid row 0
    Gtk::Label *file_label = Gtk::manage(new Gtk::Label(Glib::ustring(_("Server")) + ": "));
    grid->attach(*file_label, 0, 0, 1, 1);

    dropdown = Gtk::manage(new Gtk::ComboBoxText());
    dropdown->append(ALLDOCS);
    dropdown->append(CURRENTDOC);
    document_map[CURRENTDOC] = desktop->getDocument();
    dropdown->set_active_text(ALLDOCS);
    dropdown->set_hexpand();
    grid->attach(*dropdown, 1, 0, 1, 1);

    // Grid row 1
    Gtk::Label *fill_label = Gtk::manage(new Gtk::Label(Glib::ustring(_("Change")) + ": "));
    grid->attach(*fill_label, 0, 1, 1, 1);

    target_dropdown = Gtk::manage(new Gtk::ComboBoxText());
    target_dropdown->append(_("Fill"));
    target_dropdown->append(_("Stroke"));
    target_dropdown->set_active_text(_("Fill"));
    target_dropdown->set_hexpand();
    grid->attach(*target_dropdown, 1, 1, 1, 1);

    // Grid row 2
    icon_view = Gtk::manage(new Gtk::IconView(
        static_cast<Glib::RefPtr<Gtk::TreeModel>>(store[current_store])
    ));
    icon_view->set_tooltip_column(0);
    icon_view->set_pixbuf_column(2);
    icon_view->set_size_request(200, 300);
    icon_view->show_all_children();
    icon_view->set_selection_mode(Gtk::SELECTION_SINGLE);
    icon_view->set_activate_on_single_click(true);

    Gtk::ScrolledWindow *scroller = Gtk::manage(new Gtk::ScrolledWindow());
    scroller->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    scroller->set_hexpand();
    scroller->set_vexpand();
    scroller->add(*icon_view);
    grid->attach(*scroller, 0, 2, 2, 1);

    // Events
    target_dropdown->signal_changed().connect(
        sigc::mem_fun(*this, &PaintServersDialog::on_target_changed)
    );

    dropdown->signal_changed().connect(
        sigc::mem_fun(*this, &PaintServersDialog::on_document_changed)
    );

    icon_view->signal_item_activated().connect(
        sigc::mem_fun(*this, &PaintServersDialog::on_item_activated)
    );

    desktop->getDocument()->getDefs()->connectModified(
        sigc::mem_fun(*this, &PaintServersDialog::load_current_document)
    );

    // Get wrapper document (rectangle to fill with paint server).
    preview_document = SPDocument::createNewDocFromMem(wrapper.c_str(), wrapper.length(), true);

    SPObject *rect = preview_document->getObjectById("Rect");
    SPObject *defs = preview_document->getObjectById("Defs");
    if (!rect || !defs) {
        std::cerr << "PaintServersDialog::PaintServersDialog: Failed to get wrapper defs or rectangle!!" << std::endl;
    }

    // Set up preview document.
    unsigned key = SPItem::display_key_new(1);
    preview_document->getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    preview_document->ensureUpToDate();
    renderDrawing.setRoot(preview_document->getRoot()->invoke_show(renderDrawing, key, SP_ITEM_SHOW_DISPLAY));

    // Load paint servers from resource files
    load_sources();
}

PaintServersDialog::~PaintServersDialog() = default;

// Get url or color value.
Glib::ustring get_url(Glib::ustring paint)
{

    Glib::MatchInfo matchInfo;

    // Paint server
    static Glib::RefPtr<Glib::Regex> regex1 = Glib::Regex::create(":(url\\(#([A-z0-9\\-_\\.#])*\\))");
    regex1->match(paint, matchInfo);

    if (matchInfo.matches()) {
        return matchInfo.fetch(1);
    }

    // Color
    static Glib::RefPtr<Glib::Regex> regex2 = Glib::Regex::create(":(([A-z0-9#])*)");
    regex2->match(paint, matchInfo);

    if (matchInfo.matches()) {
        return matchInfo.fetch(1);
    }

    return Glib::ustring();
}

// This is too complicated to use selectors!
void recurse_find_paint(SPObject* in, std::vector<Glib::ustring>& list)
{

    g_return_if_fail(in != nullptr);

    // Add paint servers in <defs> section.
    if (dynamic_cast<SPPaintServer *>(in)) {
        if (in->getId()) {
            // Need to check as one can't construct Glib::ustring with nullptr.
            list.push_back (Glib::ustring("url(#") + in->getId() + ")");
        }
        // Don't recurse into paint servers.
        return;
    }

    // Add paint servers referenced by shapes.
    if (dynamic_cast<SPShape *>(in)) {
        list.push_back (get_url(in->style->fill.write()));
        list.push_back (get_url(in->style->stroke.write()));
    }

    for (auto child: in->childList(false)) {
        recurse_find_paint(child, list);
    }
}

// Load paint servers from all the files associated
void PaintServersDialog::load_sources()
{

    // Extract paints from the current file
    load_document(desktop->getDocument());

    // Extract out paints from files in share/paint.
    for (auto &path : get_filenames(Inkscape::IO::Resource::PAINT, { ".svg" })) {
        SPDocument *document = SPDocument::createNewDoc(path.c_str(), FALSE);

        load_document(document);
    }
}

Glib::RefPtr<Gdk::Pixbuf> PaintServersDialog::get_pixbuf(SPDocument *document, Glib::ustring paint, Glib::ustring *id)
{

    SPObject *rect = preview_document->getObjectById("Rect");
    SPObject *defs = preview_document->getObjectById("Defs");

    Glib::RefPtr<Gdk::Pixbuf> pixbuf(nullptr);
    if (paint.empty()) {
        return pixbuf;
    }

    // Set style on wrapper
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_set_property(css, "fill", paint.c_str());
    rect->changeCSS(css, "style");
    sp_repr_css_attr_unref(css);

    // Insert paint into defs if required
    Glib::MatchInfo matchInfo;
    static Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("url\\(#([A-Za-z0-9#._-]*)\\)");
    regex->match(paint, matchInfo);
    if (matchInfo.matches()) {
        *id = matchInfo.fetch(1);

        // Delete old paint if necessary
        std::vector<SPObject *> old_paints = preview_document->getObjectsBySelector("defs > *");
        for (auto paint : old_paints) {
            paint->deleteObject(false);
        }

        // Add new paint
        SPObject *new_paint = document->getObjectById(*id);
        if (!new_paint) {
            std::cerr << "PaintServersDialog::load_document: cannot find paint server: " << id << std::endl;
            return pixbuf;
        }

        // Create a copy repr of the paint
        Inkscape::XML::Document *xml_doc = preview_document->getReprDoc();
        Inkscape::XML::Node *repr = new_paint->getRepr()->duplicate(xml_doc);
        defs->appendChild(repr);
    } else {
        // Temporary block solid color fills.
        return pixbuf;
    }

    preview_document->getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    preview_document->ensureUpToDate();

    Geom::OptRect dbox = dynamic_cast<SPItem *>(rect)->visualBounds();

    if (!dbox) {
        return pixbuf;
    }

    double size = std::max(dbox->width(), dbox->height());

    pixbuf = Glib::wrap(render_pixbuf(renderDrawing, 1, *dbox, size));

    return pixbuf;
}

// Load paint server from the given document
void PaintServersDialog::load_document(SPDocument *document)
{
    PaintServersColumns *columns = getColumns();
    Glib::ustring document_title;
    if (!document->getRoot()->title()) {
        document_title = CURRENTDOC;
    } else {
        document_title = Glib::ustring(document->getRoot()->title());
    }
    bool has_server_elements = false;

    // Find all paints
    std::vector<Glib::ustring> paints;
    recurse_find_paint(document->getRoot(), paints);

    // Sort and remove duplicates.
    std::sort(paints.begin(), paints.end());
    paints.erase(std::unique(paints.begin(), paints.end()), paints.end());

    if (paints.size() && store.find(document_title) == store.end()) {
        store[document_title] = Gtk::ListStore::create(*getColumns());
    }

    // iterating though servers
    for (auto paint : paints) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf(nullptr);
        Glib::ustring id;
        pixbuf = get_pixbuf(document, paint, &id);
        if (!pixbuf) {
            continue;
        }

        // Save as a ListStore column
        Gtk::ListStore::iterator iter = store[ALLDOCS]->append();
        (*iter)[columns->id] = id;
        (*iter)[columns->paint] = paint;
        (*iter)[columns->pixbuf] = pixbuf;
        (*iter)[columns->document] = document_title;

        iter = store[document_title]->append();
        (*iter)[columns->id] = id;
        (*iter)[columns->paint] = paint;
        (*iter)[columns->pixbuf] = pixbuf;
        (*iter)[columns->document] = document_title;
        has_server_elements = true;
    }

    if (has_server_elements && document_map.find(document_title) == document_map.end()) {
        document_map[document_title] = document;
        dropdown->append(document_title);
    }
}

void PaintServersDialog::load_current_document(SPObject * /*object*/, guint /*flags*/)
{
    std::unique_ptr<PaintServersColumns> columns(getColumns());
    SPDocument *document = desktop->getDocument();
    Glib::RefPtr<Gtk::ListStore> current = store[CURRENTDOC];

    std::vector<Glib::ustring> paints;
    std::vector<Glib::ustring> paints_current;
    std::vector<Glib::ustring> paints_missing;
    recurse_find_paint(document->getDefs(), paints);

    std::sort(paints.begin(), paints.end());
    paints.erase(std::unique(paints.begin(), paints.end()), paints.end());

    // Delete the server from the store if it doesn't exist in the current document
    for (auto iter = current->children().begin(); iter != current->children().end();) {
        Gtk::TreeRow server = *iter;

        if (std::find(paints.begin(), paints.end(), server[columns->paint]) == paints.end()) {
            iter = current->erase(server);
        } else {
            paints_current.push_back(server[columns->paint]);
            iter++;
        }
    }

    for (auto s : paints) {
        if (std::find(paints_current.begin(), paints_current.end(), s) == paints_current.end()) {
            std::cout << "missing " << s << std::endl;
            paints_missing.push_back(s);
        }
    }

    if (!paints_missing.size()) {
        return;
    }

    for (auto paint : paints_missing) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf(nullptr);
        Glib::ustring id;
        pixbuf = get_pixbuf(document, paint, &id);
        if (!pixbuf) {
            continue;
        }

        Gtk::ListStore::iterator iter = current->append();
        (*iter)[columns->id] = id;
        (*iter)[columns->paint] = paint;
        (*iter)[columns->pixbuf] = pixbuf;
        (*iter)[columns->document] = CURRENTDOC;
    }
}

void PaintServersDialog::on_target_changed()
{
    target_selected = !target_selected;
}

void PaintServersDialog::on_document_changed()
{
    current_store = dropdown->get_active_text();
    icon_view->set_model(store[current_store]);
}

void PaintServersDialog::on_item_activated(const Gtk::TreeModel::Path& path)
{
    // Get the current selected elements
    Selection *selection = desktop->getSelection();
    std::vector<SPObject*> const selected_items(selection->items().begin(), selection->items().end());

    if (!selected_items.size()) {
        return;
    }

    PaintServersColumns *columns = getColumns();
    Gtk::ListStore::iterator iter = store[current_store]->get_iter(path);
    Glib::ustring id = (*iter)[columns->id];
    Glib::ustring paint = (*iter)[columns->paint];
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = (*iter)[columns->pixbuf];
    Glib::ustring document_title = (*iter)[columns->document];
    SPDocument *document = document_map[document_title];
    SPObject *paint_server = document->getObjectById(id);
    SPDocument *document_target = desktop->getDocument();

    bool paint_server_exists = false;
    for (auto server : store[CURRENTDOC]->children()) {
        if (server[columns->id] == id) {
            paint_server_exists = true;
            break;
        }
    }

    if (!paint_server_exists) {
        // Add the paint server to the current document definition
        Inkscape::XML::Document *xml_doc = document_target->getReprDoc();
        Inkscape::XML::Node *repr = paint_server->getRepr()->duplicate(xml_doc);
        document_target->getDefs()->appendChild(repr);
        Inkscape::GC::release(repr);

        // Add the pixbuf to the current document store
        iter = store[CURRENTDOC]->append();
        (*iter)[columns->id] = id;
        (*iter)[columns->paint] = paint;
        (*iter)[columns->pixbuf] = pixbuf;
        (*iter)[columns->document] = document_title;
    }

    // Recursively find elements in groups, if any
    std::vector<SPObject*> items;
    for (auto item : selected_items) {
        std::vector<SPObject*> current_items = extract_elements(item);
        items.insert(std::end(items), std::begin(current_items), std::end(current_items));
    }

    for (auto item : items) {
        item->style->getFillOrStroke(target_selected)->read(paint.c_str());
        item->updateRepr();
    }

    document_target->collectOrphans();
}

std::vector<SPObject*> PaintServersDialog::extract_elements(SPObject* item)
{
    std::vector<SPObject*> elements;
    std::vector<SPObject*> children = item->childList(false);
    if (!children.size()) {
        elements.push_back(item);
    } else {
        for (auto e : children) {
            std::vector<SPObject*> current_items = extract_elements(e);
            elements.insert(std::end(elements), std::begin(current_items), std::end(current_items));
        }
    }

    return elements;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
