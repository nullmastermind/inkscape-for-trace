// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SP_ATTRIBUTES_H__
#define __SP_ATTRIBUTES_H__

/** \file
 * Lookup dictionary for attributes/properties.
 */
/*
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <glibmm/value.h>
#include <vector>

/*
 * Do not change order of attributes and properties. Attribute and
 * order in an SVG file is  (optionally) determined by the order here.
 * This makes comparing different versions of a drawing easier using
 * line-by-line comparison. Also, inorder for proper parsing, some
 * properites must be before others (e.g. 'font' before 'font-family').
 */
enum class SPAttr {
    INVALID,  ///< Must have value 0.
    /* SPObject */
    ID,
    STYLE,
    INKSCAPE_COLLECT,
    INKSCAPE_LABEL,
    /* SPRoot  Put these near top as the apply to the entire SVG */
    VERSION,
    INKSCAPE_VERSION,
    WIDTH,
    HEIGHT,
    VIEWBOX,
    PRESERVEASPECTRATIO,
    ONLOAD,
    SODIPODI_DOCNAME,
    /* SPItem */
    TRANSFORM,
    SODIPODI_TYPE,
    SODIPODI_INSENSITIVE,
    CONNECTOR_AVOID,
    CONNECTION_POINTS,
    TRANSFORM_CENTER_X,
    TRANSFORM_CENTER_Y,
    INKSCAPE_PATH_EFFECT,
    INKSCAPE_HIGHLIGHT_COLOR,
    INKSCAPE_SPRAY_ORIGIN,
    /* SPAnchor */
    XLINK_HREF,
    XLINK_TYPE,
    XLINK_ROLE,
    XLINK_ARCROLE,
    XLINK_TITLE,
    XLINK_SHOW,
    XLINK_ACTUATE,
    TARGET,
    /* SPGroup */
    INKSCAPE_GROUPMODE,
    INKSCAPE_EXPANDED,
    /* SPNamedView */
    VIEWONLY,
    SHOWGUIDES,
    SHOWGRIDS,
    GRIDTOLERANCE,
    GUIDETOLERANCE,
    OBJECTTOLERANCE,
    GUIDECOLOR,
    GUIDEOPACITY,
    GUIDEHICOLOR,
    GUIDEHIOPACITY,
    SHOWBORDER,
    SHOWPAGESHADOW,
    BORDERLAYER,
    BORDERCOLOR,
    BORDEROPACITY,
    PAGECOLOR,
    FIT_MARGIN_TOP,
    FIT_MARGIN_LEFT,
    FIT_MARGIN_RIGHT,
    FIT_MARGIN_BOTTOM,
    INKSCAPE_PAGECHECKERBOARD,
    INKSCAPE_PAGEOPACITY,
    INKSCAPE_PAGESHADOW,
    INKSCAPE_ZOOM,
    INKSCAPE_ROTATION,
    INKSCAPE_CX,
    INKSCAPE_CY,
    INKSCAPE_WINDOW_WIDTH,
    INKSCAPE_WINDOW_HEIGHT,
    INKSCAPE_WINDOW_X,
    INKSCAPE_WINDOW_Y,
    INKSCAPE_WINDOW_MAXIMIZED,
    INKSCAPE_SNAP_GLOBAL,
    INKSCAPE_SNAP_PERP,
    INKSCAPE_SNAP_TANG,
    INKSCAPE_SNAP_BBOX,
    INKSCAPE_SNAP_NODE,
    INKSCAPE_SNAP_OTHERS,
    INKSCAPE_SNAP_FROM_GUIDE,
    INKSCAPE_SNAP_ROTATION_CENTER,
    INKSCAPE_SNAP_GRID,
    INKSCAPE_SNAP_GUIDE,
    INKSCAPE_SNAP_NODE_SMOOTH,
    INKSCAPE_SNAP_LINE_MIDPOINT,
    INKSCAPE_SNAP_OBJECT_MIDPOINT,
    INKSCAPE_SNAP_TEXT_BASELINE,
    INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT,
    INKSCAPE_SNAP_BBOX_MIDPOINT,
    INKSCAPE_SNAP_PATH_INTERSECTION,
    INKSCAPE_SNAP_PATH,
    INKSCAPE_SNAP_PATH_CLIP,
    INKSCAPE_SNAP_PATH_MASK,
    INKSCAPE_SNAP_NODE_CUSP,
    INKSCAPE_SNAP_BBOX_EDGE,
    INKSCAPE_SNAP_BBOX_CORNER,
    INKSCAPE_SNAP_PAGE_BORDER,
    INKSCAPE_CURRENT_LAYER,
    INKSCAPE_DOCUMENT_UNITS,
    INKSCAPE_LOCKGUIDES,
    UNITS,
    /* SPColorProfile */
    LOCAL,
    NAME,
    RENDERING_INTENT,
    /* SPGuide */
    ORIENTATION,
    POSITION,
    INKSCAPE_COLOR,
    INKSCAPE_LOCKED,
    /* SPImage, SPRect, etc. */
    X,
    Y,
    SVG_DPI,
    /* SPPath */
    // D,  Promoted to property in SVG 2
    INKSCAPE_ORIGINAL_D,
    CONNECTOR_TYPE,
    CONNECTOR_CURVATURE,
    INKSCAPE_CONNECTOR_SPACING,
    CONNECTION_START,
    CONNECTION_END,
    CONNECTION_START_POINT,
    CONNECTION_END_POINT,
    /* SPRect */
    RX,
    RY,
    /* Box3D */
    INKSCAPE_BOX3D_PERSPECTIVE_ID,
    INKSCAPE_BOX3D_CORNER0, // "upper left front" corner (as a point in 3-space)
    INKSCAPE_BOX3D_CORNER7, // "lower right rear" corner (as a point in 3-space)
    /* Box3DSide */
    INKSCAPE_BOX3D_SIDE_TYPE,
    /* Persp3D */
    INKSCAPE_PERSP3D,
    INKSCAPE_PERSP3D_VP_X,
    INKSCAPE_PERSP3D_VP_Y,
    INKSCAPE_PERSP3D_VP_Z,
    INKSCAPE_PERSP3D_ORIGIN,
    /* SPEllipse */
    R,
    CX,
    CY,
    SODIPODI_CX,
    SODIPODI_CY,
    SODIPODI_RX,
    SODIPODI_RY,
    SODIPODI_START,
    SODIPODI_END,
    SODIPODI_OPEN,
    SODIPODI_ARC_TYPE,
    /* SPStar */
    SODIPODI_SIDES,
    SODIPODI_R1,
    SODIPODI_R2,
    SODIPODI_ARG1,
    SODIPODI_ARG2,
    INKSCAPE_FLATSIDED,
    INKSCAPE_ROUNDED,
    INKSCAPE_RANDOMIZED,
    /* SPSpiral */
    SODIPODI_EXPANSION,
    SODIPODI_REVOLUTION,
    SODIPODI_RADIUS,
    SODIPODI_ARGUMENT,
    SODIPODI_T0,
    /* SPOffset */
    SODIPODI_ORIGINAL,
    INKSCAPE_ORIGINAL,
    INKSCAPE_HREF,
    INKSCAPE_RADIUS,
    /* SPLine */
    X1,
    Y1,
    X2,
    Y2,
    /* SPPolyline */
    POINTS,
    /* SPTSpan */
    DX,
    DY,
    ROTATE,
    TEXTLENGTH,
    LENGTHADJUST,
    SODIPODI_ROLE,
    /* SPText */
    SODIPODI_LINESPACING,
    /* SPTextPath */
    STARTOFFSET,
    SIDE,
    /* SPStop */
    OFFSET,
    /* SPFilter */
    FILTERUNITS,
    PRIMITIVEUNITS,
    FILTERRES,
    /* Filter primitives common */
    IN_,
    RESULT,
    /*feBlend*/
    MODE,
    IN2,
    /*feColorMatrix*/
    TYPE,
    VALUES,
    /*feComponentTransfer*/
    //TYPE,
    TABLEVALUES,
    SLOPE,
    INTERCEPT,
    AMPLITUDE,
    EXPONENT,
    //OFFSET,
    /*feComposite*/
    OPERATOR,
    K1,
    K2,
    K3,
    K4,
    //IN2,
    /*feConvolveMatrix*/
    ORDER,
    KERNELMATRIX,
    DIVISOR,
    BIAS,
    TARGETX,
    TARGETY,
    EDGEMODE,
    KERNELUNITLENGTH,
    PRESERVEALPHA,
    /*feDiffuseLighting*/
    SURFACESCALE,
    DIFFUSECONSTANT,
    //KERNELUNITLENGTH,
    /*feDisplacementMap*/
    SCALE,
    XCHANNELSELECTOR,
    YCHANNELSELECTOR,
    //IN2,
    /*feDistantLight*/
    AZIMUTH,
    ELEVATION,
    /*fePointLight*/
    Z,
    /*feSpotLight*/
    POINTSATX,
    POINTSATY,
    POINTSATZ,
    LIMITINGCONEANGLE,
    /* SPGaussianBlur */
    STDDEVIATION,
    /*feImage*/
    /*feMerge*/
    /*feMorphology*/
    //OPERATOR,
    RADIUS,
    /*feOffset*/
    //DX,
    //DY,
    /*feSpecularLighting*/
    //SURFACESCALE,
    SPECULARCONSTANT,
    SPECULAREXPONENT,
    /*feTile*/
    /*feTurbulence*/
    BASEFREQUENCY,
    NUMOCTAVES,
    SEED,
    STITCHTILES,
    //TYPE,
    /* SPGradient */
    GRADIENTUNITS,
    GRADIENTTRANSFORM,
    SPREADMETHOD,
    INKSCAPE_SWATCH,
    /* SPRadialGradient */
    FX,
    FY,
    FR,
    /* SPMeshPatch */
    TENSOR,
    //TYPE,
    /* SPPattern */
    PATTERNUNITS,
    PATTERNCONTENTUNITS,
    PATTERNTRANSFORM,
    /* SPHatch */
    HATCHUNITS,
    HATCHCONTENTUNITS,
    HATCHTRANSFORM,
    PITCH,
    /* SPClipPath */
    CLIPPATHUNITS,
    /* SPMask */
    MASKUNITS,
    MASKCONTENTUNITS,
    /* SPMarker */
    MARKERUNITS,
    REFX,
    REFY,
    MARKERWIDTH,
    MARKERHEIGHT,
    ORIENT,
    /* SPStyleElem */
    //TYPE,
    /* Animations */
    ATTRIBUTENAME,
    ATTRIBUTETYPE,
    BEGIN,
    DUR,
    END,
    MIN,
    MAX,
    RESTART,
    REPEATCOUNT,
    REPEATDUR,

    /* Interpolating animations */
    CALCMODE,
    //VALUES,
    KEYTIMES,
    KEYSPLINES,
    FROM,
    TO,
    BY,
    ADDITIVE,
    ACCUMULATE,

    /* SVGFonts */
    /* SPFont */
    HORIZ_ORIGIN_X,
    HORIZ_ORIGIN_Y,
    HORIZ_ADV_X,
    VERT_ORIGIN_X,
    VERT_ORIGIN_Y,
    VERT_ADV_Y,

    UNICODE,
    GLYPH_NAME,
    //ORIENTATION,
    ARABIC_FORM,
    LANG,

    /*<hkern> and <vkern>*/
    U1,
    G1,
    U2,
    G2,
    K,

    /*<font-face>*/
//    FONT_FAMILY,
//    FONT_STYLE,
//    FONT_VARIANT,
//    FONT_WEIGHT,
//    FONT_STRETCH,
//    FONT_SIZE,
    UNICODE_RANGE,
    UNITS_PER_EM,
    PANOSE_1,
    STEMV,
    STEMH,
    //SLOPE,
    CAP_HEIGHT,
    X_HEIGHT,
    ACCENT_HEIGHT,
    ASCENT,
    DESCENT,
    WIDTHS,
    BBOX,
    IDEOGRAPHIC,
    ALPHABETIC,
    MATHEMATICAL,
    HANGING,
    V_IDEOGRAPHIC,
    V_ALPHABETIC,
    V_MATHEMATICAL,
    V_HANGING,
    UNDERLINE_POSITION,
    UNDERLINE_THICKNESS,
    STRIKETHROUGH_POSITION,
    STRIKETHROUGH_THICKNESS,
    OVERLINE_POSITION,
    OVERLINE_THICKNESS,

    /* XML */
    XML_SPACE,
    XML_LANG,

    /* typeset */
    TEXT_NOMARKUP,
    TEXT_PANGOMARKUP,
    TEXT_INSHAPE,
    TEXT_ONPATH,
    TEXT_INBOX,
    TEXT_INCOLUMN,
    TEXT_EXCLUDE,
    LAYOUT_OPTIONS,

    /*  CSS & SVG Properties   KEEP ORDER!
     *  If first or last property changed, macro at top must be changed!
     */

    /* SVG 2 Attributes promoted to properties */
    D,

    /* Paint */
    COLOR,
    OPACITY,
    FILL,
    FILL_OPACITY,
    FILL_RULE,
    STROKE,
    STROKE_OPACITY,
    STROKE_WIDTH,
    STROKE_LINECAP,
    STROKE_LINEJOIN,
    STROKE_MITERLIMIT,
    STROKE_DASHARRAY,
    STROKE_DASHOFFSET,
    STROKE_EXTENSIONS,
    MARKER,
    MARKER_END,
    MARKER_MID,
    MARKER_START,
    PAINT_ORDER, /* SVG2 */
    SOLID_COLOR,
    SOLID_OPACITY,
    VECTOR_EFFECT,
    
    /* CSS Blending/Compositing */
    MIX_BLEND_MODE,
    ISOLATION,

    /* Misc. Display */
    DISPLAY,
    OVERFLOW_,
    VISIBILITY,

    /* Clip/Mask */
    CLIP,
    CLIP_PATH,
    CLIP_RULE,
    MASK,

    /* Font: Order is important! */
    FONT,
    FONT_FAMILY,
    INKSCAPE_FONT_SPEC,  // Remove me
    FONT_SIZE,
    FONT_SIZE_ADJUST,
    FONT_STRETCH,
    FONT_STYLE,
    FONT_VARIANT,
    FONT_WEIGHT,

    /* Font Variants CSS 3 */
    FONT_VARIANT_LIGATURES,
    FONT_VARIANT_POSITION,
    FONT_VARIANT_CAPS,
    FONT_VARIANT_NUMERIC,
    FONT_VARIANT_ALTERNATES,
    FONT_VARIANT_EAST_ASIAN,
    FONT_FEATURE_SETTINGS,

    /* Variable Fonts (CSS Fonts Module Level 4) */
    FONT_VARIATION_SETTINGS,

    /* Text Layout */
    TEXT_INDENT,
    TEXT_ALIGN,
    LINE_HEIGHT,
    LETTER_SPACING,
    WORD_SPACING,
    TEXT_TRANSFORM,

    /* Text (CSS3) */
    DIRECTION,
    WRITING_MODE,
    TEXT_ORIENTATION,
    UNICODE_BIDI,
    ALIGNMENT_BASELINE,
    BASELINE_SHIFT,
    DOMINANT_BASELINE,
    GLYPH_ORIENTATION_HORIZONTAL,
    GLYPH_ORIENTATION_VERTICAL,
    KERNING,
    TEXT_ANCHOR,
    WHITE_SPACE,

    /* SVG 2 Text Wrapping */
    SHAPE_INSIDE,
    SHAPE_SUBTRACT,
    SHAPE_PADDING,
    SHAPE_MARGIN,
    INLINE_SIZE,
    
    /* Text Decoration */
    TEXT_DECORATION,  // CSS 2/CSS3-Shorthand
    TEXT_DECORATION_LINE,
    TEXT_DECORATION_STYLE,
    TEXT_DECORATION_COLOR,
    TEXT_DECORATION_FILL,
    TEXT_DECORATION_STROKE,

    /* Filter */
    ENABLE_BACKGROUND,
    FILTER,
    FLOOD_COLOR,
    FLOOD_OPACITY,
    LIGHTING_COLOR,
    AUTO_REGION,

    /* Gradient */
    STOP_COLOR,
    STOP_OPACITY,
    STOP_PATH,

    /* Rendering */
    COLOR_INTERPOLATION,
    COLOR_INTERPOLATION_FILTERS,
    COLOR_PROFILE,
    COLOR_RENDERING,
    IMAGE_RENDERING,
    SHAPE_RENDERING,
    TEXT_RENDERING,

    /* Interactivity */
    POINTER_EVENTS,
    CURSOR,

    /* Conditional */
    SYSTEM_LANGUAGE,
    REQUIRED_FEATURES,
    REQUIRED_EXTENSIONS,

    /* LivePathEffect */
    PATH_EFFECT,

    // sentinel
    SPAttr_SIZE
};

/**
 * True iff k is a property in SVG, i.e. something that can be written either in a style attribute
 * or as its own XML attribute. This must be kept in sync with SPAttr.
 */
bool SP_ATTRIBUTE_IS_CSS(SPAttr k);

/**
 * Get attribute id by name. Return INVALID for invalid names.
 */
SPAttr sp_attribute_lookup(gchar const *key);

/**
 * Get attribute name by id. Return NULL for invalid ids.
 */
gchar const *sp_attribute_name(SPAttr id);

/**
 * Get sorted attribute name list.
 * @param css_only If true, only return CSS properties
 */
std::vector<Glib::ustring> sp_attribute_name_list(bool css_only = false);

#endif


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
