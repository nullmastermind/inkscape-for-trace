// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_STYLE_H
#define SEEN_SP_STYLE_H

/** \file
 * SPStyle - a style object for SPItem objects
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2014 Tavmjong Bah
 * Copyright (C) 2010 Jon A. Cruz
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "style-enums.h"
#include "style-internal.h"

#include <sigc++/connection.h>
#include <iostream>
#include <map>
#include <vector>

#include "3rdparty/libcroco/cr-declaration.h"
#include "3rdparty/libcroco/cr-prop-list.h"

enum class SPAttr;

// Define SPIBasePtr, a Pointer to a data member of SPStyle of type SPIBase;
typedef SPIBase SPStyle::*SPIBasePtr;

namespace Inkscape {
namespace XML {
class Node;
}
}


/// An SVG style object.
class SPStyle {

public:

    SPStyle(SPDocument *document = nullptr, SPObject *object = nullptr);// document is ignored if valid object given
    ~SPStyle();
    const std::vector<SPIBase *> properties();
    void clear();
    void clear(SPAttr id);
    void read(SPObject *object, Inkscape::XML::Node *repr);
    void readFromObject(SPObject *object);
    void readFromPrefs(Glib::ustring const &path);
    bool isSet(SPAttr id);
    void readIfUnset(SPAttr id, char const *val, SPStyleSrc const &source = SPStyleSrc::STYLE_PROP );

private:
    Glib::ustring write(unsigned int flags, SPStyleSrc style_src_req, SPStyle const *base = nullptr) const;

public:
    Glib::ustring write(unsigned int flags = SP_STYLE_FLAG_IFSET) const;
    Glib::ustring write(SPStyleSrc style_src_req) const;
    Glib::ustring writeIfDiff(SPStyle const *base) const;

    void cascade( SPStyle const *const parent );
    void merge(   SPStyle const *const parent );
    void mergeString( char const *const p );
    void mergeStatement( CRStatement *statement );
    bool operator==(const SPStyle& rhs);

    int style_ref()   { ++_refcount; return _refcount; }
    int style_unref() { --_refcount; return _refcount; }
    int refCount() { return _refcount; }

private:
    void _mergeString( char const *const p );
    void _mergeDeclList( CRDeclaration const *const decl_list, SPStyleSrc const &source );
    void _mergeDecl(     CRDeclaration const *const decl,      SPStyleSrc const &source );
    void _mergeProps( CRPropList *const props );
    void _mergeObjectStylesheet( SPObject const *const object );

private:
    int _refcount;
    static int _count; // Poor man's leak detector

// FIXME: Make private
public:
    /** Object we are attached to */
    SPObject *object;
    /** Document we are associated with */
    SPDocument *document;

private:
    /// Pointers to all the properties (for looping through them)
    std::vector<SPIBase *> _properties;

    // Shorthand for better readability
    template <SPAttr Id, class Base>
    using T = TypedSPI<Id, Base>;

public:

    /* ----------------------- THE PROPERTIES ------------------------- */
    /*                    Match order in style.cpp.                     */

    /* SVG 2 attributes promoted to properties. */

    /** Path data */
    T<SPAttr::D, SPIString> d;

    /* Font ---------------------------- */

    /** Font style */
    T<SPAttr::FONT_STYLE, SPIEnum<SPCSSFontStyle>> font_style;
    /** Which substyle of the font (CSS 2. CSS 3 redefines as shorthand) */
    T<SPAttr::FONT_VARIANT, SPIEnum<SPCSSFontVariant>> font_variant;
    /** Weight of the font */
    T<SPAttr::FONT_WEIGHT, SPIEnum<SPCSSFontWeight>> font_weight;
    /** Stretch of the font */
    T<SPAttr::FONT_STRETCH, SPIEnum<SPCSSFontStretch>> font_stretch;
    /** Size of the font */
    T<SPAttr::FONT_SIZE, SPIFontSize> font_size;
    /** Line height (css2 10.8.1) */
    T<SPAttr::LINE_HEIGHT, SPILengthOrNormal> line_height;
    /** Font family */
    T<SPAttr::FONT_FAMILY, SPIString> font_family;
    /** Font shorthand */
    T<SPAttr::FONT, SPIFont> font;
    /** Full font name, as font_factory::ConstructFontSpecification would give, for internal use. */
    T<SPAttr::INKSCAPE_FONT_SPEC, SPIString> font_specification;

    /* Font variants -------------------- */
    /** Font variant ligatures */
    T<SPAttr::FONT_VARIANT_LIGATURES, SPILigatures> font_variant_ligatures;
    /** Font variant position (subscript/superscript) */
    T<SPAttr::FONT_VARIANT_POSITION, SPIEnum<SPCSSFontVariantPosition>> font_variant_position;
    /** Font variant caps (small caps) */
    T<SPAttr::FONT_VARIANT_CAPS, SPIEnum<SPCSSFontVariantCaps>> font_variant_caps;
    /** Font variant numeric (numerical formatting) */
    T<SPAttr::FONT_VARIANT_NUMERIC, SPINumeric> font_variant_numeric;
    /** Font variant alternates (alternates/swatches) */
    T<SPAttr::FONT_VARIANT_ALTERNATES, SPIEnum<SPCSSFontVariantAlternates>> font_variant_alternates;
    /** Font variant East Asian */
    T<SPAttr::FONT_VARIANT_EAST_ASIAN, SPIEastAsian> font_variant_east_asian;
    /** Font feature settings (Low level access to TrueType tables) */
    T<SPAttr::FONT_FEATURE_SETTINGS, SPIString> font_feature_settings;

    /** Font variation settings (Low level access to OpenType variable font design-coordinate values) */
    T<SPAttr::FONT_VARIATION_SETTINGS, SPIFontVariationSettings> font_variation_settings;

    /* Text ----------------------------- */

    /** First line indent of paragraphs (css2 16.1) */
    T<SPAttr::TEXT_INDENT, SPILength> text_indent;
    /** text alignment (css2 16.2) (not to be confused with text-anchor) */
    T<SPAttr::TEXT_ALIGN, SPIEnum<SPCSSTextAlign>> text_align;

    /** letter spacing (css2 16.4) */
    T<SPAttr::LETTER_SPACING, SPILengthOrNormal> letter_spacing;
    /** word spacing (also css2 16.4) */
    T<SPAttr::WORD_SPACING, SPILengthOrNormal> word_spacing;
    /** capitalization (css2 16.5) */
    T<SPAttr::TEXT_TRANSFORM, SPIEnum<SPCSSTextTransform>> text_transform;

    /* CSS3 Text */
    /** text direction (svg1.1) */
    T<SPAttr::DIRECTION, SPIEnum<SPCSSDirection>> direction;
    /** Writing mode (svg1.1 10.7.2, CSS Writing Modes 3) */
    T<SPAttr::WRITING_MODE, SPIEnum<SPCSSWritingMode>> writing_mode;
    /** Text orientation (CSS Writing Modes 3) */
    T<SPAttr::TEXT_ORIENTATION, SPIEnum<SPCSSTextOrientation>> text_orientation;
    /** Dominant baseline (svg1.1) */
    T<SPAttr::DOMINANT_BASELINE, SPIEnum<SPCSSBaseline>> dominant_baseline;
    /** Baseline shift (svg1.1 10.9.2) */
    T<SPAttr::BASELINE_SHIFT, SPIBaselineShift> baseline_shift;

    /* SVG */
    /** Anchor of the text (svg1.1 10.9.1) */
    T<SPAttr::TEXT_ANCHOR, SPIEnum<SPTextAnchor>> text_anchor;

    /** white space (svg2) */
    T<SPAttr::WHITE_SPACE, SPIEnum<SPWhiteSpace>> white_space;

    /** SVG2 Text Wrapping */
    T<SPAttr::SHAPE_INSIDE, SPIShapes> shape_inside;
    T<SPAttr::SHAPE_SUBTRACT, SPIShapes> shape_subtract;
    T<SPAttr::SHAPE_PADDING, SPILength> shape_padding;
    T<SPAttr::SHAPE_MARGIN, SPILength> shape_margin;
    T<SPAttr::INLINE_SIZE, SPILength> inline_size;

    /* Text Decoration ----------------------- */

    /** text decoration (css2 16.3.1) */
    T<SPAttr::TEXT_DECORATION, SPITextDecoration> text_decoration;
    /** CSS 3 2.1, 2.2, 2.3 */
    /** Not done yet, test_decoration3        = css3 2.4*/
    T<SPAttr::TEXT_DECORATION_LINE, SPITextDecorationLine> text_decoration_line;
    T<SPAttr::TEXT_DECORATION_STYLE, SPITextDecorationStyle> text_decoration_style;  // SPIEnum? Only one can be set at time.
    T<SPAttr::TEXT_DECORATION_COLOR, SPIColor> text_decoration_color;
    T<SPAttr::TEXT_DECORATION_FILL, SPIPaint> text_decoration_fill;
    T<SPAttr::TEXT_DECORATION_STROKE, SPIPaint> text_decoration_stroke;
    // used to implement text_decoration, not saved to or read from SVG file
    SPITextDecorationData text_decoration_data;

    // 16.3.2 is text-shadow. That's complicated.

    /* General visual properties ------------- */

    /** clip-rule: 0 nonzero, 1 evenodd */
    T<SPAttr::CLIP_RULE, SPIEnum<SPWindRule>> clip_rule;

    /** display */
    T<SPAttr::DISPLAY, SPIEnum<SPCSSDisplay>> display;

    /** overflow */
    T<SPAttr::OVERFLOW_, SPIEnum<SPOverflow>> overflow;

    /** visibility */
    T<SPAttr::VISIBILITY, SPIEnum<SPVisibility>> visibility;

    /** opacity */
    T<SPAttr::OPACITY, SPIScale24> opacity;

    /** mix-blend-mode:  CSS Compositing and Blending Level 1 */
    T<SPAttr::ISOLATION, SPIEnum<SPIsolation>> isolation;
    T<SPAttr::MIX_BLEND_MODE, SPIEnum<SPBlendMode>> mix_blend_mode;

    T<SPAttr::PAINT_ORDER, SPIPaintOrder> paint_order;

    /** color */
    T<SPAttr::COLOR, SPIColor> color;
    /** color-interpolation */
    T<SPAttr::COLOR_INTERPOLATION, SPIEnum<SPColorInterpolation>> color_interpolation;
    /** color-interpolation-filters */
    T<SPAttr::COLOR_INTERPOLATION_FILTERS, SPIEnum<SPColorInterpolation>> color_interpolation_filters;

    /** solid-color */
    T<SPAttr::SOLID_COLOR, SPIColor> solid_color;
    /** solid-opacity */
    T<SPAttr::SOLID_OPACITY, SPIScale24> solid_opacity;

    /** vector effect */
    T<SPAttr::VECTOR_EFFECT, SPIVectorEffect> vector_effect;

    /** fill */
    T<SPAttr::FILL, SPIPaint> fill;
    /** fill-opacity */
    T<SPAttr::FILL_OPACITY, SPIScale24> fill_opacity;
    /** fill-rule: 0 nonzero, 1 evenodd */
    T<SPAttr::FILL_RULE, SPIEnum<SPWindRule>> fill_rule;

    /** stroke */
    T<SPAttr::STROKE, SPIPaint> stroke;
    /** stroke-width */
    T<SPAttr::STROKE_WIDTH, SPILength> stroke_width;
    /** stroke-linecap */
    T<SPAttr::STROKE_LINECAP, SPIEnum<SPStrokeCapType>> stroke_linecap;
    /** stroke-linejoin */
    T<SPAttr::STROKE_LINEJOIN, SPIEnum<SPStrokeJoinType>> stroke_linejoin;
    /** stroke-miterlimit */
    T<SPAttr::STROKE_MITERLIMIT, SPIFloat> stroke_miterlimit;
    /** stroke-dasharray */
    T<SPAttr::STROKE_DASHARRAY, SPIDashArray> stroke_dasharray;
    /** stroke-dashoffset */
    T<SPAttr::STROKE_DASHOFFSET, SPILength> stroke_dashoffset;
    /** stroke-opacity */
    T<SPAttr::STROKE_OPACITY, SPIScale24> stroke_opacity;
    /** -inkscape-stroke */
    T<SPAttr::STROKE_EXTENSIONS, SPIStrokeExtensions> stroke_extensions;

    /** Marker list */
    T<SPAttr::MARKER, SPIString> marker;
    T<SPAttr::MARKER_START, SPIString> marker_start;
    T<SPAttr::MARKER_MID, SPIString> marker_mid;
    T<SPAttr::MARKER_END, SPIString> marker_end;
    SPIString* marker_ptrs[SP_MARKER_LOC_QTY];

    /* Filter effects ------------------------ */

    /** Filter effect */
    T<SPAttr::FILTER, SPIFilter> filter;
    /** normally not used, but duplicates the Gaussian blur deviation (if any) from the attached
        filter when the style is used for querying */
    // TODO remove, find other logic for querying
    T<SPAttr::INVALID, SPILength> filter_gaussianBlur_deviation;
    /** enable-background, used for defining where filter effects get their background image */
    T<SPAttr::ENABLE_BACKGROUND, SPIEnum<SPEnableBackground>> enable_background;

    /** gradient-stop */
    T<SPAttr::STOP_COLOR, SPIColor> stop_color;
    T<SPAttr::STOP_OPACITY, SPIScale24> stop_opacity;

    /* Rendering hints ----------------------- */

    /** hints on how to render: e.g. speed vs. accuracy.
     * As of April, 2013, only image_rendering used. */
    T<SPAttr::COLOR_RENDERING, SPIEnum<SPColorRendering>> color_rendering;
    T<SPAttr::IMAGE_RENDERING, SPIEnum<SPImageRendering>> image_rendering;
    T<SPAttr::SHAPE_RENDERING, SPIEnum<SPShapeRendering>> shape_rendering;
    T<SPAttr::TEXT_RENDERING, SPIEnum<SPTextRendering>> text_rendering;

    /* ----------------------- END PROPERTIES ------------------------- */

    /// style belongs to a cloned object
    bool cloned;

    sigc::connection release_connection;

    sigc::connection filter_modified_connection;
    sigc::connection fill_ps_modified_connection;
    sigc::connection stroke_ps_modified_connection;
    sigc::connection fill_ps_changed_connection;
    sigc::connection stroke_ps_changed_connection;

    /**
     * Emitted when paint server object, fill paint refers to, is changed. That is
     * when the reference starts pointing to a different address in memory.
     *
     * NB It is different from fill_ps_modified signal. When paint server is modified
     * it means some of it's attributes or children change.
     */
    sigc::signal<void, SPObject *, SPObject *> signal_fill_ps_changed;
    /**
     * Emitted when paint server object, fill paint refers to, is changed. That is
     * when the reference starts pointing to a different address in memory.
     */
    sigc::signal<void, SPObject *, SPObject *> signal_stroke_ps_changed;

    SPFilter       *getFilter()          { return (filter.href) ? filter.href->getObject() : nullptr; }
    SPFilter const *getFilter()    const { return (filter.href) ? filter.href->getObject() : nullptr; }
    Inkscape::URI const *getFilterURI() const { return (filter.href) ? filter.href->getURI() : nullptr; }

    SPPaintServer       *getFillPaintServer()         { return (fill.value.href) ? fill.value.href->getObject() : nullptr; }
    SPPaintServer const *getFillPaintServer()   const { return (fill.value.href) ? fill.value.href->getObject() : nullptr; }
    Inkscape::URI const *getFillURI()           const { return (fill.value.href) ? fill.value.href->getURI() : nullptr; }

    SPPaintServer       *getStrokePaintServer()       { return (stroke.value.href) ? stroke.value.href->getObject() : nullptr; }
    SPPaintServer const *getStrokePaintServer() const { return (stroke.value.href) ? stroke.value.href->getObject() : nullptr; }
    Inkscape::URI const *getStrokeURI()         const { return (stroke.value.href) ? stroke.value.href->getURI() : nullptr; }

    /**
     * Return a font feature string useful for Pango.
     */
    std::string getFontFeatureString();

    /**
     * Get either the fill or the stroke property
     */
    SPIPaint       *getFillOrStroke(bool fill_)       { return fill_ ? fill.upcast() : stroke.upcast(); }
    SPIPaint const *getFillOrStroke(bool fill_) const { return fill_ ? fill.upcast() : stroke.upcast(); }

    /* ----------------------- EXTENDED PROPERTIES ------------------------- */
    std::map<std::string, std::string> extended_properties;
};

SPStyle *sp_style_ref(SPStyle *style); // SPStyle::ref();

SPStyle *sp_style_unref(SPStyle *style); // SPStyle::unref();

void sp_style_set_to_uri(SPStyle *style, bool isfill, Inkscape::URI const *uri); // ?

char const *sp_style_get_css_unit_string(int unit);  // No change?

#define SP_CSS_FONT_SIZE_DEFAULT 12.0
double sp_style_css_size_px_to_units(double size, int unit, double font_size = SP_CSS_FONT_SIZE_DEFAULT); // No change?
double sp_style_css_size_units_to_px(double size, int unit, double font_size = SP_CSS_FONT_SIZE_DEFAULT); // No change?


SPCSSAttr *sp_css_attr_from_style (SPStyle const *const style, unsigned int flags);
SPCSSAttr *sp_css_attr_from_object(SPObject *object, unsigned int flags = SP_STYLE_FLAG_IFSET);
SPCSSAttr *sp_css_attr_unset_text(SPCSSAttr *css);
SPCSSAttr *sp_css_attr_unset_blacklist(SPCSSAttr *css);
SPCSSAttr *sp_css_attr_unset_uris(SPCSSAttr *css);
SPCSSAttr *sp_css_attr_scale(SPCSSAttr *css, double ex);

void sp_style_unset_property_attrs(SPObject *o);

void sp_style_set_property_url (SPObject *item, char const *property, SPObject *linked, bool recursive);

void css_quote( Glib::ustring &val );   // Add quotes around CSS values
void css_unquote( Glib::ustring &val ); // Remove quotes from CSS values (style-internal.cpp, xml/repr-css.cpp)
void css_font_family_quote( Glib::ustring &val ); // style-internal.cpp, text-toolbar.cpp
void css_font_family_unquote( Glib::ustring &val ); // style-internal.cpp, text-toolbar.cpp

Glib::ustring css2_escape_quote(char const *val);

#endif // SEEN_SP_STYLE_H


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

