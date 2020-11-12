// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This is the code that moves all of the SVG loading and saving into
 * the module format.  Really Inkscape is built to handle these formats
 * internally, so this is just calling those internal functions.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ted Gould <ted@gould.cx>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002-2003 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>

#include <giomm/file.h>
#include <giomm/file.h>

#include "document.h"
#include "inkscape.h"
#include "preferences.h"
#include "extension/output.h"
#include "extension/input.h"
#include "extension/system.h"
#include "file.h"
#include "svg.h"
#include "file.h"
#include "display/cairo-utils.h"
#include "extension/system.h"
#include "extension/output.h"
#include "xml/attribute-record.h"
#include "xml/simple-document.h"

#include "object/sp-image.h"
#include "object/sp-root.h"
#include "object/sp-text.h"

#include "util/units.h"
#include "selection-chemistry.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace Extension {
namespace Internal {

#include "clear-n_.h"

using Inkscape::XML::Node;

/*
 * Removes all sodipodi and inkscape elements and attributes from an xml tree.
 * used to make plain svg output.
 */
static void pruneExtendedNamespaces( Inkscape::XML::Node *repr )
{
    if (repr) {
        if ( repr->type() == Inkscape::XML::NodeType::ELEMENT_NODE ) {
            std::vector<gchar const*> attrsRemoved;
            for ( const auto & it : repr->attributeList()) {
                const gchar* attrName = g_quark_to_string(it.key);
                if ((strncmp("inkscape:", attrName, 9) == 0) || (strncmp("sodipodi:", attrName, 9) == 0)) {
                    attrsRemoved.push_back(attrName);
                }
            }
            // Can't change the set we're iterating over while we are iterating.
            for (auto & it : attrsRemoved) {
                repr->removeAttribute(it);
            }
        }

        std::vector<Inkscape::XML::Node *> nodesRemoved;
        for ( Node *child = repr->firstChild(); child; child = child->next() ) {
            if((strncmp("inkscape:", child->name(), 9) == 0) || strncmp("sodipodi:", child->name(), 9) == 0) {
                nodesRemoved.push_back(child);
            } else {
                pruneExtendedNamespaces(child);
            }
        }
        for (auto & it : nodesRemoved) {
            repr->removeChild(it);
        }
    }
}

/*
 * Similar to the above sodipodi and inkscape prune, but used on all documents
 * to remove problematic elements (for example Adobe's i:pgf tag) only removes
 * known garbage tags.
 */
static void pruneProprietaryGarbage( Inkscape::XML::Node *repr )
{
    if (repr) {
        std::vector<Inkscape::XML::Node *> nodesRemoved;
        for ( Node *child = repr->firstChild(); child; child = child->next() ) {
            if((strncmp("i:pgf", child->name(), 5) == 0)) {
                nodesRemoved.push_back(child);
                g_warning( "An Adobe proprietary tag was found which is known to cause issues. It was removed before saving.");
            } else {
                pruneProprietaryGarbage(child);
            }
        }
        for (auto & it : nodesRemoved) {
            repr->removeChild(it);
        }
    }
}

/**
 *  \return    None
 *
 *  \brief     Create new markers where necessary to simulate the SVG 2 marker attribute 'orient'
 *             value 'auto-start-reverse'.
 *
 *  \param     repr  The current element to check.
 *  \param     defs  A pointer to the <defs> element.
 *  \param     css   The properties of the element to check.
 *  \param     property  Which property to check, either 'marker' or 'marker-start'.
 *
 */
static void remove_marker_auto_start_reverse(Inkscape::XML::Node *repr,
                                             Inkscape::XML::Node *defs,
                                             SPCSSAttr *css,
                                             Glib::ustring const &property)
{
    Glib::ustring value = sp_repr_css_property (css, property.c_str(), "");

    if (!value.empty()) {

        // Find reference <marker>
        static Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("url\\(#([A-z0-9#]*)\\)");
        Glib::MatchInfo matchInfo;
        regex->match(value, matchInfo);

        if (matchInfo.matches()) {

            auto marker_name = matchInfo.fetch(1).raw();
            Inkscape::XML::Node *marker = sp_repr_lookup_child (defs, "id", marker_name.c_str());
            if (marker) {

                // Does marker use "auto-start-reverse"?
                if (strncmp(marker->attribute("orient"), "auto-start-reverse", 17)==0) {

                    // See if a reversed marker already exists.
                    auto marker_name_reversed = marker_name + "_reversed";
                    Inkscape::XML::Node *marker_reversed =
                        sp_repr_lookup_child (defs, "id", marker_name_reversed.c_str());

                    if (!marker_reversed) {

                        // No reversed marker, need to create!
                        marker_reversed = repr->document()->createElement("svg:marker");

                        // Copy attributes
                        for (const auto & iter : marker->attributeList()) {
                            marker_reversed->setAttribute(g_quark_to_string(iter.key), iter.value);
                        }

                        // Override attributes
                        marker_reversed->setAttribute("id", marker_name_reversed);
                        marker_reversed->setAttribute("orient", "auto");

                        // Find transform
                        const char* refX = marker_reversed->attribute("refX");
                        const char* refY = marker_reversed->attribute("refY");
                        std::string transform = "rotate(180";
                        if (refX) {
                            transform += ",";
                            transform += refX;

                            if (refY) {
                                if (refX) {
                                    transform += ",";
                                    transform += refY;
                                } else {
                                    transform += ",0,";
                                    transform += refY;
                                }
                            }
                        }
                        transform += ")";

                        // We can't set a transform on a marker... must create group first.
                        Inkscape::XML::Node *group = repr->document()->createElement("svg:g");
                        group->setAttribute("transform", transform);
                        marker_reversed->addChild(group, nullptr);

                        // Copy all marker content to group.
                        for (auto child = marker->firstChild() ; child != nullptr ; child = child->next() ) {
                            auto new_child = child->duplicate(repr->document());
                            group->addChild(new_child, nullptr);
                            new_child->release();
                        }

                        // Add new marker to <defs>.
                        defs->addChild(marker_reversed, marker);
                        marker_reversed->release();
                     }

                    // Change url to reference reversed marker.
                    std::string marker_url("url(#" + marker_name_reversed + ")");
                    sp_repr_css_set_property(css, "marker-start", marker_url.c_str());

                    // Also fix up if property is marker shorthand.
                    if (property == "marker") {
                        std::string marker_old_url("url(#" + marker_name + ")");
                        sp_repr_css_unset_property(css, "marker");
                        sp_repr_css_set_property(css, "marker-mid", marker_old_url.c_str());
                        sp_repr_css_set_property(css, "marker-end", marker_old_url.c_str());
                    }

                    sp_repr_css_set(repr, css, "style");

                } // Uses auto-start-reverse
            }
        }
    }
}

// Called by remove_marker_context_paint() for each property value ("marker", "marker-start", ...).
static void remove_marker_context_paint (Inkscape::XML::Node *repr,
                                         Inkscape::XML::Node *defs,
                                         Glib::ustring property)
{
    // Value of 'marker', 'marker-start', ... property.
    std::string value("url(#");
    value += repr->attribute("id");
    value += ")";

    // Generate a list of elements that reference this marker.
    std::vector<Inkscape::XML::Node *> to_fix_fill_stroke =
        sp_repr_lookup_property_many(repr->root(), property, value);

    for (auto it: to_fix_fill_stroke) {

        // Figure out value of fill... could be inherited.
        SPCSSAttr* css = sp_repr_css_attr_inherited (it, "style");
        Glib::ustring fill   = sp_repr_css_property (css, "fill",   "");
        Glib::ustring stroke = sp_repr_css_property (css, "stroke", "");

        // Name of new marker.
        Glib::ustring marker_fixed_id = repr->attribute("id");
        if (!fill.empty()) {
            marker_fixed_id += "_F" + fill;
        }
        if (!stroke.empty()) {
            marker_fixed_id += "_S" + stroke;
        }

        // See if a fixed marker already exists.
        // Could be more robust, assumes markers are direct children of <defs>.
        Inkscape::XML::Node* marker_fixed = sp_repr_lookup_child(defs, "id", marker_fixed_id.c_str());

        if (!marker_fixed) {

            // Need to create new marker.

            marker_fixed = repr->duplicate(repr->document());
            marker_fixed->setAttribute("id", marker_fixed_id);

            // This needs to be turned into a function that fixes all descendents.
            for (auto child = marker_fixed->firstChild() ; child != nullptr ; child = child->next()) {
                // Find style.
                SPCSSAttr* css = sp_repr_css_attr ( child, "style" );

                Glib::ustring fill2   = sp_repr_css_property (css, "fill",   "");
                if (fill2 == "context-fill" ) {
                    sp_repr_css_set_property (css, "fill", fill.c_str());
                }
                if (fill2 == "context-stroke" ) {
                    sp_repr_css_set_property (css, "fill", stroke.c_str());
                }

                Glib::ustring stroke2 = sp_repr_css_property (css, "stroke", "");
                if (stroke2 == "context-fill" ) {
                    sp_repr_css_set_property (css, "stroke", fill.c_str());
                }
                if (stroke2 == "context-stroke" ) {
                    sp_repr_css_set_property (css, "stroke", stroke.c_str());
                }

                sp_repr_css_set(child, css, "style");
                sp_repr_css_attr_unref(css);
            }

            defs->addChild(marker_fixed, repr);
            marker_fixed->release();
        }

        Glib::ustring marker_value = "url(#" + marker_fixed_id + ")";
        sp_repr_css_set_property (css, property.c_str(), marker_value.c_str());
        sp_repr_css_set (it, css, "style");
        sp_repr_css_attr_unref(css);
    }
}

static void remove_marker_context_paint (Inkscape::XML::Node *repr,
                                         Inkscape::XML::Node *defs)
{
    if (strncmp("svg:marker", repr->name(), 10) == 0) {

        if (!repr->attribute("id")) {

            std::cerr << "remove_marker_context_paint: <marker> without 'id'!" << std::endl;

        } else {

            // First see if we need to do anything.
            bool need_to_fix = false;

            // This needs to be turned into a function that searches all descendents.
            for (auto child = repr->firstChild() ; child != nullptr ; child = child->next()) {

                // Find style.
                SPCSSAttr* css = sp_repr_css_attr ( child, "style" );
                Glib::ustring fill   = sp_repr_css_property (css, "fill",   "");
                Glib::ustring stroke = sp_repr_css_property (css, "stroke", "");
                if (fill   == "context-fill"   ||
                    fill   == "context-stroke" ||
                    stroke == "context-fill"   ||
                    stroke == "context-stroke" ) {
                    need_to_fix = true;
                    break;
                }
                sp_repr_css_attr_unref(css);
            }

            if (need_to_fix) {

                // Now we need to search document for all elements that use this marker.
                remove_marker_context_paint (repr, defs, "marker");
                remove_marker_context_paint (repr, defs, "marker-start");
                remove_marker_context_paint (repr, defs, "marker-mid");
                remove_marker_context_paint (repr, defs, "marker-end");
            }
        }
    }
}

/*
 * Recursively insert SVG 1.1 fallback for SVG 2 text (ignored by SVG 2 renderers including ours).
 * Notes:
 *   Text must have been layed out. Access via old document.
 */
static void insert_text_fallback( Inkscape::XML::Node *repr, SPDocument *original_doc, Inkscape::XML::Node *defs = nullptr )
{
    if (repr) {

        if (strncmp("svg:text", repr->name(), 8) == 0) {

            auto id = repr->attribute("id");
            // std::cout << "insert_text_fallback: found text!  id: " << (id?id:"null") << std::endl;

            // We need to get original SPText object to access layout.
            SPText* text = static_cast<SPText *>(original_doc->getObjectById( id ));
            if (text == nullptr) {
                std::cerr << "insert_text_fallback: bad cast" << std::endl;
                return;
            }

            if (!text->has_inline_size() &&
                !text->has_shape_inside()) {
                // No SVG 2 text, nothing to do.
                return;
            }

            // We will keep this text node but replace all children.

            // For text in a shape, We need to unset 'text-anchor' or SVG 1.1 fallback won't work.
            // Note 'text' here refers to original document while 'repr' refers to new document copy.
            if (text->has_shape_inside()) {
                SPCSSAttr *css = sp_repr_css_attr(repr, "style" );
                sp_repr_css_unset_property(css, "text-anchor");
                sp_repr_css_set(repr, css, "style");
                sp_repr_css_attr_unref(css);
            }

            // We need to put trailing white space into it's own tspan for inline size so
            // it is excluded during calculation of line position in SVG 1.1 renderers.
            bool trim = text->has_inline_size() &&
                !(text->style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_START);

            // Make a list of children to delete at end:
            std::vector<Inkscape::XML::Node *> old_children;
            for (auto child = repr->firstChild(); child; child = child->next()) {
                old_children.push_back(child);
            }

            // For round-tripping, xml:space (or 'white-space:pre') must be set.
            repr->setAttribute("xml:space", "preserve");

            double text_x = 0.0;
            double text_y = 0.0;
            sp_repr_get_double(repr, "x", &text_x);
            sp_repr_get_double(repr, "y", &text_y);
            // std::cout << "text_x: " << text_x << " text_y: " << text_y << std::endl;

            // Loop over all lines in layout.
            for (auto it = text->layout.begin() ; it != text->layout.end() ; ) {

                // Create a <tspan> with 'x' and 'y' for each line.
                Inkscape::XML::Node *line_tspan = repr->document()->createElement("svg:tspan");

                // This could be useful if one wants to edit in an old version of Inkscape but we
                // need to check if it breaks anything:
                // line_tspan->setAttribute("sodipodi:role", "line");

                // Hide overflow tspan (one line of text).
                if (text->layout.isHidden(it)) {
                    line_tspan->setAttribute("style", "visibility:hidden");
                }

                Geom::Point line_anchor_point = text->layout.characterAnchorPoint(it);
                double line_x = line_anchor_point[Geom::X];
                double line_y = line_anchor_point[Geom::Y];
                if (!text->is_horizontal()) {
                    std::swap(line_x, line_y); // Anchor points rotated & y inverted in vertical layout.
                }

                // std::cout << "  line_anchor_point: " << line_anchor_point << std::endl;
                if (line_tspan->childCount() == 0) {
                    if (text->is_horizontal()) {
                        // std::cout << "  horizontal: " << text_x << " " << line_anchor_point[Geom::Y] << std::endl;
                        if (text->has_inline_size()) {
                            // We use text_x as this is the reference for 'text-anchor'
                            // (line_x is the start of the line which gives wrong position when 'text-anchor' not start).
                            sp_repr_set_svg_double(line_tspan, "x", text_x);
                        } else {
                            // shape-inside (we don't have to worry about 'text-anchor').
                            sp_repr_set_svg_double(line_tspan, "x", line_x);
                        }
                        sp_repr_set_svg_double(line_tspan, "y", line_y); // FIXME: this will pick up the wrong end of counter-directional runs
                    } else {
                        // std::cout << "  vertical:   " << line_anchor_point[Geom::X] << " " << text_y << std::endl;
                        sp_repr_set_svg_double(line_tspan, "x", line_x); // FIXME: this will pick up the wrong end of counter-directional runs
                        if (text->has_inline_size()) {
                            sp_repr_set_svg_double(line_tspan, "y", text_y);
                        } else {
                            sp_repr_set_svg_double(line_tspan, "y", line_y);
                        }
                    }
                }

                // Inside line <tspan>, create <tspan>s for each change of style or shift. (No shifts in SVG 2 flowed text.)
                // For simple lines, this creates an unneeded <tspan> but so be it.
                Inkscape::Text::Layout::iterator it_line_end = it;
                it_line_end.nextStartOfLine();

                // Find last span in line so we can put trailing whitespace in its own tspan for SVG 1.1 fallback.
                Inkscape::Text::Layout::iterator it_last_span = it;
                it_last_span.nextStartOfLine();
                it_last_span.prevStartOfSpan();

                Glib::ustring trailing_whitespace;

                // Loop over chunks in line
                while (it != it_line_end) {

                    Inkscape::XML::Node *span_tspan = repr->document()->createElement("svg:tspan");

                    // use kerning to simulate justification and whatnot
                    Inkscape::Text::Layout::iterator it_span_end = it;
                    it_span_end.nextStartOfSpan();
                    Inkscape::Text::Layout::OptionalTextTagAttrs attrs;
                    text->layout.simulateLayoutUsingKerning(it, it_span_end, &attrs);

                    // 'dx' and 'dy' attributes are used to simulated justified text.
                    if (!text->is_horizontal()) {
                        std::swap(attrs.dx, attrs.dy);
                    }
                    TextTagAttributes(attrs).writeTo(span_tspan);
                    SPObject *source_obj = nullptr;
                    Glib::ustring::iterator span_text_start_iter;
                    text->layout.getSourceOfCharacter(it, &source_obj, &span_text_start_iter);

                    // Set tspan style
                    Glib::ustring style_text = (dynamic_cast<SPString *>(source_obj) ? source_obj->parent : source_obj)
                                                   ->style->writeIfDiff(text->style);
                    if (!style_text.empty()) {
                        span_tspan->setAttributeOrRemoveIfEmpty("style", style_text);
                    }

                    // If this tspan has no attributes, discard it and add content directly to parent element.
                    if (span_tspan->attributeList().empty()) {
                        Inkscape::GC::release(span_tspan);
                        span_tspan = line_tspan;
                    } else {
                        line_tspan->appendChild(span_tspan);
                        Inkscape::GC::release(span_tspan);
                    }

                    // Add text node
                    SPString *str = dynamic_cast<SPString *>(source_obj);
                    if (str) {
                        Glib::ustring *string = &(str->string); // TODO fixme: dangerous, unsafe premature-optimization
                        SPObject *span_end_obj = nullptr;
                        Glib::ustring::iterator span_text_end_iter;
                        text->layout.getSourceOfCharacter(it_span_end, &span_end_obj, &span_text_end_iter);
                        if (span_end_obj != source_obj) {
                            if (it_span_end == text->layout.end()) {
                                span_text_end_iter = span_text_start_iter;
                                for (int i = text->layout.iteratorToCharIndex(it_span_end) - text->layout.iteratorToCharIndex(it) ; i ; --i)
                                    ++span_text_end_iter;
                            } else
                                span_text_end_iter = string->end();    // spans will never straddle a source boundary
                        }

                        if (span_text_start_iter != span_text_end_iter) {
                            Glib::ustring new_string;
                            while (span_text_start_iter != span_text_end_iter)
                                new_string += *span_text_start_iter++;    // grr. no substr() with iterators

                            if (it == it_last_span && trim) {
                                // Found last span in line
                                const auto s = new_string.find_last_not_of(" \t"); // Any other white space characters needed?
                                trailing_whitespace = new_string.substr(s+1, new_string.length());
                                new_string.erase(s+1);
                            }

                            Inkscape::XML::Node *new_text = repr->document()->createTextNode(new_string.c_str());
                            span_tspan->appendChild(new_text);
                            Inkscape::GC::release(new_text);
                            // std::cout << "  new_string: |" << new_string << "|" << std::endl;
                        }
                    }
                    it = it_span_end;
                }

                // Add line tspan to document
                repr->appendChild(line_tspan);
                Inkscape::GC::release(line_tspan);

                // For center and end justified text, we need to remove any spaces and put them
                // into a separate tspan (alignment is done by "text chunk" and spaces at ends of
                // line will mess this up).
                if (trim && trailing_whitespace.length() != 0) {

                    Inkscape::XML::Node *space_tspan = repr->document()->createElement("svg:tspan");
                    // Set either 'x' or 'y' to force a new text chunk. To do: this really should
                    // be positioned at the end of the line (overhanging).
                    if (text->is_horizontal()) {
                        sp_repr_set_svg_double(space_tspan, "y", line_y);
                    } else {
                        sp_repr_set_svg_double(space_tspan, "x", line_x);
                    }
                    Inkscape::XML::Node *space = repr->document()->createTextNode(trailing_whitespace.c_str());
                    space_tspan->appendChild(space);
                    Inkscape::GC::release(space);
                    line_tspan->appendChild(space_tspan);
                    Inkscape::GC::release(space_tspan);
                }

            }

            for (auto i: old_children) {
                repr->removeChild (i);
            }

            return; // No need to look at children of <text>
        }

        for ( Node *child = repr->firstChild(); child; child = child->next() ) {
            insert_text_fallback (child, original_doc, defs);
        }
    }
}


static void insert_mesh_polyfill( Inkscape::XML::Node *repr )
{
    if (repr) {

        Inkscape::XML::Node *defs = sp_repr_lookup_name (repr, "svg:defs");

        if (defs == nullptr) {
            // We always put meshes in <defs>, no defs -> no mesh.
            return;
        }

        bool has_mesh = false;
        for ( Node *child = defs->firstChild(); child; child = child->next() ) {
            if (strncmp("svg:meshgradient", child->name(), 16) == 0) {
                has_mesh = true;
                break;
            }
        }

        Inkscape::XML::Node *script = sp_repr_lookup_child (repr, "id", "mesh_polyfill");

        if (has_mesh && script == nullptr) {

            script = repr->document()->createElement("svg:script");
            script->setAttribute ("id",   "mesh_polyfill");
            script->setAttribute ("type", "text/javascript");
            repr->root()->appendChild(script); // Must be last

            // Insert JavaScript via raw string literal.
            Glib::ustring js =
#include "polyfill/mesh_compressed.include"
;

            Inkscape::XML::Node *script_text = repr->document()->createTextNode(js.c_str());
            script->appendChild(script_text);
        }
    }
}


static void insert_hatch_polyfill( Inkscape::XML::Node *repr )
{
    if (repr) {

        Inkscape::XML::Node *defs = sp_repr_lookup_name (repr, "svg:defs");

        if (defs == nullptr) {
            // We always put meshes in <defs>, no defs -> no mesh.
            return;
        }

        bool has_hatch = false;
        for ( Node *child = defs->firstChild(); child; child = child->next() ) {
            if (strncmp("svg:hatch", child->name(), 16) == 0) {
                has_hatch = true;
                break;
            }
        }

        Inkscape::XML::Node *script = sp_repr_lookup_child (repr, "id", "hatch_polyfill");

        if (has_hatch && script == nullptr) {

            script = repr->document()->createElement("svg:script");
            script->setAttribute ("id",   "hatch_polyfill");
            script->setAttribute ("type", "text/javascript");
            repr->root()->appendChild(script); // Must be last

            // Insert JavaScript via raw string literal.
            Glib::ustring js =
#include "polyfill/hatch_compressed.include"
;

            Inkscape::XML::Node *script_text = repr->document()->createTextNode(js.c_str());
            script->appendChild(script_text);
        }
    }
}

/*
 * Recursively transform SVG 2 to SVG 1.1, if possible.
 */
static void transform_2_to_1( Inkscape::XML::Node *repr, Inkscape::XML::Node *defs = nullptr )
{
    if (repr) {

        // std::cout << "transform_2_to_1: " << repr->name() << std::endl;

        // Things we do once per node. -----------------------

        // Find defs, if does not exist, create.
        if (defs == nullptr) {
            defs = sp_repr_lookup_name (repr, "svg:defs");
        }
        if (defs == nullptr) {
            defs = repr->document()->createElement("svg:defs");
            repr->root()->addChild(defs, nullptr);
        }

        // Find style.
        SPCSSAttr* css = sp_repr_css_attr ( repr, "style" );

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();

        // Individual items ----------------------------------

        // SVG 2 marker attribute orient:auto-start-reverse:
        if ( prefs->getBool("/options/svgexport/marker_autostartreverse", false) ) {
            // Do "marker-start" first for efficiency reasons.
            remove_marker_auto_start_reverse(repr, defs, css, "marker-start");
            remove_marker_auto_start_reverse(repr, defs, css, "marker");
        }

        // SVG 2 paint values 'context-fill', 'context-stroke':
        if ( prefs->getBool("/options/svgexport/marker_contextpaint", false) ) {
            remove_marker_context_paint(repr, defs);
        }

        // *** To Do ***
        // Context fill & stroke outside of markers
        // Paint-Order
        // Meshes
        // Hatches

        for ( Node *child = repr->firstChild(); child; child = child->next() ) {
            transform_2_to_1 (child, defs);
        }

        sp_repr_css_attr_unref(css);
    }
}




/**
    \return   None
    \brief    What would an SVG editor be without loading/saving SVG
              files.  This function sets that up.

    For each module there is a call to Inkscape::Extension::build_from_mem
    with a rather large XML file passed in.  This is a constant string
    that describes the module.  At the end of this call a module is
    returned that is basically filled out.  The one thing that it doesn't
    have is the key function for the operation.  And that is linked at
    the end of each call.
*/
void
Svg::init()
{
    // clang-format off
    /* SVG in */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("SVG Input") "</name>\n"
            "<id>" SP_MODULE_KEY_INPUT_SVG "</id>\n"
            SVG_COMMON_INPUT_PARAMS
            "<input>\n"
                "<extension>.svg</extension>\n"
                "<mimetype>image/svg+xml</mimetype>\n"
                "<filetypename>" N_("Scalable Vector Graphic (*.svg)") "</filetypename>\n"
                "<filetypetooltip>" N_("Inkscape native file format and W3C standard") "</filetypetooltip>\n"
                "<output_extension>" SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE "</output_extension>\n"
            "</input>\n"
        "</inkscape-extension>", new Svg());

    /* SVG out Inkscape */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("SVG Output Inkscape") "</name>\n"
            "<id>" SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE "</id>\n"
            "<output>\n"
                "<extension>.svg</extension>\n"
                "<mimetype>image/x-inkscape-svg</mimetype>\n"
                "<filetypename>" N_("Inkscape SVG (*.svg)") "</filetypename>\n"
                "<filetypetooltip>" N_("SVG format with Inkscape extensions") "</filetypetooltip>\n"
                "<dataloss>false</dataloss>\n"
            "</output>\n"
        "</inkscape-extension>", new Svg());

    /* SVG out */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("SVG Output") "</name>\n"
            "<id>" SP_MODULE_KEY_OUTPUT_SVG "</id>\n"
            "<output>\n"
                "<extension>.svg</extension>\n"
                "<mimetype>image/svg+xml</mimetype>\n"
                "<filetypename>" N_("Plain SVG (*.svg)") "</filetypename>\n"
                "<filetypetooltip>" N_("Scalable Vector Graphics format as defined by the W3C") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>", new Svg());
    // clang-format on

    return;
}


/**
    \return    A new document just for you!
    \brief     This function takes in a filename of a SVG document and
               turns it into a SPDocument.
    \param     mod   Module to use
    \param     uri   The path or URI to the file (UTF-8)

    This function is really simple, it just calls sp_document_new...
    That's BS, it does all kinds of things for importing documents
    that probably should be in a separate function.

    Most of the import code was copied from gdkpixpuf-input.cpp.
*/
SPDocument *
Svg::open (Inkscape::Extension::Input *mod, const gchar *uri)
{
    // This is only used at the end... but it should go here once uri stuff is fixed.
    auto file = Gio::File::create_for_commandline_arg(uri);
    const auto path = file->get_path();

    // Fixing this means fixing a whole string of things.
    // if (path.empty()) {
    //     // We lied, the uri wasn't a uri, try as path.
    //     file = Gio::File::create_for_path(uri);
    // }

    // std::cout << "Svg::open: uri in: " << uri << std::endl;
    // std::cout << "         : uri:    " << file->get_uri() << std::endl;
    // std::cout << "         : scheme: " << file->get_uri_scheme() << std::endl;
    // std::cout << "         : path:   " << file->get_path() << std::endl;
    // std::cout << "         : parse:  " << file->get_parse_name() << std::endl;
    // std::cout << "         : base:   " << file->get_basename() << std::endl;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Get import preferences.
    bool ask_svg                   = prefs->getBool(  "/dialogs/import/ask_svg");
    Glib::ustring import_mode_svg  = prefs->getString("/dialogs/import/import_mode_svg");
    Glib::ustring scale            = prefs->getString("/dialogs/import/scale");

    // If we popped up a window asking about import preferences, get values from
    // there and update preferences.
    if(mod->get_gui() && ask_svg) {
        ask_svg         = !mod->get_param_bool("do_not_ask");
        import_mode_svg =  mod->get_param_optiongroup("import_mode_svg");
        scale           =  mod->get_param_optiongroup("scale");

        prefs->setBool(  "/dialogs/import/ask_svg",         ask_svg);
        prefs->setString("/dialogs/import/import_mode_svg", import_mode_svg );
        prefs->setString("/dialogs/import/scale",           scale );
    }

    // Do we "import" as <image>?
    if (prefs->getBool("/options/onimport", false) && import_mode_svg != "include") {
        // We import!

        // New wrapper document.
        SPDocument * doc = SPDocument::createNewDoc (nullptr, true, true);

        // Imported document
        // SPDocument * ret = SPDocument::createNewDoc(file->get_uri().c_str(), true);
        SPDocument * ret = SPDocument::createNewDoc(uri, true);

        if (!ret) {
            return nullptr;
        }

        // What is display unit doing here?
        Glib::ustring display_unit = doc->getDisplayUnit()->abbr;
        double width = ret->getWidth().value(display_unit);
        double height = ret->getHeight().value(display_unit);
        if (width < 0 || height < 0) {
            return nullptr;
        }

        // Create image node
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *image_node = xml_doc->createElement("svg:image");

        // Set default value as we honor "preserveAspectRatio".
        image_node->setAttribute("preserveAspectRatio", "none");

        double svgdpi = mod->get_param_float("svgdpi");
        image_node->setAttribute("inkscape:svg-dpi", Glib::ustring::format(svgdpi));

        image_node->setAttribute("width", Glib::ustring::format(width));
        image_node->setAttribute("height", Glib::ustring::format(height));

        // This is actually "image-rendering"
        Glib::ustring scale = prefs->getString("/dialogs/import/scale");
        if( scale != "auto") {
            SPCSSAttr *css = sp_repr_css_attr_new();
            sp_repr_css_set_property(css, "image-rendering", scale.c_str());
            sp_repr_css_set(image_node, css, "style");
            sp_repr_css_attr_unref( css );
        }

        // Do we embed or link?
        if (import_mode_svg == "embed") {
            std::unique_ptr<Inkscape::Pixbuf> pb(Inkscape::Pixbuf::create_from_file(uri, svgdpi));
            if(pb) {
                sp_embed_svg(image_node, uri);
            }
        } else {
            // Convert filename to uri (why do we need to do this, we claimed it was already a uri).
            gchar* _uri = g_filename_to_uri(uri, nullptr, nullptr);
            if(_uri) {
                // if (strcmp(_uri, uri) != 0) {
                //     std::cout << "Svg::open: _uri != uri! " << _uri << ":" << uri << std::endl;
                // }
                image_node->setAttribute("xlink:href", _uri);
                g_free(_uri);
            } else {
                image_node->setAttribute("xlink:href", uri);
            }
        }

        // Add the image to a layer.
        Inkscape::XML::Node *layer_node = xml_doc->createElement("svg:g");
        layer_node->setAttribute("inkscape:groupmode", "layer");
        layer_node->setAttribute("inkscape:label", "Image");
        doc->getRoot()->appendChildRepr(layer_node);
        layer_node->appendChild(image_node);
        Inkscape::GC::release(image_node);
        Inkscape::GC::release(layer_node);
        fit_canvas_to_drawing(doc);

        // Set viewBox if it doesn't exist. What is display unit doing here?
        if (!doc->getRoot()->viewBox_set) {
            doc->setViewBox(Geom::Rect::from_xywh(0, 0, doc->getWidth().value(doc->getDisplayUnit()), doc->getHeight().value(doc->getDisplayUnit())));
        }
        return doc;
    }

    // We are not importing as <image>. Open as new document.

    // Try to open non-local file (when does this occur?).
    if (!file->get_uri_scheme().empty()) {
        if (path.empty()) {
            try {
                char *contents;
                gsize length;
                file->load_contents(contents, length);
                return SPDocument::createNewDocFromMem(contents, length, true);
            } catch (Gio::Error &e) {
                g_warning("Could not load contents of non-local URI %s\n", uri);
                return nullptr;
            }
        } else {
            // Do we ever get here and does this actually work?
            uri = path.c_str();
        }
    }

    SPDocument *doc = SPDocument::createNewDoc(uri, true);
    // SPDocument *doc = SPDocument::createNewDoc(file->get_uri().c_str(), true);
    return doc;
}

/**
    \return    None
    \brief     This is the function that does all of the SVG saves in
               Inkscape.  It detects whether it should do a Inkscape
               namespace save internally.
    \param     mod   Extension to use.
    \param     doc   Document to save.
    \param     uri   The filename to save the file to.

    This function first checks its parameters, and makes sure that
    we're getting good data.  It also checks the module ID of the
    incoming module to figure out whether this save should include
    the Inkscape namespace stuff or not.  The result of that comparison
    is stored in the exportExtensions variable.

    If there is not to be Inkscape name spaces a new document is created
    without.  (I think, I'm not sure on this code)

    All of the internally referenced imageins are also set to relative
    paths in the file.  And the file is saved.

    This really needs to be fleshed out more, but I don't quite understand
    all of this code.  I just stole it.
*/
void
Svg::save(Inkscape::Extension::Output *mod, SPDocument *doc, gchar const *filename)
{
    g_return_if_fail(doc != nullptr);
    g_return_if_fail(filename != nullptr);
    Inkscape::XML::Document *rdoc = doc->getReprDoc();

    bool const exportExtensions = ( !mod->get_id()
      || !strcmp (mod->get_id(), SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE)
      || !strcmp (mod->get_id(), SP_MODULE_KEY_OUTPUT_SVGZ_INKSCAPE));

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool const transform_2_to_1_flag =
        prefs->getBool("/dialogs/save_as/enable_svgexport", false);

    bool const insert_text_fallback_flag =
        prefs->getBool("/options/svgexport/text_insertfallback", true);
    bool const insert_mesh_polyfill_flag =
        prefs->getBool("/options/svgexport/mesh_insertpolyfill", true);
    bool const insert_hatch_polyfill_flag =
        prefs->getBool("/options/svgexport/hatch_insertpolyfill", true);

    bool createNewDoc =
        !exportExtensions         ||
        transform_2_to_1_flag     ||
        insert_text_fallback_flag ||
        insert_mesh_polyfill_flag ||
        insert_hatch_polyfill_flag;

    // We prune the in-use document and deliberately loose data, because there
    // is no known use for this data at the present time.
    pruneProprietaryGarbage(rdoc->root());

    if (createNewDoc) {

        // We make a duplicate document so we don't prune the in-use document
        // and loose data. Perhaps the user intends to save as inkscape-svg next.
        Inkscape::XML::Document *new_rdoc = new Inkscape::XML::SimpleDocument();

        // Comments and PI nodes are not included in this duplication
        // TODO: Move this code into xml/document.h and duplicate rdoc instead of root.
        new_rdoc->setAttribute("standalone", "no");
        new_rdoc->setAttribute("version", "2.0");

        // Get a new xml repr for the svg root node
        Inkscape::XML::Node *root = rdoc->root()->duplicate(new_rdoc);

        // Add the duplicated svg node as the document's rdoc
        new_rdoc->appendChild(root);
        Inkscape::GC::release(root);

        if (!exportExtensions) {
            pruneExtendedNamespaces(root);
        }

        if (transform_2_to_1_flag) {
            transform_2_to_1 (root);
            new_rdoc->setAttribute("version", "1.1");
        }

        if (insert_text_fallback_flag) {
            insert_text_fallback (root, doc);
        }

        if (insert_mesh_polyfill_flag) {
            insert_mesh_polyfill (root);
        }

        if (insert_hatch_polyfill_flag) {
            insert_hatch_polyfill (root);
        }

        rdoc = new_rdoc;
    }

    if (!sp_repr_save_rebased_file(rdoc, filename, SP_SVG_NS_URI,
                                   doc->getDocumentBase(), //
                                   m_detachbase ? nullptr : filename)) {
        throw Inkscape::Extension::Output::save_failed();
    }

    if (createNewDoc) {
        Inkscape::GC::release(rdoc);
    }

    return;
}

} } }  /* namespace inkscape, module, implementation */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
