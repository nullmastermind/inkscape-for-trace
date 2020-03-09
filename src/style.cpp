// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG stylesheets implementation.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Peter Moulder <pmoulder@mail.csse.monash.edu.au>
 *   bulia byak <buliabyak@users.sf.net>
 *   Abhishek Sharma
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2005 Monash University
 * Copyright (C) 2012 Kris De Gussem
 * Copyright (C) 2014-2015 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "style.h"

#include <cstring>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <vector>

#include <glibmm/regex.h>

#include "attributes.h"
#include "bad-uri-exception.h"
#include "document.h"
#include "preferences.h"

#include "display/canvas-bpath.h"

#include "3rdparty/libcroco/cr-sel-eng.h"

#include "object/sp-paint-server.h"
#include "object/uri-references.h"
#include "object/uri.h"

#include "svg/css-ostringstream.h"
#include "svg/svg.h"

#include "util/units.h"

#include "xml/croco-node-iface.h"
#include "xml/simple-document.h"

using Inkscape::CSSOStringStream;

#define BMAX 8192

struct SPStyleEnum;

int SPStyle::_count = 0;

/*#########################
## FORWARD DECLARATIONS
#########################*/
void sp_style_filter_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style);
void sp_style_fill_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style);
void sp_style_stroke_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style);

static void sp_style_object_release(SPObject *object, SPStyle *style);
static CRSelEng *sp_repr_sel_eng();

/**
 * Helper class for SPStyle property member lookup by SPAttributeEnum or
 * by name, and for iterating over ordered members.
 */
class SPStylePropHelper {
    SPStylePropHelper() {
#define REGISTER_PROPERTY(id, member, name) \
        g_assert(decltype(SPStyle::member)::static_id() == id); \
        _register(reinterpret_cast<SPIBasePtr>(&SPStyle::member), id) /* name unused */

        // SVG 2: Attributes promoted to properties
        REGISTER_PROPERTY(SP_ATTR_D, d, "d");

        // 'color' must be before 'fill', 'stroke', 'text-decoration-color', ...
        REGISTER_PROPERTY(SP_PROP_COLOR, color, "color");

        // 'font-size'/'font' must be before properties that need to know em, ex size (SPILength,
        // SPILengthOrNormal)
        REGISTER_PROPERTY(SP_PROP_FONT_STYLE, font_style, "font-style");
        REGISTER_PROPERTY(SP_PROP_FONT_VARIANT, font_variant, "font-variant");
        REGISTER_PROPERTY(SP_PROP_FONT_WEIGHT, font_weight, "font-weight");
        REGISTER_PROPERTY(SP_PROP_FONT_STRETCH, font_stretch, "font-stretch");
        REGISTER_PROPERTY(SP_PROP_FONT_SIZE, font_size, "font-size");
        REGISTER_PROPERTY(SP_PROP_LINE_HEIGHT, line_height, "line-height");
        REGISTER_PROPERTY(SP_PROP_FONT_FAMILY, font_family, "font-family");
        REGISTER_PROPERTY(SP_PROP_FONT, font, "font");
        REGISTER_PROPERTY(SP_PROP_INKSCAPE_FONT_SPEC, font_specification, "-inkscape-font-specification");

        // Font variants
        REGISTER_PROPERTY(SP_PROP_FONT_VARIANT_LIGATURES, font_variant_ligatures, "font-variant-ligatures");
        REGISTER_PROPERTY(SP_PROP_FONT_VARIANT_POSITION, font_variant_position, "font-variant-position");
        REGISTER_PROPERTY(SP_PROP_FONT_VARIANT_CAPS, font_variant_caps, "font-variant-caps");
        REGISTER_PROPERTY(SP_PROP_FONT_VARIANT_NUMERIC, font_variant_numeric, "font-variant-numeric");
        REGISTER_PROPERTY(SP_PROP_FONT_VARIANT_ALTERNATES, font_variant_alternates, "font-variant-alternates");
        REGISTER_PROPERTY(SP_PROP_FONT_VARIANT_EAST_ASIAN, font_variant_east_asian, "font-variant-east-asian");
        REGISTER_PROPERTY(SP_PROP_FONT_FEATURE_SETTINGS, font_feature_settings, "font-feature-settings");

        // Variable Fonts
        REGISTER_PROPERTY(SP_PROP_FONT_VARIATION_SETTINGS, font_variation_settings, "font-variation-settings");

        REGISTER_PROPERTY(SP_PROP_TEXT_INDENT, text_indent, "text-indent");
        REGISTER_PROPERTY(SP_PROP_TEXT_ALIGN, text_align, "text-align");

        REGISTER_PROPERTY(SP_PROP_TEXT_DECORATION, text_decoration, "text-decoration");
        REGISTER_PROPERTY(SP_PROP_TEXT_DECORATION_LINE, text_decoration_line, "text-decoration-line");
        REGISTER_PROPERTY(SP_PROP_TEXT_DECORATION_STYLE, text_decoration_style, "text-decoration-style");
        REGISTER_PROPERTY(SP_PROP_TEXT_DECORATION_COLOR, text_decoration_color, "text-decoration-color");
        REGISTER_PROPERTY(SP_PROP_TEXT_DECORATION_FILL, text_decoration_fill, "text-decoration-fill");
        REGISTER_PROPERTY(SP_PROP_TEXT_DECORATION_STROKE, text_decoration_stroke, "text-decoration-stroke");

        REGISTER_PROPERTY(SP_PROP_LETTER_SPACING, letter_spacing, "letter-spacing");
        REGISTER_PROPERTY(SP_PROP_WORD_SPACING, word_spacing, "word-spacing");
        REGISTER_PROPERTY(SP_PROP_TEXT_TRANSFORM, text_transform, "text-transform");

        REGISTER_PROPERTY(SP_PROP_WRITING_MODE, writing_mode, "writing-mode");
        REGISTER_PROPERTY(SP_PROP_DIRECTION, direction, "direction");
        REGISTER_PROPERTY(SP_PROP_TEXT_ORIENTATION, text_orientation, "text-orientation");
        REGISTER_PROPERTY(SP_PROP_DOMINANT_BASELINE, dominant_baseline, "dominant-baseline");
        REGISTER_PROPERTY(SP_PROP_BASELINE_SHIFT, baseline_shift, "baseline-shift");
        REGISTER_PROPERTY(SP_PROP_TEXT_ANCHOR, text_anchor, "text-anchor");
        REGISTER_PROPERTY(SP_PROP_WHITE_SPACE, white_space, "white-space");

        REGISTER_PROPERTY(SP_PROP_SHAPE_INSIDE, shape_inside, "shape-inside");
        REGISTER_PROPERTY(SP_PROP_SHAPE_SUBTRACT, shape_subtract, "shape-subtract");
        REGISTER_PROPERTY(SP_PROP_SHAPE_PADDING, shape_padding, "shape-padding");
        REGISTER_PROPERTY(SP_PROP_SHAPE_MARGIN, shape_margin, "shape-margin");
        REGISTER_PROPERTY(SP_PROP_INLINE_SIZE, inline_size, "inline-size");

        REGISTER_PROPERTY(SP_PROP_CLIP_RULE, clip_rule, "clip-rule");
        REGISTER_PROPERTY(SP_PROP_DISPLAY, display, "display");
        REGISTER_PROPERTY(SP_PROP_OVERFLOW, overflow, "overflow");
        REGISTER_PROPERTY(SP_PROP_VISIBILITY, visibility, "visibility");
        REGISTER_PROPERTY(SP_PROP_OPACITY, opacity, "opacity");

        REGISTER_PROPERTY(SP_PROP_ISOLATION, isolation, "isolation");
        REGISTER_PROPERTY(SP_PROP_MIX_BLEND_MODE, mix_blend_mode, "mix-blend-mode");

        REGISTER_PROPERTY(SP_PROP_COLOR_INTERPOLATION, color_interpolation, "color-interpolation");
        REGISTER_PROPERTY(SP_PROP_COLOR_INTERPOLATION_FILTERS, color_interpolation_filters, "color-interpolation-filters");

        REGISTER_PROPERTY(SP_PROP_SOLID_COLOR, solid_color, "solid-color");
        REGISTER_PROPERTY(SP_PROP_SOLID_OPACITY, solid_opacity, "solid-opacity");

        REGISTER_PROPERTY(SP_PROP_VECTOR_EFFECT, vector_effect, "vector-effect");

        REGISTER_PROPERTY(SP_PROP_FILL, fill, "fill");
        REGISTER_PROPERTY(SP_PROP_FILL_OPACITY, fill_opacity, "fill-opacity");
        REGISTER_PROPERTY(SP_PROP_FILL_RULE, fill_rule, "fill-rule");

        REGISTER_PROPERTY(SP_PROP_STROKE, stroke, "stroke");
        REGISTER_PROPERTY(SP_PROP_STROKE_WIDTH, stroke_width, "stroke-width");
        REGISTER_PROPERTY(SP_PROP_STROKE_LINECAP, stroke_linecap, "stroke-linecap");
        REGISTER_PROPERTY(SP_PROP_STROKE_LINEJOIN, stroke_linejoin, "stroke-linejoin");
        REGISTER_PROPERTY(SP_PROP_STROKE_MITERLIMIT, stroke_miterlimit, "stroke-miterlimit");
        REGISTER_PROPERTY(SP_PROP_STROKE_DASHARRAY, stroke_dasharray, "stroke-dasharray");
        REGISTER_PROPERTY(SP_PROP_STROKE_DASHOFFSET, stroke_dashoffset, "stroke-dashoffset");
        REGISTER_PROPERTY(SP_PROP_STROKE_OPACITY, stroke_opacity, "stroke-opacity");

        REGISTER_PROPERTY(SP_PROP_MARKER, marker, "marker");
        REGISTER_PROPERTY(SP_PROP_MARKER_START, marker_start, "marker-start");
        REGISTER_PROPERTY(SP_PROP_MARKER_MID, marker_mid, "marker-mid");
        REGISTER_PROPERTY(SP_PROP_MARKER_END, marker_end, "marker-end");

        REGISTER_PROPERTY(SP_PROP_PAINT_ORDER, paint_order, "paint-order");

        REGISTER_PROPERTY(SP_PROP_FILTER, filter, "filter");

        REGISTER_PROPERTY(SP_PROP_COLOR_RENDERING, color_rendering, "color-rendering");
        REGISTER_PROPERTY(SP_PROP_IMAGE_RENDERING, image_rendering, "image-rendering");
        REGISTER_PROPERTY(SP_PROP_SHAPE_RENDERING, shape_rendering, "shape-rendering");
        REGISTER_PROPERTY(SP_PROP_TEXT_RENDERING, text_rendering, "text-rendering");

        REGISTER_PROPERTY(SP_PROP_ENABLE_BACKGROUND, enable_background, "enable-background");

        REGISTER_PROPERTY(SP_PROP_STOP_COLOR, stop_color, "stop-color");
        REGISTER_PROPERTY(SP_PROP_STOP_OPACITY, stop_opacity, "stop-opacity");
    }

    // this is a singleton, copy not allowed
    SPStylePropHelper(SPStylePropHelper const&) = delete;
public:

    /**
     * Singleton instance
     */
    static SPStylePropHelper &instance() {
        static SPStylePropHelper _instance;
        return _instance;
    }

    /**
     * Get property pointer by enum
     */
    SPIBase *get(SPStyle *style, SPAttributeEnum id) {
        auto it = m_id_map.find(id);
        if (it != m_id_map.end()) {
            return _get(style, it->second);
        }
        return nullptr;
    }

    /**
     * Get property pointer by name
     */
    SPIBase *get(SPStyle *style, const std::string &name) {
        return get(style, sp_attribute_lookup(name.c_str()));
    }

    /**
     * Get a vector of property pointers
     * \todo provide iterator instead
     */
    std::vector<SPIBase *> get_vector(SPStyle *style) {
        std::vector<SPIBase *> v;
        v.reserve(m_vector.size());
        for (auto ptr : m_vector) {
            v.push_back(_get(style, ptr));
        }
        return v;
    }

private:
    SPIBase *_get(SPStyle *style, SPIBasePtr ptr) { return &(style->*ptr); }

    void _register(SPIBasePtr ptr, SPAttributeEnum id) {
        m_vector.push_back(ptr);

        if (id != SP_ATTR_INVALID) {
            m_id_map[id] = ptr;
        }
    }

    std::unordered_map</* SPAttributeEnum */ int, SPIBasePtr> m_id_map;
    std::vector<SPIBasePtr> m_vector;
};

auto &_prop_helper = SPStylePropHelper::instance();

// C++11 allows one constructor to call another... might be useful. The original C code
// had separate calls to create SPStyle, one with only SPDocument and the other with only
// SPObject as parameters.
SPStyle::SPStyle(SPDocument *document_in, SPObject *object_in) :

    // Unimplemented SVG 1.1: alignment-baseline, clip, clip-path, color-profile, cursor,
    // dominant-baseline, flood-color, flood-opacity, font-size-adjust,
    // glyph-orientation-horizontal, glyph-orientation-vertical, kerning, lighting-color,
    // pointer-events, unicode-bidi

    // 'font', 'font-size', and 'font-family' must come first as other properties depend on them
    // for calculated values (through 'em' and 'ex'). ('ex' is currently not read.)
    // The following properties can depend on 'em' and 'ex':
    //   baseline-shift, kerning, letter-spacing, stroke-dash-offset, stroke-width, word-spacing,
    //   Non-SVG 1.1: text-indent, line-spacing

    // Hidden in SPIFontStyle:  (to be refactored)
    //   font-family
    //   font-specification


    // SVG 2 attributes promoted to properties. (When geometry properties are added, move after font.)
    d(                      false),  // SPIString Not inherited!

    // Font related properties and 'font' shorthand
    font_style(             SP_CSS_FONT_STYLE_NORMAL),
    font_variant(           SP_CSS_FONT_VARIANT_NORMAL),
    font_weight(            SP_CSS_FONT_WEIGHT_NORMAL),
    font_stretch(           SP_CSS_FONT_STRETCH_NORMAL),
    font_size(),
    line_height(            1.25 ),         // SPILengthOrNormal
    font_family(            ),  // SPIString w/default
    font(),                                                      // SPIFont
    font_specification(     ),              // SPIString

    // Font variants (Features)
    font_variant_ligatures( ),
    font_variant_position(  SP_CSS_FONT_VARIANT_POSITION_NORMAL),
    font_variant_caps(      SP_CSS_FONT_VARIANT_CAPS_NORMAL),
    font_variant_numeric(   ),
    font_variant_alternates(SP_CSS_FONT_VARIANT_ALTERNATES_NORMAL),
    font_variant_east_asian(),
    font_feature_settings(  ),

    // Variable Fonts
    font_variation_settings(),  // SPIFontVariationSettings

    // Text related properties
    text_indent(            ),  // SPILength
    text_align(             SP_CSS_TEXT_ALIGN_START),

    letter_spacing(         ),  // SPILengthOrNormal
    word_spacing(           ),  // SPILengthOrNormal
    text_transform(         SP_CSS_TEXT_TRANSFORM_NONE),

    direction(              SP_CSS_DIRECTION_LTR),
    writing_mode(           SP_CSS_WRITING_MODE_LR_TB),
    text_orientation(       SP_CSS_TEXT_ORIENTATION_MIXED),
    dominant_baseline(      SP_CSS_BASELINE_AUTO),
    baseline_shift(),
    text_anchor(            SP_CSS_TEXT_ANCHOR_START),
    white_space(            SP_CSS_WHITE_SPACE_NORMAL),

    // SVG 2 Text Wrapping
    shape_inside(           ), // SPIString
    shape_subtract(         ), // SPIString
    shape_padding(          ), // SPILength for now
    shape_margin(           ), // SPILength for now
    inline_size(            ), // SPILength for now

    text_decoration(),
    text_decoration_line(),
    text_decoration_style(),
    text_decoration_color(  ),  // SPIColor
    text_decoration_fill(   ),  // SPIPaint
    text_decoration_stroke( ),  // SPIPaint

    // General visual properties
    clip_rule(              SP_WIND_RULE_NONZERO),
    display(                SP_CSS_DISPLAY_INLINE,   false),
    overflow(               SP_CSS_OVERFLOW_VISIBLE, false),
    visibility(             SP_CSS_VISIBILITY_VISIBLE),
    opacity(                false),

    isolation(              SP_CSS_ISOLATION_AUTO),
    mix_blend_mode(         SP_CSS_BLEND_NORMAL),

    paint_order(), // SPIPaintOrder

    // Color properties
    color(                  ),  // SPIColor
    color_interpolation(    SP_CSS_COLOR_INTERPOLATION_SRGB),
    color_interpolation_filters(SP_CSS_COLOR_INTERPOLATION_LINEARRGB),

    // Solid color properties
    solid_color(            ), // SPIColor
    solid_opacity(          ),

    // Vector effects
    vector_effect(),

    // Fill properties
    fill(                   ),  // SPIPaint
    fill_opacity(           ),
    fill_rule(              SP_WIND_RULE_NONZERO),

    // Stroke properites
    stroke(                 ),  // SPIPaint
    stroke_width(           1.0),  // SPILength
    stroke_linecap(         SP_STROKE_LINECAP_BUTT),
    stroke_linejoin(        SP_STROKE_LINEJOIN_MITER),
    stroke_miterlimit(      4), // SPIFloat (only use of float!)
    stroke_dasharray(),                                          // SPIDashArray
    stroke_dashoffset(      ),  // SPILength for now

    stroke_opacity(         ),

    marker(                 ),  // SPIString
    marker_start(           ),  // SPIString
    marker_mid(             ),  // SPIString
    marker_end(             ),  // SPIString

    // Filter properties
    filter(),
    enable_background(      SP_CSS_BACKGROUND_ACCUMULATE, false),

    // Rendering hint properties
    color_rendering(        SP_CSS_COLOR_RENDERING_AUTO),
    image_rendering(        SP_CSS_IMAGE_RENDERING_AUTO),
    shape_rendering(        SP_CSS_SHAPE_RENDERING_AUTO),
    text_rendering(         SP_CSS_TEXT_RENDERING_AUTO),

    // Stop color and opacity
    stop_color(             false),       // SPIColor, does not inherit
    stop_opacity(           false)        // Does not inherit
{
    // std::cout << "SPStyle::SPStyle( SPDocument ): Entrance: (" << _count << ")" << std::endl;
    // std::cout << "                      Document: " << (document_in?"present":"null") << std::endl;
    // std::cout << "                        Object: "
    //           << (object_in?(object_in->getId()?object_in->getId():"id null"):"object null") << std::endl;

    // static bool first = true;
    // if( first ) {
    //     std::cout << "Size of SPStyle: " << sizeof(SPStyle) << std::endl;
    //     std::cout << "        SPIBase: " << sizeof(SPIBase) << std::endl;
    //     std::cout << "       SPIFloat: " << sizeof(SPIFloat) << std::endl;
    //     std::cout << "     SPIScale24: " << sizeof(SPIScale24) << std::endl;
    //     std::cout << "      SPILength: " << sizeof(SPILength) << std::endl;
    //     std::cout << "  SPILengthOrNormal: " << sizeof(SPILengthOrNormal) << std::endl;
    //     std::cout << "       SPIColor: " << sizeof(SPIColor) << std::endl;
    //     std::cout << "       SPIPaint: " << sizeof(SPIPaint) << std::endl;
    //     std::cout << "  SPITextDecorationLine" << sizeof(SPITextDecorationLine) << std::endl;
    //     std::cout << "   Glib::ustring:" << sizeof(Glib::ustring) << std::endl;
    //     std::cout << "        SPColor: " << sizeof(SPColor) << std::endl;
    //     first = false;
    // }

    ++_count; // Poor man's memory leak detector

    _refcount = 1;

    cloned = false;

    object = object_in;
    if( object ) {
        g_assert( SP_IS_OBJECT(object) );
        document = object->document;
        release_connection =
            object->connectRelease(sigc::bind<1>(sigc::ptr_fun(&sp_style_object_release), this));

        cloned = object->cloned;

    } else {
        document = document_in;
    }

    // 'font' shorthand requires access to included properties.
    font.setStylePointer(              this );

    // Properties that depend on 'font-size' for calculating lengths.
    baseline_shift.setStylePointer(    this );
    text_indent.setStylePointer(       this );
    line_height.setStylePointer(       this );
    letter_spacing.setStylePointer(    this );
    word_spacing.setStylePointer(      this );
    stroke_width.setStylePointer(      this );
    stroke_dashoffset.setStylePointer( this );
    shape_padding.setStylePointer(     this );
    shape_margin.setStylePointer(      this );
    inline_size.setStylePointer(       this );

    // Properties that depend on 'color'
    text_decoration_color.setStylePointer( this );
    fill.setStylePointer(                  this );
    stroke.setStylePointer(                this );
    // color.setStylePointer( this ); // Doesn't need reference to self

    // 'text_decoration' shorthand requires access to included properties.
    text_decoration.setStylePointer( this );

    // SPIPaint, SPIFilter needs access to 'this' (SPStyle)
    // for setting up signals...  'fill', 'stroke' already done
    filter.setStylePointer( this );
    shape_inside.setStylePointer( this );
    shape_subtract.setStylePointer( this );

    // Used to iterate over markers
    marker_ptrs[SP_MARKER_LOC]       = &marker;
    marker_ptrs[SP_MARKER_LOC_START] = &marker_start;
    marker_ptrs[SP_MARKER_LOC_MID]   = &marker_mid;
    marker_ptrs[SP_MARKER_LOC_END]   = &marker_end;


    // This might be too resource hungary... but for now it possible to loop over properties
    _properties = _prop_helper.get_vector(this);
}

SPStyle::~SPStyle() {

    // std::cout << "SPStyle::~SPStyle" << std::endl;
    --_count; // Poor man's memory leak detector.

    // Remove connections
    release_connection.disconnect();
    fill_ps_changed_connection.disconnect();
    stroke_ps_changed_connection.disconnect();

    // The following should be moved into SPIPaint and SPIFilter
    if (fill.value.href) {
        fill_ps_modified_connection.disconnect();
    }

    if (stroke.value.href) {
        stroke_ps_modified_connection.disconnect();
    }

    if (filter.href) {
        filter_modified_connection.disconnect();
    }

    // Conjecture: all this SPStyle ref counting is not needed. SPObject creates an instance of
    // SPStyle when it is constructed and deletes it when it is destructed. The refcount is
    // incremented and decremented only in the files: display/drawing-item.cpp,
    // display/nr-filter-primitive.cpp, and libnrtype/Layout-TNG-Input.cpp.
    if( _refcount > 1 ) {
        std::cerr << "SPStyle::~SPStyle: ref count greater than 1! " << _refcount << std::endl;
    }
    // std::cout << "SPStyle::~SPStyle(): Exit\n" << std::endl;
}

const std::vector<SPIBase *> SPStyle::properties() { return this->_properties; }

void
SPStyle::clear(SPAttributeEnum id) {
    SPIBase *p = _prop_helper.get(this, id);
    if (p) {
        p->clear();
    } else {
        g_warning("Unimplemented style property %d", id);
    }
}

void
SPStyle::clear() {
    for (auto * p : _properties) {
        p->clear();
    }

    // Release connection to object, created in constructor.
    release_connection.disconnect();

    // href->detach() called in fill->clear()...
    fill_ps_modified_connection.disconnect();
    if (fill.value.href) {
        delete fill.value.href;
        fill.value.href = nullptr;
    }
    stroke_ps_modified_connection.disconnect();
    if (stroke.value.href) {
        delete stroke.value.href;
        stroke.value.href = nullptr;
    }
    filter_modified_connection.disconnect();
    if (filter.href) {
        delete filter.href;
        filter.href = nullptr;
    }

    if (document) {
        filter.href = new SPFilterReference(document);
        filter.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_filter_ref_changed), this));

        fill.value.href = new SPPaintServerReference(document);
        fill_ps_changed_connection = fill.value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_fill_paint_server_ref_changed), this));

        stroke.value.href = new SPPaintServerReference(document);
        stroke_ps_changed_connection = stroke.value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_stroke_paint_server_ref_changed), this));
    }

    cloned = false;

}

// Matches void sp_style_read(SPStyle *style, SPObject *object, Inkscape::XML::Node *repr)
void
SPStyle::read( SPObject *object, Inkscape::XML::Node *repr ) {

    // std::cout << "SPstyle::read( SPObject, Inkscape::XML::Node ): Entrance: "
    //           << (object?(object->getId()?object->getId():"id null"):"object null") << " "
    //           << (repr?(repr->name()?repr->name():"no name"):"repr null")
    //           << std::endl;
    g_assert(repr != nullptr);
    g_assert(!object || (object->getRepr() == repr));

    // // Uncomment to verify that we don't need to call clear.
    // std::cout << " Creating temp style for testing" << std::endl;
    // SPStyle *temp = new SPStyle();
    // if( !(*temp == *this ) ) std::cout << "SPStyle::read: Need to clear" << std::endl;
    // delete temp;

    clear(); // FIXME, If this isn't here, EVERYTHING stops working! Why?

    if (object && object->cloned) {
        cloned = true;
    }

    /* 1. Style attribute */
    // std::cout << " MERGING STYLE ATTRIBUTE" << std::endl;
    gchar const *val = repr->attribute("style");
    if( val != nullptr && *val ) {
        _mergeString( val );
    }

    /* 2 Style sheet */
    if (object) {
        _mergeObjectStylesheet( object );
    }

    /* 3 Presentation attributes */
    for (auto * p : _properties) {
        // Shorthands are not allowed as presentation properites. Note: text-decoration and
        // font-variant are converted to shorthands in CSS 3 but can still be read as a
        // non-shorthand for compatibility with older renders, so they should not be in this list.
        if (p->id() != SP_PROP_FONT && p->id() != SP_PROP_MARKER) {
            p->readAttribute( repr );
        }
    }

    /* 4 Cascade from parent */
    if( object ) {
        if( object->parent ) {
            cascade( object->parent->style );
        }
    } else if( repr->parent() ) { // When does this happen?
        // std::cout << "SPStyle::read(): reading via repr->parent()" << std::endl;
        SPStyle *parent = new SPStyle();
        parent->read( nullptr, repr->parent() );
        cascade( parent );
        delete parent;
    }
}

/**
 * Read style properties from object's repr.
 *
 * 1. Reset existing object style
 * 2. Load current effective object style
 * 3. Load i attributes from immediate parent (which has to be up-to-date)
 */
void
SPStyle::readFromObject( SPObject *object ) {

    // std::cout << "SPStyle::readFromObject: "<< (object->getId()?object->getId():"null")<< std::endl;

    g_return_if_fail(object != nullptr);
    g_return_if_fail(SP_IS_OBJECT(object));

    Inkscape::XML::Node *repr = object->getRepr();
    g_return_if_fail(repr != nullptr);

    read( object, repr );
}

/**
 * Read style properties from preferences.
 * @param path Preferences directory from which the style should be read
 */
void
SPStyle::readFromPrefs(Glib::ustring const &path) {

    g_return_if_fail(!path.empty());

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // not optimal: we reconstruct the node based on the prefs, then pass it to
    // sp_style_read for actual processing.
    Inkscape::XML::SimpleDocument *tempdoc = new Inkscape::XML::SimpleDocument;
    Inkscape::XML::Node *tempnode = tempdoc->createElement("prefs");

    std::vector<Inkscape::Preferences::Entry> attrs = prefs->getAllEntries(path);
    for (auto & attr : attrs) {
        tempnode->setAttribute(attr.getEntryName(), attr.getString());
    }

    read( nullptr, tempnode );

    Inkscape::GC::release(tempnode);
    Inkscape::GC::release(tempdoc);
    delete tempdoc;
}

// Matches sp_style_merge_property(SPStyle *style, gint id, gchar const *val)
void
SPStyle::readIfUnset(SPAttributeEnum id, gchar const *val, SPStyleSrc const &source ) {

    // std::cout << "SPStyle::readIfUnset: Entrance: " << id << ": " << (val?val:"null") << std::endl;
    // To Do: If it is not too slow, use std::map instead of std::vector inorder to remove switch()
    // (looking up SP_PROP_xxxx already uses a hash).
    g_return_if_fail(val != nullptr);

    switch (id) {
            /* SVG */
            /* Clip/Mask */
        case SP_PROP_CLIP_PATH:
            /** \todo
             * This is a workaround. Inkscape only supports 'clip-path' as SVG attribute, not as
             * style property. By having both CSS and SVG attribute set, editing of clip-path
             * will fail, since CSS always overwrites SVG attributes.
             * Fixes Bug #324849
             */
            g_warning("attribute 'clip-path' given as CSS");

            //XML Tree being directly used here.
            if (object) {
                object->setAttribute("clip-path", val);
            }
            return;
        case SP_PROP_MASK:
            /** \todo
             * See comment for SP_PROP_CLIP_PATH
             */
            g_warning("attribute 'mask' given as CSS");
            
            //XML Tree being directly used here.
            if (object) {
                object->setAttribute("mask", val);
            }
            return;
        case SP_PROP_FILTER:
            if( !filter.inherit ) filter.readIfUnset( val, source );
            return;
        case SP_PROP_COLOR_INTERPOLATION:
            // We read it but issue warning
            color_interpolation.readIfUnset( val, source );
            if( color_interpolation.value != SP_CSS_COLOR_INTERPOLATION_SRGB ) {
                g_warning("Inkscape currently only supports color-interpolation = sRGB");
            }
            return;
    }

    auto p = _prop_helper.get(this, id);
    if (p) {
        p->readIfUnset(val, source);
    } else {
        g_warning("Unimplemented style property %d", id);
    }
}

// return if is seted property
bool SPStyle::isSet(SPAttributeEnum id)
{
    bool set = false;
    switch (id) {
        case SP_PROP_CLIP_PATH:
            return set;
        case SP_PROP_MASK:
            return set;
        case SP_PROP_FILTER:
            if (!filter.inherit)
                set = filter.set;
            return set;
        case SP_PROP_COLOR_INTERPOLATION:
            // We read it but issue warning
            return color_interpolation.set;
    }

    auto p = _prop_helper.get(this, id);
    if (p) {
        return p->set;
    } else {
        g_warning("Unimplemented style property %d", id);
        return set;
    }
}

/**
 * Outputs the style to a CSS string.
 *
 * Use with SP_STYLE_FLAG_ALWAYS for copying an object's complete cascaded style to
 * style_clipboard.
 *
 * Use with SP_STYLE_FLAG_IFDIFF and a pointer to the parent class when you need a CSS string for
 * an object in the document tree.
 *
 * \pre flags in {IFSET, ALWAYS, IFDIFF}.
 * \pre base.
 * \post ret != NULL.
 */
Glib::ustring
SPStyle::write( guint const flags, SPStyleSrc const &style_src_req, SPStyle const *const base ) const {

    // std::cout << "SPStyle::write: flags: " << flags << std::endl;

    Glib::ustring style_string;
    for(std::vector<SPIBase*>::size_type i = 0; i != _properties.size(); ++i) {
        if( base != nullptr ) {
            style_string += _properties[i]->write( flags, style_src_req, base->_properties[i] );
        } else {
            style_string += _properties[i]->write( flags, style_src_req, nullptr );
        }
    }

    // Remove trailing ';'
    if( style_string.size() > 0 ) {
        style_string.erase( style_string.size() - 1 );
    }
    return style_string;
}

// Corresponds to sp_style_merge_from_parent()
/**
 * Sets computed values in \a style, which may involve inheriting from (or in some other way
 * calculating from) corresponding computed values of \a parent.
 *
 * References: http://www.w3.org/TR/SVG11/propidx.html shows what properties inherit by default.
 * http://www.w3.org/TR/SVG11/styling.html#Inheritance gives general rules as to what it means to
 * inherit a value.  http://www.w3.org/TR/REC-CSS2/cascade.html#computed-value is more precise
 * about what the computed value is (not obvious for lengths).
 *
 * \pre \a parent's computed values are already up-to-date.
 */
void
SPStyle::cascade( SPStyle const *const parent ) {
    // std::cout << "SPStyle::cascade: " << (object->getId()?object->getId():"null") << std::endl;
    for(std::vector<SPIBase*>::size_type i = 0; i != _properties.size(); ++i) {
        _properties[i]->cascade( parent->_properties[i] );
    }
}

// Corresponds to sp_style_merge_from_dying_parent()
/**
 * Combine \a style and \a parent style specifications into a single style specification that
 * preserves (as much as possible) the effect of the existing \a style being a child of \a parent.
 *
 * Called when the parent repr is to be removed (e.g. the parent is a \<use\> element that is being
 * unlinked), in which case we copy/adapt property values that are explicitly set in \a parent,
 * trying to retain the same visual appearance once the parent is removed.  Interesting cases are
 * when there is unusual interaction with the parent's value (opacity, display) or when the value
 * can be specified as relative to the parent computed value (font-size, font-weight etc.).
 *
 * Doesn't update computed values of \a style.  For correctness, you should subsequently call
 * sp_style_merge_from_parent against the new parent (presumably \a parent's parent) even if \a
 * style was previously up-to-date wrt \a parent.
 *
 * \pre \a parent's computed values are already up-to-date.
 *   (\a style's computed values needn't be up-to-date.)
 */
void
SPStyle::merge( SPStyle const *const parent ) {
    // std::cout << "SPStyle::merge" << std::endl;
    for(std::vector<SPIBase*>::size_type i = 0; i != _properties.size(); ++i) {
        _properties[i]->merge( parent->_properties[i] );
    }
}

/**
 * Parses a style="..." string and merges it with an existing SPStyle.
 */
void
SPStyle::mergeString( gchar const *const p ) {
    _mergeString( p );
}

/**
  * Append an existing css statement into this style, used in css editing
  * always appends declarations as STYLE_SHEET properties.
  */
void
SPStyle::mergeStatement( CRStatement *statement ) {
    if (statement->type != RULESET_STMT) {
        return;
    }
    CRDeclaration *decl_list = nullptr;
    cr_statement_ruleset_get_declarations (statement, &decl_list);
    if (decl_list) {
        _mergeDeclList(decl_list, SP_STYLE_SRC_STYLE_SHEET);
    }
}

// Mostly for unit testing
bool
SPStyle::operator==(const SPStyle& rhs) {

    // Uncomment for testing
    // for(std::vector<SPIBase*>::size_type i = 0; i != _properties.size(); ++i) {
    //     if( *_properties[i] != *rhs._properties[i])
    //     std::cout << _properties[i]->name << ": "
    //               << _properties[i]->write(SP_STYLE_FLAG_ALWAYS,NULL) << " " 
    //               << rhs._properties[i]->write(SP_STYLE_FLAG_ALWAYS,NULL)
    //               << (*_properties[i]  == *rhs._properties[i]) << std::endl;
    // }

    for(std::vector<SPIBase*>::size_type i = 0; i != _properties.size(); ++i) {
        if( *_properties[i] != *rhs._properties[i]) return false;
    }
    return true;
}

void
SPStyle::_mergeString( gchar const *const p ) {

    // std::cout << "SPStyle::_mergeString: " << (p?p:"null") << std::endl;
    CRDeclaration *const decl_list
        = cr_declaration_parse_list_from_buf(reinterpret_cast<guchar const *>(p), CR_UTF_8);
    if (decl_list) {
        _mergeDeclList( decl_list, SP_STYLE_SRC_STYLE_PROP );
        cr_declaration_destroy(decl_list);
    }
}

void
SPStyle::_mergeDeclList( CRDeclaration const *const decl_list, SPStyleSrc const &source ) {

    // std::cout << "SPStyle::_mergeDeclList" << std::endl;

    // In reverse order, as later declarations to take precedence over earlier ones.
    // (Properties are only set if not previously set. See:
    // Ref: http://www.w3.org/TR/REC-CSS2/cascade.html#cascading-order point 4.)
    if (decl_list->next) {
        _mergeDeclList( decl_list->next, source );
    }
    _mergeDecl( decl_list, source );
}

void
SPStyle::_mergeDecl(  CRDeclaration const *const decl, SPStyleSrc const &source ) {

    // std::cout << "SPStyle::_mergeDecl" << std::endl;

    auto prop_idx = sp_attribute_lookup(decl->property->stryng->str);
    if (prop_idx != SP_ATTR_INVALID) {
        /** \todo
         * effic: Test whether the property is already set before trying to
         * convert to string. Alternatively, set from CRTerm directly rather
         * than converting to string.
         */
        if (!isSet(prop_idx) || decl->important) {
            guchar *const str_value_unsigned = cr_term_to_string(decl->value);
            gchar *const str_value = reinterpret_cast<gchar *>(str_value_unsigned);

            // Add "!important" rule if necessary as this is not handled by cr_term_to_string().
            gchar const *important = decl->important ? " !important" : "";
            Inkscape::CSSOStringStream os;
            os << str_value << important;

            readIfUnset(prop_idx, os.str().c_str(), source);
            g_free(str_value);
        }
    }
}

void
SPStyle::_mergeProps( CRPropList *const props ) {

    // std::cout << "SPStyle::_mergeProps" << std::endl;

    // In reverse order, as later declarations to take precedence over earlier ones.
    if (props) {
        _mergeProps( cr_prop_list_get_next( props ) );
        CRDeclaration *decl = nullptr;
        cr_prop_list_get_decl(props, &decl);
        _mergeDecl( decl, SP_STYLE_SRC_STYLE_SHEET );
    }
}

void
SPStyle::_mergeObjectStylesheet( SPObject const *const object ) {

    // std::cout << "SPStyle::_mergeObjectStylesheet: " << (object->getId()?object->getId():"null") << std::endl;

    static CRSelEng *sel_eng = nullptr;
    if (!sel_eng) {
        sel_eng = sp_repr_sel_eng();
    }

    CRPropList *props = nullptr;

    //XML Tree being directly used here while it shouldn't be.
    CRStatus status =
        cr_sel_eng_get_matched_properties_from_cascade(sel_eng,
                                                       object->document->getStyleCascade(),
                                                       object->getRepr(),
                                                       &props);
    g_return_if_fail(status == CR_OK);
    /// \todo Check what errors can occur, and handle them properly.
    if (props) {
        _mergeProps(props);
        cr_prop_list_destroy(props);
    }
}

std::string
SPStyle::getFontFeatureString() {

    std::string feature_string;

    if ( !(font_variant_ligatures.value & SP_CSS_FONT_VARIANT_LIGATURES_COMMON) )
        feature_string += "liga 0, clig 0, ";
    if (   font_variant_ligatures.value & SP_CSS_FONT_VARIANT_LIGATURES_DISCRETIONARY )
        feature_string += "dlig, ";
    if (   font_variant_ligatures.value & SP_CSS_FONT_VARIANT_LIGATURES_HISTORICAL )
        feature_string += "hlig, ";
    if ( !(font_variant_ligatures.value & SP_CSS_FONT_VARIANT_LIGATURES_CONTEXTUAL) )
        feature_string += "calt 0, ";

    switch (font_variant_position.value) {
        case SP_CSS_FONT_VARIANT_POSITION_SUB:
            feature_string += "subs, ";
            break;
        case SP_CSS_FONT_VARIANT_POSITION_SUPER:
            feature_string += "sups, ";
    }

    switch (font_variant_caps.value) {
        case SP_CSS_FONT_VARIANT_CAPS_SMALL:
            feature_string += "smcp, ";
            break;
        case SP_CSS_FONT_VARIANT_CAPS_ALL_SMALL:
            feature_string += "smcp, c2sc, ";
            break;
        case SP_CSS_FONT_VARIANT_CAPS_PETITE:
            feature_string += "pcap, ";
            break;
        case SP_CSS_FONT_VARIANT_CAPS_ALL_PETITE:
            feature_string += "pcap, c2pc, ";
            break;
        case SP_CSS_FONT_VARIANT_CAPS_UNICASE:
            feature_string += "unic, ";
            break;
        case SP_CSS_FONT_VARIANT_CAPS_TITLING:
            feature_string += "titl, ";
    }

    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS )
        feature_string += "lnum, ";
    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS )
        feature_string += "onum, ";
    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS )
        feature_string += "pnum, ";
    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS )
        feature_string += "tnum, ";
    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS )
        feature_string += "frac, ";
    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS )
        feature_string += "afrc, ";
    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_ORDINAL )
        feature_string += "ordn, ";
    if ( font_variant_numeric.value & SP_CSS_FONT_VARIANT_NUMERIC_SLASHED_ZERO )
        feature_string += "zero, ";

    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS78 )
        feature_string += "jp78, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS83 )
        feature_string += "jp83, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS90 )
        feature_string += "jp90, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS04 )
        feature_string += "jp04, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED )
        feature_string += "smpl, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL )
        feature_string += "trad, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH )
        feature_string += "fwid, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL_WIDTH )
        feature_string += "pwid, ";
    if( font_variant_east_asian.value & SP_CSS_FONT_VARIANT_EAST_ASIAN_RUBY )
        feature_string += "ruby, ";

    char const *val = font_feature_settings.value();
    if (val[0] && strcmp(val, "normal")) {
        // We do no sanity checking...
        feature_string += val;
        feature_string += ", ";
    }

    if (feature_string.empty()) {
        feature_string = "normal";
    } else {
        // Remove last ", "
        feature_string.resize(feature_string.size() - 2);
    }

    return feature_string;
}


// Internal
/**
 * Release callback.
 */
static void
sp_style_object_release(SPObject *object, SPStyle *style)
{
    (void)object; // TODO
    style->object = nullptr;
}

// Internal
/**
 * Emit style modified signal on style's object if the filter changed.
 */
static void
sp_style_filter_ref_modified(SPObject *obj, guint flags, SPStyle *style)
{
    (void)flags; // TODO
    SPFilter *filter=static_cast<SPFilter *>(obj);
    if (style->getFilter() == filter)
    {
        if (style->object) {
            style->object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    }
}

// Internal
/**
 * Gets called when the filter is (re)attached to the style
 */
void
sp_style_filter_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style)
{
    if (old_ref) {
        (dynamic_cast<SPFilter *>( old_ref ))->_refcount--;
        style->filter_modified_connection.disconnect();
    }
    if ( SP_IS_FILTER(ref))
    {
       (dynamic_cast<SPFilter *>( ref ))->_refcount++;
        style->filter_modified_connection =
           ref->connectModified(sigc::bind(sigc::ptr_fun(&sp_style_filter_ref_modified), style));
    }

    sp_style_filter_ref_modified(ref, 0, style);
}

/**
 * Emit style modified signal on style's object if server is style's fill
 * or stroke paint server.
 */
static void
sp_style_paint_server_ref_modified(SPObject *obj, guint flags, SPStyle *style)
{
    (void)flags; // TODO
    SPPaintServer *server = static_cast<SPPaintServer *>(obj);

    if ((style->fill.isPaintserver())
        && style->getFillPaintServer() == server)
    {
        if (style->object) {
            /** \todo
             * fixme: I do not know, whether it is optimal - we are
             * forcing reread of everything (Lauris)
             */
            /** \todo
             * fixme: We have to use object_modified flag, because parent
             * flag is only available downstreams.
             */
            style->object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    } else if ((style->stroke.isPaintserver())
        && style->getStrokePaintServer() == server)
    {
        if (style->object) {
            /// \todo fixme:
            style->object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    } else if (server) {
        g_assert_not_reached();
    }
}

/**
 * Gets called when the paintserver is (re)attached to the style
 */
void
sp_style_fill_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style)
{
    if (old_ref) {
        style->fill_ps_modified_connection.disconnect();
    }
    if (SP_IS_PAINT_SERVER(ref)) {
        style->fill_ps_modified_connection =
           ref->connectModified(sigc::bind(sigc::ptr_fun(&sp_style_paint_server_ref_modified), style));
    }

    style->signal_fill_ps_changed.emit(old_ref, ref);
    sp_style_paint_server_ref_modified(ref, 0, style);
}

/**
 * Gets called when the paintserver is (re)attached to the style
 */
void
sp_style_stroke_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style)
{
    if (old_ref) {
        style->stroke_ps_modified_connection.disconnect();
    }
    if (SP_IS_PAINT_SERVER(ref)) {
        style->stroke_ps_modified_connection =
          ref->connectModified(sigc::bind(sigc::ptr_fun(&sp_style_paint_server_ref_modified), style));
    }

    style->signal_stroke_ps_changed.emit(old_ref, ref);
    sp_style_paint_server_ref_modified(ref, 0, style);
}

// Called in display/drawing-item.cpp, display/nr-filter-primitive.cpp, libnrtype/Layout-TNG-Input.cpp
/**
 * Increase refcount of style.
 */
SPStyle *
sp_style_ref(SPStyle *style)
{
    g_return_val_if_fail(style != nullptr, NULL);

    style->style_ref(); // Increase ref count

    return style;
}

// Called in display/drawing-item.cpp, display/nr-filter-primitive.cpp, libnrtype/Layout-TNG-Input.cpp
/**
 * Decrease refcount of style with possible destruction.
 */
SPStyle *
sp_style_unref(SPStyle *style)
{
    g_return_val_if_fail(style != nullptr, NULL);
    if (style->style_unref() < 1) {
        delete style;
        return nullptr;
    }
    return style;
}

static CRSelEng *
sp_repr_sel_eng()
{
    CRSelEng *const ret = cr_sel_eng_new();
    cr_sel_eng_set_node_iface(ret, &Inkscape::XML::croco_node_iface);

    /** \todo
     * Check whether we need to register any pseudo-class handlers.
     * libcroco has its own default handlers for first-child and lang.
     *
     * We probably want handlers for link and arguably visited (though
     * inkscape can't visit links at the time of writing).  hover etc.
     * more useful in inkview than the editor inkscape.
     *
     * http://www.w3.org/TR/SVG11/styling.html#StylingWithCSS says that
     * the following should be honoured, at least by inkview:
     * :hover, :active, :focus, :visited, :link.
     */

    g_assert(ret);
    return ret;
}

// The following functions should be incorporated into SPIPaint. FIXME
// Called in: style.cpp, style-internal.cpp
void
sp_style_set_ipaint_to_uri(SPStyle *style, SPIPaint *paint, const Inkscape::URI *uri, SPDocument *document)
{
    if (!paint->value.href) {

        if (style->object) {
            // Should not happen as href should have been created in SPIPaint. (TODO: Removed code duplication.)
            paint->value.href = new SPPaintServerReference(style->object);

        } else if (document) {
            // Used by desktop style (no object to attach to!).
            paint->value.href = new SPPaintServerReference(document);

        } else {
            std::cerr << "sp_style_set_ipaint_to_uri: No valid object or document!" << std::endl;
            return;
        }

        if (paint == &style->fill) {
            style->fill_ps_changed_connection = paint->value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_fill_paint_server_ref_changed), style));
        } else {
            style->stroke_ps_changed_connection = paint->value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_stroke_paint_server_ref_changed), style));
        }
    }

    if (paint->value.href){
        if (paint->value.href->getObject()){
            paint->value.href->detach();
        }

        try {
            paint->value.href->attach(*uri);
        } catch (Inkscape::BadURIException &e) {
            g_warning("%s", e.what());
            paint->value.href->detach();
        }
    }
}

// Called in: style.cpp, style-internal.cpp
void
sp_style_set_ipaint_to_uri_string (SPStyle *style, SPIPaint *paint, const gchar *uri)
{
    try {
        const Inkscape::URI IURI(uri);
        sp_style_set_ipaint_to_uri(style, paint, &IURI, style->document);
    } catch (...) {
        g_warning("URI failed to parse: %s", uri);
    }
}

// Called in: desktop-style.cpp
void sp_style_set_to_uri(SPStyle *style, bool isfill, Inkscape::URI const *uri)
{
    sp_style_set_ipaint_to_uri(style, style->getFillOrStroke(isfill), uri, style->document);
}

// Called in: widgets/font-selector.cpp, widgets/text-toolbar.cpp, ui/dialog/text-edit.cpp
gchar const *
sp_style_get_css_unit_string(int unit)
{
    // specify px by default, see inkscape bug 1221626, mozilla bug 234789
    // This is a problematic fix as some properties (e.g. 'line-height') have
    // different behaviour if there is no unit.
    switch (unit) {

        case SP_CSS_UNIT_NONE: return "px";
        case SP_CSS_UNIT_PX: return "px";
        case SP_CSS_UNIT_PT: return "pt";
        case SP_CSS_UNIT_PC: return "pc";
        case SP_CSS_UNIT_MM: return "mm";
        case SP_CSS_UNIT_CM: return "cm";
        case SP_CSS_UNIT_IN: return "in";
        case SP_CSS_UNIT_EM: return "em";
        case SP_CSS_UNIT_EX: return "ex";
        case SP_CSS_UNIT_PERCENT: return "%";
        default: return "px";
    }
    return "px";
}

// Called in: style-internal.cpp, widgets/text-toolbar.cpp, ui/dialog/text-edit.cpp
/*
 * Convert a size in pixels into another CSS unit size
 */
double
sp_style_css_size_px_to_units(double size, int unit, double font_size)
{
    double unit_size = size;

    if (font_size == 0) {
        g_warning("sp_style_get_css_font_size_units: passed in zero font_size");
        font_size = SP_CSS_FONT_SIZE_DEFAULT;
    }

    switch (unit) {

        case SP_CSS_UNIT_NONE: unit_size = size; break;
        case SP_CSS_UNIT_PX: unit_size = size; break;
        case SP_CSS_UNIT_PT: unit_size = Inkscape::Util::Quantity::convert(size, "px", "pt");  break;
        case SP_CSS_UNIT_PC: unit_size = Inkscape::Util::Quantity::convert(size, "px", "pc");  break;
        case SP_CSS_UNIT_MM: unit_size = Inkscape::Util::Quantity::convert(size, "px", "mm");  break;
        case SP_CSS_UNIT_CM: unit_size = Inkscape::Util::Quantity::convert(size, "px", "cm");  break;
        case SP_CSS_UNIT_IN: unit_size = Inkscape::Util::Quantity::convert(size, "px", "in");  break;
        case SP_CSS_UNIT_EM: unit_size = size / font_size; break;
        case SP_CSS_UNIT_EX: unit_size = size * 2.0 / font_size ; break;
        case SP_CSS_UNIT_PERCENT: unit_size = size * 100.0 / font_size; break;

        default:
            g_warning("sp_style_get_css_font_size_units conversion to %d not implemented.", unit);
            break;
    }

    return unit_size;
}

// Called in: widgets/text-toolbar.cpp, ui/dialog/text-edit.cpp
/*
 * Convert a size in a CSS unit size to pixels
 */
double
sp_style_css_size_units_to_px(double size, int unit, double font_size)
{
    if (unit == SP_CSS_UNIT_PX) {
        return size;
    }
    //g_message("sp_style_css_size_units_to_px %f %d = %f px", size, unit, out);
    return size * (size / sp_style_css_size_px_to_units(size, unit, font_size));;
}


// FIXME: Everything below this line belongs in a different file - css-chemistry?

void
sp_style_set_property_url (SPObject *item, gchar const *property, SPObject *linked, bool recursive)
{
    Inkscape::XML::Node *repr = item->getRepr();

    if (repr == nullptr) return;

    SPCSSAttr *css = sp_repr_css_attr_new();
    if (linked) {
        gchar *val = g_strdup_printf("url(#%s)", linked->getId());
        sp_repr_css_set_property(css, property, val);
        g_free(val);
    } else {
        sp_repr_css_unset_property(css, "filter");
    }

    if (recursive) {
        sp_repr_css_change_recursive(repr, css, "style");
    } else {
        sp_repr_css_change(repr, css, "style");
    }
    sp_repr_css_attr_unref(css);
}

/**
 * \pre style != NULL.
 * \pre flags in {IFSET, ALWAYS}.
 * Only used by sp_css_attr_from_object() and in splivarot.cpp - sp_item_path_outline().
 */
SPCSSAttr *
sp_css_attr_from_style(SPStyle const *const style, guint const flags)
{
    g_return_val_if_fail(style != nullptr, NULL);
    g_return_val_if_fail(((flags & SP_STYLE_FLAG_IFSET) ||
                          (flags & SP_STYLE_FLAG_ALWAYS)),
                         NULL);
    Glib::ustring style_str = style->write(flags);
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css, style_str.c_str());
    return css;
}

// Called in: selection-chemistry.cpp, widgets/stroke-marker-selector.cpp, widgets/stroke-style.cpp,
// ui/tools/freehand-base.cpp
/**
 * \pre object != NULL
 * \pre flags in {IFSET, ALWAYS}.
 */
SPCSSAttr *sp_css_attr_from_object(SPObject *object, guint const flags)
{
    g_return_val_if_fail(((flags == SP_STYLE_FLAG_IFSET) ||
                          (flags == SP_STYLE_FLAG_ALWAYS)  ),
                         NULL);
    SPCSSAttr * result = nullptr;    
    if (object->style) {
        result = sp_css_attr_from_style(object->style, flags);
    }
    return result;
}

// Called in: selection-chemistry.cpp, ui/dialog/inkscape-preferences.cpp
/**
 * Unset any text-related properties
 */
SPCSSAttr *
sp_css_attr_unset_text(SPCSSAttr *css)
{
    sp_repr_css_set_property(css, "font", nullptr);
    sp_repr_css_set_property(css, "-inkscape-font-specification", nullptr);
    sp_repr_css_set_property(css, "font-size", nullptr);
    sp_repr_css_set_property(css, "font-size-adjust", nullptr); // not implemented yet
    sp_repr_css_set_property(css, "font-style", nullptr);
    sp_repr_css_set_property(css, "font-variant", nullptr);
    sp_repr_css_set_property(css, "font-weight", nullptr);
    sp_repr_css_set_property(css, "font-stretch", nullptr);
    sp_repr_css_set_property(css, "font-family", nullptr);
    sp_repr_css_set_property(css, "text-indent", nullptr);
    sp_repr_css_set_property(css, "text-align", nullptr);
    sp_repr_css_set_property(css, "line-height", nullptr);
    sp_repr_css_set_property(css, "letter-spacing", nullptr);
    sp_repr_css_set_property(css, "word-spacing", nullptr);
    sp_repr_css_set_property(css, "text-transform", nullptr);
    sp_repr_css_set_property(css, "direction", nullptr);
    sp_repr_css_set_property(css, "writing-mode", nullptr);
    sp_repr_css_set_property(css, "text-orientation", nullptr);
    sp_repr_css_set_property(css, "text-anchor", nullptr);
    sp_repr_css_set_property(css, "white-space", nullptr);
    sp_repr_css_set_property(css, "shape-inside", nullptr);
    sp_repr_css_set_property(css, "shape-subtract", nullptr);
    sp_repr_css_set_property(css, "shape-padding", nullptr);
    sp_repr_css_set_property(css, "shape-margin", nullptr);
    sp_repr_css_set_property(css, "inline-size", nullptr);
    sp_repr_css_set_property(css, "kerning", nullptr); // not implemented yet
    sp_repr_css_set_property(css, "dominant-baseline", nullptr); // not implemented yet
    sp_repr_css_set_property(css, "alignment-baseline", nullptr); // not implemented yet
    sp_repr_css_set_property(css, "baseline-shift", nullptr);

    sp_repr_css_set_property(css, "text-decoration", nullptr);
    sp_repr_css_set_property(css, "text-decoration-line", nullptr);
    sp_repr_css_set_property(css, "text-decoration-color", nullptr);
    sp_repr_css_set_property(css, "text-decoration-style", nullptr);

    sp_repr_css_set_property(css, "font-variant-ligatures", nullptr);
    sp_repr_css_set_property(css, "font-variant-position", nullptr);
    sp_repr_css_set_property(css, "font-variant-caps", nullptr);
    sp_repr_css_set_property(css, "font-variant-numeric", nullptr);
    sp_repr_css_set_property(css, "font-variant-alternates", nullptr);
    sp_repr_css_set_property(css, "font-variant-east-asian", nullptr);
    sp_repr_css_set_property(css, "font-feature-settings", nullptr);

    return css;
}

// ui/dialog/inkscape-preferences.cpp
/**
 * Unset properties that should not be set for default tool style.
 * This list needs to be reviewed.
 */
SPCSSAttr *
sp_css_attr_unset_blacklist(SPCSSAttr *css)
{
    sp_repr_css_set_property(css, "color",               nullptr);
    sp_repr_css_set_property(css, "clip-rule",           nullptr);
    sp_repr_css_set_property(css, "d",                   nullptr);
    sp_repr_css_set_property(css, "display",             nullptr);
    sp_repr_css_set_property(css, "overflow",            nullptr);
    sp_repr_css_set_property(css, "visibility",          nullptr);
    sp_repr_css_set_property(css, "isolation",           nullptr);
    sp_repr_css_set_property(css, "mix-blend-mode",      nullptr);
    sp_repr_css_set_property(css, "color-interpolation", nullptr);
    sp_repr_css_set_property(css, "color-interpolation-filters", nullptr);
    sp_repr_css_set_property(css, "solid-color",         nullptr);
    sp_repr_css_set_property(css, "solid-opacity",       nullptr);
    sp_repr_css_set_property(css, "fill-rule",           nullptr);
    sp_repr_css_set_property(css, "color-rendering",     nullptr);
    sp_repr_css_set_property(css, "image-rendering",     nullptr);
    sp_repr_css_set_property(css, "shape-rendering",     nullptr);
    sp_repr_css_set_property(css, "text-rendering",      nullptr);
    sp_repr_css_set_property(css, "enable-background",   nullptr);

    return css;
}

// Called in style.cpp
static bool
is_url(char const *p)
{
    if (p == nullptr)
        return false;
/** \todo
 * FIXME: I'm not sure if this applies to SVG as well, but CSS2 says any URIs
 * in property values must start with 'url('.
 */
    return (g_ascii_strncasecmp(p, "url(", 4) == 0);
}

// Called in: ui/dialog/inkscape-preferences.cpp, ui/tools/tweek-tool.cpp
/**
 * Unset any properties that contain URI values.
 *
 * Used for storing style that will be reused across documents when carrying
 * the referenced defs is impractical.
 */
SPCSSAttr *
sp_css_attr_unset_uris(SPCSSAttr *css)
{
// All properties that may hold <uri> or <paint> according to SVG 1.1
    if (is_url(sp_repr_css_property(css, "clip-path", nullptr))) sp_repr_css_set_property(css, "clip-path", nullptr);
    if (is_url(sp_repr_css_property(css, "color-profile", nullptr))) sp_repr_css_set_property(css, "color-profile", nullptr);
    if (is_url(sp_repr_css_property(css, "cursor", nullptr))) sp_repr_css_set_property(css, "cursor", nullptr);
    if (is_url(sp_repr_css_property(css, "filter", nullptr))) sp_repr_css_set_property(css, "filter", nullptr);
    if (is_url(sp_repr_css_property(css, "marker", nullptr))) sp_repr_css_set_property(css, "marker", nullptr);
    if (is_url(sp_repr_css_property(css, "marker-start", nullptr))) sp_repr_css_set_property(css, "marker-start", nullptr);
    if (is_url(sp_repr_css_property(css, "marker-mid", nullptr))) sp_repr_css_set_property(css, "marker-mid", nullptr);
    if (is_url(sp_repr_css_property(css, "marker-end", nullptr))) sp_repr_css_set_property(css, "marker-end", nullptr);
    if (is_url(sp_repr_css_property(css, "mask", nullptr))) sp_repr_css_set_property(css, "mask", nullptr);
    if (is_url(sp_repr_css_property(css, "fill", nullptr))) sp_repr_css_set_property(css, "fill", nullptr);
    if (is_url(sp_repr_css_property(css, "stroke", nullptr))) sp_repr_css_set_property(css, "stroke", nullptr);

    return css;
}

// Called in style.cpp
/**
 * Scale a single-value property.
 */
static void
sp_css_attr_scale_property_single(SPCSSAttr *css, gchar const *property,
                                  double ex, bool only_with_units = false)
{
    gchar const *w = sp_repr_css_property(css, property, nullptr);
    if (w) {
        gchar *units = nullptr;
        double wd = g_ascii_strtod(w, &units) * ex;
        if (w == units) {// nothing converted, non-numeric value
            return;
        }
        if (only_with_units && (units == nullptr || *units == '\0' || *units == '%' || *units == 'e')) {
            // only_with_units, but no units found, so do nothing.
            // 'e' matches 'em' or 'ex'
            return;
        }
        Inkscape::CSSOStringStream os;
        os << wd << units; // reattach units
        sp_repr_css_set_property(css, property, os.str().c_str());
    }
}

// Called in style.cpp for stroke-dasharray
/**
 * Scale a list-of-values property.
 */
static void
sp_css_attr_scale_property_list(SPCSSAttr *css, gchar const *property, double ex)
{
    gchar const *string = sp_repr_css_property(css, property, nullptr);
    if (string) {
        Inkscape::CSSOStringStream os;
        gchar **a = g_strsplit(string, ",", 10000);
        bool first = true;
        for (gchar **i = a; i != nullptr; i++) {
            gchar *w = *i;
            if (w == nullptr)
                break;
            gchar *units = nullptr;
            double wd = g_ascii_strtod(w, &units) * ex;
            if (w == units) {// nothing converted, non-numeric value ("none" or "inherit"); do nothing
                g_strfreev(a);
                return;
            }
            if (!first) {
                os << ",";
            }
            os << wd << units; // reattach units
            first = false;
        }
        sp_repr_css_set_property(css, property, os.str().c_str());
        g_strfreev(a);
    }
}

// Called in: text-editing.cpp, 
/**
 * Scale any properties that may hold <length> by ex.
 */
SPCSSAttr *
sp_css_attr_scale(SPCSSAttr *css, double ex)
{
    sp_css_attr_scale_property_single(css, "baseline-shift", ex);
    sp_css_attr_scale_property_single(css, "stroke-width", ex);
    sp_css_attr_scale_property_list  (css, "stroke-dasharray", ex);
    sp_css_attr_scale_property_single(css, "stroke-dashoffset", ex);
    sp_css_attr_scale_property_single(css, "font-size", ex, true);
    sp_css_attr_scale_property_single(css, "kerning", ex);
    sp_css_attr_scale_property_single(css, "letter-spacing", ex);
    sp_css_attr_scale_property_single(css, "word-spacing", ex);
    sp_css_attr_scale_property_single(css, "line-height", ex, true);

    return css;
}


/**
 * Quote and/or escape string for writing to CSS, changing strings in place.
 * See: http://www.w3.org/TR/CSS21/syndata.html#value-def-identifier
 */
void
css_quote(Glib::ustring &val)
{
    Glib::ustring out;
    bool quote = false;

    // Can't wait for C++11!
    for( Glib::ustring::iterator it = val.begin(); it != val.end(); ++it) {
        if(g_ascii_isalnum(*it) || *it=='-' || *it=='_' || *it > 0xA0) {
            out += *it;
        } else if (*it == '\'') {
            // Single quotes require escaping and quotes.
            out += '\\';
            out += *it;
            quote = true;
        } else {
            // Quote everything else including spaces.
            // (CSS Fonts Level 3 recommends quoting with spaces.)
            out += *it;
            quote = true;
        }
        if( it == val.begin() && !g_ascii_isalpha(*it) ) {
            // A non-ASCII/non-alpha initial value on any identifier needs quotes.
            // (Actually it's a bit more complicated but as it never hurts to quote...)
            quote = true;
        }
    }
    if( quote ) {
        out.insert( out.begin(), '\'' );
        out += '\'';
    }
    val = out;
}


/**
 * Quote font names in font-family lists, changing string in place.
 * We use unquoted names internally but some need to be quoted in CSS.
 */
void
css_font_family_quote(Glib::ustring &val)
{
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s*,\\s*", val );

    val.erase();
    for(auto & token : tokens) {
        css_quote( token );
        val += token + ", ";
    }
    if( val.size() > 1 )
        val.erase( val.size() - 2 ); // Remove trailing ", "
}


// Called in style-internal.cpp, xml/repr-css.cpp
/**
 * Remove paired single and double quotes from a string, changing string in place.
 */
void
css_unquote(Glib::ustring &val)
{
  if( val.size() > 1 &&
      ( (val[0] == '"'  && val[val.size()-1] == '"'  ) ||
	(val[0] == '\'' && val[val.size()-1] == '\'' ) ) ) {

    val.erase( 0, 1 );
    val.erase( val.size()-1 );
  }
}

// Called in style-internal.cpp, text-toolbar.cpp
/**
 * Remove paired single and double quotes from font names in font-family lists,
 * changing string in place.
 * We use unquoted family names internally but CSS sometimes uses quoted names.
 */
void
css_font_family_unquote(Glib::ustring &val)
{
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s*,\\s*", val );

    val.erase();
    for(auto & token : tokens) {
        css_unquote( token );
        val += token + ", ";
    }
    if( val.size() > 1 )
        val.erase( val.size() - 2 ); // Remove trailing ", "
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
