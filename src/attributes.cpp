// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "attributes.h"
#include <cstring>
#include <map>

#include <algorithm>
#include <glib.h> // g_assert()


struct SPStyleProp {
    SPAttr code;
    gchar const *name;
};

/**
 * Lookup dictionary for attributes/properties.
 */

static SPStyleProp const props[] = {
    {SPAttr::INVALID, nullptr},
    /* SPObject */
    {SPAttr::ID, "id"},
    {SPAttr::STYLE, "style"},
    {SPAttr::INKSCAPE_COLLECT, "inkscape:collect"},
    {SPAttr::INKSCAPE_LABEL, "inkscape:label"},
    /* SPRoot */
    {SPAttr::VERSION, "version"},
    {SPAttr::INKSCAPE_VERSION, "inkscape:version"},
    {SPAttr::WIDTH, "width"},
    {SPAttr::HEIGHT, "height"},
    {SPAttr::VIEWBOX, "viewBox"},
    {SPAttr::PRESERVEASPECTRATIO, "preserveAspectRatio"},
    {SPAttr::ONLOAD, "onload"},
    {SPAttr::SODIPODI_DOCNAME, "sodipodi:docname"},
    /* SPItem */
    {SPAttr::TRANSFORM, "transform"},
    {SPAttr::SODIPODI_TYPE, "sodipodi:type"},
    {SPAttr::SODIPODI_INSENSITIVE, "sodipodi:insensitive"},
    {SPAttr::CONNECTOR_AVOID, "inkscape:connector-avoid"},
    {SPAttr::CONNECTION_POINTS, "inkscape:connection-points"},
    {SPAttr::TRANSFORM_CENTER_X, "inkscape:transform-center-x"},
    {SPAttr::TRANSFORM_CENTER_Y, "inkscape:transform-center-y"},
    {SPAttr::INKSCAPE_PATH_EFFECT, "inkscape:path-effect"},
    {SPAttr::INKSCAPE_HIGHLIGHT_COLOR, "inkscape:highlight-color"},
    {SPAttr::INKSCAPE_SPRAY_ORIGIN, "inkscape:spray-origin"},
    /* SPAnchor */
    {SPAttr::XLINK_HREF, "xlink:href"},
    {SPAttr::XLINK_TYPE, "xlink:type"},
    {SPAttr::XLINK_ROLE, "xlink:role"},
    {SPAttr::XLINK_ARCROLE, "xlink:arcrole"},
    {SPAttr::XLINK_TITLE, "xlink:title"},
    {SPAttr::XLINK_SHOW, "xlink:show"},
    {SPAttr::XLINK_ACTUATE, "xlink:actuate"},
    {SPAttr::TARGET, "target"},
    {SPAttr::INKSCAPE_GROUPMODE, "inkscape:groupmode"},
    {SPAttr::INKSCAPE_EXPANDED, "inkscape:expanded"},
    /* SPNamedView */
    {SPAttr::VIEWONLY, "viewonly"},
    {SPAttr::SHOWGUIDES, "showguides"},
    {SPAttr::SHOWGRIDS, "showgrid"},
    {SPAttr::GRIDTOLERANCE, "gridtolerance"},
    {SPAttr::GUIDETOLERANCE, "guidetolerance"},
    {SPAttr::OBJECTTOLERANCE, "objecttolerance"},
    {SPAttr::GUIDECOLOR, "guidecolor"},
    {SPAttr::GUIDEOPACITY, "guideopacity"},
    {SPAttr::GUIDEHICOLOR, "guidehicolor"},
    {SPAttr::GUIDEHIOPACITY, "guidehiopacity"},
    {SPAttr::SHOWBORDER, "showborder"},
    {SPAttr::SHOWPAGESHADOW, "inkscape:showpageshadow"},
    {SPAttr::BORDERLAYER, "borderlayer"},
    {SPAttr::BORDERCOLOR, "bordercolor"},
    {SPAttr::BORDEROPACITY, "borderopacity"},
    {SPAttr::PAGECOLOR, "pagecolor"},
    {SPAttr::FIT_MARGIN_TOP, "fit-margin-top"},
    {SPAttr::FIT_MARGIN_LEFT, "fit-margin-left"},
    {SPAttr::FIT_MARGIN_RIGHT, "fit-margin-right"},
    {SPAttr::FIT_MARGIN_BOTTOM, "fit-margin-bottom"},
    {SPAttr::INKSCAPE_PAGECHECKERBOARD, "inkscape:pagecheckerboard"},
    {SPAttr::INKSCAPE_PAGEOPACITY, "inkscape:pageopacity"},
    {SPAttr::INKSCAPE_PAGESHADOW, "inkscape:pageshadow"},
    {SPAttr::INKSCAPE_ZOOM, "inkscape:zoom"},
    {SPAttr::INKSCAPE_ROTATION, "inkscape:rotation"},
    {SPAttr::INKSCAPE_CX, "inkscape:cx"},
    {SPAttr::INKSCAPE_CY, "inkscape:cy"},
    {SPAttr::INKSCAPE_WINDOW_WIDTH, "inkscape:window-width"},
    {SPAttr::INKSCAPE_WINDOW_HEIGHT, "inkscape:window-height"},
    {SPAttr::INKSCAPE_WINDOW_X, "inkscape:window-x"},
    {SPAttr::INKSCAPE_WINDOW_Y, "inkscape:window-y"},
    {SPAttr::INKSCAPE_WINDOW_MAXIMIZED, "inkscape:window-maximized"},
    {SPAttr::INKSCAPE_SNAP_GLOBAL, "inkscape:snap-global"},
    {SPAttr::INKSCAPE_SNAP_PERP, "inkscape:snap-perpendicular"},
    {SPAttr::INKSCAPE_SNAP_TANG, "inkscape:snap-tangential"},
    {SPAttr::INKSCAPE_SNAP_BBOX, "inkscape:snap-bbox"},
    {SPAttr::INKSCAPE_SNAP_NODE, "inkscape:snap-nodes"},
    {SPAttr::INKSCAPE_SNAP_OTHERS, "inkscape:snap-others"},
    {SPAttr::INKSCAPE_SNAP_FROM_GUIDE, "inkscape:snap-from-guide"},
    {SPAttr::INKSCAPE_SNAP_ROTATION_CENTER, "inkscape:snap-center"},
    {SPAttr::INKSCAPE_SNAP_GRID, "inkscape:snap-grids"},
    {SPAttr::INKSCAPE_SNAP_GUIDE, "inkscape:snap-to-guides"},
    {SPAttr::INKSCAPE_SNAP_NODE_SMOOTH, "inkscape:snap-smooth-nodes"},
    {SPAttr::INKSCAPE_SNAP_LINE_MIDPOINT, "inkscape:snap-midpoints"},
    {SPAttr::INKSCAPE_SNAP_OBJECT_MIDPOINT, "inkscape:snap-object-midpoints"},
    {SPAttr::INKSCAPE_SNAP_TEXT_BASELINE, "inkscape:snap-text-baseline"},
    {SPAttr::INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT, "inkscape:snap-bbox-edge-midpoints"},
    {SPAttr::INKSCAPE_SNAP_BBOX_MIDPOINT, "inkscape:snap-bbox-midpoints"},
    {SPAttr::INKSCAPE_SNAP_PATH_INTERSECTION, "inkscape:snap-intersection-paths"},
    {SPAttr::INKSCAPE_SNAP_PATH, "inkscape:object-paths"},
    {SPAttr::INKSCAPE_SNAP_PATH_CLIP, "inkscape:snap-path-clip"},
    {SPAttr::INKSCAPE_SNAP_PATH_MASK, "inkscape:snap-path-mask"},
    {SPAttr::INKSCAPE_SNAP_NODE_CUSP, "inkscape:object-nodes"},
    {SPAttr::INKSCAPE_SNAP_BBOX_EDGE, "inkscape:bbox-paths"},
    {SPAttr::INKSCAPE_SNAP_BBOX_CORNER, "inkscape:bbox-nodes"},
    {SPAttr::INKSCAPE_SNAP_PAGE_BORDER, "inkscape:snap-page"},
    {SPAttr::INKSCAPE_CURRENT_LAYER, "inkscape:current-layer"},
    {SPAttr::INKSCAPE_DOCUMENT_UNITS, "inkscape:document-units"},  // This setting sets the Display units, *not* the units used in SVG
    {SPAttr::INKSCAPE_LOCKGUIDES, "inkscape:lockguides"},
    {SPAttr::UNITS, "units"},
    /* SPColorProfile */
    {SPAttr::LOCAL, "local"},
    {SPAttr::NAME, "name"},
    {SPAttr::RENDERING_INTENT, "rendering-intent"},
    /* SPGuide */
    {SPAttr::ORIENTATION, "orientation"},
    {SPAttr::POSITION, "position"},
    {SPAttr::INKSCAPE_COLOR, "inkscape:color"},
    {SPAttr::INKSCAPE_LOCKED, "inkscape:locked"},
    /* SPImage */
    {SPAttr::X, "x"},
    {SPAttr::Y, "y"},
    {SPAttr::SVG_DPI, "inkscape:svg-dpi"},
    /* SPPath */
    {SPAttr::INKSCAPE_ORIGINAL_D, "inkscape:original-d"},
    /* (Note: XML representation of connectors may change in future.) */
    {SPAttr::CONNECTOR_TYPE, "inkscape:connector-type"},
    {SPAttr::CONNECTOR_CURVATURE, "inkscape:connector-curvature"},
    {SPAttr::INKSCAPE_CONNECTOR_SPACING, "inkscape:connector-spacing"},
    {SPAttr::CONNECTION_START, "inkscape:connection-start"},
    {SPAttr::CONNECTION_END, "inkscape:connection-end"},
    {SPAttr::CONNECTION_START_POINT, "inkscape:connection-start-point"},
    {SPAttr::CONNECTION_END_POINT, "inkscape:connection-end-point"},
    /* SPRect */
    {SPAttr::RX, "rx"},
    {SPAttr::RY, "ry"},
    /* Box3D */
    {SPAttr::INKSCAPE_BOX3D_PERSPECTIVE_ID, "inkscape:perspectiveID"},
    {SPAttr::INKSCAPE_BOX3D_CORNER0, "inkscape:corner0"},
    {SPAttr::INKSCAPE_BOX3D_CORNER7, "inkscape:corner7"},
    /* Box3DSide */
    {SPAttr::INKSCAPE_BOX3D_SIDE_TYPE, "inkscape:box3dsidetype"}, // XYfront, etc.
    /* Persp3D */
    {SPAttr::INKSCAPE_PERSP3D, "inkscape:persp3d"},
    {SPAttr::INKSCAPE_PERSP3D_VP_X, "inkscape:vp_x"},
    {SPAttr::INKSCAPE_PERSP3D_VP_Y, "inkscape:vp_y"},
    {SPAttr::INKSCAPE_PERSP3D_VP_Z, "inkscape:vp_z"},
    {SPAttr::INKSCAPE_PERSP3D_ORIGIN, "inkscape:persp3d-origin"},
    /* SPEllipse */
    {SPAttr::R, "r"},
    {SPAttr::CX, "cx"},
    {SPAttr::CY, "cy"},
    {SPAttr::SODIPODI_CX, "sodipodi:cx"},
    {SPAttr::SODIPODI_CY, "sodipodi:cy"},
    {SPAttr::SODIPODI_RX, "sodipodi:rx"},
    {SPAttr::SODIPODI_RY, "sodipodi:ry"},
    {SPAttr::SODIPODI_START, "sodipodi:start"},
    {SPAttr::SODIPODI_END, "sodipodi:end"},
    {SPAttr::SODIPODI_OPEN, "sodipodi:open"}, // Deprecated
    {SPAttr::SODIPODI_ARC_TYPE, "sodipodi:arc-type"},
    /* SPStar */
    {SPAttr::SODIPODI_SIDES, "sodipodi:sides"},
    {SPAttr::SODIPODI_R1, "sodipodi:r1"},
    {SPAttr::SODIPODI_R2, "sodipodi:r2"},
    {SPAttr::SODIPODI_ARG1, "sodipodi:arg1"},
    {SPAttr::SODIPODI_ARG2, "sodipodi:arg2"},
    {SPAttr::INKSCAPE_FLATSIDED, "inkscape:flatsided"},
    {SPAttr::INKSCAPE_ROUNDED, "inkscape:rounded"},
    {SPAttr::INKSCAPE_RANDOMIZED, "inkscape:randomized"},
    /* SPSpiral */
    {SPAttr::SODIPODI_EXPANSION, "sodipodi:expansion"},
    {SPAttr::SODIPODI_REVOLUTION, "sodipodi:revolution"},
    {SPAttr::SODIPODI_RADIUS, "sodipodi:radius"},
    {SPAttr::SODIPODI_ARGUMENT, "sodipodi:argument"},
    {SPAttr::SODIPODI_T0, "sodipodi:t0"},
    /* SPOffset */
    {SPAttr::SODIPODI_ORIGINAL, "sodipodi:original"},
    {SPAttr::INKSCAPE_ORIGINAL, "inkscape:original"},
    {SPAttr::INKSCAPE_HREF, "inkscape:href"},
    {SPAttr::INKSCAPE_RADIUS, "inkscape:radius"},
    /* SPLine */
    {SPAttr::X1, "x1"},
    {SPAttr::Y1, "y1"},
    {SPAttr::X2, "x2"},
    {SPAttr::Y2, "y2"},
    /* SPPolyline */
    {SPAttr::POINTS, "points"},
    /* SPTSpan */
    {SPAttr::DX, "dx"},
    {SPAttr::DY, "dy"},
    {SPAttr::ROTATE, "rotate"},
    {SPAttr::TEXTLENGTH, "textLength"},
    {SPAttr::LENGTHADJUST, "lengthAdjust"},
    {SPAttr::SODIPODI_ROLE, "sodipodi:role"},
    /* SPText */
    {SPAttr::SODIPODI_LINESPACING, "sodipodi:linespacing"},
    /* SPTextPath */
    {SPAttr::STARTOFFSET, "startOffset"},
    {SPAttr::SIDE, "side"},
    /* SPStop */
    {SPAttr::OFFSET, "offset"},
    /* SPFilter */
    {SPAttr::FILTERUNITS, "filterUnits"},
    {SPAttr::PRIMITIVEUNITS, "primitiveUnits"},
    {SPAttr::FILTERRES, "filterRes"},
    /* Filter primitives common */
    {SPAttr::IN_, "in"},
    {SPAttr::RESULT, "result"},
    /*feBlend*/
    {SPAttr::MODE, "mode"},
    {SPAttr::IN2, "in2"},
    /*feColorMatrix*/
    {SPAttr::TYPE, "type"},
    {SPAttr::VALUES, "values"},
    /*feComponentTransfer*/
    //{SPAttr::TYPE, "type"},
    {SPAttr::TABLEVALUES, "tableValues"},
    {SPAttr::SLOPE, "slope"},
    {SPAttr::INTERCEPT, "intercept"},
    {SPAttr::AMPLITUDE, "amplitude"},
    {SPAttr::EXPONENT, "exponent"},
    //{SPAttr::OFFSET, "offset"},
    /*feComposite*/
    {SPAttr::OPERATOR, "operator"},
    {SPAttr::K1, "k1"},
    {SPAttr::K2, "k2"},
    {SPAttr::K3, "k3"},
    {SPAttr::K4, "k4"},
    //{SPAttr::IN2, "in2"},
    /*feConvolveMatrix*/
    {SPAttr::ORDER, "order"},
    {SPAttr::KERNELMATRIX, "kernelMatrix"},
    {SPAttr::DIVISOR, "divisor"},
    {SPAttr::BIAS, "bias"},
    {SPAttr::TARGETX, "targetX"},
    {SPAttr::TARGETY, "targetY"},
    {SPAttr::EDGEMODE, "edgeMode"},
    {SPAttr::KERNELUNITLENGTH, "kernelUnitLength"},
    {SPAttr::PRESERVEALPHA, "preserveAlpha"},
    /*feDiffuseLighting*/
    {SPAttr::SURFACESCALE, "surfaceScale"},
    {SPAttr::DIFFUSECONSTANT, "diffuseConstant"},
    //{SPAttr::KERNELUNITLENGTH, "kernelUnitLength"},
    /*feDisplacementMap*/
    {SPAttr::SCALE, "scale"},
    {SPAttr::XCHANNELSELECTOR, "xChannelSelector"},
    {SPAttr::YCHANNELSELECTOR, "yChannelSelector"},
    //{SPAttr::IN2, "in2"},
    /*feDistantLight*/
    {SPAttr::AZIMUTH, "azimuth"},
    {SPAttr::ELEVATION, "elevation"},
    /*fePointLight*/
    {SPAttr::Z, "z"},
    /*feSpotLight*/
    {SPAttr::POINTSATX, "pointsAtX"},
    {SPAttr::POINTSATY, "pointsAtY"},
    {SPAttr::POINTSATZ, "pointsAtZ"},
    {SPAttr::LIMITINGCONEANGLE, "limitingConeAngle"},
    /* SPGaussianBlur */
    {SPAttr::STDDEVIATION, "stdDeviation"},
    /*feImage*/
    /*feMerge*/
    /*feMorphology*/
    //{SPAttr::OPERATOR, "operator"},
    {SPAttr::RADIUS, "radius"},
    /*feOffset*/
    //{SPAttr::DX, "dx"},
    //{SPAttr::DY, "dy"},
    /*feSpecularLighting*/
    {SPAttr::SPECULARCONSTANT, "specularConstant"},
    {SPAttr::SPECULAREXPONENT, "specularExponent"},
    /*feTile*/
    /*feTurbulence*/
    {SPAttr::BASEFREQUENCY, "baseFrequency"},
    {SPAttr::NUMOCTAVES, "numOctaves"},
    {SPAttr::SEED, "seed"},
    {SPAttr::STITCHTILES, "stitchTiles"},
    //{SPAttr::TYPE, "type"},
    /* SPGradient */
    {SPAttr::GRADIENTUNITS, "gradientUnits"},
    {SPAttr::GRADIENTTRANSFORM, "gradientTransform"},
    {SPAttr::SPREADMETHOD, "spreadMethod"},
    {SPAttr::INKSCAPE_SWATCH, "inkscape:swatch"},
    /* SPRadialGradient */
    {SPAttr::FX, "fx"},
    {SPAttr::FY, "fy"},
    {SPAttr::FR, "fr"},
    /* SPMeshPatch */
    {SPAttr::TENSOR, "tensor"},
    //{SPAttr::TYPE, "type"},
    /* SPPattern */
    {SPAttr::PATTERNUNITS, "patternUnits"},
    {SPAttr::PATTERNCONTENTUNITS, "patternContentUnits"},
    {SPAttr::PATTERNTRANSFORM, "patternTransform"},
    /* SPHatch */
    {SPAttr::HATCHUNITS, "hatchUnits"},
    {SPAttr::HATCHCONTENTUNITS, "hatchContentUnits"},
    {SPAttr::HATCHTRANSFORM, "hatchTransform"},
    {SPAttr::PITCH, "pitch"},
    /* SPClipPath */
    {SPAttr::CLIPPATHUNITS, "clipPathUnits"},
    /* SPMask */
    {SPAttr::MASKUNITS, "maskUnits"},
    {SPAttr::MASKCONTENTUNITS, "maskContentUnits"},
    /* SPMarker */
    {SPAttr::MARKERUNITS, "markerUnits"},
    {SPAttr::REFX, "refX"},
    {SPAttr::REFY, "refY"},
    {SPAttr::MARKERWIDTH, "markerWidth"},
    {SPAttr::MARKERHEIGHT, "markerHeight"},
    {SPAttr::ORIENT, "orient"},
    /* SPStyleElem */
    //{SPAttr::TYPE, "type"},
    /* Animations */
    {SPAttr::ATTRIBUTENAME, "attributeName"},
    {SPAttr::ATTRIBUTETYPE, "attributeType"},
    {SPAttr::BEGIN, "begin"},
    {SPAttr::DUR, "dur"},
    {SPAttr::END, "end"},
    {SPAttr::MIN, "min"},
    {SPAttr::MAX, "max"},
    {SPAttr::RESTART, "restart"},
    {SPAttr::REPEATCOUNT, "repeatCount"},
    {SPAttr::REPEATDUR, "repeatDur"},
    /* Interpolating animations */
    {SPAttr::CALCMODE, "calcMode"},
    //{SPAttr::VALUES, "values"},
    {SPAttr::KEYTIMES, "keyTimes"},
    {SPAttr::KEYSPLINES, "keySplines"},
    {SPAttr::FROM, "from"},
    {SPAttr::TO, "to"},
    {SPAttr::BY, "by"},
    {SPAttr::ADDITIVE, "additive"},
    {SPAttr::ACCUMULATE, "accumulate"},

    /* SVGFonts */
    /*<font>*/
    {SPAttr::HORIZ_ORIGIN_X, "horiz-origin-x"},
    {SPAttr::HORIZ_ORIGIN_Y, "horiz-origin-y"},
    {SPAttr::HORIZ_ADV_X, "horiz-adv-x"},
    {SPAttr::VERT_ORIGIN_X, "vert-origin-x"},
    {SPAttr::VERT_ORIGIN_Y, "vert-origin-y"},
    {SPAttr::VERT_ADV_Y, "vert-adv-y"},

    /*<glyph>*/
    {SPAttr::UNICODE, "unicode"},
    {SPAttr::GLYPH_NAME, "glyph-name"},
    //{SPAttr::ORIENTATION, "orientation"},
    {SPAttr::ARABIC_FORM, "arabic-form"},
    {SPAttr::LANG, "lang"},

    /*<hkern> and <vkern>*/
    {SPAttr::U1, "u1"},
    {SPAttr::G1, "g1"},
    {SPAttr::U2, "u2"},
    {SPAttr::G2, "g2"},
    {SPAttr::K, "k"},

    /*<font-face>*/
    //{SPAttr::FONT_FAMILY, "font-family"}, these are already set for CSS2 (SPAttr::FONT_FAMILY, SPAttr::FONT_STYLE, SPAttr::FONT_VARIANT etc...)
    //{SPAttr::FONT_STYLE, "font-style"},
    //{SPAttr::FONT_VARIANT, "font-variant"},
    //{SPAttr::FONT_WEIGHT, "font-weight"},
    //{SPAttr::FONT_STRETCH, "font-stretch"},
    //{SPAttr::FONT_SIZE, "font-size"},
    {SPAttr::UNICODE_RANGE, "unicode-range"},
    {SPAttr::UNITS_PER_EM, "units-per-em"},
    {SPAttr::PANOSE_1, "panose-1"},
    {SPAttr::STEMV, "stemv"},
    {SPAttr::STEMH, "stemh"},
    //{SPAttr::SLOPE, "slope"},
    {SPAttr::CAP_HEIGHT, "cap-height"},
    {SPAttr::X_HEIGHT, "x-height"},
    {SPAttr::ACCENT_HEIGHT, "accent-height"},
    {SPAttr::ASCENT, "ascent"},
    {SPAttr::DESCENT, "descent"},
    {SPAttr::WIDTHS, "widths"},
    {SPAttr::BBOX, "bbox"},
    {SPAttr::IDEOGRAPHIC, "ideographic"},
    {SPAttr::ALPHABETIC, "alphabetic"},
    {SPAttr::MATHEMATICAL, "mathematical"},
    {SPAttr::HANGING, "hanging"},
    {SPAttr::V_IDEOGRAPHIC, "v-ideographic"},
    {SPAttr::V_ALPHABETIC, "v-alphabetic"},
    {SPAttr::V_MATHEMATICAL, "v-mathematical"},
    {SPAttr::V_HANGING, "v-hanging"},
    {SPAttr::UNDERLINE_POSITION, "underline-position"},
    {SPAttr::UNDERLINE_THICKNESS, "underline-thickness"},
    {SPAttr::STRIKETHROUGH_POSITION, "strikethrough-position"},
    {SPAttr::STRIKETHROUGH_THICKNESS, "strikethrough-thickness"},
    {SPAttr::OVERLINE_POSITION, "overline-position"},
    {SPAttr::OVERLINE_THICKNESS, "overline-thickness"},

    /* XML */
    {SPAttr::XML_SPACE, "xml:space"},
    {SPAttr::XML_LANG,  "xml:lang"},

    /* typeset */
    {SPAttr::TEXT_NOMARKUP, "inkscape:srcNoMarkup"},
    {SPAttr::TEXT_PANGOMARKUP, "inkscape:srcPango" },
    {SPAttr::TEXT_INSHAPE, "inkscape:dstShape"},
    {SPAttr::TEXT_ONPATH, "inkscape:dstPath"},
    {SPAttr::TEXT_INBOX,"inkscape:dstBox"},
    {SPAttr::TEXT_INCOLUMN,"inkscape:dstColumn"},
    {SPAttr::TEXT_EXCLUDE,"inkscape:excludeShape"},
    {SPAttr::LAYOUT_OPTIONS,"inkscape:layoutOptions"},

    /* CSS & SVG Properites */

    /* SVG 2 Attributes promoted to properties */
    {SPAttr::D, "d"},

    /* Paint */
    {SPAttr::COLOR, "color"},
    {SPAttr::OPACITY, "opacity"},
    {SPAttr::FILL, "fill"},
    {SPAttr::FILL_OPACITY, "fill-opacity"},
    {SPAttr::FILL_RULE, "fill-rule"},
    {SPAttr::STROKE, "stroke"},
    {SPAttr::STROKE_OPACITY, "stroke-opacity"},
    {SPAttr::STROKE_WIDTH, "stroke-width"},
    {SPAttr::STROKE_LINECAP, "stroke-linecap"},
    {SPAttr::STROKE_LINEJOIN, "stroke-linejoin"},
    {SPAttr::STROKE_MITERLIMIT, "stroke-miterlimit"},
    {SPAttr::STROKE_DASHARRAY, "stroke-dasharray"},
    {SPAttr::STROKE_DASHOFFSET, "stroke-dashoffset"},
    {SPAttr::STROKE_EXTENSIONS, "-inkscape-stroke"},
    {SPAttr::MARKER, "marker"},
    {SPAttr::MARKER_END, "marker-end"},
    {SPAttr::MARKER_MID, "marker-mid"},
    {SPAttr::MARKER_START, "marker-start"},
    {SPAttr::PAINT_ORDER, "paint-order" },
    {SPAttr::SOLID_COLOR, "solid-color"},
    {SPAttr::SOLID_OPACITY, "solid-opacity"},
    {SPAttr::VECTOR_EFFECT, "vector-effect"},

    /* CSS Blending/Compositing */
    {SPAttr::MIX_BLEND_MODE, "mix-blend-mode"},
    {SPAttr::ISOLATION, "isolation"},

    /* Misc. Display */
    {SPAttr::DISPLAY, "display"},
    {SPAttr::OVERFLOW_, "overflow"},
    {SPAttr::VISIBILITY, "visibility"},

    /* Clip/Mask */
    {SPAttr::CLIP, "clip"},
    {SPAttr::CLIP_PATH, "clip-path"},
    {SPAttr::CLIP_RULE, "clip-rule"},
    {SPAttr::MASK, "mask"},

    /* Font */
    {SPAttr::FONT, "font"},
    {SPAttr::FONT_FAMILY, "font-family"},
    {SPAttr::INKSCAPE_FONT_SPEC, "-inkscape-font-specification"},
    {SPAttr::FONT_SIZE, "font-size"},
    {SPAttr::FONT_SIZE_ADJUST, "font-size-adjust"},
    {SPAttr::FONT_STRETCH, "font-stretch"},
    {SPAttr::FONT_STYLE, "font-style"},
    {SPAttr::FONT_VARIANT, "font-variant"},
    {SPAttr::FONT_WEIGHT, "font-weight"},

    /* Font Variants CSS 3 */
    {SPAttr::FONT_VARIANT_LIGATURES,  "font-variant-ligatures"},
    {SPAttr::FONT_VARIANT_POSITION,   "font-variant-position"},
    {SPAttr::FONT_VARIANT_CAPS,       "font-variant-caps"},
    {SPAttr::FONT_VARIANT_NUMERIC,    "font-variant-numeric"},
    {SPAttr::FONT_VARIANT_ALTERNATES, "font-variant-alternates"},
    {SPAttr::FONT_VARIANT_EAST_ASIAN, "font-variant-east-asian"},
    {SPAttr::FONT_FEATURE_SETTINGS,   "font-feature-settings"},

    /* Variable Fonts (CSS Fonts Module Level 4) */
    {SPAttr::FONT_VARIATION_SETTINGS,   "font-variation-settings"},

    /* Text */
    {SPAttr::TEXT_INDENT, "text-indent"},
    {SPAttr::TEXT_ALIGN, "text-align"},
    {SPAttr::LINE_HEIGHT, "line-height"},
    {SPAttr::LETTER_SPACING, "letter-spacing"},
    {SPAttr::WORD_SPACING, "word-spacing"},
    {SPAttr::TEXT_TRANSFORM, "text-transform"},

    /* Text (CSS3) */
    {SPAttr::DIRECTION, "direction"},
    {SPAttr::WRITING_MODE, "writing-mode"},
    {SPAttr::TEXT_ORIENTATION, "text-orientation"},
    {SPAttr::UNICODE_BIDI, "unicode-bidi"},
    {SPAttr::ALIGNMENT_BASELINE, "alignment-baseline"},
    {SPAttr::BASELINE_SHIFT, "baseline-shift"},
    {SPAttr::DOMINANT_BASELINE, "dominant-baseline"},
    {SPAttr::GLYPH_ORIENTATION_HORIZONTAL, "glyph-orientation-horizontal"},
    {SPAttr::GLYPH_ORIENTATION_VERTICAL, "glyph-orientation-vertical"},
    {SPAttr::KERNING, "kerning"},
    {SPAttr::TEXT_ANCHOR, "text-anchor"},
    {SPAttr::WHITE_SPACE, "white-space"},

    /* SVG 2 Text Wrapping */
    {SPAttr::SHAPE_INSIDE,  "shape-inside"},
    {SPAttr::SHAPE_SUBTRACT,"shape-subtract"},
    {SPAttr::SHAPE_PADDING, "shape-padding"},
    {SPAttr::SHAPE_MARGIN,  "shape-margin"},
    {SPAttr::INLINE_SIZE,   "inline-size"},

    /* Text Decoration */
    {SPAttr::TEXT_DECORATION,       "text-decoration"},  // CSS 2/CSS3-Shorthand
    {SPAttr::TEXT_DECORATION_LINE,  "text-decoration-line"},
    {SPAttr::TEXT_DECORATION_STYLE, "text-decoration-style"},
    {SPAttr::TEXT_DECORATION_COLOR, "text-decoration-color"},
    {SPAttr::TEXT_DECORATION_FILL,  "text-decoration-fill"},
    {SPAttr::TEXT_DECORATION_STROKE,"text-decoration-stroke"},

    /* Filter */
    {SPAttr::ENABLE_BACKGROUND, "enable-background"},
    {SPAttr::FILTER, "filter"},
    {SPAttr::FLOOD_COLOR, "flood-color"},
    {SPAttr::FLOOD_OPACITY, "flood-opacity"},
    {SPAttr::LIGHTING_COLOR, "lighting-color"},
    {SPAttr::AUTO_REGION, "inkscape:auto-region"},

    /* Gradient */
    {SPAttr::STOP_COLOR, "stop-color"},
    {SPAttr::STOP_OPACITY, "stop-opacity"},
    {SPAttr::STOP_PATH, "path"},

    /* Rendering */
    {SPAttr::COLOR_INTERPOLATION, "color-interpolation"},
    {SPAttr::COLOR_INTERPOLATION_FILTERS, "color-interpolation-filters"},
    {SPAttr::COLOR_PROFILE, "color-profile"},
    {SPAttr::COLOR_RENDERING, "color-rendering"},
    {SPAttr::IMAGE_RENDERING, "image-rendering"},
    {SPAttr::SHAPE_RENDERING, "shape-rendering"},
    {SPAttr::TEXT_RENDERING, "text-rendering"},

    /* Interactivity */
    {SPAttr::POINTER_EVENTS, "pointer-events"},
    {SPAttr::CURSOR, "cursor"},

    /* Conditional */
    {SPAttr::SYSTEM_LANGUAGE, "systemLanguage"},
    {SPAttr::REQUIRED_FEATURES, "requiredFeatures"},
    {SPAttr::REQUIRED_EXTENSIONS, "requiredExtensions"},

    /* LivePathEffect */
    {SPAttr::PATH_EFFECT, "effect"},

};

#define n_attrs (sizeof(props) / sizeof(props[0]))

static_assert(n_attrs == (int)SPAttr::SPAttr_SIZE, "");

/**
 * Inverse to the \c props array for lookup by name.
 */
class AttributeLookupImpl {
    friend SPAttr sp_attribute_lookup(gchar const *key);

    struct cstrless {
        bool operator()(char const *lhs, char const *rhs) const { return std::strcmp(lhs, rhs) < 0; }
    };

    std::map<char const *, SPAttr, cstrless> m_map;

    AttributeLookupImpl()
    {
        for (unsigned int i = 1; i < n_attrs; i++) {
            // sanity check: order of props array must match SPAttr
            g_assert( (int)(props[i].code) == i);

            m_map[props[i].name] = props[i].code;
        }
    }
};

SPAttr
sp_attribute_lookup(gchar const *key)
{
    static AttributeLookupImpl const _instance;
    auto it = _instance.m_map.find(key);
    if (it != _instance.m_map.end()) {
        return it->second;
    }
    // std::cerr << "sp_attribute_lookup: invalid attribute: "
    //           << (key?key:"Null") << std::endl;
    return SPAttr::INVALID;
}

gchar const *
sp_attribute_name(SPAttr id)
{
    g_assert((int)id < n_attrs);
    return props[(int)id].name;
}

std::vector<Glib::ustring> sp_attribute_name_list(bool css_only)
{
    std::vector<Glib::ustring> result;
    for (auto prop : props) {
        if (!css_only || SP_ATTRIBUTE_IS_CSS(prop.code)) {
            result.emplace_back(prop.name);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool SP_ATTRIBUTE_IS_CSS(SPAttr k) { //
    return (k >= SPAttr::D) && (k < SPAttr::SYSTEM_LANGUAGE);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
