// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SPDocument manipulation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2004-2005 MenTaLguY
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2012 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/** \class SPDocument
 * SPDocument serves as the container of both model trees (agnostic XML
 * and typed object tree), and implements all of the document-level
 * functionality used by the program. Many document level operations, like
 * load, save, print, export and so on, use SPDocument as their basic datatype.
 *
 * SPDocument implements undo and redo stacks and an id-based object
 * dictionary.  Thanks to unique id attributes, the latter can be used to
 * map from the XML tree back to the object tree.
 *
 * SPDocument performs the basic operations needed for asynchronous
 * update notification (SPObject ::modified virtual method), and implements
 * the 'modified' signal, as well.
 */


#define noSP_DOCUMENT_DEBUG_IDLE
#define noSP_DOCUMENT_DEBUG_UNDO

#include <vector>
#include <string>
#include <cstring>

#include <2geom/transforms.h>

#include "desktop.h"
#include "io/dir-util.h"
#include "document-undo.h"
#include "file.h"
#include "id-clash.h"
#include "inkscape-version.h"
#include "inkscape.h"
#include "inkscape-window.h"
#include "profile-manager.h"
#include "rdf.h"

#include "actions/actions-canvas-snapping.h"

#include "display/drawing.h"

#include "3rdparty/adaptagrams/libavoid/router.h"

#include "3rdparty/libcroco/cr-parser.h"
#include "3rdparty/libcroco/cr-sel-eng.h"
#include "3rdparty/libcroco/cr-selector.h"

#include "live_effects/lpeobject.h"
#include "object/persp3d.h"
#include "object/sp-defs.h"
#include "object/sp-factory.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "object/sp-symbol.h"

#include "widgets/desktop-widget.h"

#include "xml/croco-node-iface.h"
#include "xml/rebase-hrefs.h"

using Inkscape::DocumentUndo;
using Inkscape::Util::unit_table;

// Higher number means lower priority.
#define SP_DOCUMENT_UPDATE_PRIORITY (G_PRIORITY_HIGH_IDLE - 2)

// Should have a lower priority than SP_DOCUMENT_UPDATE_PRIORITY,
// since we want it to happen when there are no more updates.
#define SP_DOCUMENT_REROUTING_PRIORITY (G_PRIORITY_HIGH_IDLE - 1)

bool sp_no_convert_text_baseline_spacing = false;

//gboolean sp_document_resource_list_free(gpointer key, gpointer value, gpointer data);

static gint doc_count = 0;
static gint doc_mem_count = 0;

static unsigned long next_serial = 0;

SPDocument::SPDocument() :
    keepalive(false),
    virgin(true),
    rdoc(nullptr),
    rroot(nullptr),
    root(nullptr),
    style_cascade(cr_cascade_new(nullptr, nullptr, nullptr)),
    document_uri(nullptr),
    document_base(nullptr),
    document_name(nullptr),
    actionkey(),
    profileManager(nullptr), // deferred until after other initialization
    router(new Avoid::Router(Avoid::PolyLineRouting|Avoid::OrthogonalRouting)),
    oldSignalsConnected(false),
    current_persp3d(nullptr),
    current_persp3d_impl(nullptr),
    _parent_document(nullptr),
    _node_cache_valid(false),
    _activexmltree(nullptr)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (!prefs->getBool("/options/yaxisdown", true)) {
        _doc2dt[3] = -1;
    }

    // Penalise libavoid for choosing paths with needless extra segments.
    // This results in much better looking orthogonal connector paths.
    router->setRoutingPenalty(Avoid::segmentPenalty);

    _serial = next_serial++;

    sensitive = false;
    partial = nullptr;
    history_size = 0;
    seeking = false;

    // Once things are set, hook in the manager
    profileManager = new Inkscape::ProfileManager(this);

    // XXX only for testing!
    undoStackObservers.add(console_output_undo_observer);
    _node_cache = std::deque<SPItem*>();

    // Actions
    action_group = Gio::SimpleActionGroup::create();
    add_actions_canvas_snapping(this);
}

SPDocument::~SPDocument() {
    destroySignal.emit();

    // kill/unhook this first
    if ( profileManager ) {
        delete profileManager;
        profileManager = nullptr;
    }

    if (router) {
        delete router;
        router = nullptr;
    }

    if (oldSignalsConnected) {
        selChangeConnection.disconnect();
        desktopActivatedConnection.disconnect();
    } else {
        _selection_changed_connection.disconnect();
        _desktop_activated_connection.disconnect();
    }

    if (partial) {
        sp_repr_free_log(partial);
        partial = nullptr;
    }

    DocumentUndo::clearRedo(this);
    DocumentUndo::clearUndo(this);

    if (root) {
        root->releaseReferences();
        sp_object_unref(root);
        root = nullptr;
    }

    if (rdoc) Inkscape::GC::release(rdoc);

    /* Free resources */
    resources.clear();

    // This also destroys all attached stylesheets
    cr_cascade_unref(style_cascade);
    style_cascade = nullptr;

    if (document_name) {
        g_free(document_name);
        document_name = nullptr;
    }
    if (document_base) {
        g_free(document_base);
        document_base = nullptr;
    }
    if (document_uri) {
        g_free(document_uri);
        document_uri = nullptr;
    }

    modified_connection.disconnect();
    rerouting_connection.disconnect();

    if (keepalive) {
        inkscape_unref(INKSCAPE);
        keepalive = false;
    }

    if (this->current_persp3d_impl)
        delete this->current_persp3d_impl;
    this->current_persp3d_impl = nullptr;

    // This is at the end of the destructor, because preceding code adds new orphans to the queue
    collectOrphans();
}

Inkscape::XML::Node *SPDocument::getReprNamedView()
{
    return sp_repr_lookup_name (rroot, "sodipodi:namedview");
}

SPDefs *SPDocument::getDefs()
{
    if (!root) {
        return nullptr;
    }
    return root->defs;
}

Persp3D *SPDocument::getCurrentPersp3D() {
    // Check if current_persp3d is still valid
    std::vector<Persp3D*> plist;
    getPerspectivesInDefs(plist);
    for (auto & i : plist) {
        if (current_persp3d == i)
            return current_persp3d;
    }

    // If not, return the first perspective in defs (which may be NULL of none exists)
    current_persp3d = Persp3D::document_first_persp (this);

    return current_persp3d;
}

void SPDocument::setCurrentPersp3D(Persp3D * const persp) {
    current_persp3d = persp;
    //current_persp3d_impl = persp->perspective_impl;
}

void SPDocument::getPerspectivesInDefs(std::vector<Persp3D*> &list) const
{
    for (auto& i: root->defs->children) {
        if (SP_IS_PERSP3D(&i)) {
            list.push_back(SP_PERSP3D(&i));
        }
    }
}

/**
void SPDocument::initialize_current_persp3d()
{
    this->current_persp3d = Persp3D::document_first_persp(this);
    if (!this->current_persp3d) {
        this->current_persp3d = Persp3D::create_xml_element(this);
    }
}
**/

void SPDocument::queueForOrphanCollection(SPObject *object) {
    g_return_if_fail(object != nullptr);
    g_return_if_fail(object->document == this);

    sp_object_ref(object, nullptr);
    _collection_queue.push_back(object);
}

void SPDocument::collectOrphans() {
    while (!_collection_queue.empty()) {
        std::vector<SPObject *> objects(_collection_queue);
        _collection_queue.clear();
        for (auto object : objects) {
            object->collectOrphan();
            sp_object_unref(object, nullptr);
        }
    }
}

SPDocument *SPDocument::createDoc(Inkscape::XML::Document *rdoc,
                                  gchar const *document_uri,
                                  gchar const *document_base,
                                  gchar const *document_name,
                                  bool keepalive,
                                  SPDocument *parent)
{
    SPDocument *document = new SPDocument();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Inkscape::XML::Node *rroot = rdoc->root();

    document->keepalive = keepalive;

    document->rdoc = rdoc;
    document->rroot = rroot;
    if (parent) {
        document->_parent_document = parent;
        parent->_child_documents.push_back(document);
    }

    if (document->document_uri){
        g_free(document->document_uri);
        document->document_uri = nullptr;
    }
    if (document->document_base){
        g_free(document->document_base);
        document->document_base = nullptr;
    }
    if (document->document_name){
        g_free(document->document_name);
        document->document_name = nullptr;
    }
#ifndef _WIN32
    document->document_uri = prepend_current_dir_if_relative(document_uri);
#else
    // FIXME: it may be that prepend_current_dir_if_relative works OK on windows too, test!
    document->document_uri = document_uri? g_strdup(document_uri) : NULL;
#endif

    // base is simply the part of the path before filename; e.g. when running "inkscape ../file.svg" the base is "../"
    // which is why we use g_get_current_dir() in calculating the abs path above
    //This is NULL for a new document
    if (document_base) {
        document->document_base = g_strdup(document_base);
    } else {
        document->document_base = nullptr;
    }
    document->document_name = g_strdup(document_name);

    // Create SPRoot element
    const std::string typeString = NodeTraits::get_type_string(*rroot);
    SPObject* rootObj = SPFactory::createObject(typeString);
    document->root = dynamic_cast<SPRoot*>(rootObj);

    if (document->root == nullptr) {
    	// Node is not a valid root element
    	delete rootObj;

    	// fixme: what to do here?
    	throw;
    }

    // Recursively build object tree
    document->root->invoke_build(document, rroot, false);

    /* Eliminate obsolete sodipodi:docbase, for privacy reasons */
    rroot->removeAttribute("sodipodi:docbase");

    /* Eliminate any claim to adhere to a profile, as we don't try to */
    rroot->removeAttribute("baseProfile");

    // creating namedview
    if (!sp_item_group_get_child_by_name(document->root, nullptr, "sodipodi:namedview")) {
        // if there's none in the document already,
        Inkscape::XML::Node *rnew = nullptr;

        rnew = rdoc->createElement("sodipodi:namedview");
        //rnew->setAttribute("id", "base");

        // Add namedview data from the preferences
        // we can't use getAllEntries because this could produce non-SVG doubles
        Glib::ustring pagecolor = prefs->getString("/template/base/pagecolor");

        rnew->setAttributeOrRemoveIfEmpty("pagecolor", pagecolor.data());

        Glib::ustring bordercolor = prefs->getString("/template/base/bordercolor");

        rnew->setAttributeOrRemoveIfEmpty("bordercolor", bordercolor.data());

        sp_repr_set_svg_double(rnew, "borderopacity",
            prefs->getDouble("/template/base/borderopacity", 1.0));
        sp_repr_set_svg_double(rnew, "objecttolerance",
            prefs->getDouble("/template/base/objecttolerance", 10.0));
        sp_repr_set_svg_double(rnew, "gridtolerance",
            prefs->getDouble("/template/base/gridtolerance", 10.0));
        sp_repr_set_svg_double(rnew, "guidetolerance",
            prefs->getDouble("/template/base/guidetolerance", 10.0));
        sp_repr_set_svg_double(rnew, "inkscape:pageopacity",
            prefs->getDouble("/template/base/inkscape:pageopacity", 0.0));
        sp_repr_set_int(rnew, "inkscape:pageshadow",
            prefs->getInt("/template/base/inkscape:pageshadow", 2));
        sp_repr_set_int(rnew, "inkscape:window-width",
            prefs->getInt("/template/base/inkscape:window-width", 640));
        sp_repr_set_int(rnew, "inkscape:window-height",
            prefs->getInt("/template/base/inkscape:window-height", 480));

        // insert into the document
        rroot->addChild(rnew, nullptr);
        // clean up
        Inkscape::GC::release(rnew);
    }

    // Defs
    if (!document->root->defs) {
        Inkscape::XML::Node *r = rdoc->createElement("svg:defs");
        rroot->addChild(r, nullptr);
        Inkscape::GC::release(r);
        g_assert(document->root->defs);
    }

    /* Default RDF */
    rdf_set_defaults( document );

    if (keepalive) {
        inkscape_ref(INKSCAPE);
    }

    // Check if the document already has a perspective (e.g., when opening an existing
    // document). If not, create a new one and set it as the current perspective.
    document->setCurrentPersp3D(Persp3D::document_first_persp(document));
    if (!document->getCurrentPersp3D()) {
        //document->setCurrentPersp3D(Persp3D::create_xml_element (document));
        Persp3DImpl *persp_impl = new Persp3DImpl();
        document->setCurrentPersp3DImpl(persp_impl);
    }

    DocumentUndo::setUndoSensitive(document, true);

    // reset undo key when selection changes, so that same-key actions on different objects are not coalesced
    document->selChangeConnection = INKSCAPE.signal_selection_changed.connect(
                sigc::hide(sigc::bind(
                sigc::ptr_fun(&DocumentUndo::resetKey), document)
    ));
    document->desktopActivatedConnection = INKSCAPE.signal_activate_desktop.connect(
                sigc::hide(sigc::bind(
                sigc::ptr_fun(&DocumentUndo::resetKey), document)
    ));
    document->oldSignalsConnected = true;


    // ************* Fix Document **************
    // Move to separate function?

    /** Fix OSB **/
    sp_file_fix_osb(document->getRoot());


    /** Fix baseline spacing (pre-92 files) **/
    if ( (!sp_no_convert_text_baseline_spacing)
         && sp_version_inside_range( document->root->version.inkscape, 0, 1, 0, 92 ) ) {
        sp_file_convert_text_baseline_spacing(document);
    }

    /** Fix font names in legacy documents (pre-92 files) **/
    if ( sp_version_inside_range( document->root->version.inkscape, 0, 1, 0, 92 ) ) {
        sp_file_convert_font_name(document);
    }

    /** Fix first line spacing in legacy documents (pre-1.0 files) **/
    if (sp_version_inside_range(document->root->version.inkscape, 0, 1, 1, 0)) {
        sp_file_fix_empty_lines(document);
    }

    /** Fix dpi (pre-92 files). With GUI fixed in Inkscape::Application::fix_document. **/
    if ( !(INKSCAPE.use_gui()) && sp_version_inside_range( document->root->version.inkscape, 0, 1, 0, 92 ) ) {
        sp_file_convert_dpi(document);
    }

    // Update LPE's   See: Related bug:#1769679 #18
    SPDefs * defs = document->getDefs();
    if (defs) {
        defs->emitModified(SP_OBJECT_MODIFIED_CASCADE);
    }

    // Update document level action settings
    set_actions_canvas_snapping(document);

    return document;
}

/**
 * Fetches a document and attaches it to the current document as a child href
 */
SPDocument *SPDocument::createChildDoc(std::string const &document_uri)
{
    SPDocument *parent = this;
    SPDocument *document = nullptr;

    while(parent != nullptr && parent->getDocumentURI() != nullptr && document == nullptr) {
        // Check myself and any parents in the chain
        if(document_uri == parent->getDocumentURI()) {
            document = parent;
            break;
        }
        // Then check children of those.
        boost::ptr_list<SPDocument>::iterator iter;
        for (iter = parent->_child_documents.begin();
          iter != parent->_child_documents.end(); ++iter) {
            if(document_uri == iter->getDocumentURI()) {
                document = &*iter;
                break;
            }
        }
        parent = parent->_parent_document;
    }

    // Load a fresh document from the svg source.
    if(!document) {
        std::string path;
        if (Glib::path_is_absolute(document_uri)) {
            path = document_uri;
        } else {
            path = this->getDocumentBase() + document_uri;
        }
        document = createNewDoc(path.c_str(), false, false, this);
    }
    return document;
}
/**
 * Fetches document from URI, or creates new, if NULL; public document
 * appears in document list.
 */
SPDocument *SPDocument::createNewDoc(gchar const *document_uri, bool keepalive, bool make_new, SPDocument *parent)
{
    Inkscape::XML::Document *rdoc = nullptr;
    gchar *document_base = nullptr;
    gchar *document_name = nullptr;

    if (document_uri) {
        Inkscape::XML::Node *rroot;
        /* Try to fetch repr from file */
        rdoc = sp_repr_read_file(document_uri, SP_SVG_NS_URI);
        /* If file cannot be loaded, return NULL without warning */
        if (rdoc == nullptr) return nullptr;
        rroot = rdoc->root();
        /* If xml file is not svg, return NULL without warning */
        /* fixme: destroy document */
        if (strcmp(rroot->name(), "svg:svg") != 0) return nullptr;

        if (make_new) {
            document_base = nullptr;
            document_uri = nullptr;
            document_name = g_strdup_printf(_("New document %d"), ++doc_count);
        } else {
            document_base = g_path_get_dirname(document_uri);
            document_name = g_path_get_basename(document_uri);
            if (strcmp(document_base, ".") == 0) {
                g_free(document_base);
                document_base = nullptr;
            }
        }
    } else {
        if (make_new) {
            document_name = g_strdup_printf(_("Memory document %d"), ++doc_mem_count);
        }

        rdoc = sp_repr_document_new("svg:svg");
    }

    //# These should be set by now
    g_assert(document_name);

    SPDocument *doc = createDoc(rdoc, document_uri, document_base, document_name, keepalive, parent);

    g_free(document_base);
    g_free(document_name);

    return doc;
}

SPDocument *SPDocument::createNewDocFromMem(gchar const *buffer, gint length, bool keepalive)
{
    SPDocument *doc = nullptr;

    Inkscape::XML::Document *rdoc = sp_repr_read_mem(buffer, length, SP_SVG_NS_URI);
    if ( rdoc ) {
        // Only continue to create a non-null doc if it could be loaded
        Inkscape::XML::Node *rroot = rdoc->root();
        if ( strcmp(rroot->name(), "svg:svg") != 0 ) {
            // If xml file is not svg, return NULL without warning
            // TODO fixme: destroy document
        } else {
            Glib::ustring document_name = Glib::ustring::compose( _("Memory document %1"), ++doc_mem_count );
            doc = createDoc(rdoc, nullptr, nullptr, document_name.c_str(), keepalive, nullptr);
        }
    }

    return doc;
}

std::unique_ptr<SPDocument> SPDocument::doRef()
{
    Inkscape::GC::anchor(this);
    return std::unique_ptr<SPDocument>(this);
}

/// guaranteed not to return nullptr
Inkscape::Util::Unit const* SPDocument::getDisplayUnit() const
{
    SPNamedView const* nv = sp_document_namedview(this, nullptr);
    return nv ? nv->getDisplayUnit() : unit_table.getUnit("px");
}

/// Sets document scale (by changing viewBox)
void SPDocument::setDocumentScale( double scaleX, double scaleY ) {

    root->viewBox = Geom::Rect::from_xywh(
        root->viewBox.left(),
        root->viewBox.top(),
        root->width.computed  * scaleX,
        root->height.computed * scaleY );
    root->viewBox_set = true;
    root->updateRepr();
}

/// Sets document scale (by changing viewBox, x and y scaling equal)
void SPDocument::setDocumentScale( double scale ) {
    setDocumentScale( scale, scale );
}

/// Returns document scale as defined by width/height (in pixels) and viewBox (real world to
/// user-units).
Geom::Scale SPDocument::getDocumentScale() const
{
    Geom::Scale scale;
    if( root->viewBox_set ) {
        double scale_x = 1.0;
        double scale_y = 1.0;
        if( root->viewBox.width() > 0.0 ) {
            scale_x = root->width.computed / root->viewBox.width();
        }
        if( root->viewBox.height() > 0.0 ) {
            scale_y = root->height.computed / root->viewBox.height();
        }
        scale = Geom::Scale(scale_x, scale_y);
    }
    // std::cout << "SPDocument::getDocumentScale():\n" << scale << std::endl;
    return scale;
}

// Avoid calling root->updateRepr() twice by combining setting width and height.
// (As done on every delete as clipboard calls this via fitToRect(). Also called in page-sizer.cpp)
void SPDocument::setWidthAndHeight(const Inkscape::Util::Quantity &width, const Inkscape::Util::Quantity &height, bool changeSize)
{
    Inkscape::Util::Unit const *old_width_units = unit_table.getUnit("px");
    if (root->width.unit)
        old_width_units = unit_table.getUnit(root->width.unit);
    gdouble old_width_converted;  // old width converted to new units
    if (root->width.unit == SVGLength::PERCENT)
        old_width_converted = Inkscape::Util::Quantity::convert(root->width.computed, "px", width.unit);
    else
        old_width_converted = Inkscape::Util::Quantity::convert(root->width.value, old_width_units, width.unit);

    root->width.computed = width.value("px");
    root->width.value = width.quantity;
    root->width.unit = (SVGLength::Unit) width.unit->svgUnit();

    Inkscape::Util::Unit const *old_height_units = unit_table.getUnit("px");
    if (root->height.unit)
        old_height_units = unit_table.getUnit(root->height.unit);
    gdouble old_height_converted;  // old height converted to new units
    if (root->height.unit == SVGLength::PERCENT)
        old_height_converted = Inkscape::Util::Quantity::convert(root->height.computed, "px", height.unit);
    else
        old_height_converted = Inkscape::Util::Quantity::convert(root->height.value, old_height_units, height.unit);

    root->height.computed = height.value("px");
    root->height.value = height.quantity;
    root->height.unit = (SVGLength::Unit) height.unit->svgUnit();

    // viewBox scaled by relative change in page size (maintains document scale).
    if (root->viewBox_set && changeSize) {
        root->viewBox.setMax(Geom::Point(
        root->viewBox.left() + (root->width.value /  old_width_converted ) * root->viewBox.width(),
        root->viewBox.top()  + (root->height.value / old_height_converted) * root->viewBox.height()));
    }
    root->updateRepr();
}

Inkscape::Util::Quantity SPDocument::getWidth() const
{
    g_return_val_if_fail(this->root != nullptr, Inkscape::Util::Quantity(0.0, unit_table.getUnit("")));

    gdouble result = root->width.value;
    SVGLength::Unit u = root->width.unit;
    if (root->width.unit == SVGLength::PERCENT && root->viewBox_set) {
        result = root->viewBox.width();
        u = SVGLength::PX;
    }
    if (u == SVGLength::NONE) {
        u = SVGLength::PX;
    }
    return Inkscape::Util::Quantity(result, unit_table.getUnit(u));
}

void SPDocument::setWidth(const Inkscape::Util::Quantity &width, bool changeSize)
{
    Inkscape::Util::Unit const *old_width_units = unit_table.getUnit("px");
    if (root->width.unit)
        old_width_units = unit_table.getUnit(root->width.unit);
    gdouble old_width_converted;  // old width converted to new units
    if (root->width.unit == SVGLength::PERCENT)
        old_width_converted = Inkscape::Util::Quantity::convert(root->width.computed, "px", width.unit);
    else
        old_width_converted = Inkscape::Util::Quantity::convert(root->width.value, old_width_units, width.unit);

    root->width.computed = width.value("px");
    root->width.value = width.quantity;
    root->width.unit = (SVGLength::Unit) width.unit->svgUnit();

    if (root->viewBox_set && changeSize)
        root->viewBox.setMax(Geom::Point(root->viewBox.left() + (root->width.value / old_width_converted) * root->viewBox.width(), root->viewBox.bottom()));

    root->updateRepr();
}


Inkscape::Util::Quantity SPDocument::getHeight() const
{
    g_return_val_if_fail(this->root != nullptr, Inkscape::Util::Quantity(0.0, unit_table.getUnit("")));

    gdouble result = root->height.value;
    SVGLength::Unit u = root->height.unit;
    if (root->height.unit == SVGLength::PERCENT && root->viewBox_set) {
        result = root->viewBox.height();
        u = SVGLength::PX;
    }
    if (u == SVGLength::NONE) {
        u = SVGLength::PX;
    }
    return Inkscape::Util::Quantity(result, unit_table.getUnit(u));
}

void SPDocument::setHeight(const Inkscape::Util::Quantity &height, bool changeSize)
{
    Inkscape::Util::Unit const *old_height_units = unit_table.getUnit("px");
    if (root->height.unit)
        old_height_units = unit_table.getUnit(root->height.unit);
    gdouble old_height_converted;  // old height converted to new units
    if (root->height.unit == SVGLength::PERCENT)
        old_height_converted = Inkscape::Util::Quantity::convert(root->height.computed, "px", height.unit);
    else
        old_height_converted = Inkscape::Util::Quantity::convert(root->height.value, old_height_units, height.unit);

    root->height.computed = height.value("px");
    root->height.value = height.quantity;
    root->height.unit = (SVGLength::Unit) height.unit->svgUnit();

    if (root->viewBox_set && changeSize)
        root->viewBox.setMax(Geom::Point(root->viewBox.right(), root->viewBox.top() + (root->height.value / old_height_converted) * root->viewBox.height()));

    root->updateRepr();
}

const Geom::Affine &SPDocument::doc2dt() const
{
    if (root && !is_yaxisdown()) {
        _doc2dt[5] = root->height.computed;
    }

    return _doc2dt;
}

Geom::Rect SPDocument::getViewBox() const
{
    Geom::Rect viewBox;
    if (root->viewBox_set) {
        viewBox = root->viewBox;
    } else {
        viewBox = Geom::Rect::from_xywh( 0, 0, getWidth().value("px"), getHeight().value("px"));
    }
    return viewBox;
}

/**
 * Set default viewbox calculated from document properties.
 */
void SPDocument::setViewBox()
{
    setViewBox(Geom::Rect(0,
                          0,
                          getWidth().value(getDisplayUnit()),
                          getHeight().value(getDisplayUnit())));
}

void SPDocument::setViewBox(const Geom::Rect &viewBox)
{
    root->viewBox_set = true;
    root->viewBox = viewBox;
    root->updateRepr();
}

Geom::Point SPDocument::getDimensions() const
{
    return Geom::Point(getWidth().value("px"), getHeight().value("px"));
}

Geom::OptRect SPDocument::preferredBounds() const
{
    return Geom::OptRect( Geom::Point(0, 0), getDimensions() );
}

/**
 * Given a Geom::Rect that may, for example, correspond to the bbox of an object,
 * this function fits the canvas to that rect by resizing the canvas
 * and translating the document root into position.
 * \param rect fit document size to this, in document coordinates
 * \param with_margins add margins to rect, by taking margins from this
 *        document's namedview (<sodipodi:namedview> "fit-margin-..."
 *        attributes, and "units")
 */
void SPDocument::fitToRect(Geom::Rect const &rect, bool with_margins)
{
    double const w = rect.width();
    double const h = rect.height();

    Inkscape::Util::Unit const *nv_units = unit_table.getUnit("px");
    if (root->height.unit && (root->height.unit != SVGLength::PERCENT))
        nv_units = unit_table.getUnit(root->height.unit);
    SPNamedView *nv = sp_document_namedview(this, nullptr);

    /* in px */
    double margin_top = 0.0;
    double margin_left = 0.0;
    double margin_right = 0.0;
    double margin_bottom = 0.0;

    if (with_margins && nv) {
        if (nv != nullptr) {
            margin_top = nv->getMarginLength("fit-margin-top", nv_units, unit_table.getUnit("px"), w, h, false);
            margin_left = nv->getMarginLength("fit-margin-left", nv_units, unit_table.getUnit("px"), w, h, true);
            margin_right = nv->getMarginLength("fit-margin-right", nv_units, unit_table.getUnit("px"), w, h, true);
            margin_bottom = nv->getMarginLength("fit-margin-bottom", nv_units, unit_table.getUnit("px"), w, h, false);
            margin_top = Inkscape::Util::Quantity::convert(margin_top, nv_units, "px");
            margin_left = Inkscape::Util::Quantity::convert(margin_left, nv_units, "px");
            margin_right = Inkscape::Util::Quantity::convert(margin_right, nv_units, "px");
            margin_bottom = Inkscape::Util::Quantity::convert(margin_bottom, nv_units, "px");
        }
    }

    double y_dir = yaxisdir();

    Geom::Rect const rect_with_margins(
            rect.min() - Geom::Point(margin_left, margin_top),
            rect.max() + Geom::Point(margin_right, margin_bottom));

    // rect in desktop coordinates before changing document dimensions
    auto rect_with_margins_dt_old = rect_with_margins * doc2dt();

    setWidthAndHeight(
        Inkscape::Util::Quantity(Inkscape::Util::Quantity::convert(rect_with_margins.width(),  "px", nv_units), nv_units),
        Inkscape::Util::Quantity(Inkscape::Util::Quantity::convert(rect_with_margins.height(), "px", nv_units), nv_units)
        );

    // rect in desktop coordinates after changing document dimensions
    auto rect_with_margins_dt_new = rect_with_margins * doc2dt();

    Geom::Translate const tr(-rect_with_margins_dt_new.min());
    root->translateChildItems(tr);

    if(nv) {
        Geom::Translate tr2(-rect_with_margins_dt_old.min());
        nv->translateGuides(tr2);
        nv->translateGrids(tr2);

        // update the viewport so the drawing appears to stay where it was
        nv->scrollAllDesktops(-tr2[0], -tr2[1] * y_dir, false);
    }
}

void SPDocument::setDocumentBase( gchar const* document_base )
{
    if (this->document_base) {
        g_free(this->document_base);
        this->document_base = nullptr;
    }
    if (document_base) {
        this->document_base = g_strdup(document_base);
    }
}

void SPDocument::do_change_uri(gchar const *const filename, bool const rebase)
{
    gchar *new_document_base = nullptr;
    gchar *new_document_name = nullptr;
    gchar *new_document_uri = nullptr;
    if (filename) {

#ifndef _WIN32
        new_document_uri = prepend_current_dir_if_relative(filename);
#else
        // FIXME: it may be that prepend_current_dir_if_relative works OK on windows too, test!
        new_document_uri = g_strdup(filename);
#endif

        new_document_base = g_path_get_dirname(new_document_uri);
        new_document_name = g_path_get_basename(new_document_uri);
    } else {
        new_document_uri = g_strdup_printf(_("Unnamed document %d"), ++doc_count);
        new_document_base = nullptr;
        new_document_name = g_strdup(this->document_uri);
    }

    // Update saveable repr attributes.
    Inkscape::XML::Node *repr = getReprRoot();

    // Changing uri in the document repr must not be not undoable.
    bool const saved = DocumentUndo::getUndoSensitive(this);
    DocumentUndo::setUndoSensitive(this, false);

    if (rebase) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool use_sodipodi_absref = prefs->getBool("/options/svgoutput/usesodipodiabsref", false);
        Inkscape::XML::rebase_hrefs(this, new_document_base, use_sodipodi_absref);
    }

    if (strncmp(new_document_name, "ink_ext_XXXXXX", 14))	// do not use temporary filenames
        repr->setAttribute("sodipodi:docname", new_document_name);
    DocumentUndo::setUndoSensitive(this, saved);


    g_free(this->document_name);
    g_free(this->document_base);
    g_free(this->document_uri);
    this->document_name = new_document_name;
    this->document_base = new_document_base;
    this->document_uri = new_document_uri;

    this->uri_set_signal.emit(this->document_uri);
}

/**
 * Sets base, name and uri members of \a document.  Doesn't update
 * any relative hrefs in the document: thus, this is primarily for
 * newly-created documents.
 *
 * \see sp_document_change_uri_and_hrefs
 */
void SPDocument::setDocumentUri(gchar const *filename)
{
    do_change_uri(filename, false);
}

/**
 * Changes the base, name and uri members of \a document, and updates any
 * relative hrefs in the document to be relative to the new base.
 *
 * \see sp_document_set_uri
 */
void SPDocument::changeUriAndHrefs(gchar const *filename)
{
    do_change_uri(filename, true);
}

void SPDocument::bindObjectToId(gchar const *id, SPObject *object) {
    GQuark idq = g_quark_from_string(id);

    if (object) {
        if(object->getId())
            iddef.erase(object->getId());
        g_assert(iddef.find(id)==iddef.end());
        iddef[id] = object;
    } else {
        g_assert(iddef.find(id)!=iddef.end());
        iddef.erase(id);
    }

    SPDocument::IDChangedSignalMap::iterator pos;

    pos = id_changed_signals.find(idq);
    if ( pos != id_changed_signals.end() ) {
        if (!(*pos).second.empty()) {
            (*pos).second.emit(object);
        } else { // discard unused signal
            id_changed_signals.erase(pos);
        }
    }
}

SPObject *SPDocument::getObjectById(Glib::ustring const &id) const
{
    if (iddef.empty()) {
        return nullptr;
    }

    std::map<std::string, SPObject *>::const_iterator rv = iddef.find(id);
    if (rv != iddef.end()) {
        return (rv->second);
    } else {
        return nullptr;
    }
}

SPObject *SPDocument::getObjectById(gchar const *id) const
{
    if (id == nullptr) {
        return nullptr;
    }

    return getObjectById(Glib::ustring(id));
}

void _getObjectsByClassRecursive(Glib::ustring const &klass, SPObject *parent, std::vector<SPObject *> &objects)
{
    if (parent) {
        Glib::ustring class_attribute;
        char const *temp = parent->getAttribute("class");
        if (temp) {
            class_attribute = temp;
        }

        if (class_attribute.find( klass ) != std::string::npos) {
            objects.push_back( parent );
        }

        // Check children
        for (auto& child : parent->children) {
            _getObjectsByClassRecursive( klass, &child, objects );
        }
    }
}

std::vector<SPObject *> SPDocument::getObjectsByClass(Glib::ustring const &klass) const
{
    std::vector<SPObject *> objects;
    g_return_val_if_fail(!klass.empty(), objects);

    _getObjectsByClassRecursive(klass, root, objects);
    return objects;
}

void _getObjectsByElementRecursive(Glib::ustring const &element, SPObject *parent,
                                   std::vector<SPObject *> &objects)
{
    if (parent) {
        Glib::ustring prefixed = "svg:" + element;
        if (parent->getRepr()->name() == prefixed) {
            objects.push_back(parent);
        }

        // Check children
        for (auto& child : parent->children) {
            _getObjectsByElementRecursive(element, &child, objects);
        }
    }
}

std::vector<SPObject *> SPDocument::getObjectsByElement(Glib::ustring const &element) const
{
    std::vector<SPObject *> objects;
    g_return_val_if_fail(!element.empty(), objects);

    _getObjectsByElementRecursive(element, root, objects);
    return objects;
}

void _getObjectsBySelectorRecursive(SPObject *parent,
                                    CRSelEng *sel_eng, CRSimpleSel *simple_sel,
                                    std::vector<SPObject *> &objects)
{
    if (parent) {
        gboolean result = false;
        cr_sel_eng_matches_node( sel_eng, simple_sel, parent->getRepr(), &result );
        if (result) {
            objects.push_back(parent);
        }

        // Check children
        for (auto& child : parent->children) {
            _getObjectsBySelectorRecursive(&child, sel_eng, simple_sel, objects);
        }
    }
}

std::vector<SPObject *> SPDocument::getObjectsBySelector(Glib::ustring const &selector) const
{
    // std::cout << "\nSPDocument::getObjectsBySelector: " << selector << std::endl;

    std::vector<SPObject *> objects;
    g_return_val_if_fail(!selector.empty(), objects);

    static CRSelEng *sel_eng = nullptr;
    if (!sel_eng) {
        sel_eng = cr_sel_eng_new(&Inkscape::XML::croco_node_iface);
    }

    Glib::ustring my_selector = selector + " {";  // Parsing fails sometimes without '{'. Fix me
    CRSelector *cr_selector = cr_selector_parse_from_buf ((guchar*)my_selector.c_str(), CR_UTF_8);
    // char * cr_string = (char*)cr_selector_to_string( cr_selector );
    // std::cout << "  selector: |" << (cr_string?cr_string:"Empty") << "|" << std::endl;
    CRSelector const *cur = nullptr;
    for (cur = cr_selector; cur; cur = cur->next) {
        if (cur->simple_sel ) {
            _getObjectsBySelectorRecursive(root, sel_eng, cur->simple_sel, objects);
        }
    }
    return objects;
}

void SPDocument::bindObjectToRepr(Inkscape::XML::Node *repr, SPObject *object)
{
    if (object) {
        g_assert(reprdef.find(repr)==reprdef.end());
        reprdef[repr] = object;
    } else {
        g_assert(reprdef.find(repr)!=reprdef.end());
        reprdef.erase(repr);
    }
}

SPObject *SPDocument::getObjectByRepr(Inkscape::XML::Node *repr) const
{
    g_return_val_if_fail(repr != nullptr, NULL);
    std::map<Inkscape::XML::Node *, SPObject *>::const_iterator rv = reprdef.find(repr);
    if(rv != reprdef.end())
        return (rv->second);
    else
        return nullptr;
}

/** Returns preferred document languages (from most to least preferred)
 *
 * This currently includes (in order):
 * - language set in RDF metadata
 * - languages suitable for system locale (influenced by Inkscape GUI locale preference)
 */
std::vector<Glib::ustring> SPDocument::getLanguages() const
{
    std::vector<Glib::ustring> document_languages;

    // get language from RDF
    gchar const *rdf_language = rdf_get_work_entity(this, rdf_find_entity("language"));
    if (rdf_language) {
        gchar *rdf_language_stripped = g_strstrip(g_strdup(rdf_language));
        if (strcmp(rdf_language_stripped, "") != 0) {
            document_languages.emplace_back(rdf_language_stripped);
        }
        g_free(rdf_language_stripped);
    }

    // get language from system locale (will also match the interface language preference as we set LANG accordingly)
    // TODO: This includes locales with encodings like "de_DE.UTF-8" - is this useful or should we skip these?
    // TODO: This includes the default "C" locale - is this useful or should we skip it?
    const gchar * const * names = g_get_language_names();
    for (int i=0; names[i]; i++) {
        document_languages.emplace_back(names[i]);
    }

    return document_languages;
}

/* Object modification root handler */

void SPDocument::requestModified()
{
    if (modified_connection.empty()) {
        modified_connection =
            Glib::signal_idle().connect(sigc::mem_fun(*this, &SPDocument::idle_handler),
                                        SP_DOCUMENT_UPDATE_PRIORITY);
    }

    if (rerouting_connection.empty()) {
        rerouting_connection =
            Glib::signal_idle().connect(sigc::mem_fun(*this, &SPDocument::rerouting_handler),
                                        SP_DOCUMENT_REROUTING_PRIORITY);
    }
}

void SPDocument::setupViewport(SPItemCtx *ctx)
{
    ctx->flags = 0;
    ctx->i2doc = Geom::identity();
    // Set up viewport in case svg has it defined as percentages
    if (root->viewBox_set) { // if set, take from viewBox
        ctx->viewport = root->viewBox;
    } else { // as a last resort, set size to A4
        ctx->viewport = Geom::Rect::from_xywh(0, 0, Inkscape::Util::Quantity::convert(210, "mm", "px"), Inkscape::Util::Quantity::convert(297, "mm", "px"));
    }
    ctx->i2vp = Geom::identity();
}

/**
 * Tries to update the document state based on the modified and
 * "update required" flags, and return true if the document has
 * been brought fully up to date.
 */
bool
SPDocument::_updateDocument(int update_flags)
{
    /* Process updates */
    if (this->root->uflags || this->root->mflags) {
        if (this->root->uflags) {
            SPItemCtx ctx;
            setupViewport(&ctx);

            DocumentUndo::ScopedInsensitive _no_undo(this);

            this->root->updateDisplay((SPCtx *)&ctx, update_flags);
        }
        this->_emitModified();
    }

    return !(this->root->uflags || this->root->mflags);
}


/**
 * Repeatedly works on getting the document updated, since sometimes
 * it takes more than one pass to get the document updated.  But it
 * usually should not take more than a few loops, and certainly never
 * more than 32 iterations.  So we bail out if we hit 32 iterations,
 * since this typically indicates we're stuck in an update loop.
 */
gint SPDocument::ensureUpToDate()
{
    // Bring the document up-to-date, specifically via the following:
    //   1a) Process all document updates.
    //   1b) When completed, process connector routing changes.
    //   2a) Process any updates resulting from connector reroutings.
    int counter = 32;
    for (unsigned int pass = 1; pass <= 2; ++pass) {
        // Process document updates.
        while (!_updateDocument(0)) {
            if (counter == 0) {
                g_warning("More than 32 iteration while updating document '%s'", document_uri);
                break;
            }
            counter--;
        }
        if (counter == 0)
        {
            break;
        }

        // After updates on the first pass we get libavoid to process all the
        // changed objects and provide new routings.  This may cause some objects
            // to be modified, hence the second update pass.
        if (pass == 1) {
            router->processTransaction();
        }
    }

    // Remove handlers
    modified_connection.disconnect();
    rerouting_connection.disconnect();

    return (counter > 0);
}

/**
 * An idle handler to update the document.  Returns true if
 * the document needs further updates.
 */
bool
SPDocument::idle_handler()
{
    bool status = !_updateDocument(0); // method TRUE if it does NOT need further modification, so invert
    if (!status) {
        modified_connection.disconnect();
    }
    return status;
}

/**
 * An idle handler to reroute connectors in the document.
 */
bool
SPDocument::rerouting_handler()
{
    // Process any queued movement actions and determine new routings for
    // object-avoiding connectors.  Callbacks will be used to update and
    // redraw affected connectors.
    router->processTransaction();

    // We don't need to handle rerouting again until there are further
    // diagram updates.
    return false;
}

static bool is_within(Geom::Rect const &area, Geom::Rect const &box)
{
    return area.contains(box);
}

static bool overlaps(Geom::Rect const &area, Geom::Rect const &box)
{
    return area.intersects(box);
}

/**
 * @param area Area in document coordinates
 */
static std::vector<SPItem*> &find_items_in_area(std::vector<SPItem*> &s,
                                                SPGroup *group, unsigned int dkey,
                                                Geom::Rect const &area,
                                                bool (*test)(Geom::Rect const &, Geom::Rect const &),
                                                bool take_hidden = false,
                                                bool take_insensitive = false,
                                                bool take_groups = true,
                                                bool enter_groups = false)
{
    g_return_val_if_fail(SP_IS_GROUP(group), s);

    for (auto& o: group->children) {
        if (SPItem *item = dynamic_cast<SPItem *>(&o)) {
            if (!take_insensitive && item->isLocked()) {
                continue;
            }

            if (!take_hidden && item->isHidden()) {
                continue;
            }

            if (SPGroup * childgroup = dynamic_cast<SPGroup *>(item)) {
                bool is_layer = childgroup->effectiveLayerMode(dkey) == SPGroup::LAYER;
                if (is_layer || (enter_groups)) {
                    s = find_items_in_area(s, childgroup, dkey, area, test, take_hidden, take_insensitive, take_groups, enter_groups);
                }
                if (!take_groups || is_layer) {
                    continue;
                }
            }
            Geom::OptRect box = item->documentVisualBounds();
            if (box && test(area, *box)) {
                s.push_back(item);
            }
        }
    }
    return s;
}

SPItem *SPDocument::getItemFromListAtPointBottom(unsigned int dkey, SPGroup *group, std::vector<SPItem*> const &list,Geom::Point const &p, bool take_insensitive)
{
    g_return_val_if_fail(group, NULL);
    SPItem *bottomMost = nullptr;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gdouble delta = prefs->getDouble("/options/cursortolerance/value", 1.0);

    for (auto& o: group->children) {
        if (bottomMost) {
            break;
        }
        if (SP_IS_ITEM(&o)) {
            SPItem *item = SP_ITEM(&o);
            Inkscape::DrawingItem *arenaitem = item->get_arenaitem(dkey);

            if (arenaitem) {
                arenaitem->drawing().update();
            }

            if (arenaitem && arenaitem->pick(p, delta, 1) != nullptr
                && (take_insensitive || item->isVisibleAndUnlocked(dkey))) {
                if (find(list.begin(), list.end(), item) != list.end()) {
                    bottomMost = item;
                }
            }

            if (!bottomMost && SP_IS_GROUP(&o)) {
                // return null if not found:
                bottomMost = getItemFromListAtPointBottom(dkey, SP_GROUP(&o), list, p, take_insensitive);
            }
        }
    }
    return bottomMost;
}

/**
Turn the SVG DOM into a flat list of nodes that can be searched from top-down.
The list can be persisted, which improves "find at multiple points" speed.
*/
// TODO: study add `gboolean with_groups = false` as parameter.
void SPDocument::build_flat_item_list(unsigned int dkey, SPGroup *group, gboolean into_groups) const
{
    for (auto& o: group->children) {
        if (!SP_IS_ITEM(&o)) {
            continue;
        }

        if (SP_IS_GROUP(&o) && (SP_GROUP(&o)->effectiveLayerMode(dkey) == SPGroup::LAYER || into_groups)) {
            build_flat_item_list(dkey, SP_GROUP(&o), into_groups);
        } else {
            SPItem *child = SP_ITEM(&o);
            if (child->isVisibleAndUnlocked(dkey)) {
                _node_cache.push_front(child);
            }
        }
    }
}

/**
Returns the topmost (in z-order) item from the descendants of group (recursively) which
is at the point p, or NULL if none. Honors into_groups on whether to recurse into
non-layer groups or not. Honors take_insensitive on whether to return insensitive
items. If upto != NULL, then if item upto is encountered (at any level), stops searching
upwards in z-order and returns what it has found so far (i.e. the found item is
guaranteed to be lower than upto). Requires a list of nodes built by
build_flat_item_list.
 */
static SPItem *find_item_at_point(std::deque<SPItem*> *nodes, unsigned int dkey, Geom::Point const &p, SPItem* upto=nullptr)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gdouble delta = prefs->getDouble("/options/cursortolerance/value", 1.0);

    SPItem *seen = nullptr;
    SPItem *child;
    bool seen_upto = (!upto);
    for (auto node : *nodes) {
        child = node;
        if (!seen_upto){
            if(child == upto)
                seen_upto = true;
            continue;
        }
        Inkscape::DrawingItem *arenaitem = child->get_arenaitem(dkey);
        if (arenaitem) {
            arenaitem->drawing().update();
            if (arenaitem->pick(p, delta, 1) != nullptr) {
                seen = child;
                break;
            }
        }
    }

    return seen;
}

/**
Returns the topmost non-layer group from the descendants of group which is at point
p, or NULL if none. Recurses into layers but not into groups.
 */
static SPItem *find_group_at_point(unsigned int dkey, SPGroup *group, Geom::Point const &p)
{
    SPItem *seen = nullptr;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gdouble delta = prefs->getDouble("/options/cursortolerance/value", 1.0);

    for (auto& o: group->children) {
        if (!SP_IS_ITEM(&o)) {
            continue;
        }
        if (SP_IS_GROUP(&o) && SP_GROUP(&o)->effectiveLayerMode(dkey) == SPGroup::LAYER) {
            SPItem *newseen = find_group_at_point(dkey, SP_GROUP(&o), p);
            if (newseen) {
                seen = newseen;
            }
        }
        if (SP_IS_GROUP(&o) && SP_GROUP(&o)->effectiveLayerMode(dkey) != SPGroup::LAYER ) {
            SPItem *child = SP_ITEM(&o);
            Inkscape::DrawingItem *arenaitem = child->get_arenaitem(dkey);
            if (arenaitem) {
                arenaitem->drawing().update();
            }

            // seen remembers the last (topmost) of groups pickable at this point
            if (arenaitem && arenaitem->pick(p, delta, 1) != nullptr) {
                seen = child;
            }
        }
    }
    return seen;
}


/**
 * Return list of items, contained in box
 *
 * @param box area to find items, in document coordinates
 */

std::vector<SPItem*> SPDocument::getItemsInBox(unsigned int dkey, Geom::Rect const &box, bool take_hidden, bool take_insensitive, bool take_groups, bool enter_groups) const
{
    std::vector<SPItem*> x;
    return find_items_in_area(x, SP_GROUP(this->root), dkey, box, is_within, take_hidden, take_insensitive, take_groups, enter_groups);
}

/**
 * Get items whose bounding box overlaps with given area.
 * @param dkey desktop view in use
 * @param box area to find items, in document coordinates
 * @param take_hidden get hidden items
 * @param take_insensitive get insensitive items
 * @param take_groups get also the groups
 * @param enter_groups get items inside groups
 * @return Return list of items, that the parts of the item contained in box
 */

std::vector<SPItem*> SPDocument::getItemsPartiallyInBox(unsigned int dkey, Geom::Rect const &box, bool take_hidden, bool take_insensitive, bool take_groups, bool enter_groups) const
{
    std::vector<SPItem*> x;
    return find_items_in_area(x, SP_GROUP(this->root), dkey, box, overlaps, take_hidden, take_insensitive, take_groups, enter_groups);
}

std::vector<SPItem*> SPDocument::getItemsAtPoints(unsigned const key, std::vector<Geom::Point> points, bool all_layers, size_t limit) const
{
    std::vector<SPItem*> items;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // When picking along the path, we don't want small objects close together
    // (such as hatching strokes) to obscure each other by their deltas,
    // so we temporarily set delta to a small value
    gdouble saved_delta = prefs->getDouble("/options/cursortolerance/value", 1.0);
    prefs->setDouble("/options/cursortolerance/value", 0.25);

    // Cache a flattened SVG DOM to speed up selection.
    if(!_node_cache_valid){
        _node_cache.clear();
        build_flat_item_list(key, SP_GROUP(this->root), true);
        _node_cache_valid=true;
    }
    SPObject *current_layer = nullptr;
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    Inkscape::LayerModel *layer_model = nullptr;
    if(desktop){
        current_layer = desktop->currentLayer();
        layer_model = desktop->layers;
    }
    size_t item_counter = 0;
    for(int i = points.size()-1;i>=0; i--) {
        SPItem *item = find_item_at_point(&_node_cache, key, points[i]);
        if (item && items.end()==find(items.begin(),items.end(), item))
            if(all_layers || (layer_model && layer_model->layerForObject(item) == current_layer)){
                items.push_back(item);
                item_counter++;
                //limit 0 = no limit
                if(item_counter == limit){
                    prefs->setDouble("/options/cursortolerance/value", saved_delta);
                    return items;
                }
            }
    }

    // and now we restore it back
    prefs->setDouble("/options/cursortolerance/value", saved_delta);

    return items;
}

SPItem *SPDocument::getItemAtPoint( unsigned const key, Geom::Point const &p,
                                    bool const into_groups, SPItem *upto) const
{
    // Build a flattened SVG DOM for find_item_at_point.
    std::deque<SPItem*> bak(_node_cache);
    if(!into_groups){
        _node_cache.clear();
        build_flat_item_list(key, SP_GROUP(this->root), into_groups);
    }
    if(!_node_cache_valid && into_groups){
        _node_cache.clear();
        build_flat_item_list(key, SP_GROUP(this->root), true);
        _node_cache_valid=true;
    }

    SPItem *res = find_item_at_point(&_node_cache, key, p, upto);
    if(!into_groups)
        _node_cache = bak;
    return res;
}

SPItem *SPDocument::getGroupAtPoint(unsigned int key, Geom::Point const &p) const
{
    return find_group_at_point(key, SP_GROUP(this->root), p);
}

// Resource management

bool SPDocument::addResource(gchar const *key, SPObject *object)
{
    g_return_val_if_fail(key != nullptr, false);
    g_return_val_if_fail(*key != '\0', false);
    g_return_val_if_fail(object != nullptr, false);
    g_return_val_if_fail(SP_IS_OBJECT(object), false);

    bool result = false;

    if ( !object->cloned ) {
        std::vector<SPObject *> rlist = resources[key];
        g_return_val_if_fail(std::find(rlist.begin(),rlist.end(),object) == rlist.end(), false);
        resources[key].insert(resources[key].begin(),object);

        GQuark q = g_quark_from_string(key);

        /*in general, do not send signal if the object has no id (yet),
        it means the object is not completely built.
        (happens when pasting swatches across documents, cf bug 1495106)
        [this check should be more generally presend on emit() calls since
        the backtrace is unusable with crashed from this cause]
        */
        if(object->getId() || dynamic_cast<SPGroup*>(object) )
            resources_changed_signals[q].emit();

        result = true;
    }

    return result;
}

bool SPDocument::removeResource(gchar const *key, SPObject *object)
{
    g_return_val_if_fail(key != nullptr, false);
    g_return_val_if_fail(*key != '\0', false);
    g_return_val_if_fail(object != nullptr, false);
    g_return_val_if_fail(SP_IS_OBJECT(object), false);

    bool result = false;

    if ( !object->cloned ) {
        std::vector<SPObject *> rlist = resources[key];
        g_return_val_if_fail(!rlist.empty(), false);
        std::vector<SPObject*>::iterator it = std::find(resources[key].begin(),resources[key].end(),object);
        g_return_val_if_fail(it != rlist.end(), false);
        resources[key].erase(it);

        GQuark q = g_quark_from_string(key);
        resources_changed_signals[q].emit();

        result = true;
    }

    return result;
}

std::vector<SPObject *> const SPDocument::getResourceList(gchar const *key)
{
    std::vector<SPObject *> emptyset;
    g_return_val_if_fail(key != nullptr, emptyset);
    g_return_val_if_fail(*key != '\0', emptyset);

    return resources[key];
}

/* Helpers */

static unsigned int count_objects_recursive(SPObject *obj, unsigned int count)
{
    count++; // obj itself

    for (auto& i: obj->children) {
        count = count_objects_recursive(&i, count);
    }

    return count;
}

/**
 * Count the number of objects in a given document recursively using the count_objects_recursive helper function
 *
 * @param[in] document Pointer to the document for counting objects
 * @return Number of objects in the document
 */
static unsigned int objects_in_document(SPDocument *document)
{
    return count_objects_recursive(document->getRoot(), 0);
}

/**
 * Remove unused definitions etc. recursively from an object and its siblings
 *
 * @param[inout] obj Object which shall be "cleaned"
 */
static void vacuum_document_recursive(SPObject *obj)
{
    if (SP_IS_DEFS(obj)) {
        for (auto& def: obj->children) {
            // fixme: some inkscape-internal nodes in the future might not be collectable
            def.requestOrphanCollection();
        }
    } else {
        for (auto& i: obj->children) {
            vacuum_document_recursive(&i);
        }
    }
}

/**
 * Remove unused definitions etc. recursively from an entire document.
 *
 * @return Number of removed objects
 */
unsigned int SPDocument::vacuumDocument()
{
    unsigned int start = objects_in_document(this);
    unsigned int end;
    unsigned int newend = start;

    unsigned int iterations = 0;

    do {
        end = newend;

        vacuum_document_recursive(root);
        this->collectOrphans();
        iterations++;

        newend = objects_in_document(this);

    } while (iterations < 100 && newend < end);
    // We stop if vacuum_document_recursive doesn't remove any more objects or after 100 iterations, whichever occurs first.

    return start - newend;
}

/**
 * Indicate to the user if the document has been modified since the last save by displaying a "*" in front of the name of the file in the window title.
 *
 * @param[in] modified True if the document has been modified.
 */
void SPDocument::setModifiedSinceSave(bool modified) {
    this->modified_since_save = modified;
    this->modified_since_autosave = modified;
    if (SP_ACTIVE_DESKTOP) {
        InkscapeWindow *window = SP_ACTIVE_DESKTOP->getInkscapeWindow();
        if (window) { // during load, SP_ACTIVE_DESKTOP may be !nullptr, but parent might still be nullptr
            SPDesktopWidget *dtw = window->get_desktop_widget();
            dtw->updateTitle( this->getDocumentName() );
        }
    }
}


/**
 * Paste SVG defs from the document retrieved from the clipboard or imported document into the active document.
 * @param clipdoc The document to paste.
 * @pre @c clipdoc != NULL and pasting into the active document is possible.
 */
void SPDocument::importDefs(SPDocument *source)
{
    Inkscape::XML::Node *root = source->getReprRoot();
    Inkscape::XML::Node *target_defs = this->getDefs()->getRepr();
    std::vector<Inkscape::XML::Node const *> defsNodes = sp_repr_lookup_name_many(root, "svg:defs");

    prevent_id_clashes(source, this);

    for (auto & defsNode : defsNodes) {
       _importDefsNode(source, const_cast<Inkscape::XML::Node *>(defsNode), target_defs);
    }
}

void SPDocument::_importDefsNode(SPDocument *source, Inkscape::XML::Node *defs, Inkscape::XML::Node *target_defs)
{
    int stagger=0;

    /*  Note, "clipboard" throughout the comments means "the document that is either the clipboard
        or an imported document", as importDefs is called in both contexts.

        The order of the records in the clipboard is unpredictable and there may be both
        forward and backwards references to other records within it.  There may be definitions in
        the clipboard that duplicate definitions in the present document OR that duplicate other
        definitions in the clipboard.  (Inkscape will not have created these, but they may be read
        in from other SVG sources.)

        There are 3 passes to clean this up:

        In the first find and mark definitions in the clipboard that are duplicates of those in the
        present document.  Change the ID to "RESERVED_FOR_INKSCAPE_DUPLICATE_DEF_XXXXXXXXX".
        (Inkscape will not reuse an ID, and the XXXXXXXXX keeps it from automatically creating new ones.)
        References in the clipboard to the old clipboard name are converted to the name used
        in the current document.

        In the second find and mark definitions in the clipboard that are duplicates of earlier
        definitions in the clipbard.  Unfortunately this is O(n^2) and could be very slow for a large
        SVG with thousands of definitions.  As before, references are adjusted to reflect the name
        going forward.

        In the final cycle copy over those records not marked with that ID.

        If an SVG file uses the special ID it will cause problems!

        If this function is called because of the paste of a true clipboard the caller will have passed in a
        COPY of the clipboard items.  That is good, because this routine modifies that document.  If the calling
        behavior ever changes, so that the same document is passed in on multiple pastes, this routine will break
        as in the following example:
        1.  Paste clipboard containing B same as A into document containing A.  Result, B is dropped
        and all references to it will point to A.
        2.  Paste same clipboard into a new document.  It will not contain A, so there will be unsatisfied
        references in that window.
    */

    std::string DuplicateDefString = "RESERVED_FOR_INKSCAPE_DUPLICATE_DEF";

    /* First pass: remove duplicates in clipboard of definitions in document */
    for (Inkscape::XML::Node *def = defs->firstChild() ; def ; def = def->next()) {
        if(def->type() != Inkscape::XML::NodeType::ELEMENT_NODE)continue;
        /* If this  clipboard has been pasted into one document, and is now being pasted into another,
        or pasted again into the same, it will already have been processed.  If we detect that then
        skip the rest of this pass. */
        Glib::ustring defid = def->attribute("id");
        if( defid.find( DuplicateDefString ) != Glib::ustring::npos )break;

        SPObject *src = source->getObjectByRepr(def);

        // Prevent duplicates of solid swatches by checking if equivalent swatch already exists
        SPGradient *s_gr = dynamic_cast<SPGradient *>(src);
        LivePathEffectObject *s_lpeobj = dynamic_cast<LivePathEffectObject *>(src);
        if (src && (s_gr || s_lpeobj)) {
            for (auto& trg: getDefs()->children) {
                SPGradient *t_gr = dynamic_cast<SPGradient *>(&trg);
                if (src != &trg && s_gr && t_gr) {
                    if (s_gr->isEquivalent(t_gr)) {
                        // Change object references to the existing equivalent gradient
                        Glib::ustring newid = trg.getId();
                        if (newid != defid) { // id could be the same if it is a second paste into the same document
                            change_def_references(src, &trg);
                        }
                        gchar *longid = g_strdup_printf("%s_%9.9d", DuplicateDefString.c_str(), stagger++);
                        def->setAttribute("id", longid);
                        g_free(longid);
                        // do NOT break here, there could be more than 1 duplicate!
                    }
                }
                LivePathEffectObject *t_lpeobj = dynamic_cast<LivePathEffectObject *>(&trg);
                if (src != &trg && s_lpeobj && t_lpeobj) {
                    if (t_lpeobj->is_similar(s_lpeobj)) {
                        // Change object references to the existing equivalent gradient
                        Glib::ustring newid = trg.getId();
                        if (newid != defid) { // id could be the same if it is a second paste into the same document
                            change_def_references(src, &trg);
                        }
                        gchar *longid = g_strdup_printf("%s_%9.9d", DuplicateDefString.c_str(), stagger++);
                        def->setAttribute("id", longid);
                        g_free(longid);
                        // do NOT break here, there could be more than 1 duplicate!
                    }
                }
            }
        }
    }

    /* Second pass: remove duplicates in clipboard of earlier definitions in clipboard */
    for (Inkscape::XML::Node *def = defs->firstChild() ; def ; def = def->next()) {
        if(def->type() != Inkscape::XML::NodeType::ELEMENT_NODE)continue;
        Glib::ustring defid = def->attribute("id");
        if( defid.find( DuplicateDefString ) != Glib::ustring::npos )continue; // this one already handled
        SPObject *src = source->getObjectByRepr(def);
        LivePathEffectObject *s_lpeobj = dynamic_cast<LivePathEffectObject *>(src);
        SPGradient *s_gr = dynamic_cast<SPGradient *>(src);
        if (src && (s_gr || s_lpeobj)) {
            for (Inkscape::XML::Node *laterDef = def->next() ; laterDef ; laterDef = laterDef->next()) {
                SPObject *trg = source->getObjectByRepr(laterDef);
                SPGradient *t_gr = dynamic_cast<SPGradient *>(trg);
                if (trg && (src != trg) && s_gr && t_gr) {
                    Glib::ustring newid = trg->getId();
                    if (newid.find(DuplicateDefString) != Glib::ustring::npos)
                        continue; // this one already handled
                    if (t_gr && s_gr->isEquivalent(t_gr)) {
                        // Change object references to the existing equivalent gradient
                        // two id's in the clipboard should never be the same, so always change references
                        change_def_references(trg, src);
                        gchar *longid = g_strdup_printf("%s_%9.9d", DuplicateDefString.c_str(), stagger++);
                        laterDef->setAttribute("id", longid);
                        g_free(longid);
                        // do NOT break here, there could be more than 1 duplicate!
                    }
                }
                LivePathEffectObject *t_lpeobj = dynamic_cast<LivePathEffectObject *>(trg);
                if (trg && (src != trg) && s_lpeobj && t_lpeobj) {
                    Glib::ustring newid = trg->getId();
                    if (newid.find(DuplicateDefString) != Glib::ustring::npos)
                        continue; // this one already handled
                    if (t_lpeobj->is_similar(s_lpeobj)) {
                        // Change object references to the existing equivalent gradient
                        // two id's in the clipboard should never be the same, so always change references
                        change_def_references(trg, src);
                        gchar *longid = g_strdup_printf("%s_%9.9d", DuplicateDefString.c_str(), stagger++);
                        laterDef->setAttribute("id", longid);
                        g_free(longid);
                        // do NOT break here, there could be more than 1 duplicate!
                    }
                }
            }
        }
    }

    /* Final pass: copy over those parts which are not duplicates  */
    for (Inkscape::XML::Node *def = defs->firstChild() ; def ; def = def->next()) {
        if(def->type() != Inkscape::XML::NodeType::ELEMENT_NODE)continue;

        /* Ignore duplicate defs marked in the first pass */
        Glib::ustring defid = def->attribute("id");
        if( defid.find( DuplicateDefString ) != Glib::ustring::npos )continue;

        bool duplicate = false;
        SPObject *src = source->getObjectByRepr(def);

        // Prevent duplication of symbols... could be more clever.
        // The tag "_inkscape_duplicate" is added to "id" by ClipboardManagerImpl::copySymbol().
        // We assume that symbols are in defs section (not required by SVG spec).
        if (src && SP_IS_SYMBOL(src)) {

            Glib::ustring id = src->getRepr()->attribute("id");
            size_t pos = id.find( "_inkscape_duplicate" );
            if( pos != Glib::ustring::npos ) {

                // This is our symbol, now get rid of tag
                id.erase( pos );

                // Check that it really is a duplicate
                for (auto& trg: getDefs()->children) {
                    if (SP_IS_SYMBOL(&trg) && src != &trg) {
                        std::string id2 = trg.getRepr()->attribute("id");

                        if( !id.compare( id2 ) ) {
                            duplicate = true;
                            break;
                        }
                    }
                }
                if ( !duplicate ) {
                    src->setAttribute("id", id);
                }
            }
        }

        if (!duplicate) {
            Inkscape::XML::Node * dup = def->duplicate(this->getReprDoc());
            target_defs->appendChild(dup);
            Inkscape::GC::release(dup);
        }
    }
}

// Signals ------------------------------

void
SPDocument::addUndoObserver(Inkscape::UndoStackObserver& observer)
{
    this->undoStackObservers.add(observer);
}

void
SPDocument::removeUndoObserver(Inkscape::UndoStackObserver& observer)
{
    this->undoStackObservers.remove(observer);
}

sigc::connection SPDocument::connectDestroy(sigc::signal<void>::slot_type slot)
{
    return destroySignal.connect(slot);
}

sigc::connection SPDocument::connectModified(SPDocument::ModifiedSignal::slot_type slot)
{
    return modified_signal.connect(slot);
}

sigc::connection SPDocument::connectURISet(SPDocument::URISetSignal::slot_type slot)
{
    return uri_set_signal.connect(slot);
}

sigc::connection SPDocument::connectResized(SPDocument::ResizedSignal::slot_type slot)
{
    return resized_signal.connect(slot);
}

sigc::connection SPDocument::connectCommit(SPDocument::CommitSignal::slot_type slot)
{
    return commit_signal.connect(slot);
}

sigc::connection SPDocument::connectIdChanged(gchar const *id,
                                              SPDocument::IDChangedSignal::slot_type slot)
{
    return id_changed_signals[g_quark_from_string(id)].connect(slot);
}

sigc::connection SPDocument::connectResourcesChanged(gchar const *key,
                                                     SPDocument::ResourcesChangedSignal::slot_type slot)
{
    GQuark q = g_quark_from_string(key);
    return resources_changed_signals[q].connect(slot);
}

sigc::connection
SPDocument::connectReconstructionStart(SPDocument::ReconstructionStart::slot_type slot)
{
    return _reconstruction_start_signal.connect(slot);
}

sigc::connection
SPDocument::connectReconstructionFinish(SPDocument::ReconstructionFinish::slot_type  slot)
{
    return _reconstruction_finish_signal.connect(slot);
}

void SPDocument::_emitModified() {
    static guint const flags = SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG;
    root->emitModified(0);
    modified_signal.emit(flags);
    _node_cache_valid=false;
}

void
SPDocument::emitReconstructionStart()
{
    // printf("Starting Reconstruction\n");
    _reconstruction_start_signal.emit();
}

void
SPDocument::emitReconstructionFinish()
{
    // printf("Finishing Reconstruction\n");
    _reconstruction_finish_signal.emit();
    // indicates that gradients are reloaded (to rebuild the Auto palette)
    resources_changed_signals[g_quark_from_string("gradient")].emit();
    resources_changed_signals[g_quark_from_string("filter")].emit();

/**
    // Reference to the old persp3d object is invalid after reconstruction.
    initialize_current_persp3d();
**/
}

void SPDocument::emitResizedSignal(gdouble width, gdouble height)
{
    this->resized_signal.emit(width, height);
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
