// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Combobox for selecting dash patterns - implementation.
 */
/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "marker-combo-box.h"

#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <gtkmm/icontheme.h>

#include "desktop-style.h"
#include "path-prefix.h"

#include "helper/stock-items.h"
#include "ui/icon-loader.h"

#include "io/resource.h"
#include "io/sys.h"

#include "object/sp-defs.h"
#include "object/sp-marker.h"
#include "object/sp-root.h"
#include "style.h"

#include "ui/cache/svg_preview_cache.h"
#include "ui/dialog-events.h"
#include "ui/util.h"

#include "ui/widget/spinbutton.h"
#include "ui/widget/stroke-style.h"

static Inkscape::UI::Cache::SvgPreview svg_preview_cache;

namespace Inkscape {
namespace UI {
namespace Widget {

MarkerComboBox::MarkerComboBox(gchar const *id, int l) :
            combo_id(id),
            loc(l),
            updating(false),
            markerCount(0)
{

    marker_store = Gtk::ListStore::create(marker_columns);
    set_model(marker_store);
    pack_start(image_renderer, false);
    set_cell_data_func(image_renderer, sigc::mem_fun(*this, &MarkerComboBox::prepareImageRenderer));
    gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(gobj()), MarkerComboBox::separator_cb, nullptr, nullptr);
    empty_image = sp_get_icon_image("no-marker", Gtk::ICON_SIZE_SMALL_TOOLBAR);

    sandbox = ink_markers_preview_doc ();

    init_combo();
    this->get_style_context()->add_class("combobright");

    show();
}

MarkerComboBox::~MarkerComboBox() {
    delete combo_id;
    delete sandbox;
    delete empty_image;

    if (doc) {
        modified_connection.disconnect();
    }
}

void MarkerComboBox::setDocument(SPDocument *document)
{
    if (doc != document) {

        if (doc) {
            modified_connection.disconnect();
        }

        doc = document;

        if (doc) {
            modified_connection = doc->getDefs()->connectModified( sigc::hide(sigc::hide(sigc::bind(sigc::ptr_fun(&MarkerComboBox::handleDefsModified), this))) );
        }

        refreshHistory();
    }
}

void
MarkerComboBox::handleDefsModified(MarkerComboBox *self)
{
    self->refreshHistory();
}

void
MarkerComboBox::refreshHistory()
{
    if (updating)
        return;

    updating = true;

    std::vector<SPMarker *> ml = get_marker_list(doc);

    /*
     * Seems to be no way to get notified of changes just to markers,
     * so listen to changes in all defs and check if the number of markers has changed here
     * to avoid unnecessary refreshes when things like gradients change
    */
    if (markerCount != ml.size()) {
        auto iter = get_active();
        const char *active = iter ? iter->get_value(marker_columns.marker) : nullptr;
        sp_marker_list_from_doc(doc, true);
        set_selected(active);
        markerCount = ml.size();
    }

    updating = false;
}

/**
 * Init the combobox widget to display markers from markers.svg
 */
void
MarkerComboBox::init_combo()
{
    if (updating)
        return;

    static SPDocument *markers_doc = nullptr;

    // add separator
    Gtk::TreeModel::Row row_sep = *(marker_store->append());
    row_sep[marker_columns.label] = "Separator";
    row_sep[marker_columns.marker] = g_strdup("None");
    row_sep[marker_columns.image] = NULL;
    row_sep[marker_columns.stock] = false;
    row_sep[marker_columns.history] = false;
    row_sep[marker_columns.separator] = true;

    // find and load markers.svg
    if (markers_doc == nullptr) {
        using namespace Inkscape::IO::Resource;
        auto markers_source = get_path_string(SYSTEM, MARKERS, "markers.svg");
        if (Glib::file_test(markers_source, Glib::FILE_TEST_IS_REGULAR)) {
            markers_doc = SPDocument::createNewDoc(markers_source.c_str(), false);
        }
    }

    // load markers from markers.svg
    if (markers_doc) {
        sp_marker_list_from_doc(markers_doc, false);
    }

    set_sensitive(true);
}

/**
 * Sets the current marker in the marker combobox.
 */
void MarkerComboBox::set_current(SPObject *marker)
{
    updating = true;

    if (marker != nullptr) {
        gchar *markname = g_strdup(marker->getRepr()->attribute("id"));
        set_selected(markname);
        g_free (markname);
    }
    else {
        set_selected(nullptr);
    }

    updating = false;

}
/**
 * Return a uri string representing the current selected marker used for setting the marker style in the document
 */
const gchar * MarkerComboBox::get_active_marker_uri()
{
    /* Get Marker */
    const gchar *markid = get_active()->get_value(marker_columns.marker);
    if (!markid)
    {
        return nullptr;
    }

    gchar const *marker = "";
    if (strcmp(markid, "none")) {
        bool stockid = get_active()->get_value(marker_columns.stock);

        gchar *markurn;
        if (stockid)
        {
            markurn = g_strconcat("urn:inkscape:marker:",markid,NULL);
        }
        else
        {
            markurn = g_strdup(markid);
        }
        SPObject *mark = get_stock_item(markurn, stockid);
        g_free(markurn);
        if (mark) {
            Inkscape::XML::Node *repr = mark->getRepr();
            marker = g_strconcat("url(#", repr->attribute("id"), ")", NULL);
        }
    } else {
        marker = g_strdup(markid);
    }

    return marker;
}

void MarkerComboBox::set_selected(const gchar *name, gboolean retry/*=true*/) {

    if (!name) {
        set_active(0);
        return;
    }

    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.marker] &&
                    !strcmp(row[marker_columns.marker], name)) {
                set_active(iter);
                return;
            }
    }

    // Didn't find it in the list, try refreshing from the doc
    if (retry) {
        sp_marker_list_from_doc(doc, true);
        set_selected(name, false);
    }
}


/**
 * Pick up all markers from source, except those that are in
 * current_doc (if non-NULL), and add items to the combo.
 */
void MarkerComboBox::sp_marker_list_from_doc(SPDocument *source, gboolean history)
{
    std::vector<SPMarker *> ml = get_marker_list(source);

    remove_markers(history); // Seem to need to remove 2x
    remove_markers(history);
    add_markers(ml, source, history);
}

/**
 *  Returns a list of markers in the defs of the given source document as a vector
 *  Returns NULL if there are no markers in the document.
 */
std::vector<SPMarker *> MarkerComboBox::get_marker_list (SPDocument *source)
{
    std::vector<SPMarker *> ml;
    if (source == nullptr)
        return ml;

    SPDefs *defs = source->getDefs();
    if (!defs) {
        return ml;
    }

    for (auto& child: defs->children)
    {
        if (SP_IS_MARKER(&child)) {
            ml.push_back(SP_MARKER(&child));
        }
    }
    return ml;
}

/**
 * Remove history or non-history markers from the combo
 */
void MarkerComboBox::remove_markers (gboolean history)
{
    // Having the model set causes assertions when erasing rows, temporarily disconnect
    unset_model();
    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.history] == history && row[marker_columns.separator] == false) {
                marker_store->erase(iter);
                iter = marker_store->children().begin();
            }
    }

    set_model(marker_store);
}

/**
 * Adds markers in marker_list to the combo
 */
void MarkerComboBox::add_markers (std::vector<SPMarker *> const& marker_list, SPDocument *source, gboolean history)
{
    // Do this here, outside of loop, to speed up preview generation:
    Inkscape::Drawing drawing;
    unsigned const visionkey = SPItem::display_key_new(1);
    drawing.setRoot(sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
    // Find the separator,
    Gtk::TreeIter sep_iter;
    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.separator]) {
                sep_iter = iter;
            }
    }

    if (history) {
        // add "None"
        Gtk::TreeModel::Row row = *(marker_store->prepend());
        row[marker_columns.label] = C_("Marker", "None");
        row[marker_columns.stock] = false;
        row[marker_columns.marker] = g_strdup("None");
        row[marker_columns.image] = NULL;
        row[marker_columns.history] = true;
        row[marker_columns.separator] = false;
    }

    for (auto i:marker_list) {

        Inkscape::XML::Node *repr = i->getRepr();
        gchar const *markid = repr->attribute("inkscape:stockid") ? repr->attribute("inkscape:stockid") : repr->attribute("id");

        // generate preview
        Gtk::Image *prv = create_marker_image (24, repr->attribute("id"), source, drawing, visionkey);
        prv->show();

        // Add history before separator, others after
        Gtk::TreeModel::Row row;
        if (history)
            row = *(marker_store->insert(sep_iter));
        else
            row = *(marker_store->append());

        row[marker_columns.label] = ink_ellipsize_text(markid, 20);
        // Non "stock" markers can also have "inkscape:stockid" (when using extension ColorMarkers),
        // So use !is_history instead to determine is it is "stock" (ie in the markers.svg file)
        row[marker_columns.stock] = !history;
        row[marker_columns.marker] = repr->attribute("id");
        row[marker_columns.image] = prv;
        row[marker_columns.history] = history;
        row[marker_columns.separator] = false;

    }

    sandbox->getRoot()->invoke_hide(visionkey);
}

/*
 * Remove from the cache and recreate a marker image
 */
void
MarkerComboBox::update_marker_image(gchar const *mname)
{
    gchar *cache_name = g_strconcat(combo_id, mname, NULL);
    Glib::ustring key = svg_preview_cache.cache_key(doc->getDocumentURI(), cache_name, 24);
    g_free (cache_name);
    svg_preview_cache.remove_preview_from_cache(key);

    Inkscape::Drawing drawing;
    unsigned const visionkey = SPItem::display_key_new(1);
    drawing.setRoot(sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
    Gtk::Image *prv = create_marker_image(24, mname, doc, drawing, visionkey);
    if (prv) {
        prv->show();
    }
    sandbox->getRoot()->invoke_hide(visionkey);

    for(const auto & iter : marker_store->children()) {
            Gtk::TreeModel::Row row = iter;
            if (row[marker_columns.marker] && row[marker_columns.history] &&
                    !strcmp(row[marker_columns.marker], mname)) {
                row[marker_columns.image] = prv;
                return;
            }
    }

}
/**
 * Creates a copy of the marker named mname, determines its visible and renderable
 * area in the bounding box, and then renders it.  This allows us to fill in
 * preview images of each marker in the marker combobox.
 */
Gtk::Image *
MarkerComboBox::create_marker_image(unsigned psize, gchar const *mname,
                   SPDocument *source,  Inkscape::Drawing &drawing, unsigned /*visionkey*/)
{
    // Retrieve the marker named 'mname' from the source SVG document
    SPObject const *marker = source->getObjectById(mname);
    if (marker == nullptr) {
        return nullptr;
    }

    /* Get from cache right away */
    gchar *cache_name = g_strconcat(combo_id, mname, NULL);
    Glib::ustring key = svg_preview_cache.cache_key(source->getDocumentURI(), cache_name, psize);
    g_free (cache_name);
    GdkPixbuf *pixbuf = svg_preview_cache.get_preview_from_cache(key); // no ref created
    if(pixbuf) {
        Gtk::Image *pb = Glib::wrap(GTK_IMAGE(gtk_image_new_from_pixbuf(pixbuf)));
        return pb;
    }

    // Create a copy repr of the marker with id="sample"
    Inkscape::XML::Document *xml_doc = sandbox->getReprDoc();
    Inkscape::XML::Node *mrepr = marker->getRepr()->duplicate(xml_doc);
    mrepr->setAttribute("id", "sample");

    // Replace the old sample in the sandbox by the new one
    Inkscape::XML::Node *defsrepr = sandbox->getObjectById("defs")->getRepr();
    SPObject *oldmarker = sandbox->getObjectById("sample");
    if (oldmarker) {
        oldmarker->deleteObject(false);
    }

    // TODO - This causes a SIGTRAP on windows
    defsrepr->appendChild(mrepr);

    Inkscape::GC::release(mrepr);

    // If the marker color is a url link to a pattern or gradient copy that too
    SPObject *mk = source->getObjectById(mname);
    SPCSSAttr *css_marker = sp_css_attr_from_object(mk->firstChild(), SP_STYLE_FLAG_ALWAYS);
    //const char *mfill = sp_repr_css_property(css_marker, "fill", "none");
    const char *mstroke = sp_repr_css_property(css_marker, "fill", "none");

    if (!strncmp (mstroke, "url(", 4)) {
        SPObject *linkObj = getMarkerObj(mstroke, source);
        if (linkObj) {
            Inkscape::XML::Node *grepr = linkObj->getRepr()->duplicate(xml_doc);
            SPObject *oldmarker = sandbox->getObjectById(linkObj->getId());
            if (oldmarker) {
                oldmarker->deleteObject(false);
            }
            defsrepr->appendChild(grepr);
            Inkscape::GC::release(grepr);

            if (SP_IS_GRADIENT(linkObj)) {
                SPGradient *vector = sp_gradient_get_forked_vector_if_necessary (SP_GRADIENT(linkObj), false);
                if (vector) {
                    Inkscape::XML::Node *grepr = vector->getRepr()->duplicate(xml_doc);
                    SPObject *oldmarker = sandbox->getObjectById(vector->getId());
                    if (oldmarker) {
                        oldmarker->deleteObject(false);
                    }
                    defsrepr->appendChild(grepr);
                    Inkscape::GC::release(grepr);
                }
            }
        }
    }

// Uncomment this to get the sandbox documents saved (useful for debugging)
    //FILE *fp = fopen (g_strconcat(combo_id, mname, ".svg", NULL), "w");
    //sp_repr_save_stream(sandbox->getReprDoc(), fp);
    //fclose (fp);

    // object to render; note that the id is the same as that of the combo we're building
    SPObject *object = sandbox->getObjectById(combo_id);
    sandbox->getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    sandbox->ensureUpToDate();

    if (object == nullptr || !SP_IS_ITEM(object)) {
        return nullptr; // sandbox broken?
    }

    SPItem *item = SP_ITEM(object);
    // Find object's bbox in document
    Geom::OptRect dbox = item->documentVisualBounds();

    if (!dbox) {
        return nullptr;
    }

    /* Update to renderable state */
    pixbuf = render_pixbuf(drawing, 0.8, *dbox, psize);
    svg_preview_cache.set_preview_in_cache(key, pixbuf);
    g_object_unref(pixbuf); // reference is held by svg_preview_cache

    // Create widget
    Gtk::Image *pb = Glib::wrap(GTK_IMAGE(gtk_image_new_from_pixbuf(pixbuf)));
    return pb;
}

void MarkerComboBox::prepareImageRenderer( Gtk::TreeModel::const_iterator const &row ) {

    Gtk::Image *image = (*row)[marker_columns.image];
    if (image)
        image_renderer.property_pixbuf() = image->get_pixbuf();
    else
        image_renderer.property_pixbuf() = empty_image->get_pixbuf();
}

gboolean MarkerComboBox::separator_cb (GtkTreeModel *model, GtkTreeIter *iter, gpointer /*data*/) {

    gboolean sep = FALSE;
    gtk_tree_model_get(model, iter, 4, &sep, -1);
    return sep;
}

/**
 * Returns a new document containing default start, mid, and end markers.
 */
SPDocument *MarkerComboBox::ink_markers_preview_doc ()
{
gchar const *buffer = R"A(
    <svg xmlns="http://www.w3.org/2000/svg"
         xmlns:xlink="http://www.w3.org/1999/xlink"
         id="MarkerSample">

    <defs id="defs"/>

    <g id="marker-start">
      <path style="fill:white;stroke:black;stroke-width:1.7;marker-start:url(#sample)"
       d="M 12.5,13 L 25,13"/>
      <rect x="0" y="0" width="25" height="25" style="fill:none;stroke:none"/>
    </g>

    <g id="marker-mid">
      <path style="fill:white;stroke:black;stroke-width:1.7;marker-mid:url(#sample)"
       d="M 0,113 L 12.5,113 L 25,113"/>
      <rect x="0" y="100" width="25" height="25" style="fill:none;stroke:none"/>
    </g>

    <g id="marker-end">
      <path style="fill:white;stroke:black;stroke-width:1.7;marker-end:url(#sample)"
       d="M 0,213 L 12.5,213"/>
      <rect x="0" y="200" width="25" height="25" style="fill:none;stroke:none"/>
    </g>

  </svg>
)A";

    return SPDocument::createNewDocFromMem (buffer, strlen(buffer), FALSE);
}

} // namespace Widget
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
