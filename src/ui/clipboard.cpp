// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * System-wide clipboard management - implementation.
 *//*
 * Authors:
 * see git history
 *   Krzysztof Kosiński <tweenk@o2.pl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Incorporates some code from selection-chemistry.cpp, see that file for more credits.
 *   Abhishek Sharma
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "clipboard.h"

#include <giomm/application.h>
#include <glib/gstdio.h> // for g_file_set_contents etc., used in _onGet and paste
#include <glibmm/i18n.h>
#include <gtkmm/clipboard.h>

#include <2geom/transforms.h>
#include <2geom/path-sink.h>

// TODO: reduce header bloat if possible

#include "context-fns.h"
#include "desktop-style.h" // for sp_desktop_set_style, used in _pasteStyle
#include "desktop.h"
#include "document.h"
#include "file.h"          // for file_import, used in _pasteImage
#include "gradient-drag.h"
#include "inkscape.h"
#include "message-stack.h"
#include "path-chemistry.h"
#include "selection-chemistry.h"
#include "style.h"
#include "text-chemistry.h"
#include "text-editing.h"

#include "extension/db.h" // extension database
#include "extension/find_extension_by_mime.h"
#include "extension/input.h"
#include "extension/output.h"

#include "helper/png-write.h"

#include "inkgc/gc-core.h"

#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"
#include "live_effects/parameter/path.h"

#include "object/box3d.h"
#include "object/persp3d.h"
#include "object/sp-clippath.h"
#include "object/sp-defs.h"
#include "object/sp-flowtext.h"
#include "object/sp-gradient-reference.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-mesh-gradient.h"
#include "object/sp-hatch.h"
#include "object/sp-item-transform.h"
#include "object/sp-marker.h"
#include "object/sp-mask.h"
#include "object/sp-pattern.h"
#include "object/sp-rect.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "object/sp-textpath.h"
#include "object/sp-use.h"
#include "object/sp-path.h"

#include "svg/css-ostringstream.h" // used in copy
#include "svg/svg-color.h"
#include "svg/svg.h"               // for sp_svg_transform_write, used in _copySelection

#include "ui/tools/dropper-tool.h" // used in copy()
#include "ui/tools/text-tool.h"
#include "ui/tools/node-tool.h"
#include "ui/tool/multi-path-manipulator.h"

#include "util/units.h"
#include "display/curve.h"

#include "xml/repr.h"
#include "xml/sp-css-attr.h"

/// Made up mimetype to represent Gdk::Pixbuf clipboard contents.
#define CLIPBOARD_GDK_PIXBUF_TARGET "image/x-gdk-pixbuf"

#define CLIPBOARD_TEXT_TARGET "text/plain"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Inkscape {
namespace UI {

/**
 * Default implementation of the clipboard manager.
 */
class ClipboardManagerImpl : public ClipboardManager {
public:
    void copy(ObjectSet *set) override;
    void copyPathParameter(Inkscape::LivePathEffect::PathParam *) override;
    void copySymbol(Inkscape::XML::Node* symbol, gchar const* style, bool user_symbol) override;
    bool paste(SPDesktop *desktop, bool in_place) override;
    bool pasteStyle(ObjectSet *set) override;
    bool pasteSize(ObjectSet *set, bool separately, bool apply_x, bool apply_y) override;
    bool pastePathEffect(ObjectSet *set) override;
    Glib::ustring getPathParameter(SPDesktop* desktop) override;
    Glib::ustring getShapeOrTextObjectId(SPDesktop *desktop) override;
    std::vector<Glib::ustring> getElementsOfType(SPDesktop *desktop, gchar const* type = "*", gint maxdepth = -1) override;
    Glib::ustring getFirstObjectID() override;

    ClipboardManagerImpl();
    ~ClipboardManagerImpl() override;

private:
    void _copySelection(ObjectSet *);
    void _copyUsedDefs(SPItem *);
    void _copyGradient(SPGradient *);
    void _copyPattern(SPPattern *);
    void _copyHatch(SPHatch *);
    void _copyTextPath(SPTextPath *);
    Inkscape::XML::Node *_copyNode(Inkscape::XML::Node *, Inkscape::XML::Document *, Inkscape::XML::Node *);
    Inkscape::XML::Node *_copyIgnoreDup(Inkscape::XML::Node *, Inkscape::XML::Document *, Inkscape::XML::Node *);

    bool _pasteImage(SPDocument *doc);
    bool _pasteText(SPDesktop *desktop);
    void _applyPathEffect(SPItem *, gchar const *);
    std::unique_ptr<SPDocument> _retrieveClipboard(Glib::ustring = "");

    // clipboard callbacks
    void _onGet(Gtk::SelectionData &, guint);
    void _onClear();

    // various helpers
    void _createInternalClipboard();
    void _discardInternalClipboard();
    Inkscape::XML::Node *_createClipNode();
    Geom::Scale _getScale(SPDesktop *desktop, Geom::Point const &min, Geom::Point const &max, Geom::Rect const &obj_rect, bool apply_x, bool apply_y);
    Glib::ustring _getBestTarget();
    void _setClipboardTargets();
    void _setClipboardColor(guint32);
    void _userWarn(SPDesktop *, char const *);

    // private properites
    std::unique_ptr<SPDocument> _clipboardSPDoc; ///< Document that stores the clipboard until someone requests it
    Inkscape::XML::Node *_defs; ///< Reference to the clipboard document's defs node
    Inkscape::XML::Node *_root; ///< Reference to the clipboard's root node
    Inkscape::XML::Node *_clipnode; ///< The node that holds extra information
    Inkscape::XML::Document *_doc; ///< Reference to the clipboard's Inkscape::XML::Document
    std::set<SPItem*> cloned_elements;
    std::vector<SPCSSAttr*> te_selected_style;
    std::vector<unsigned> te_selected_style_positions;
    int nr_blocks = 0;
    unsigned copied_style_length = 0;


    // we need a way to copy plain text AND remember its style;
    // the standard _clipnode is only available in an SVG tree, hence this special storage
    SPCSSAttr *_text_style; ///< Style copied along with plain text fragment

    Glib::RefPtr<Gtk::Clipboard> _clipboard; ///< Handle to the system wide clipboard - for convenience
    std::list<Glib::ustring> _preferred_targets; ///< List of supported clipboard targets
};


ClipboardManagerImpl::ClipboardManagerImpl()
    : _clipboardSPDoc(nullptr),
      _defs(nullptr),
      _root(nullptr),
      _clipnode(nullptr),
      _doc(nullptr),
      _text_style(nullptr),
      _clipboard( Gtk::Clipboard::get() )
{
    // Clipboard Formats: http://msdn.microsoft.com/en-us/library/ms649013(VS.85).aspx
    // On Windows, most graphical applications can handle CF_DIB/CF_BITMAP and/or CF_ENHMETAFILE
    // GTK automatically presents an "image/bmp" target as CF_DIB/CF_BITMAP
    // Presenting "image/x-emf" as CF_ENHMETAFILE must be done by Inkscape ?

    // push supported clipboard targets, in order of preference
    _preferred_targets.emplace_back("image/x-inkscape-svg");
    _preferred_targets.emplace_back("image/svg+xml");
    _preferred_targets.emplace_back("image/svg+xml-compressed");
    _preferred_targets.emplace_back("image/x-emf");
    _preferred_targets.emplace_back("CF_ENHMETAFILE");
    _preferred_targets.emplace_back("WCF_ENHMETAFILE"); // seen on Wine
    _preferred_targets.emplace_back("application/pdf");
    _preferred_targets.emplace_back("image/x-adobe-illustrator");

    // Clipboard requests on app termination can cause undesired extension
    // popup windows. Clearing the clipboard can prevent this.
    auto application = Gio::Application::get_default();
    if (application) {
        application->signal_shutdown().connect_notify([this]() { this->_discardInternalClipboard(); });
    }
}


ClipboardManagerImpl::~ClipboardManagerImpl() = default;


/**
 * Copy selection contents to the clipboard.
 */
void ClipboardManagerImpl::copy(ObjectSet *set)
{
    if ( set->desktop() ) {
        SPDesktop *desktop = set->desktop();

        // Special case for when the gradient dragger is active - copies gradient color
        if (desktop->event_context->get_drag()) {
            GrDrag *drag = desktop->event_context->get_drag();
            if (drag->hasSelection()) {
                guint32 col = drag->getColor();

                // set the color as clipboard content (text in RRGGBBAA format)
                _setClipboardColor(col);

                // create a style with this color on fill and opacity in master opacity, so it can be
                // pasted on other stops or objects
                if (_text_style) {
                    sp_repr_css_attr_unref(_text_style);
                    _text_style = nullptr;
                }
                _text_style = sp_repr_css_attr_new();
                // print and set properties
                gchar color_str[16];
                g_snprintf(color_str, 16, "#%06x", col >> 8);
                sp_repr_css_set_property(_text_style, "fill", color_str);
                float opacity = SP_RGBA32_A_F(col);
                if (opacity > 1.0) {
                    opacity = 1.0; // safeguard
                }
                Inkscape::CSSOStringStream opcss;
                opcss << opacity;
                sp_repr_css_set_property(_text_style, "opacity", opcss.str().data());

                _discardInternalClipboard();
                return;
            }
        }

        // Special case for when the color picker ("dropper") is active - copies color under cursor
        auto dt = dynamic_cast<Inkscape::UI::Tools::DropperTool *>(desktop->event_context);
        if (dt) {
            _setClipboardColor(SP_DROPPER_CONTEXT(desktop->event_context)->get_color(false, true));
            _discardInternalClipboard();
            return;
        }

        // Special case for when the text tool is active - if some text is selected, copy plain text,
        // not the object that holds it; also copy the style at cursor into
        auto tt = dynamic_cast<Inkscape::UI::Tools::TextTool *>(desktop->event_context);
        if (tt) {
            _discardInternalClipboard();
            Glib::ustring selected_text = Inkscape::UI::Tools::sp_text_get_selected_text(desktop->event_context);
            _clipboard->set_text(selected_text);
            if (_text_style) {
                sp_repr_css_attr_unref(_text_style);
                _text_style = nullptr;
            }
            _text_style = Inkscape::UI::Tools::sp_text_get_style_at_cursor(desktop->event_context);
            return;
        }

        // Special case for copying part of a path instead of the whole selected object.
        auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool *>(desktop->event_context);
        if (node_tool && node_tool->_selected_nodes) {
            _discardInternalClipboard();
            _createInternalClipboard();

            SPObject *first_path = nullptr;
            for (auto obj : set->items()) {
                if(SP_IS_PATH(obj)) {
                    first_path = obj;
                    break;
                }
            }

            auto builder = new Geom::PathBuilder();
            node_tool->_multipath->copySelectedPath(builder);
            Geom::PathVector pathv = builder->peek();
            // Were any nodes actually copied?
            if (!pathv.empty()) {
                Inkscape::XML::Node *pathRepr = _doc->createElement("svg:path");

                pathRepr->setAttribute("d", sp_svg_write_path(pathv));

                if(first_path) {
                    pathRepr->setAttribute("style", first_path->style->write(SP_STYLE_FLAG_IFSET) );
                }

                _root->appendChild(pathRepr);
                Inkscape::GC::release(pathRepr);
                fit_canvas_to_drawing(_clipboardSPDoc.get());
                _setClipboardTargets();
                return;
            }
        }
    }
    if (set->isEmpty()) {  // check whether something is selected
        _userWarn(set->desktop(), _("Nothing was copied."));
        return;
    }
    _discardInternalClipboard();

    _createInternalClipboard();   // construct a new clipboard document
    _copySelection(set);   // copy all items in the selection to the internal clipboard
    fit_canvas_to_drawing(_clipboardSPDoc.get());

    _setClipboardTargets();
}


/**
 * Copy a Live Path Effect path parameter to the clipboard.
 * @param pp The path parameter to store in the clipboard.
 */
void ClipboardManagerImpl::copyPathParameter(Inkscape::LivePathEffect::PathParam *pp)
{
    if ( pp == nullptr ) {
        return;
    }
    auto svgd = sp_svg_write_path(pp->get_pathvector());
    if (svgd.empty()) {
        return;
    }

    _discardInternalClipboard();
    _createInternalClipboard();

    Inkscape::XML::Node *pathnode = _doc->createElement("svg:path");
    pathnode->setAttribute("d", svgd);
    _root->appendChild(pathnode);
    Inkscape::GC::release(pathnode);

    fit_canvas_to_drawing(_clipboardSPDoc.get());
    _setClipboardTargets();
}

/**
 * Copy a symbol from the symbol dialog.
 * @param symbol The Inkscape::XML::Node for the symbol.
 */
void ClipboardManagerImpl::copySymbol(Inkscape::XML::Node* symbol, gchar const* style, bool user_symbol)
{
    //std::cout << "ClipboardManagerImpl::copySymbol" << std::endl;
    if ( symbol == nullptr ) {
        return;
    }

    _discardInternalClipboard();
    _createInternalClipboard();

    // We add "_duplicate" to have a well defined symbol name that
    // bypasses the "prevent_id_classes" routine. We'll get rid of it
    // when we paste.
    Inkscape::XML::Node *repr = symbol->duplicate(_doc);
    Glib::ustring symbol_name = repr->attribute("id");

    symbol_name += "_inkscape_duplicate";
    repr->setAttribute("id",    symbol_name);
    _defs->appendChild(repr);

    Glib::ustring id("#");
    id += symbol->attribute("id");

    gdouble scale_units = 1; // scale from "px" to "document-units"
    Inkscape::XML::Node *nv_repr = SP_ACTIVE_DESKTOP->getNamedView()->getRepr();
    if (nv_repr->attribute("inkscape:document-units"))
        scale_units = Inkscape::Util::Quantity::convert(1, "px", nv_repr->attribute("inkscape:document-units"));
    SPObject *cmobj = _clipboardSPDoc->getObjectByRepr(repr);
    if (cmobj && !user_symbol) { // convert only stock symbols
        if (!Geom::are_near(scale_units, 1.0, Geom::EPSILON)) {
            dynamic_cast<SPGroup *>(cmobj)->scaleChildItemsRec(
                Geom::Scale(scale_units), Geom::Point(0, SP_ACTIVE_DESKTOP->getDocument()->getHeight().value("px")),
                false);
        }
    }

    Inkscape::XML::Node *use = _doc->createElement("svg:use");
    use->setAttribute("xlink:href", id );
    // Set a default style in <use> rather than <symbol> so it can be changed.
    use->setAttribute("style", style );
    if (!Geom::are_near(scale_units, 1.0, Geom::EPSILON)) {
        auto transform_str = sp_svg_transform_write(Geom::Scale(1.0 / scale_units));
        assert(!transform_str.empty());
        use->setAttribute("transform", transform_str);
    }
    _root->appendChild(use);

    // This min and max sets offsets, we don't have any so set to zero.
    sp_repr_set_point(_clipnode, "min", Geom::Point(0,0));
    sp_repr_set_point(_clipnode, "max", Geom::Point(0,0));

    fit_canvas_to_drawing(_clipboardSPDoc.get());
    _setClipboardTargets();
}

/**
 * Paste from the system clipboard into the active desktop.
 * @param in_place Whether to put the contents where they were when copied.
 */
bool ClipboardManagerImpl::paste(SPDesktop *desktop, bool in_place)
{
    // do any checking whether we really are able to paste before requesting the contents
    if ( desktop == nullptr ) {
        return false;
    }
    if ( Inkscape::have_viable_layer(desktop, desktop->getMessageStack()) == false ) {
        return false;
    }
    
    Glib::ustring target = _getBestTarget();

    // Special cases of clipboard content handling go here
    // Note that target priority is determined in _getBestTarget.
    // TODO: Handle x-special/gnome-copied-files and text/uri-list to support pasting files

    // if there is an image on the clipboard, paste it
    if ( target == CLIPBOARD_GDK_PIXBUF_TARGET ) {
        return _pasteImage(desktop->doc());
    }
    // if there's only text, paste it into a selected text object or create a new one
    if ( target == CLIPBOARD_TEXT_TARGET ) {
        return _pasteText(desktop);
    }

    // otherwise, use the import extensions
    auto tempdoc = _retrieveClipboard(target);
    if ( tempdoc == nullptr ) {
        _userWarn(desktop, _("Nothing on the clipboard."));
        return false;
    }

    /* Special paste nodes handle; only if:
     *   node tool selected
     *   one path selected in target
     *   one path in source
     */
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool *>(desktop->event_context);
    if (node_tool && desktop->selection->objects().size() == 1) { 
        SPObject *obj = desktop->selection->objects().back();
        auto target_path = SP_PATH(obj);
        if (target_path) {
            auto clipdoc = tempdoc.get();
            auto source_scale = clipdoc->getDocumentScale();
            auto target_trans = target_path->i2doc_affine();
            for (auto node = clipdoc->getReprRoot()->firstChild(); node ;node = node->next()) {
                auto source_path = SP_PATH(clipdoc->getObjectByRepr(node));
                if (source_path) {
                    auto source_curve = SPCurve::copy(source_path->curveForEdit());
                    auto target_curve = SPCurve::copy(target_path->curveForEdit());

                    // Convert curve from source units (usually px so 1:1)
                    source_curve->transform(source_scale);

                    // Move the source curve to the mouse pointer, units are px so do before target_trans
                    auto to_mouse = Geom::Translate(desktop->point() - source_path->geometricBounds()->midpoint());
                    source_curve->transform(to_mouse);

                    // Finally convert the curve into path's item tcoordinate system
                    source_curve->transform(target_trans.inverse());

                    // Add the source curve to the target copy
                    target_curve->append(*source_curve);

                    // Set the attribute to keep the document up to date (fixes undo)
                    auto str = sp_svg_write_path(target_curve->get_pathvector());
                    target_path->setAttribute("d", str);
                }
            }
            return true;
        }
    }

    sp_import_document(desktop, tempdoc.get(), in_place);

    // _copySelection() has put all items in groups, now ungroup them (preserves transform
    // relationships of clones, text-on-path, etc.)
    desktop->selection->ungroup();

    return true;
}

/**
 * Returns the id of the first visible copied object.
 */
Glib::ustring ClipboardManagerImpl::getFirstObjectID()
{
    auto tempdoc = _retrieveClipboard("image/x-inkscape-svg");
    if ( tempdoc == nullptr ) {
        return {};
    }

    Inkscape::XML::Node *root = tempdoc->getReprRoot();

    if (!root) {
        return {};
    }

    Inkscape::XML::Node *ch = root->firstChild();
    Inkscape::XML::Node *child = nullptr;
    // now clipboard is wrapped on copy since 202d57ea fix
    while (ch != nullptr &&
           g_strcmp0(ch->name(), "svg:g") &&
           g_strcmp0(child?child->name():nullptr, "svg:g") &&
           g_strcmp0(child?child->name():nullptr, "svg:path") &&
           g_strcmp0(child?child->name():nullptr, "svg:use") &&
           g_strcmp0(child?child->name():nullptr, "svg:text") &&
           g_strcmp0(child?child->name():nullptr, "svg:image") &&
           g_strcmp0(child?child->name():nullptr, "svg:rect") &&
           g_strcmp0(child?child->name():nullptr, "svg:ellipse") &&
           g_strcmp0(child?child->name():nullptr, "svg:circle")
        ) {
        ch = ch->next();
        child = ch ? ch->firstChild(): nullptr;
    }

    if (child) {
        char const *id = child->attribute("id");
        if (id) {
            return id;
        }
    }

    return {};
}


/**
 * Implements the Paste Style action.
 */
bool ClipboardManagerImpl::pasteStyle(ObjectSet *set)
{
    if (set->desktop() == nullptr) {
        return false;
    }

    // check whether something is selected
    if (set->isEmpty()) {
        _userWarn(set->desktop(), _("Select <b>object(s)</b> to paste style to."));
        return false;
    }

    auto tempdoc = _retrieveClipboard("image/x-inkscape-svg");
    if ( tempdoc == nullptr ) {
        // no document, but we can try _text_style
        if (_text_style) {
            sp_desktop_set_style(set, set->desktop(), _text_style);
            return true;
        } else {
            _userWarn(set->desktop(), _("No style on the clipboard."));
            return false;
        }
    }

    Inkscape::XML::Node *root = tempdoc->getReprRoot();
    Inkscape::XML::Node *clipnode = sp_repr_lookup_name(root, "inkscape:clipboard", 1);

    bool pasted = false;

    if (clipnode) {
        set->document()->importDefs(tempdoc.get());
        SPCSSAttr *style = sp_repr_css_attr(clipnode, "style");
        sp_desktop_set_style(set, set->desktop(), style);
        pasted = true;
    }
    else {
        _userWarn(set->desktop(), _("No style on the clipboard."));
    }

    return pasted;
}


/**
 * Resize the selection or each object in the selection to match the clipboard's size.
 * @param separately Whether to scale each object in the selection separately
 * @param apply_x Whether to scale the width of objects / selection
 * @param apply_y Whether to scale the height of objects / selection
 */
bool ClipboardManagerImpl::pasteSize(ObjectSet *set, bool separately, bool apply_x, bool apply_y)
{
    if (!apply_x && !apply_y) {
        return false; // pointless parameters
    }

/*    if ( desktop == NULL ) {
        return false;
    }
    Inkscape::Selection *selection = desktop->getSelection();*/
    if (set->isEmpty()) {
        if(set->desktop())
            _userWarn(set->desktop(), _("Select <b>object(s)</b> to paste size to."));
        return false;
    }

    // FIXME: actually, this should accept arbitrary documents
    auto tempdoc = _retrieveClipboard("image/x-inkscape-svg");
    if ( tempdoc == nullptr ) {
        if(set->desktop())
            _userWarn(set->desktop(), _("No size on the clipboard."));
        return false;
    }

    // retrieve size information from the clipboard
    Inkscape::XML::Node *root = tempdoc->getReprRoot();
    Inkscape::XML::Node *clipnode = sp_repr_lookup_name(root, "inkscape:clipboard", 1);
    bool pasted = false;
    if (clipnode) {
        Geom::Point min, max;
        bool visual_bbox = !Inkscape::Preferences::get()->getInt("/tools/bounding_box");
        sp_repr_get_point(clipnode, (visual_bbox ? "min" : "geom-min"), &min);
        sp_repr_get_point(clipnode, (visual_bbox ? "max" : "geom-max"), &max);

        // resize each object in the selection
        if (separately) {
            auto itemlist= set->items();
            for(auto i=itemlist.begin();i!=itemlist.end();++i){
                SPItem *item = *i;
                if (item) {
                    Geom::OptRect obj_size = item->desktopPreferredBounds();
                    if ( obj_size ) {
                        item->scale_rel(_getScale(set->desktop(), min, max, *obj_size, apply_x, apply_y));
                    }
                } else {
                    g_assert_not_reached();
                }
            }
        }
        // resize the selection as a whole
        else {
            Geom::OptRect sel_size = set->preferredBounds();
            if ( sel_size ) {
                set->setScaleRelative(sel_size->midpoint(),
                                             _getScale(set->desktop(), min, max, *sel_size, apply_x, apply_y));
            }
        }
        pasted = true;
    }
    return pasted;
}


/**
 * Applies a path effect from the clipboard to the selected path.
 */
bool ClipboardManagerImpl::pastePathEffect(ObjectSet *set)
{
    /** @todo FIXME: pastePathEffect crashes when moving the path with the applied effect,
        segfaulting in fork_private_if_necessary(). */

    if ( set->desktop() == nullptr ) {
        return false;
    }

    //Inkscape::Selection *selection = desktop->getSelection();
    if (!set || set->isEmpty()) {
        _userWarn(set->desktop(), _("Select <b>object(s)</b> to paste live path effect to."));
        return false;
    }

    auto tempdoc = _retrieveClipboard("image/x-inkscape-svg");
    if ( tempdoc ) {
        Inkscape::XML::Node *root = tempdoc->getReprRoot();
        Inkscape::XML::Node *clipnode = sp_repr_lookup_name(root, "inkscape:clipboard", 1);
        if ( clipnode ) {
            gchar const *effectstack = clipnode->attribute("inkscape:path-effect");
            if ( effectstack ) {
                set->document()->importDefs(tempdoc.get());
                // make sure all selected items are converted to paths first (i.e. rectangles)
                set->toLPEItems();
                auto itemlist= set->items();
                for(auto i=itemlist.begin();i!=itemlist.end();++i){
                    SPItem *item = *i;
                    _applyPathEffect(item, effectstack);
                }

                return true;
            }
        }
    }

    // no_effect:
    _userWarn(set->desktop(), _("No effect on the clipboard."));
    return false;
}


/**
 * Get LPE path data from the clipboard.
 * @return The retrieved path data (contents of the d attribute), or "" if no path was found
 */
Glib::ustring ClipboardManagerImpl::getPathParameter(SPDesktop* desktop)
{
    auto tempdoc = _retrieveClipboard(); // any target will do here
    if ( tempdoc == nullptr ) {
        _userWarn(desktop, _("Nothing on the clipboard."));
        return "";
    }
    Inkscape::XML::Node *root = tempdoc->getReprRoot();
    Inkscape::XML::Node *path = sp_repr_lookup_name(root, "svg:path", -1); // unlimited search depth
    if ( path == nullptr ) {
        _userWarn(desktop, _("Clipboard does not contain a path."));
        return "";
    }
    gchar const *svgd = path->attribute("d");
    return svgd ? svgd : "";
}


/**
 * Get object id of a shape or text item from the clipboard.
 * @return The retrieved id string (contents of the id attribute), or "" if no shape or text item was found.
 */
Glib::ustring ClipboardManagerImpl::getShapeOrTextObjectId(SPDesktop *desktop)
{
    // https://bugs.launchpad.net/inkscape/+bug/1293979
    // basically, when we do a depth-first search, we're stopping
    // at the first object to be <svg:path> or <svg:text>.
    // but that could then return the id of the object's
    // clip path or mask, not the original path!

    auto tempdoc = _retrieveClipboard(); // any target will do here
    if ( tempdoc == nullptr ) {
        _userWarn(desktop, _("Nothing on the clipboard."));
        return "";
    }
    Inkscape::XML::Node *root = tempdoc->getReprRoot();

    // 1293979: strip out the defs of the document
    root->removeChild(tempdoc->getDefs()->getRepr());

    Inkscape::XML::Node *repr = sp_repr_lookup_name(root, "svg:path", -1); // unlimited search depth
    if ( repr == nullptr ) {
        repr = sp_repr_lookup_name(root, "svg:text", -1);
    }
    if (repr == nullptr) {
        repr = sp_repr_lookup_name(root, "svg:ellipse", -1);
    }
    if (repr == nullptr) {
        repr = sp_repr_lookup_name(root, "svg:rect", -1);
    }
    if (repr == nullptr) {
        repr = sp_repr_lookup_name(root, "svg:circle", -1);
    }


    if ( repr == nullptr ) {
        _userWarn(desktop, _("Clipboard does not contain a path."));
        return "";
    }
    gchar const *svgd = repr->attribute("id");
    return svgd ? svgd : "";
}

/**
 * Get all objects id  from the clipboard.
 * @return A vector containing all IDs or empty if no shape or text item was found.
 * type. Set to "*" to retrieve all elements of the types vector inside, feel free to populate more
 */
std::vector<Glib::ustring> ClipboardManagerImpl::getElementsOfType(SPDesktop *desktop, gchar const* type, gint maxdepth)
{
    std::vector<Glib::ustring> result;
    auto tempdoc = _retrieveClipboard(); // any target will do here
    if ( tempdoc == nullptr ) {
        _userWarn(desktop, _("Nothing on the clipboard."));
        return result;
    }
    Inkscape::XML::Node *root = tempdoc->getReprRoot();

    // 1293979: strip out the defs of the document
    root->removeChild(tempdoc->getDefs()->getRepr());
    std::vector<Inkscape::XML::Node const *> reprs;
    if (strcmp(type, "*") == 0){
        //TODO:Fill vector with all possible elements
        std::vector<Glib::ustring> types;
        types.push_back((Glib::ustring)"svg:path");
        types.push_back((Glib::ustring)"svg:circle");
        types.push_back((Glib::ustring)"svg:rect");
        types.push_back((Glib::ustring)"svg:ellipse");
        types.push_back((Glib::ustring)"svg:text");
        types.push_back((Glib::ustring)"svg:use");
        types.push_back((Glib::ustring)"svg:g");
        types.push_back((Glib::ustring)"svg:image");
        for (auto type_elem : types) {
            std::vector<Inkscape::XML::Node const *> reprs_found = sp_repr_lookup_name_many(root, type_elem.c_str(), maxdepth); // unlimited search depth
            reprs.insert(reprs.end(), reprs_found.begin(), reprs_found.end());
        }
    } else {
        reprs = sp_repr_lookup_name_many(root, type, maxdepth);
    }
    for (auto node : reprs) {
        result.emplace_back(node->attribute("id"));
    }
    if ( result.empty() ) {
        _userWarn(desktop, (Glib::ustring::compose(_("Clipboard does not contain any objects of type \"%1\"."), type)).c_str());
        return result;
    }
    return result;
}

/**
 * Iterate over a list of items and copy them to the clipboard.
 */
void ClipboardManagerImpl::_copySelection(ObjectSet *selection)
{
    // copy the defs used by all items
    auto itemlist= selection->items();
    cloned_elements.clear();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        if (item) {
            _copyUsedDefs(item);
        } else {
            g_assert_not_reached();
        }
    }

    // copy the representation of the items
    std::vector<SPObject*> sorted_items(itemlist.begin(), itemlist.end());
    {
        // Get external text references and add them to sorted_items
        auto ext_refs = text_categorize_refs(selection->document(),
                sorted_items.begin(), sorted_items.end(),
                TEXT_REF_EXTERNAL);
        for (auto const &ext_ref : ext_refs) {
            sorted_items.push_back(selection->document()->getObjectById(ext_ref.first));
        }
    }
    sort(sorted_items.begin(), sorted_items.end(), sp_object_compare_position_bool);

    //remove already copied elements from cloned_elements
    std::vector<SPItem*>tr;
    for(auto cloned_element : cloned_elements){
        if(std::find(sorted_items.begin(),sorted_items.end(),cloned_element)!=sorted_items.end())
            tr.push_back(cloned_element);
    }
    for(auto & it : tr){
        cloned_elements.erase(it);
    }

    // One group per shared parent
    std::map<SPObject const *, Inkscape::XML::Node *> groups;

    sorted_items.insert(sorted_items.end(),cloned_elements.begin(),cloned_elements.end());
    for(auto sorted_item : sorted_items){
        SPItem *item = dynamic_cast<SPItem*>(sorted_item);
        if (item) {
            // Create a group with the parent transform. This group will be ungrouped when pasting
            // und takes care of transform relationships of clones, text-on-path, etc.
            auto &group = groups[item->parent];
            if (!group) {
                group = _doc->createElement("svg:g");
                _root->appendChild(group);
                Inkscape::GC::release(group);

                if (auto parent = dynamic_cast<SPItem *>(item->parent)) {
                    auto transform_str = sp_svg_transform_write(parent->i2doc_affine());
                    group->setAttributeOrRemoveIfEmpty("transform", transform_str);
                }
            }

            Inkscape::XML::Node *obj = item->getRepr();
            Inkscape::XML::Node *obj_copy;
            if(cloned_elements.find(item)==cloned_elements.end())
                obj_copy = _copyNode(obj, _doc, group);
            else
                obj_copy = _copyNode(obj, _doc, _clipnode);

            // copy complete inherited style
            SPCSSAttr *css = sp_repr_css_attr_inherited(obj, "style");
            for (auto iter : item->style->properties()) {
                if (iter->style_src == SPStyleSrc::STYLE_SHEET) {
                    css->setAttributeOrRemoveIfEmpty(iter->name(), iter->get_value());
                }
            }
            sp_repr_css_set(obj_copy, css, "style");
            sp_repr_css_attr_unref(css);
        }
    }

    // copy style for Paste Style action
    if (!sorted_items.empty()) {
        SPObject *object = sorted_items[0];
        SPItem *item = dynamic_cast<SPItem *>(object);
        if (item) {
            SPCSSAttr *style = take_style_from_item(item);
            sp_repr_css_set(_clipnode, style, "style");
            sp_repr_css_attr_unref(style);
        }
        // copy path effect from the first path
        if (object) {
            gchar const *effect =object->getRepr()->attribute("inkscape:path-effect");
            if (effect) {
                _clipnode->setAttribute("inkscape:path-effect", effect);
            }
        }
    }

    if (Geom::OptRect size = selection->visualBounds()) {
        sp_repr_set_point(_clipnode, "min", size->min());
        sp_repr_set_point(_clipnode, "max", size->max());
    }
    if (Geom::OptRect geom_size = selection->geometricBounds()) {
        sp_repr_set_point(_clipnode, "geom-min", geom_size->min());
        sp_repr_set_point(_clipnode, "geom-max", geom_size->max());
    }
}


/**
 * Recursively copy all the definitions used by a given item to the clipboard defs.
 */
void ClipboardManagerImpl::_copyUsedDefs(SPItem *item)
{
    SPUse *use=dynamic_cast<SPUse *>(item);
    if (use && use->get_original()) {
        if(cloned_elements.insert(use->get_original()).second)
            _copyUsedDefs(use->get_original());
    }

    // copy fill and stroke styles (patterns and gradients)
    SPStyle *style = item->style;

    if (style && (style->fill.isPaintserver())) {
        SPPaintServer *server = item->style->getFillPaintServer();
        if ( dynamic_cast<SPLinearGradient *>(server) || dynamic_cast<SPRadialGradient *>(server) || dynamic_cast<SPMeshGradient *>(server) ) {
            _copyGradient(dynamic_cast<SPGradient *>(server));
        }
        SPPattern *pattern = dynamic_cast<SPPattern *>(server);
        if (pattern) {
            _copyPattern(pattern);
        }
        SPHatch *hatch = dynamic_cast<SPHatch *>(server);
        if (hatch) {
            _copyHatch(hatch);
        }
    }
    if (style && (style->stroke.isPaintserver())) {
        SPPaintServer *server = item->style->getStrokePaintServer();
        if ( dynamic_cast<SPLinearGradient *>(server) || dynamic_cast<SPRadialGradient *>(server) || dynamic_cast<SPMeshGradient *>(server) ) {
            _copyGradient(dynamic_cast<SPGradient *>(server));
        }
        SPPattern *pattern = dynamic_cast<SPPattern *>(server);
        if (pattern) {
            _copyPattern(pattern);
        }
        SPHatch *hatch = dynamic_cast<SPHatch *>(server);
        if (hatch) {
            _copyHatch(hatch);
        }
    }

    // For shapes, copy all of the shape's markers
    SPShape *shape = dynamic_cast<SPShape *>(item);
    if (shape) {
        for (auto & i : shape->_marker) {
            if (i) {
                _copyNode(i->getRepr(), _doc, _defs);
            }
        }
    }

    // For 3D boxes, copy perspectives
    {
        SPBox3D *box = dynamic_cast<SPBox3D *>(item);
        if (box) {
            _copyNode(box->get_perspective()->getRepr(), _doc, _defs);
        }
    }

    // Copy text paths
    {
        SPText *text = dynamic_cast<SPText *>(item);
        SPTextPath *textpath = (text) ? dynamic_cast<SPTextPath *>(text->firstChild()) : nullptr;
        if (textpath) {
            _copyTextPath(textpath);
        }
        if (text) {
            for (auto &&shape_prop_ptr : {
                    reinterpret_cast<SPIShapes SPStyle::*>(&SPStyle::shape_inside),
                    reinterpret_cast<SPIShapes SPStyle::*>(&SPStyle::shape_subtract) }) {
                for (auto *href : (text->style->*shape_prop_ptr).hrefs) {
                    auto shape_obj = href->getObject();
                    if (!shape_obj)
                        continue;
                    auto shape_repr = shape_obj->getRepr();
                    if (sp_repr_is_def(shape_repr)) {
                        _copyIgnoreDup(shape_repr, _doc, _defs);
                    }
                }
            }
        }
    }

    // Copy clipping objects
    if (SPObject *clip = item->getClipObject()) {
        _copyNode(clip->getRepr(), _doc, _defs);
    }
    // Copy mask objects
    if (SPObject *mask = item->getMaskObject()) {
            _copyNode(mask->getRepr(), _doc, _defs);
            // recurse into the mask for its gradients etc.
            for(auto& o: mask->children) {
                SPItem *childItem = dynamic_cast<SPItem *>(&o);
                if (childItem) {
                    _copyUsedDefs(childItem);
                }
            }
    }

    // Copy filters
    if (style->getFilter()) {
        SPObject *filter = style->getFilter();
        if (dynamic_cast<SPFilter *>(filter)) {
            _copyNode(filter->getRepr(), _doc, _defs);
        }
    }

    // For lpe items, copy lpe stack if applicable
    SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(item);
    if (lpeitem) {
        if (lpeitem->hasPathEffect()) {
            PathEffectList path_effect_list( *lpeitem->path_effect_list);
            for (auto &lperef : path_effect_list) {
                LivePathEffectObject *lpeobj = lperef->lpeobject;
                if (lpeobj) {
                  _copyNode(lpeobj->getRepr(), _doc, _defs);
                }
            }
        }
    }

    // recurse
    for(auto& o: item->children) {
        SPItem *childItem = dynamic_cast<SPItem *>(&o);
        if (childItem) {
            _copyUsedDefs(childItem);
        }
    }
}

/**
 * Copy a single gradient to the clipboard's defs element.
 */
void ClipboardManagerImpl::_copyGradient(SPGradient *gradient)
{
    while (gradient) {
        // climb up the refs, copying each one in the chain
        _copyNode(gradient->getRepr(), _doc, _defs);
        if (gradient->ref){
            gradient = gradient->ref->getObject();
        }
        else {
            gradient = nullptr;
        }
    }
}


/**
 * Copy a single pattern to the clipboard document's defs element.
 */
void ClipboardManagerImpl::_copyPattern(SPPattern *pattern)
{
    // climb up the references, copying each one in the chain
    while (pattern) {
        _copyNode(pattern->getRepr(), _doc, _defs);

        // items in the pattern may also use gradients and other patterns, so recurse
        for (auto& child: pattern->children) {
            SPItem *childItem = dynamic_cast<SPItem *>(&child);
            if (childItem) {
                _copyUsedDefs(childItem);
            }
        }
        if (pattern->ref){
            pattern = pattern->ref->getObject();
        }
        else{
            pattern = nullptr;
        }
    }
}

/**
 * Copy a single hatch to the clipboard document's defs element.
 */
void ClipboardManagerImpl::_copyHatch(SPHatch *hatch)
{
    // climb up the references, copying each one in the chain
    while (hatch) {
        _copyNode(hatch->getRepr(), _doc, _defs);

        for (auto &child : hatch->children) {
            SPItem *childItem = dynamic_cast<SPItem *>(&child);
            if (childItem) {
                _copyUsedDefs(childItem);
            }
        }
        if (hatch->ref) {
            hatch = hatch->ref->getObject();
        } else {
            hatch = nullptr;
        }
    }
}


/**
 * Copy a text path to the clipboard's defs element.
 */
void ClipboardManagerImpl::_copyTextPath(SPTextPath *tp)
{
    SPItem *path = sp_textpath_get_path_item(tp);
    if (!path) {
        return;
    }
    // textpaths that aren't in defs (on the canvas) shouldn't be copied because if
    // both objects are being copied already, this ends up stealing the refs id.
    if(path->parent && SP_IS_DEFS(path->parent)) {
        _copyIgnoreDup(path->getRepr(), _doc, _defs);
    }
}


/**
 * Copy a single XML node from one document to another.
 * @param node The node to be copied
 * @param target_doc The document to which the node is to be copied
 * @param parent The node in the target document which will become the parent of the copied node
 * @return Pointer to the copied node
 */
Inkscape::XML::Node *ClipboardManagerImpl::_copyNode(Inkscape::XML::Node *node, Inkscape::XML::Document *target_doc, Inkscape::XML::Node *parent)
{
    Inkscape::XML::Node *dup = node->duplicate(target_doc);
    parent->appendChild(dup);
    Inkscape::GC::release(dup);
    return dup;
}

Inkscape::XML::Node *ClipboardManagerImpl::_copyIgnoreDup(Inkscape::XML::Node *node, Inkscape::XML::Document *target_doc, Inkscape::XML::Node *parent)
{
    if (sp_repr_lookup_child(_root, "id", node->attribute("id"))) {
        // node already copied
        return nullptr;
    }
    Inkscape::XML::Node *dup = node->duplicate(target_doc);
    parent->appendChild(dup);
    Inkscape::GC::release(dup);
    return dup;
}


/**
 * Retrieve a bitmap image from the clipboard and paste it into the active document.
 */
bool ClipboardManagerImpl::_pasteImage(SPDocument *doc)
{
    if ( doc == nullptr ) {
        return false;
    }

    // retrieve image data
    Glib::RefPtr<Gdk::Pixbuf> img = _clipboard->wait_for_image();
    if (!img) {
        return false;
    }

    Inkscape::Extension::Extension *png = Inkscape::Extension::find_by_mime("image/png");
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring attr_saved = prefs->getString("/dialogs/import/link");
    bool ask_saved = prefs->getBool("/dialogs/import/ask");
    prefs->setString("/dialogs/import/link", "embed");
    prefs->setBool("/dialogs/import/ask", false);
    png->set_gui(false);

    gchar *filename = g_build_filename( g_get_user_cache_dir(), "inkscape-clipboard-import", NULL );
    img->save(filename, "png");
    file_import(doc, filename, png);
    g_free(filename);
    prefs->setString("/dialogs/import/link", attr_saved);
    prefs->setBool("/dialogs/import/ask", ask_saved);
    png->set_gui(true);

    return true;
}

/**
 * Paste text into the selected text object or create a new one to hold it.
 */
bool ClipboardManagerImpl::_pasteText(SPDesktop *desktop)
{
    if ( desktop == nullptr ) {
        return false;
    }

    // if the text editing tool is active, paste the text into the active text object
    if (dynamic_cast<Inkscape::UI::Tools::TextTool *>(desktop->event_context)) {
        return Inkscape::UI::Tools::sp_text_paste_inline(desktop->event_context);
    }
    return false;
        /* return false;
        //apply the saved style to pasted text
        Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();
        Glib::ustring const clip_text = refClipboard->wait_for_text();
        Glib::ustring text(clip_text);
        if(text.length() == copied_style_length)
        {
            Inkscape::UI::Tools::TextTool *tc = SP_TEXT_CONTEXT(desktop->event_context);
            // we realy only want to inherit container style (to act as 0.92 and faster performance)
            // maybe for 1.0 we can make a special type of clipboard
            // that handle layout or maybe we can use the last desktop text style
            // so I comment unneded code.
            Inkscape::Text::Layout const *layout = te_get_layout(tc->text);
            Inkscape::Text::Layout::iterator it_next;
            Inkscape::Text::Layout::iterator it = tc->text_sel_end;
            SPText *textitem = dynamic_cast<SPText *>(tc->text);
            if (textitem) {
                textitem->rebuildLayout();
            }
            SPFlowtext *flowtext = dynamic_cast<SPFlowtext *>(tc->text);
            if (flowtext) {
                flowtext->rebuildLayout();
            }
            // we realy only want to inherit container style
            SPCSSAttr *css = take_style_from_item(tc->text);
            for (int i = 0; i < nr_blocks; ++i)
            {
                gchar const *w = sp_repr_css_property(css, "font-size", "0px");

                // Don't set font-size if it wasn't set.
                if (w && strcmp(w, "0px") != 0) {
                    sp_repr_css_set_property(te_selected_style[i], "font-size", w);
                }
            }

            for (unsigned int i = 0; i < text.length(); ++i)
                it.prevCharacter();

            it_next = layout->charIndexToIterator(layout->iteratorToCharIndex(it));

            for (int i = 0; i < nr_blocks; ++i)
            {
                for (unsigned int j = te_selected_style_positions[i]; j < te_selected_style_positions[i+1]; ++j)
                    it_next.nextCharacter();

                // sp_te_apply_style(tc->text, it, it_next, te_selected_style[i]);
                te_update_layout_now_recursive(tc->text);
                tc->text_sel_end = it;
                for (unsigned int j = te_selected_style_positions[i]; j < te_selected_style_positions[i+1]; ++j)
                    it.nextCharacter();
            }
        }
        return true;

    }
    // old(try to parse the text as a color and, if successful, apply it as the current style)
    // we realy only want to inherit container style
    // maybe for 1.0 we can make a special type of clipboard
    // that handle layout or maybe we can use the last desktop text style
    SPCSSAttr *css = sp_repr_css_attr_parse_color_to_fill(_clipboard->wait_for_text());
    if (css) {
        sp_desktop_set_style(desktop, css);
        return true;
    }
    return false;
    */
}


/**
 * Applies a pasted path effect to a given item.
 */
void ClipboardManagerImpl::_applyPathEffect(SPItem *item, gchar const *effectstack)
{
    if ( item == nullptr ) {
        return;
    }

    SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(item);
    if (lpeitem && effectstack) {
        std::istringstream iss(effectstack);
        std::string href;
        while (std::getline(iss, href, ';'))
        {
            SPObject *obj = sp_uri_reference_resolve(_clipboardSPDoc.get(), href.c_str());
            if (!obj) {
                return;
            }
            LivePathEffectObject *lpeobj = dynamic_cast<LivePathEffectObject *>(obj);
            if (lpeobj) {
                lpeitem->addPathEffect(lpeobj);
            }
        }
        // for each effect in the stack, check if we need to fork it before adding it to the item
        lpeitem->forkPathEffectsIfNecessary(1);
    }
}


/**
 * Retrieve the clipboard contents as a document.
 * @return Clipboard contents converted to SPDocument, or NULL if no suitable content was present
 */
std::unique_ptr<SPDocument> ClipboardManagerImpl::_retrieveClipboard(Glib::ustring required_target)
{
    Glib::ustring best_target;
    if ( required_target == "" ) {
        best_target = _getBestTarget();
    } else {
        best_target = required_target;
    }

    if ( best_target == "" ) {
        return nullptr;
    }

    // FIXME: Temporary hack until we add memory input.
    // Save the clipboard contents to some file, then read it
    gchar *filename = g_build_filename( g_get_user_cache_dir(), "inkscape-clipboard-import", NULL );

    bool file_saved = false;
    Glib::ustring target = best_target;

#ifdef _WIN32
    if (best_target == "CF_ENHMETAFILE" || best_target == "WCF_ENHMETAFILE")
    {   // Try to save clipboard data as en emf file (using win32 api)
        if (OpenClipboard(NULL)) {
            HGLOBAL hglb = GetClipboardData(CF_ENHMETAFILE);
            if (hglb) {
                HENHMETAFILE hemf = CopyEnhMetaFile((HENHMETAFILE) hglb, filename);
                if (hemf) {
                    file_saved = true;
                    target = "image/x-emf";
                    DeleteEnhMetaFile(hemf);
                }
            }
            CloseClipboard();
        }
    }
#endif

    if (!file_saved) {
        if ( !_clipboard->wait_is_target_available(best_target) ) {
            return nullptr;
        }

        // doing this synchronously makes better sense
        // TODO: use another method because this one is badly broken imo.
        // from documentation: "Returns: A SelectionData object, which will be invalid if retrieving the given target failed."
        // I don't know how to check whether an object is 'valid' or not, unusable if that's not possible...
        Gtk::SelectionData sel = _clipboard->wait_for_contents(best_target);
        target = sel.get_target();  // this can crash if the result was invalid of last function. No way to check for this :(

        // FIXME: Temporary hack until we add memory input.
        // Save the clipboard contents to some file, then read it
        g_file_set_contents(filename, (const gchar *) sel.get_data(), sel.get_length(), nullptr);
    }

    // there is no specific plain SVG input extension, so if we can paste the Inkscape SVG format,
    // we use the image/svg+xml mimetype to look up the input extension
    if (target == "image/x-inkscape-svg") {
        target = "image/svg+xml";
    }
    // Use the EMF extension to import metafiles
    if (target == "CF_ENHMETAFILE" || target == "WCF_ENHMETAFILE") {
        target = "image/x-emf";
    }

    Inkscape::Extension::DB::InputList inlist;
    Inkscape::Extension::db.get_input_list(inlist);
    Inkscape::Extension::DB::InputList::const_iterator in = inlist.begin();
    for (; in != inlist.end() && target != (*in)->get_mimetype() ; ++in) {
    };
    if ( in == inlist.end() ) {
        return nullptr; // this shouldn't happen unless _getBestTarget returns something bogus
    }

    SPDocument *tempdoc = nullptr;
    try {
        tempdoc = (*in)->open(filename);
    } catch (...) {
    }
    g_unlink(filename);
    g_free(filename);

    return std::unique_ptr<SPDocument>(tempdoc);
}


/**
 * Callback called when some other application requests data from Inkscape.
 *
 * Finds a suitable output extension to save the internal clipboard document,
 * then saves it to memory and sets the clipboard contents.
 */
void ClipboardManagerImpl::_onGet(Gtk::SelectionData &sel, guint /*info*/)
{
    if (_clipboardSPDoc == nullptr)
        return;

    Glib::ustring target = sel.get_target();
    if (target == "") {
        return; // this shouldn't happen
    }

    if (target == CLIPBOARD_TEXT_TARGET) {
        target = "image/x-inkscape-svg";
    }

    Inkscape::Extension::DB::OutputList outlist;
    Inkscape::Extension::db.get_output_list(outlist);
    Inkscape::Extension::DB::OutputList::const_iterator out = outlist.begin();
    for ( ; out != outlist.end() && target != (*out)->get_mimetype() ; ++out) {
    };
    if ( out == outlist.end() && target != "image/png") {
        // This happens when hitting "optpng" extensions
        return;
    }

    // FIXME: Temporary hack until we add support for memory output.
    // Save to a temporary file, read it back and then set the clipboard contents
    gchar *filename = g_build_filename( g_get_user_cache_dir(), "inkscape-clipboard-export", NULL );
    gchar *data = nullptr;
    gsize len;

    // XXX This is a crude fix for clipboards accessing extensions
    // Remove when gui is extracted from extension execute and uses exceptions.
    bool previous_gui = INKSCAPE.use_gui();
    INKSCAPE.use_gui(false);

    try {
        if (out == outlist.end() && target == "image/png")
        {
            gdouble dpi = Inkscape::Util::Quantity::convert(1, "in", "px");
            guint32 bgcolor = 0x00000000;

            Geom::Point origin (_clipboardSPDoc->getRoot()->x.computed, _clipboardSPDoc->getRoot()->y.computed);
            Geom::Rect area = Geom::Rect(origin, origin + _clipboardSPDoc->getDimensions());

            unsigned long int width = (unsigned long int) (area.width() + 0.5);
            unsigned long int height = (unsigned long int) (area.height() + 0.5);

            // read from namedview
            Inkscape::XML::Node *nv = _clipboardSPDoc->getReprNamedView();
            if (nv && nv->attribute("pagecolor")) {
                bgcolor = sp_svg_read_color(nv->attribute("pagecolor"), 0xffffff00);
            }
            if (nv && nv->attribute("inkscape:pageopacity")) {
                double opacity = 1.0;
                sp_repr_get_double(nv, "inkscape:pageopacity", &opacity);
                bgcolor |= SP_COLOR_F_TO_U(opacity);
            }
            std::vector<SPItem*> x;
            sp_export_png_file(_clipboardSPDoc.get(), filename, area, width, height, dpi, dpi, bgcolor, nullptr,
                               nullptr, true, x);
        }
        else
        {
            if (!(*out)->loaded()) {
                // Need to load the extension.
                (*out)->set_state(Inkscape::Extension::Extension::STATE_LOADED);
            }

            (*out)->save(_clipboardSPDoc.get(), filename, true);
        }
        g_file_get_contents(filename, &data, &len, nullptr);

        sel.set(8, (guint8 const *) data, len);
    } catch (...) {
    }

    INKSCAPE.use_gui(previous_gui);
    g_unlink(filename); // delete the temporary file
    g_free(filename);
    g_free(data);
}


/**
 * Callback when someone else takes the clipboard.
 *
 * When the clipboard owner changes, this callback clears the internal clipboard document
 * to reduce memory usage.
 */
void ClipboardManagerImpl::_onClear()
{
    // why is this called before _onGet???
    //_discardInternalClipboard();
}


/**
 * Creates an internal clipboard document from scratch.
 */
void ClipboardManagerImpl::_createInternalClipboard()
{
    if ( _clipboardSPDoc == nullptr ) {
        _clipboardSPDoc.reset(SPDocument::createNewDoc(nullptr, false, true));
        //g_assert( _clipboardSPDoc != NULL );
        _defs = _clipboardSPDoc->getDefs()->getRepr();
        _doc = _clipboardSPDoc->getReprDoc();
        _root = _clipboardSPDoc->getReprRoot();

        if (SP_ACTIVE_DOCUMENT) {
            _clipboardSPDoc->setDocumentBase(SP_ACTIVE_DOCUMENT->getDocumentBase());
        }

        _clipnode = _doc->createElement("inkscape:clipboard");
        _root->appendChild(_clipnode);
        Inkscape::GC::release(_clipnode);

        // once we create a SVG document, style will be stored in it, so flush _text_style
        if (_text_style) {
            sp_repr_css_attr_unref(_text_style);
            _text_style = nullptr;
        }
    }
}


/**
 * Deletes the internal clipboard document.
 */
void ClipboardManagerImpl::_discardInternalClipboard()
{
    if ( _clipboardSPDoc != nullptr ) {
        _clipboardSPDoc = nullptr;
        _defs = nullptr;
        _doc = nullptr;
        _root = nullptr;
        _clipnode = nullptr;
    }
}


/**
 * Get the scale to resize an item, based on the command and desktop state.
 */
Geom::Scale ClipboardManagerImpl::_getScale(SPDesktop *desktop, Geom::Point const &min, Geom::Point const &max, Geom::Rect const &obj_rect, bool apply_x, bool apply_y)
{
    double scale_x = 1.0;
    double scale_y = 1.0;

    if (apply_x) {
        scale_x = (max[Geom::X] - min[Geom::X]) / obj_rect[Geom::X].extent();
    }
    if (apply_y) {
        scale_y = (max[Geom::Y] - min[Geom::Y]) / obj_rect[Geom::Y].extent();
    }
    // If the "lock aspect ratio" button is pressed and we paste only a single coordinate,
    // resize the second one by the same ratio too
    if (desktop && desktop->isToolboxButtonActive("lock")) {
        if (apply_x && !apply_y) {
            scale_y = scale_x;
        }
        if (apply_y && !apply_x) {
            scale_x = scale_y;
        }
    }

    return Geom::Scale(scale_x, scale_y);
}


/**
 * Find the most suitable clipboard target.
 */
Glib::ustring ClipboardManagerImpl::_getBestTarget()
{
    auto targets = _clipboard->wait_for_targets();

    // clipboard target debugging snippet
    /*
    g_message("Begin clipboard targets");
    for ( std::list<Glib::ustring>::iterator x = targets.begin() ; x != targets.end(); ++x )
        g_message("Clipboard target: %s", (*x).data());
    g_message("End clipboard targets\n");
    //*/

    for (auto & _preferred_target : _preferred_targets)
    {
        if ( std::find(targets.begin(), targets.end(), _preferred_target) != targets.end() ) {
            return _preferred_target;
        }
    }
#ifdef _WIN32
    if (OpenClipboard(NULL))
    {   // If both bitmap and metafile are present, pick the one that was exported first.
        UINT format = EnumClipboardFormats(0);
        while (format) {
            if (format == CF_ENHMETAFILE || format == CF_DIB || format == CF_BITMAP) {
                break;
            }
            format = EnumClipboardFormats(format);
        }
        CloseClipboard();

        if (format == CF_ENHMETAFILE) {
            return "CF_ENHMETAFILE";
        }
        if (format == CF_DIB || format == CF_BITMAP) {
            return CLIPBOARD_GDK_PIXBUF_TARGET;
        }
    }

    if (IsClipboardFormatAvailable(CF_ENHMETAFILE)) {
        return "CF_ENHMETAFILE";
    }
#endif
    if (_clipboard->wait_is_image_available()) {
        return CLIPBOARD_GDK_PIXBUF_TARGET;
    }
    if (_clipboard->wait_is_text_available()) {
        return CLIPBOARD_TEXT_TARGET;
    }

    return "";
}


/**
 * Set the clipboard targets to reflect the mimetypes Inkscape can output.
 */
void ClipboardManagerImpl::_setClipboardTargets()
{
    Inkscape::Extension::DB::OutputList outlist;
    Inkscape::Extension::db.get_output_list(outlist);
    std::vector<Gtk::TargetEntry> target_list;

    bool plaintextSet = false;
    for (Inkscape::Extension::DB::OutputList::const_iterator out = outlist.begin() ; out != outlist.end() ; ++out) {
        if ( !(*out)->deactivated() ) {
            Glib::ustring mime = (*out)->get_mimetype();
            if (mime != CLIPBOARD_TEXT_TARGET) {
                if ( !plaintextSet && (mime.find("svg") == Glib::ustring::npos) ) {
                    target_list.emplace_back(CLIPBOARD_TEXT_TARGET);
                    plaintextSet = true;
                }
                target_list.emplace_back(mime);
            }
        }
    }

    // Add PNG export explicitly since there is no extension for this...
    // On Windows, GTK will also present this as a CF_DIB/CF_BITMAP
    target_list.emplace_back( "image/png" );

    _clipboard->set(target_list,
        sigc::mem_fun(*this, &ClipboardManagerImpl::_onGet),
        sigc::mem_fun(*this, &ClipboardManagerImpl::_onClear));

#ifdef _WIN32
    // If the "image/x-emf" target handled by the emf extension would be
    // presented as a CF_ENHMETAFILE automatically (just like an "image/bmp"
    // is presented as a CF_BITMAP) this code would not be needed.. ???
    // Or maybe there is some other way to achieve the same?

    // Note: Metafile is the only format that is rendered and stored in clipboard
    // on Copy, all other formats are rendered only when needed by a Paste command.

    // FIXME: This should at least be rewritten to use "delayed rendering".
    //        If possible make it delayed rendering by using GTK API only.

    if (OpenClipboard(NULL)) {
        if ( _clipboardSPDoc != NULL ) {
            const Glib::ustring target = "image/x-emf";

            Inkscape::Extension::DB::OutputList outlist;
            Inkscape::Extension::db.get_output_list(outlist);
            Inkscape::Extension::DB::OutputList::const_iterator out = outlist.begin();
            for ( ; out != outlist.end() && target != (*out)->get_mimetype() ; ++out) {
            }
            if ( out != outlist.end() ) {
                // FIXME: Temporary hack until we add support for memory output.
                // Save to a temporary file, read it back and then set the clipboard contents
                gchar *filename = g_build_filename( g_get_user_cache_dir(), "inkscape-clipboard-export.emf", NULL );

                try {
                    (*out)->save(_clipboardSPDoc.get(), filename);
                    HENHMETAFILE hemf = GetEnhMetaFileA(filename);
                    if (hemf) {
                        SetClipboardData(CF_ENHMETAFILE, hemf);
                        DeleteEnhMetaFile(hemf);
                    }
                } catch (...) {
                }
                g_unlink(filename); // delete the temporary file
                g_free(filename);
            }
        }
        CloseClipboard();
    }
#endif
}


/**
 * Set the string representation of a 32-bit RGBA color as the clipboard contents.
 */
void ClipboardManagerImpl::_setClipboardColor(guint32 color)
{
    gchar colorstr[16];
    g_snprintf(colorstr, 16, "%08x", color);
    _clipboard->set_text(colorstr);
}


/**
 * Put a notification on the message stack.
 */
void ClipboardManagerImpl::_userWarn(SPDesktop *desktop, char const *msg)
{
    if(desktop)
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, msg);
}

/* #######################################
          ClipboardManager class
   ####################################### */

ClipboardManager *ClipboardManager::_instance = nullptr;

ClipboardManager::ClipboardManager() = default;
ClipboardManager::~ClipboardManager() = default;
ClipboardManager *ClipboardManager::get()
{
    if ( _instance == nullptr ) {
        _instance = new ClipboardManagerImpl;
    }

    return _instance;
}

} // namespace Inkscape
} // namespace IO

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
