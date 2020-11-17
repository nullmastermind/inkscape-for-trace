// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG stylesheets implementation - Classes used by SPStyle class.
 */

/* Authors:
 * C++ conversion:
 *   Tavmjong Bah <tavmjong@free.fr>
 * Legacy C implementation:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Peter Moulder <pmoulder@mail.csse.monash.edu.au>
 *   bulia byak <buliabyak@users.sf.net>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2005 Monash University
 * Copyright (C) 2012 Kris De Gussem
 * Copyright (C) 2014, 2018 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/regex.h>

#include "style-internal.h"
#include "style.h"

#include "bad-uri-exception.h"
#include "extract-uri.h"
#include "inkscape.h"
#include "preferences.h"
#include "streq.h"
#include "strneq.h"

#include "object/sp-text.h"

#include "svg/svg.h"
#include "svg/svg-color.h"
#include "svg/css-ostringstream.h"

#include "util/units.h"

// TODO REMOVE OR MAKE MEMBER FUNCTIONS
void sp_style_fill_paint_server_ref_changed(  SPObject *old_ref, SPObject *ref, SPStyle *style);
void sp_style_stroke_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style);
void sp_style_filter_ref_changed(             SPObject *old_ref, SPObject *ref, SPStyle *style);
void sp_style_set_ipaint_to_uri(SPStyle *style, SPIPaint *paint, const Inkscape::URI *uri, SPDocument *document);
void sp_style_set_ipaint_to_uri_string (SPStyle *style, SPIPaint *paint, const gchar *uri);

using Inkscape::CSSOStringStream;

// SPIBase --------------------------------------------------------------

Glib::ustring const &SPIBase::name() const
{
    static Glib::ustring names[(int)SPAttr::SPAttr_SIZE];
    auto &name = names[(int)id()];
    if (name.empty()) {
        auto const *namecstr = sp_attribute_name(id());
        name = namecstr ? namecstr : "anonymous";
    }
    return name;
}

/**
 * Check if this property should be written. This function won't do any writing so can
 * be used as a quick way to check if specific kinds of style attrs have been changed.
 *
 * @param flags Conditions bitmask of `SP_STYLE_FLAG_*` bits
 * @param style_src_req For `SP_STYLE_FLAG_IFSRC`
 * @param base Parent node style for `SP_STYLE_FLAG_IFDIFF`, can be NULL
 *
 * @return True if property should be written, false otherwise
 */
bool SPIBase::shall_write(guint const flags, SPStyleSrc const &style_src_req, SPIBase const *const base) const
{
    // flags SP_STYLE_FLAG_IFSET and SP_STYLE_FLAG_IFDIFF are ignored, their
    // information is redundant FIXME remove those flags

    // pointer equality handled in SPStyle::write, not expected here
    assert(base != this);

    if ((flags & SP_STYLE_FLAG_ALWAYS)) {
        assert(!(flags & SP_STYLE_FLAG_IFSRC));
        assert(!base);
        return true;
    }

    if (!set) {
        return false;
    }

    if ((flags & SP_STYLE_FLAG_IFSRC) && style_src_req != style_src) {
        return false;
    }

    if (base && inherits && *base == *this) {
        return false;
    }

    return true;
}

/**
 * Compile this style conditionally into a 'name: value' string suitable for css attrs.
 * see shall_write for details on conditional flags.
 *
 * @param flags Conditions bitmask of `SP_STYLE_FLAG_*` bits
 * @param style_src_req For `SP_STYLE_FLAG_IFSRC`
 * @param base Parent node style for `SP_STYLE_FLAG_IFDIFF`, can be NULL
 *
 * @return completed css string or empty string if conditions not met.
 */
const Glib::ustring SPIBase::write(guint const flags, SPStyleSrc const &style_src_req, SPIBase const *const base) const
{
    if (shall_write(flags, style_src_req, base)) {
        auto value = this->get_value();
        if ( !value.empty() ) {
            return (name() + ":" + value + important_str() + ";");
        }
    }
    return Glib::ustring("");
}


/**
 * If str.endswith("!important") then assign stripped = str[:-10].rstrip() and return true.
 * Otherwise, leave stripped unmodified and return false.
 */
static bool strip_important(gchar const *str, std::string &stripped)
{
    assert(str != nullptr);

    constexpr size_t N = 10; // strlen("!important")
    auto pos = strlen(str);

    if (pos >= N && strncmp(str + pos - N, "!important", N) == 0) {
        pos -= N;

        // strip whitespace from the right
        while (pos > 0 && g_ascii_isspace(str[pos - 1])) {
            --pos;
        }

        stripped.assign(str, pos);
        return true;
    }

    return false;
}

void SPIBase::readIfUnset(gchar const *str, SPStyleSrc source)
{
    if (!str)
        return;

    bool has_important = false;
    std::string stripped;

    // '!important' is invalid on attributes
    if (source != SPStyleSrc::ATTRIBUTE) {
        has_important = strip_important(str, stripped);
        if (has_important) {
            str = stripped.c_str();
        }
    }

    if (!set || (has_important && !important)) {
        style_src = source;
        read(str);
        if (set) {
            if (has_important) {
                important = true;
            }
        }
    }
}


// SPIFloat -------------------------------------------------------------

void
SPIFloat::read( gchar const *str ) {

    if( !str ) return;

    if ( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else {
        gfloat value_tmp;
        if (sp_svg_number_read_f(str, &value_tmp)) {
            set = true;
            inherit = false;
            value = value_tmp;
        }
    }
}

const Glib::ustring SPIFloat::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    return Glib::ustring::format(this->value);
}

void
SPIFloat::cascade( const SPIBase* const parent ) {
    if( const SPIFloat* p = dynamic_cast<const SPIFloat*>(parent) ) {
        if( (inherits && !set) || inherit ) value = p->value;
    } else {
        std::cerr << "SPIFloat::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPIFloat::merge( const SPIBase* const parent ) {
    if( const SPIFloat* p = dynamic_cast<const SPIFloat*>(parent) ) {
        if( inherits ) {
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set     = p->set;
                inherit = p->inherit;
                value   = p->value;
            }
        }
    } else {
        std::cerr << "SPIFloat::merge(): Incorrect parent type" << std::endl;
    }
}

bool
SPIFloat::operator==(const SPIBase& rhs) const {
    if( const SPIFloat* r = dynamic_cast<const SPIFloat*>(&rhs) ) {
        return (value == r->value && SPIBase::operator==(rhs));
    } else {
        return false;
    }
}



// SPIScale24 -----------------------------------------------------------

void
SPIScale24::read( gchar const *str ) {

    if( !str ) return;

    if ( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else {
        gfloat value_in;
        if (sp_svg_number_read_f(str, &value_in)) {
            set = true;
            inherit = false;
            value_in = CLAMP(value_in, 0.0, 1.0);
            value = SP_SCALE24_FROM_FLOAT( value_in );
        }
    }
}

const Glib::ustring SPIScale24::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    return Glib::ustring::format(SP_SCALE24_TO_FLOAT(this->value));
}

void
SPIScale24::cascade( const SPIBase* const parent ) {
    if( const SPIScale24* p = dynamic_cast<const SPIScale24*>(parent) ) {
        if( (inherits && !set) || inherit ) value = p->value;
    } else {
        std::cerr << "SPIScale24::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPIScale24::merge( const SPIBase* const parent ) {
    if( const SPIScale24* p = dynamic_cast<const SPIScale24*>(parent) ) {
        if( inherits ) {
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set     = p->set;
                inherit = p->inherit;
                value   = p->value;
            }
        } else {
            // Needed only for 'opacity' and 'stop-opacity' which do not inherit. See comment at bottom of file.
            if (id() != SPAttr::OPACITY && id() != SPAttr::STOP_OPACITY)
                std::cerr << "SPIScale24::merge: unhandled property: " << name() << std::endl;
            if( !set || (!inherit && value == SP_SCALE24_MAX) ) {
                value = p->value;
                set = (value != SP_SCALE24_MAX);
            } else {
                if( inherit ) value = p->value; // Insures child is up-to-date
                value = SP_SCALE24_MUL( value, p->value );
                inherit = (inherit && p->inherit && (p->value == 0 || p->value == SP_SCALE24_MAX) );
                set = (inherit || value < SP_SCALE24_MAX);
            }
        }
    } else {
        std::cerr << "SPIScale24::merge(): Incorrect parent type" << std::endl;
    }
}

bool
SPIScale24::operator==(const SPIBase& rhs) const {
    if( const SPIScale24* r = dynamic_cast<const SPIScale24*>(&rhs) ) {
        return (value == r->value && SPIBase::operator==(rhs));
    } else {
        return false;
    }
}



// SPILength ------------------------------------------------------------

void
SPILength::read( gchar const *str ) {

    if( !str ) return;

    if (!strcmp(str, "inherit")) {
        set = true;
        inherit = true;
        unit = SP_CSS_UNIT_NONE;
        value = computed = 0.0;
    } else {
        gdouble value_tmp;
        gchar *e;
        /** \todo fixme: Move this to standard place (Lauris) */
        value_tmp = g_ascii_strtod(str, &e);
        if ( !std::isfinite(value_tmp) ) { // fix for bug lp:935157
            return;
        }
        if ((gchar const *) e != str) {

            value = value_tmp;
            if (!*e) {
                /* Userspace */
                unit = SP_CSS_UNIT_NONE;
                computed = value;
            } else if (!strcmp(e, "px")) {
                /* Userspace */
                unit = SP_CSS_UNIT_PX;
                computed = value;
            } else if (!strcmp(e, "pt")) {
                /* Userspace / DEVICESCALE */
                unit = SP_CSS_UNIT_PT;
                computed = Inkscape::Util::Quantity::convert(value, "pt", "px");
            } else if (!strcmp(e, "pc")) {
                unit = SP_CSS_UNIT_PC;
                computed = Inkscape::Util::Quantity::convert(value, "pc", "px");
            } else if (!strcmp(e, "mm")) {
                unit = SP_CSS_UNIT_MM;
                computed = Inkscape::Util::Quantity::convert(value, "mm", "px");
            } else if (!strcmp(e, "cm")) {
                unit = SP_CSS_UNIT_CM;
                computed = Inkscape::Util::Quantity::convert(value, "cm", "px");
            } else if (!strcmp(e, "in")) {
                unit = SP_CSS_UNIT_IN;
                computed = Inkscape::Util::Quantity::convert(value, "in", "px");
            } else if (!strcmp(e, "em")) {
                /* EM square */
                unit = SP_CSS_UNIT_EM;
                if( style ) {
                    computed = value * style->font_size.computed;
                } else {
                    computed = value * SPIFontSize::font_size_default;
                }
            } else if (!strcmp(e, "ex")) {
                /* ex square */
                unit = SP_CSS_UNIT_EX;
                if( style ) {
                    computed = value * style->font_size.computed * 0.5; // FIXME
                } else {
                    computed = value * SPIFontSize::font_size_default * 0.5;
                }
            } else if (!strcmp(e, "%")) {
                /* Percentage */
                unit = SP_CSS_UNIT_PERCENT;
                value = value * 0.01;
                if (id() == SPAttr::LINE_HEIGHT) {
                    // See: http://www.w3.org/TR/CSS2/visudet.html#propdef-line-height
                    if( style ) {
                        computed = value * style->font_size.computed;
                    } else {
                        computed = value * SPIFontSize::font_size_default;
                    }
                }
            } else {
                /* Invalid */
                return;
            }
            set = true;
            inherit = false;
        }
    }
}

const Glib::ustring SPILength::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    auto value = this->computed;
    auto unit_out = Glib::ustring("");
    switch (this->unit) {
        case SP_CSS_UNIT_NONE:
            break;
        case SP_CSS_UNIT_PX:
            unit_out = "px";
            break;
        case SP_CSS_UNIT_PT:
        case SP_CSS_UNIT_PC:
        case SP_CSS_UNIT_MM:
        case SP_CSS_UNIT_CM:
        case SP_CSS_UNIT_IN:
            unit_out = sp_style_get_css_unit_string(this->unit);
            value = Inkscape::Util::Quantity::convert(this->computed, "px", unit_out);
            break;
        case SP_CSS_UNIT_EM:
        case SP_CSS_UNIT_EX:
            unit_out = sp_style_get_css_unit_string(this->unit);
            value = this->value;
            break;
        case SP_CSS_UNIT_PERCENT:
            unit_out = "%";
            value = this->value * 100.0;
            break;
        default:
            /* Invalid */
            break;
    }
    return Glib::ustring::format(value) + unit_out;
}

void
SPILength::cascade( const SPIBase* const parent ) {
    if( const SPILength* p = dynamic_cast<const SPILength*>(parent) ) {
        if( (inherits && !set) || inherit ) {
            unit     = p->unit;
            value    = p->value;
            computed = p->computed;
        } else {
            // Recalculate based on new font-size, font-family inherited from parent
            double const em = style->font_size.computed;
            if (unit == SP_CSS_UNIT_EM) {
                computed = value * em;
            } else if (unit == SP_CSS_UNIT_EX) {
                // FIXME: Get x height from libnrtype or pango.
                computed = value * em * 0.5;
            } else if (unit == SP_CSS_UNIT_PERCENT && id() == SPAttr::LINE_HEIGHT) {
                // Special case
                computed = value * em;
            }
        }
    } else {
        std::cerr << "SPILength::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPILength::merge( const SPIBase* const parent ) {
    if( const SPILength* p = dynamic_cast<const SPILength*>(parent) ) {
        if( inherits ) {
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set      = p->set;
                inherit  = p->inherit;
                unit     = p->unit;
                value    = p->value;
                computed = p->computed;

                // Fix up so values are correct
                switch (p->unit) {
                    case SP_CSS_UNIT_EM:
                    case SP_CSS_UNIT_EX:
                        value *= p->style->font_size.computed / style->font_size.computed;
                        /** \todo
                         * FIXME: Have separate ex ratio parameter.
                         * Get x height from libnrtype or pango.
                         */
                        if (!std::isfinite(value)) {
                          value = computed;
                          unit = SP_CSS_UNIT_NONE;
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    } else {
        std::cerr << "SPIFloat::merge(): Incorrect parent type" << std::endl;
    }
}

void SPILength::setDouble(double v)
{
    unit = SP_CSS_UNIT_NONE;
    value = v;
    computed = v;
    value_default = v;
}

// Generate a string and allow emove name for parsing dasharray, etc.
const Glib::ustring SPILength::toString(bool wname) const
{
    CSSOStringStream os;
    if (wname) {
        os << name() << ":";
    }
    os << this->get_value();
    if (wname) {
        os << important_str();
        os << ";";
    }
    return os.str();
}

bool
SPILength::operator==(const SPIBase& rhs) const {
    if( const SPILength* r = dynamic_cast<const SPILength*>(&rhs) ) {

        if( unit != r->unit ) return false;

        // If length depends on external parameter, lengths cannot be equal.
        if (unit    == SP_CSS_UNIT_EM)      return false;
        if (unit    == SP_CSS_UNIT_EX)      return false;
        if (unit    == SP_CSS_UNIT_PERCENT) return false;
        if (r->unit == SP_CSS_UNIT_EM)      return false;
        if (r->unit == SP_CSS_UNIT_EX)      return false;
        if (r->unit == SP_CSS_UNIT_PERCENT) return false;

        return (computed == r->computed );
    } else {
        return false;
    }
}

// SPILengthOrNormal ----------------------------------------------------

void
SPILengthOrNormal::read( gchar const *str ) {

    if( !str ) return;

    if ( !strcmp(str, "normal") ) {
        set = true;
        inherit = false;
        unit = SP_CSS_UNIT_NONE;
        value = computed = 0.0;
        normal = true;
    } else {
        SPILength::read( str );
        normal = false;
    }
};

const Glib::ustring SPILengthOrNormal::get_value() const
{
    if (this->normal) return Glib::ustring("normal");
    return SPILength::get_value();
}

void
SPILengthOrNormal::cascade( const SPIBase* const parent ) {
    if( const SPILengthOrNormal* p = dynamic_cast<const SPILengthOrNormal*>(parent) ) {
        if( (inherits && !set) || inherit ) {
            normal   = p->normal;
        }
        SPILength::cascade( parent );
    } else {
        std::cerr << "SPILengthOrNormal::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPILengthOrNormal::merge( const SPIBase* const parent ) {
    if( const SPILengthOrNormal* p = dynamic_cast<const SPILengthOrNormal*>(parent) ) {
        if( inherits ) {
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                normal = p->normal;
                SPILength::merge( parent );
            }
        }
    }
}

bool
SPILengthOrNormal::operator==(const SPIBase& rhs) const {
    if( const SPILengthOrNormal* r = dynamic_cast<const SPILengthOrNormal*>(&rhs) ) {
        if( normal && r->normal ) { return true; }
        if( normal != r->normal ) { return false; }
        return SPILength::operator==(rhs);
    } else {
        return false;
    }
}


// SPIFontVariationSettings ----------------------------------------------------

void
SPIFontVariationSettings::read( gchar const *str ) {

    if( !str ) return;

    if ( !strcmp(str, "normal") ) {
        set = true;
        inherit = false;
        normal = true;
        axes.clear();
        return;
    }


    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", str);

    // Match a pattern of a CSS <string> of length 4, whitespace, CSS <number>.
    // (CSS string is quoted with double quotes).

    // Matching must use a Glib::ustring or matching may produce
    // subtle errors which may be shown by an "Invalid byte sequence
    // in conversion input" error.
    Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create(
            "[\"'](\\w{4})[\"']"
            "\\s+([-+]?\\d*\\.?\\d+([eE][-+]?\\d+)?)");
    Glib::MatchInfo matchInfo;

    for (auto token: tokens) {
        regex->match(token, matchInfo);
        if (matchInfo.matches()) {
            float value = std::stod(matchInfo.fetch(2));
            axes.insert(std::pair<Glib::ustring,float>(matchInfo.fetch(1), value));
        }
    }

    if (!axes.empty()) {
        set = true;
        inherit = false;
        normal = false;
    }
};

const Glib::ustring SPIFontVariationSettings::get_value() const
{
    if (this->normal) return Glib::ustring("normal");
    auto ret = Glib::ustring("");
    for(auto it: axes) {
        ret += "'" + it.first + "' " + Glib::ustring::format(it.second) + ", ";
    }
    if (!ret.empty()) ret.erase(ret.size() - 2);
    return ret;
}

void
SPIFontVariationSettings::cascade( const SPIBase* const parent ) {
    if( const SPIFontVariationSettings* p = dynamic_cast<const SPIFontVariationSettings*>(parent) ) {
        if( !set || inherit ) {  // Always inherits
            normal   = p->normal;
            axes.clear();
            axes     = p->axes;
        }
    } else {
        std::cerr << "SPIFontVariationSettings::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPIFontVariationSettings::merge( const SPIBase* const parent ) {
    if( const SPIFontVariationSettings* p = dynamic_cast<const SPIFontVariationSettings*>(parent) ) {
        // if( inherits ) {  'font-variation-settings' always inherits.
        if( (!set || inherit) && p->set && !(p->inherit) ) {
            set     = p->set;
            inherit = p->inherit;
            normal  = p->normal;
            axes    = p->axes;
        }
    }
}

bool
SPIFontVariationSettings::operator==(const SPIBase& rhs) const {
    if( const SPIFontVariationSettings* r = dynamic_cast<const SPIFontVariationSettings*>(&rhs) ) {
        if( normal && r->normal ) { return true; }
        if( normal != r->normal ) { return false; }
        return axes == r->axes;
    } else {
        return false;
    }
}

// Generate a string useful for passing to Pango, etc.
const Glib::ustring
SPIFontVariationSettings::toString() const {

    Inkscape::CSSOStringStream os;
    for (const auto & axe : axes){
        os << axe.first << "=" << axe.second << ",";
    }

    std::string string = os.str(); // Glib::ustring doesn't have pop_back()
    if (!string.empty()) {
        string.pop_back(); // Delete extra comma at end
    }

    return string;
}

// Helpers for SPIEnum -----------------------------------------------------

// The default exists to satisfy linking of derived classes but must never be called
template <typename T> static SPStyleEnum const *get_enums() { g_assert_not_reached(); return nullptr; }

template <> SPStyleEnum const *get_enums<SPBlendMode>() { return enum_blend_mode; }
template <> SPStyleEnum const *get_enums<SPColorInterpolation>() { return enum_color_interpolation; }
template <> SPStyleEnum const *get_enums<SPColorRendering>() { return enum_color_rendering; }
template <> SPStyleEnum const *get_enums<SPCSSBaseline>() { return enum_baseline; }
template <> SPStyleEnum const *get_enums<SPCSSDirection>() { return enum_direction; }
template <> SPStyleEnum const *get_enums<SPCSSDisplay>() { return enum_display; }
template <> SPStyleEnum const *get_enums<SPCSSFontVariantAlternates>() { return enum_font_variant_alternates; }
template <> SPStyleEnum const *get_enums<SPCSSTextAlign>() { return enum_text_align; }
template <> SPStyleEnum const *get_enums<SPCSSTextOrientation>() { return enum_text_orientation; }
template <> SPStyleEnum const *get_enums<SPCSSTextTransform>() { return enum_text_transform; }
template <> SPStyleEnum const *get_enums<SPCSSWritingMode>() { return enum_writing_mode; }
template <> SPStyleEnum const *get_enums<SPEnableBackground>() { return enum_enable_background; }
template <> SPStyleEnum const *get_enums<SPImageRendering>() { return enum_image_rendering; }
template <> SPStyleEnum const *get_enums<SPIsolation>() { return enum_isolation; }
template <> SPStyleEnum const *get_enums<SPOverflow>() { return enum_overflow; }
template <> SPStyleEnum const *get_enums<SPShapeRendering>() { return enum_shape_rendering; }
template <> SPStyleEnum const *get_enums<SPStrokeCapType>() { return enum_stroke_linecap; }
template <> SPStyleEnum const *get_enums<SPStrokeJoinType>() { return enum_stroke_linejoin; }
template <> SPStyleEnum const *get_enums<SPTextAnchor>() { return enum_text_anchor; }
template <> SPStyleEnum const *get_enums<SPTextRendering>() { return enum_text_rendering; }
template <> SPStyleEnum const *get_enums<SPVisibility>() { return enum_visibility; }
template <> SPStyleEnum const *get_enums<SPWhiteSpace>() { return enum_white_space; }
template <> SPStyleEnum const *get_enums<SPWindRule>() { return enum_clip_rule; }
template <> SPStyleEnum const *get_enums<SPCSSFontStyle>() { return enum_font_style; }
template <> SPStyleEnum const *get_enums<SPCSSFontVariant>() { return enum_font_variant; }
template <> SPStyleEnum const *get_enums<SPCSSFontWeight>() { return enum_font_weight; }
template <> SPStyleEnum const *get_enums<SPCSSFontStretch>() { return enum_font_stretch; }
template <> SPStyleEnum const *get_enums<SPCSSFontVariantPosition>() { return enum_font_variant_position; }
template <> SPStyleEnum const *get_enums<SPCSSFontVariantCaps>() { return enum_font_variant_caps; }

// SPIEnum --------------------------------------------------------------

template <typename T>
void SPIEnum<T>::update_computed()
{
    computed = value;
}

template <>
void SPIEnum<SPCSSFontWeight>::update_computed()
{
    // The following is defined in CSS 2.1
    if (value == SP_CSS_FONT_WEIGHT_NORMAL) {
        computed = SP_CSS_FONT_WEIGHT_400;
    } else if (value == SP_CSS_FONT_WEIGHT_BOLD) {
        computed = SP_CSS_FONT_WEIGHT_700;
    } else {
        computed = value;
    }
}

template <typename T>
void SPIEnum<T>::read(gchar const *str)
{

    if( !str ) return;

    if( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else {
        auto const *enums = get_enums<T>();
        for (unsigned i = 0; enums[i].key; i++) {
            if (!strcmp(str, enums[i].key)) {
                set = true;
                inherit = false;
                value = static_cast<T>(enums[i].value);
                /* Save copying for values not needing it */
                break;
            }
        }

        // type-specialized subroutine
        update_computed();
    }
}

template <typename T>
const Glib::ustring SPIEnum<T>::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    auto const *enums = get_enums<T>();
    for (unsigned i = 0; enums[i].key; ++i) {
        if (enums[i].value == static_cast< gint > (this->value) ) {
            return Glib::ustring(enums[i].key);
        }
    }
    return Glib::ustring("");
}

template <>
void SPIEnum<SPCSSFontWeight>::update_computed_cascade(SPCSSFontWeight const &p_computed)
{
    // strictly, 'bolder' and 'lighter' should go to the next weight
    // expressible in the current font family, but that's difficult to
    // find out, so jumping by 3 seems an appropriate approximation
    if (value == SP_CSS_FONT_WEIGHT_LIGHTER) {
        computed = static_cast<SPCSSFontWeight>(std::max<int>(SP_CSS_FONT_WEIGHT_100, int(p_computed) - 3));
    } else if (value == SP_CSS_FONT_WEIGHT_BOLDER) {
        computed = static_cast<SPCSSFontWeight>(std::min<int>(SP_CSS_FONT_WEIGHT_900, p_computed + 3));
    }
}

template <>
void SPIEnum<SPCSSFontStretch>::update_computed_cascade(SPCSSFontStretch const &p_computed)
{
    if (value == SP_CSS_FONT_STRETCH_NARROWER) {
        computed =
            static_cast<SPCSSFontStretch>(std::max<int>(SP_CSS_FONT_STRETCH_ULTRA_CONDENSED, int(p_computed) - 1));
    } else if (value == SP_CSS_FONT_STRETCH_WIDER) {
        computed = static_cast<SPCSSFontStretch>(std::min<int>(SP_CSS_FONT_STRETCH_ULTRA_EXPANDED, p_computed + 1));
    }
}

template <typename T>
void SPIEnum<T>::cascade(const SPIBase *const parent)
{
    if (const auto *p = dynamic_cast<const SPIEnum<T> *>(parent)) {
        if( inherits && (!set || inherit) ) {
            computed = p->computed;
        } else {
            // type-specialized subroutine
            update_computed_cascade(p->computed);
        }
    } else {
        std::cerr << "SPIEnum<T>::cascade(): Incorrect parent type" << std::endl;
    }
}

// FIXME Handle font_stretch and font_weight (relative values) New derived class?
template <typename T>
void SPIEnum<T>::update_value_merge(SPIEnum<T> const &p, T smaller, T larger)
{
    g_assert(set);

    if (value == p.value) {
        // Leave as is, what does applying "wider" twice do?
    } else if ((value == smaller && p.value == larger) || //
               (value == larger && p.value == smaller)) {
        // Values cancel, unset
        set = false;
    } else if (value == smaller || value == larger) {
        value = computed;
        inherit = false;
    }
}

template <>
void SPIEnum<SPCSSFontWeight>::update_value_merge(SPIEnum<SPCSSFontWeight> const &p)
{
    update_value_merge(p, SP_CSS_FONT_WEIGHT_LIGHTER, SP_CSS_FONT_WEIGHT_BOLDER);
}

template <>
void SPIEnum<SPCSSFontStretch>::update_value_merge(SPIEnum<SPCSSFontStretch> const &p)
{
    update_value_merge(p, SP_CSS_FONT_STRETCH_NARROWER, SP_CSS_FONT_STRETCH_WIDER);
}

template <typename T>
void SPIEnum<T>::merge(const SPIBase *const parent)
{
    if (const auto *p = dynamic_cast<const SPIEnum<T> *>(parent)) {
        if( inherits ) {
            if( p->set && !p->inherit ) {
                if( !set || inherit ) {
                    set      = p->set;
                    inherit  = p->inherit;
                    value    = p->value;
                    computed = p->computed; // Different from value for font-weight and font-stretch
                } else {
                    // type-specialized subroutine
                    update_value_merge(*p);
                }
            }
        }
    }
}

template <typename T>
bool SPIEnum<T>::operator==(const SPIBase &rhs) const
{
    if (auto *r = dynamic_cast<const SPIEnum<T> *>(&rhs)) {
        return (computed == r->computed && SPIBase::operator==(rhs));
    } else {
        return false;
    }
}


#if 0
// SPIEnumBits ----------------------------------------------------------
// Used for 'font-variant-xxx'
void
SPIEnumBits::read( gchar const *str ) {

    if( !str ) return;
    if( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else {
        for (unsigned i = 0; enums[i].key; i++) {
            if (!strcmp(str, enums[i].key)) {
                set = true;
                inherit = false;
                value |= enums[i].value;
            }
        }
        /* Save copying for values not needing it */
        computed = value;
    }
}

const Glib::ustring SPIEnumBits::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->value == 0) return Glib::ustring("normal");
    auto ret = Glib::ustring("");
    for (unsigned i = 0; enums[i].key; ++i) {
        if (this->value & enums[i].value) {
            if (!ret.empty()) ret += " ";
            ret += enums[i].key;
        }
    }
    return ret;
}
#endif

// SPILigatures -----------------------------------------------------
// Used for 'font-variant-ligatures'
void
SPILigatures::read( gchar const *str ) {

    if( !str ) return;

    value = SP_CSS_FONT_VARIANT_LIGATURES_NORMAL;
    if( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else if (!strcmp(str, "normal" )) {
        // Defaults for TrueType
        inherit = false;
        set = true;
    } else if (!strcmp(str, "none" )) {
        value = SP_CSS_FONT_VARIANT_LIGATURES_NONE;
        inherit = false;
        set = true;
    } else {
        // We need to parse in order
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s+", str );
        auto const *enums = enum_font_variant_ligatures;
        for(auto & token : tokens) {
            for (unsigned j = 0; enums[j].key; ++j ) {
                if (token.compare( enums[j].key ) == 0 ) {
                    set = true;
                    inherit = false;
                    if( enums[j].value < SP_CSS_FONT_VARIANT_LIGATURES_NOCOMMON ) {
                        // Turn on
                        value |= enums[j].value;
                    } else {
                        // Turn off
                        value &= ~(enums[j].value >> 4);
                    }
                }
            }
        }
    }
    computed = value;
}

/* FIXME:: This whole class is bogus and should be an SPIBitEnum TODO */

const Glib::ustring SPILigatures::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->value == SP_CSS_FONT_VARIANT_LIGATURES_NONE) return Glib::ustring("none");
    if (this->value == SP_CSS_FONT_VARIANT_LIGATURES_NORMAL) return Glib::ustring("normal");
    auto ret = Glib::ustring("");
    if (!(value & SP_CSS_FONT_VARIANT_LIGATURES_COMMON))
        ret += "no-common-ligatures ";
    if (value & SP_CSS_FONT_VARIANT_LIGATURES_DISCRETIONARY)
        ret += "discretionary-ligatures ";
    if (value & SP_CSS_FONT_VARIANT_LIGATURES_HISTORICAL )
        ret += "historical-ligatures ";
    if ( !(value & SP_CSS_FONT_VARIANT_LIGATURES_CONTEXTUAL) )
        ret += "no-contextual ";
    ret.erase(ret.size() - 1);
    return ret;
}

// SPINumeric -----------------------------------------------------
// Used for 'font-variant-numeric'
void
SPINumeric::read( gchar const *str ) {

    if( !str ) return;

    value = SP_CSS_FONT_VARIANT_NUMERIC_NORMAL;
    if( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else if (!strcmp(str, "normal" )) {
        // Defaults for TrueType
        inherit = false;
        set = true;
    } else {
        // We need to parse in order
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s+", str );
        auto const *enums = enum_font_variant_numeric;
        for(auto & token : tokens) {
            for (unsigned j = 0; enums[j].key; ++j ) {
                if (token.compare( enums[j].key ) == 0 ) {
                    set = true;
                    inherit = false;
                    value |=  enums[j].value;

                    // Must switch off incompatible value
                    switch (enums[j].value ) {
                        case SP_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS:
                            value &= ~SP_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS;
                            break;
                        case SP_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS:
                            value &= ~SP_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS;
                            break;

                        case SP_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS:
                            value &= ~SP_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS;
                            break;
                        case SP_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS:
                            value &= ~SP_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS;
                            break;

                        case SP_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS:
                            value &= ~SP_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS;
                            break;
                        case SP_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS:
                            value &= ~SP_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS;
                            break;

                        case SP_CSS_FONT_VARIANT_NUMERIC_NORMAL:
                        case SP_CSS_FONT_VARIANT_NUMERIC_ORDINAL:
                        case SP_CSS_FONT_VARIANT_NUMERIC_SLASHED_ZERO:
                            // Do nothing
                            break;

                        default:
                            std::cerr << "SPINumeric::read(): Invalid value." << std::endl;
                            break;
                    }
                }
            }
        }
    }
    computed = value;
}

/* FIXME:: This whole class is bogus and should be an SPIBitEnum TODO */
const Glib::ustring SPINumeric::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->value == 0) return Glib::ustring("normal");
    auto ret = Glib::ustring("");
    auto enums = enum_font_variant_numeric;
    for (unsigned i = 1; enums[i].key; ++i) {
        // Bitmap is shifted by 1 because normal is zero
        if (this->value & (1 << (i - 1))) {
            if (!ret.empty()) ret += " ";
            ret += enums[i].key;
        }
    }
    return ret;
}

// SPIEastAsian ---------------------------------------------------
// Used for 'font-variant-east-asian'
void
SPIEastAsian::read( gchar const *str ) {

    if( !str ) return;

    value = SP_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL;
    if( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else if (!strcmp(str, "normal" )) {
        // Defaults for TrueType
        inherit = false;
        set = true;
    } else {
        // We need to parse in order
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s+", str );
        auto const *enums = enum_font_variant_east_asian;
        for(auto & token : tokens) {
            for (unsigned j = 0; enums[j].key; ++j ) {
                if (token.compare( enums[j].key ) == 0 ) {
                    set = true;
                    inherit = false;

                    // Must switch off incompatible value (turn on correct one below)
                    switch (enums[j].value ) {
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS78:
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS83:
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS90:
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS04:
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED:
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL:
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS78;
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS83;
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS90;
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS04;
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED;
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL;
                            break;

                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH:
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL_WIDTH;
                            break;
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL_WIDTH:
                            value &= ~SP_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH;
                            break;

                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL:
                        case SP_CSS_FONT_VARIANT_EAST_ASIAN_RUBY:
                            // Do nothing
                            break;

                        default:
                            std::cerr << "SPIEastasian::read(): Invalid value." << std::endl;
                            break;
                    }

                    value |=  enums[j].value;
                }
            }
        }
    }
    computed = value;
}

const Glib::ustring SPIEastAsian::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->value == 0) return Glib::ustring("normal");
    auto ret = Glib::ustring("");
    auto enums = enum_font_variant_east_asian;
    for (unsigned i = 0; enums[i].key; ++i) {
        if (this->value & (1 << i)) {
            if (!ret.empty()) ret += " ";
            ret += enums[i].key;
        }
    }
    return ret;
}

// SPIString ------------------------------------------------------------

void
SPIString::read( gchar const *str ) {

    if( !str ) return;

    clear();

    if (style_src == SPStyleSrc::ATTRIBUTE && id() == SPAttr::D) {
        return;
    }

    if (!strcmp(str, "inherit")) {
        set = true;
        inherit = true;
    } else if (!g_strcmp0(str, get_default_value())) {
        // no need to copy string
        set = true;
    } else {
        Glib::ustring str_temp;

        if (id() == SPAttr::FONT_FAMILY) {
            // Family names may be quoted in CSS, internally we use unquoted names.
            str_temp = str;
            css_font_family_unquote( str_temp );
            str = str_temp.c_str();
        } else if (id() == SPAttr::INKSCAPE_FONT_SPEC) {
            str_temp = str;
            css_unquote( str_temp );
            str = str_temp.c_str();
        }

        set = true;
        _value = g_strdup(str);
    }
}


/**
 * Value as it should be written to CSS representation, including quotes if needed.
 */
const Glib::ustring SPIString::get_value() const
{
    Glib::ustring val;

    if (set && inherit) {
        val = "inherit";
    } else if (auto *v = value()) {
        val = v;

        if (id() == SPAttr::FONT_FAMILY) {
            css_font_family_quote(val);
        } else if (id() == SPAttr::INKSCAPE_FONT_SPEC) {
            css_quote(val);
        }
    }

    return val;
}

char const *SPIString::value() const
{
    return _value ? _value : get_default_value();
}

char const *SPIString::get_default_value() const
{
    switch (id()) {
        case SPAttr::FONT_FAMILY:
            return "sans-serif";
        case SPAttr::FONT_FEATURE_SETTINGS:
            return "normal";
        default:
            return nullptr;
    }
}


void
SPIString::clear() {
    SPIBase::clear();
    g_free(_value);
    _value = nullptr;
}

void
SPIString::cascade( const SPIBase* const parent ) {
    if( const SPIString* p = dynamic_cast<const SPIString*>(parent) ) {
        if( inherits && (!set || inherit) ) {
            g_free(_value);
            _value = g_strdup(p->_value);
        }
    } else {
        std::cerr << "SPIString::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPIString::merge( const SPIBase* const parent ) {
    if( const SPIString* p = dynamic_cast<const SPIString*>(parent) ) {
        if( inherits ) {
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set     = p->set;
                inherit = p->inherit;
                g_free(_value);
                _value = g_strdup(p->_value);
            }
        }
    }
}

bool
SPIString::operator==(const SPIBase& rhs) const {
    if( const SPIString* r = dynamic_cast<const SPIString*>(&rhs) ) {
        return g_strcmp0(_value, r->_value) == 0 && SPIBase::operator==(rhs);
    } else {
        return false;
    }
}


// SPIShapes ------------------------------------------------------------

SPIShapes::~SPIShapes() {
    hrefs_clear();
}

SPIShapes::SPIShapes()
    : SPIString(false)
{
}


//SPIShapes::~SPIShapes() {
//    clear(); // Will segfault if called here. Seems to be already cleared.
//}


// Used to add/remove listeners for text wrapped in shapes.
// Note: this is done differently than for patterns, etc. where presentation attributes can be used.
// 'shape-inside' and 'shape-subtract' are only properties.
void
SPIShapes::read( gchar const *str) {

    if (!style) {
        std::cerr << "SPIShapes::read: no style!" << std::endl;
        return;
    }

    if( !str ) return;

    SPIString::read(str);
    assert(set);

    // The object/repr this property is connected to..
    SPObject* object = style->object;
    if (!object) {
        std::cout << "  No object" << std::endl;
        return;
    }

    // clear(); // Already cleared! (In SPStyle::read.) Calling again causes segfault.

    // Add new listeners
    std::vector<Glib::ustring> shapes_url = Glib::Regex::split_simple(" ", str);
    for (auto shape_url : shapes_url) {
        if ( shape_url.compare(0,5,"url(#") != 0 || shape_url.compare(shape_url.size()-1,1,")") != 0 ){
            std::cerr << "SPIShapes::read: Invalid shape value: " << shape_url << std::endl;
        } else {
            auto uri = extract_uri(shape_url.c_str()); // Do before we erase "url(#"

            // This ups the href count of the shape. This is required so that vacuuming a
            // document does not delete shapes stored in <defs>.
            SPShapeReference *href = new SPShapeReference(object);

            if (href->try_attach(uri.c_str())) {
                hrefs.emplace_back(href);
            } else {
                delete href;
            }
        }
    }
}

void
SPIShapes::clear() {

    SPIBase::clear();

    hrefs_clear();
}

void SPIShapes::hrefs_clear()
{
    for (auto href : hrefs) {
        delete href;
    }
    hrefs.clear();
}

// SPIColor -------------------------------------------------------------

// Used for 'color', 'text-decoration-color', 'flood-color', 'lighting-color', and 'stop-color'.
// (The last three have yet to be implemented.)
// CSS3: 'currentcolor' is allowed value and is equal to inherit for the 'color' property.
// FIXME: We should preserve named colors, hsl colors, etc.
void SPIColor::read( gchar const *str ) {

    if( !str ) return;

    set = false;
    inherit = false;
    currentcolor = false;
    if ( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else if ( !strcmp(str, "currentColor") ) {
        set = true;
        currentcolor = true;
        if (id() == SPAttr::COLOR) {
            inherit = true;  // CSS3
        } else {
            setColor( style->color.value.color );
        }
    } else {
        guint32 const rgb0 = sp_svg_read_color(str, 0xff);
        if (rgb0 != 0xff) {
            setColor(rgb0);
            set = true;
        }
    }
}

const Glib::ustring SPIColor::get_value() const
{
    // currentcolor goes first to handle special case for 'color' property
    if (this->currentcolor) return Glib::ustring("currentColor");
    if (this->inherit) return Glib::ustring("inherit");
    return this->value.color.toString();
}

void
SPIColor::cascade( const SPIBase* const parent ) {
    if( const SPIColor* p = dynamic_cast<const SPIColor*>(parent) ) {
        if( (inherits && !set) || inherit) { // FIXME verify for 'color'
            if( !(inherit && currentcolor) ) currentcolor = p->currentcolor;
            setColor( p->value.color );
        } else {
            // Add CSS4 Color: Lighter, Darker
        }
    } else {
        std::cerr << "SPIColor::cascade(): Incorrect parent type" << std::endl;
    }

}

void
SPIColor::merge( const SPIBase* const parent ) {
    if( const SPIColor* p = dynamic_cast<const SPIColor*>(parent) ) {
        if( inherits ) {
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set           = p->set;
                inherit       = p->inherit;
                currentcolor  = p->currentcolor;
                value.color   = p->value.color;
            }
        }
    }
}

bool
SPIColor::operator==(const SPIBase& rhs) const {
    if( const SPIColor* r = dynamic_cast<const SPIColor*>(&rhs) ) {

        if ( (this->currentcolor    != r->currentcolor    ) ||
             (this->value.color     != r->value.color     ) ||
             (this->value.color.icc != r->value.color.icc ) ||
             (this->value.color.icc && r->value.color.icc &&
              this->value.color.icc->colorProfile != r->value.color.icc->colorProfile &&
              this->value.color.icc->colors       != r->value.color.icc->colors ) ) {
            return false;
        }

        return SPIBase::operator==(rhs);

    } else {
        return false;
    }
}



// SPIPaint -------------------------------------------------------------

// Paint is used for 'fill' and 'stroke'. SPIPaint perhaps should be derived from SPIColor.
// 'style' is set in SPStyle::SPStyle or in the legacy SPIPaint::read( gchar, style, document )
// It is needed for computed value when value is 'currentColor'. It is also needed to
// find the object for creating an href (this is done through document but should be done
// directly so document not needed.. FIXME).

SPIPaint::~SPIPaint() {
    if( value.href ) {
        clear();
        delete value.href;
        value.href = nullptr;
    }
}

/**
 * Set SPIPaint object from string.
 *
 * \pre paint == \&style.fill || paint == \&style.stroke.
 */
void
SPIPaint::read( gchar const *str ) {

    // std::cout << "SPIPaint::read: Entrance: " <<  "  |" << (str?str:"null") << "|" << std::endl;
    // if( style ) {
    //     std::cout << "   document: " << (void*)style->document << std::endl;
    //     std::cout << "     object: " << (style->object?"present":"null") << std::endl;
    //     if( style->object )
    //         std::cout << "       : " << (style->object->getId()?style->object->getId():"no ID")
    //                   << " document: " << (style->object->document?"yes":"no") << std::endl;
    // }

    if(!str ) return;

    reset( false ); // Do not init

    // Is this necessary?
    while (g_ascii_isspace(*str)) {
        ++str;
    }

    if (streq(str, "inherit")) {
        set = true;
        inherit = true;
    } else {
        // Read any URL first. The other values can be stand-alone or backup to the URL.

        if ( strneq(str, "url", 3) ) {

            // FIXME: THE FOLLOWING CODE SHOULD BE PUT IN A PRIVATE FUNCTION FOR REUSE
            auto uri = extract_uri(str, &str); // std::string
            if(uri.empty()) {
                std::cerr << "SPIPaint::read: url is empty or invalid" << std::endl;
            } else if (!style ) {
                std::cerr << "SPIPaint::read: url with empty SPStyle pointer" << std::endl;
            } else {
                set = true;
                SPDocument *document = (style->object) ? style->object->document : nullptr;

                // Create href if not done already
                if (!value.href) {

                    if (style->object) {
                        value.href = new SPPaintServerReference(style->object);
                    } else if (document) {
                        value.href = new SPPaintServerReference(document);
                    } else {
                        std::cerr << "SPIPaint::read: No valid object or document!" << std::endl;
                        return;
                    }

                    if (this == &style->fill) {
                        style->fill_ps_changed_connection = value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_fill_paint_server_ref_changed), style));
                    } else {
                        style->stroke_ps_changed_connection = value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_stroke_paint_server_ref_changed), style));
                    }
                }

                // TODO check what this does in light of move away from union
                sp_style_set_ipaint_to_uri_string(style, this, uri.c_str());
            }
        }

        while ( g_ascii_isspace(*str) ) {
            ++str;
        }

        if (streq(str, "currentColor")) {
            set = true;
            paintOrigin = SP_CSS_PAINT_ORIGIN_CURRENT_COLOR;
            if (style) {
                setColor( style->color.value.color );
            } else {
                // Normally an SPIPaint is part of an SPStyle and the value of 'color' is
                // available.  SPIPaint can be used 'stand-alone' (e.g. to parse color values) in
                // which case a value of 'currentColor' is meaningless, thus we shouldn't reach
                // here.
                std::cerr << "SPIPaint::read(): value is 'currentColor' but 'color' not available." << std::endl;
                setColor( 0 );
            }
        } else if (streq(str, "context-fill")) {
            set = true;
            paintOrigin = SP_CSS_PAINT_ORIGIN_CONTEXT_FILL;
        } else if (streq(str, "context-stroke")) {
            set = true;
            paintOrigin = SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE;
        } else if (streq(str, "none")) {
            set = true;
            noneSet = true;
        } else {
            guint32 const rgb0 = sp_svg_read_color(str, &str, 0xff);
            if (rgb0 != 0xff) {
                setColor( rgb0 );
                set = true;

                while (g_ascii_isspace(*str)) {
                    ++str;
                }
                if (strneq(str, "icc-color(", 10)) {
                    SVGICCColor* tmp = new SVGICCColor();
                    if ( ! sp_svg_read_icc_color( str, &str, tmp ) ) {
                        delete tmp;
                        tmp = nullptr;
                    }
                    value.color.icc = tmp;
                }
            }
        }
    }
}

// Stand-alone read (Legacy read()), used multiple places, e.g. sp-stop.cpp
// This function should not be necessary. FIXME
void
SPIPaint::read( gchar const *str, SPStyle &style_in, SPDocument *document_in ) {
    style = &style_in;
    style->document = document_in;
    read( str );
}

const Glib::ustring SPIPaint::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->noneSet) return Glib::ustring("none");
    // url must go first as other values can serve as fallbacks
    auto ret = Glib::ustring("");
    if (this->value.href && this->value.href->getURI()) {
        ret += this->value.href->getURI()->cssStr();
    }
    switch(this->paintOrigin) {
        case SP_CSS_PAINT_ORIGIN_CURRENT_COLOR:
            if (!ret.empty()) ret += " ";
            ret += "currentColor";
            break;
        case SP_CSS_PAINT_ORIGIN_CONTEXT_FILL:
            if (!ret.empty()) ret += " ";
            ret += "context-fill";
            break;
        case SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE:
            if (!ret.empty()) ret += " ";
            ret += "context-stroke";
            break;
        case SP_CSS_PAINT_ORIGIN_NORMAL:
            if (this->colorSet) {
                char color_buf[8];
                sp_svg_write_color(color_buf, sizeof(color_buf), this->value.color.toRGBA32(0));
                if (!ret.empty()) ret += " ";
                ret += color_buf;
            }
            if (this->value.color.icc) {
                ret += " icc-color(";
                ret += this->value.color.icc->colorProfile;
                for(auto i: this->value.color.icc->colors) {
                    ret += ", " + Glib::ustring::format(i);
                }
                ret += ")";
            }
            break;
    }
    return ret;
}

void
SPIPaint::clear() {
    // std::cout << "SPIPaint::clear(): " << name << std::endl;
    reset( true ); // Reset and Init
}

void
SPIPaint::reset( bool init ) {

    // std::cout << "SPIPaint::reset(): " << name << " " << init << std::endl;
    SPIBase::clear();
    paintOrigin = SP_CSS_PAINT_ORIGIN_NORMAL;
    colorSet = false;
    noneSet = false;
    value.color.set( false );
    if (value.href){
        if (value.href->getObject()) {
            value.href->detach();
        }
    }
    if( init ) {
        if (id() == SPAttr::FILL) {
            // 'black' is default for 'fill'
            setColor(0.0, 0.0, 0.0);
        } else if (id() == SPAttr::TEXT_DECORATION_COLOR) {
            // currentcolor = true;
        }
    }
}

void
SPIPaint::cascade( const SPIBase* const parent ) {

    // std::cout << "SPIPaint::cascade" << std::endl;
    if( const SPIPaint* p = dynamic_cast<const SPIPaint*>(parent) ) {
        if (!set || inherit) {  // Always inherits

            reset( false ); // Do not init

            if( p->isPaintserver() ) {
                if( p->value.href) {
                    // Why can we use p->document ?
                    sp_style_set_ipaint_to_uri( style, this, p->value.href->getURI(), p->value.href->getOwnerDocument());
                } else {
                    std::cerr << "SPIPaint::cascade: Expected paint server not found." << std::endl;
                }
            } else if( p->isColor() ) {
                setColor( p->value.color );
            } else if( p->isNoneSet() ) {
                noneSet = true;
            } else if( p->paintOrigin == SP_CSS_PAINT_ORIGIN_CURRENT_COLOR ) {
                paintOrigin = SP_CSS_PAINT_ORIGIN_CURRENT_COLOR;
                setColor( style->color.value.color );
            } else if( isNone() ) {
                //
            } else {
                g_assert_not_reached();
            }
        } else {
            if( paintOrigin == SP_CSS_PAINT_ORIGIN_CURRENT_COLOR ) {
                // Update in case color value changed.
                setColor( style->color.value.color );
            }
        }

    } else {
        std::cerr << "SPIPaint::cascade(): Incorrect parent type" << std::endl;
    }

}

void
SPIPaint::merge( const SPIBase* const parent ) {
    if( const SPIPaint* p = dynamic_cast<const SPIPaint*>(parent) ) {
        //    if( inherits ) {  Paint always inherits
        if( (!set || inherit) && p->set && !(p->inherit) ) {
            this->cascade( parent );  // Must call before setting 'set'
            set     = p->set;
            inherit = p->inherit;
        }
    }
}

bool
SPIPaint::operator==(const SPIBase& rhs) const {

    if( const SPIPaint* r = dynamic_cast<const SPIPaint*>(&rhs) ) {

        if ( (this->isColor()       != r->isColor()       )  ||
             (this->isPaintserver() != r->isPaintserver() )  ||
             (this->paintOrigin     != r->paintOrigin     ) ) {
            return false;
        }

        if ( this->isPaintserver() ) {
            if( this->value.href == nullptr || r->value.href == nullptr ||
                this->value.href->getObject() != r->value.href->getObject() ) {
                return false;
            }
        }

        if ( this->isColor() ) {
            if ( (this->value.color     != r->value.color     ) ||
                 (this->value.color.icc != r->value.color.icc ) ||
                 (this->value.color.icc && r->value.color.icc &&
                  this->value.color.icc->colorProfile != r->value.color.icc->colorProfile &&
                  this->value.color.icc->colors       != r->value.color.icc->colors ) ) {
                return false;
            }
        }

        return SPIBase::operator==(rhs);

    } else {
        return false;
    }
}



// SPIPaintOrder --------------------------------------------------------

void
SPIPaintOrder::read( gchar const *str ) {

    if( !str ) return;

    g_free(value);
    set = false;
    inherit = false;

    if (!strcmp(str, "inherit")) {
        set = true;
        inherit = true;
    } else {
        set = true;
        value = g_strdup(str);

        if (!strcmp(value, "normal")) {
            layer[0] = SP_CSS_PAINT_ORDER_NORMAL;
            layer_set[0] = true;
        } else {
            // This certainly can be done more efficiently
            gchar** c = g_strsplit(value, " ", PAINT_ORDER_LAYERS + 1);
            bool used[3] = {false, false, false};
            unsigned int i = 0;
            for( ; i < PAINT_ORDER_LAYERS; ++i ) {
                if( c[i] ) {
                    layer_set[i] = false;
                    if( !strcmp( c[i], "fill")) {
                        layer[i] = SP_CSS_PAINT_ORDER_FILL;
                        layer_set[i] = true;
                        used[0] = true;
                    } else if( !strcmp( c[i], "stroke")) {
                        layer[i] = SP_CSS_PAINT_ORDER_STROKE;
                        layer_set[i] = true;
                        used[1] = true;
                    } else if( !strcmp( c[i], "markers")) {
                        layer[i] = SP_CSS_PAINT_ORDER_MARKER;
                        layer_set[i] = true;
                        used[2] = true;
                    } else {
                        std::cerr << "sp_style_read_ipaintorder: illegal value: " << c[i] << std::endl;
                        break;
                    }
                } else {
                    break;
                }
            }
            g_strfreev(c);

            // Fill out rest of the layers using the default order
            if( !used[0] && i < PAINT_ORDER_LAYERS ) {
                layer[i] = SP_CSS_PAINT_ORDER_FILL;
                layer_set[i] = false;
                ++i;
            }
            if( !used[1] && i < PAINT_ORDER_LAYERS ) {
                layer[i] = SP_CSS_PAINT_ORDER_STROKE;
                layer_set[i] = false;
                ++i;
            }
            if( !used[2] && i < PAINT_ORDER_LAYERS ) {
                layer[i] = SP_CSS_PAINT_ORDER_MARKER;
                layer_set[i] = false;
            }
        }
    }
}

const Glib::ustring SPIPaintOrder::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    auto ret = Glib::ustring("");
    for( unsigned i = 0; i < PAINT_ORDER_LAYERS; ++i ) {
        if (layer_set[i]) {
            if (!ret.empty()) ret += " ";
            switch (this->layer[i]) {
                case SP_CSS_PAINT_ORDER_NORMAL:
                    ret += "normal";
                    assert( i == 0 );
                    break;
                case SP_CSS_PAINT_ORDER_FILL:
                    ret += "fill";
                    break;
                case SP_CSS_PAINT_ORDER_STROKE:
                    ret += "stroke";
                    break;
                case SP_CSS_PAINT_ORDER_MARKER:
                    ret += "markers";
                    break;
            }
        } else {
            break;
        }
    }
    return ret;
}

void
SPIPaintOrder::cascade( const SPIBase* const parent ) {
    if( const SPIPaintOrder* p = dynamic_cast<const SPIPaintOrder*>(parent) ) {
        if (!set || inherit) {  // Always inherits
            for( unsigned i = 0; i < PAINT_ORDER_LAYERS; ++i ) {
                layer[i]     = p->layer[i];
                layer_set[i] = p->layer_set[i];
            }
            g_free( value );
            value = g_strdup(p->value);
        }
    } else {
        std::cerr << "SPIPaintOrder::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPIPaintOrder::merge( const SPIBase* const parent ) {
    if( const SPIPaintOrder* p = dynamic_cast<const SPIPaintOrder*>(parent) ) {
        //    if( inherits ) {  PaintOrder always inherits
        if( (!set || inherit) && p->set && !(p->inherit) ) {
            this->cascade( parent );  // Must call be setting 'set'
            set     = p->set;
            inherit = p->inherit;
        }
    }
}

bool
SPIPaintOrder::operator==(const SPIBase& rhs) const {
    if( const SPIPaintOrder* r = dynamic_cast<const SPIPaintOrder*>(&rhs) ) {
        if( layer[0] == SP_CSS_PAINT_ORDER_NORMAL &&
            r->layer[0] == SP_CSS_PAINT_ORDER_NORMAL ) return SPIBase::operator==(rhs);
        for (unsigned i = 0; i < PAINT_ORDER_LAYERS; ++i ) {
            if( layer[i] != r->layer[i] ) return false;
        }
        return SPIBase::operator==(rhs);
    } else {
        return false;
    }
}



// SPIFilter ------------------------------------------------------------

SPIFilter::~SPIFilter() {
    if( href ) {
        clear();
        delete href;
        href = nullptr;
    }
}

void
SPIFilter::read( gchar const *str ) {

    if( !str ) return;

    clear();

    if ( streq(str, "inherit") ) {
        set = true;
        inherit = true;
    } else if (streq(str, "none")) {
        set = true;
    } else if (strneq(str, "url", 3)) {
        auto uri = extract_uri(str);
        if (uri.empty()) {
            std::cerr << "SPIFilter::read: url is empty or invalid" << std::endl;
            return;
        } else if (!style) {
            std::cerr << "SPIFilter::read: url with empty SPStyle pointer" << std::endl;
            return;
        }
        set = true;

        // Create href if not already done.
        if (!href) {
            if (style->object) {
                href = new SPFilterReference(style->object);
            }
            // Do we have href now?
            if ( href ) {
                href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_filter_ref_changed), style));
            } else {
                std::cerr << "SPIFilter::read(): Could not allocate 'href'" << std::endl;
                return;
            }
        }

        // We have href
        try {
            href->attach(Inkscape::URI(uri.c_str()));
        } catch (Inkscape::BadURIException &e) {
            std::cerr << "SPIFilter::read() " << e.what() << std::endl;
            href->detach();
        }

    } else {
        std::cerr << "SPIFilter::read(): malformed value: " << str << std::endl;
    }
}

const Glib::ustring SPIFilter::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->href) return this->href->getURI()->cssStr();
    return Glib::ustring("");
}

void
SPIFilter::clear() {

    SPIBase::clear();
    if( href ) {
        if( href->getObject() ) {
            href->detach();
        }
    }
}

void
SPIFilter::cascade( const SPIBase* const parent ) {
    if( const SPIFilter* p = dynamic_cast<const SPIFilter*>(parent) ) {
        if( inherit ) {  // Only inherits if 'inherit' true/
            // FIXME: This is rather unlikely so ignore for now.
            (void)p;
            std::cerr << "SPIFilter::cascade: value 'inherit' not supported." << std::endl;
        } else {
            // Do nothing
        }
    } else {
        std::cerr << "SPIFilter::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPIFilter::merge( const SPIBase* const parent ) {
    if( const SPIFilter* p = dynamic_cast<const SPIFilter*>(parent) ) {
        // The "correct" thing to do is to combine the filter primitives.
        // The next best thing is to keep any filter on this object. If there
        // is no filter on this object, then use any filter on the parent.
        if( (!set || inherit) && p->href && p->href->getObject() ) { // is the getObject()  needed?
            set     = p->set;
            inherit = p->inherit;
            if( href ) {
                // If we already have an href, use it (unlikely but heck...)
                if( href->getObject() ) {
                    href->detach();
                }
            } else {
                // If we don't have an href, create it
                if( style->document ) { // FIXME
                    href = new SPFilterReference(style->document);
                    //href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_filter_ref_changed), style));
                } else if (style->object) {
                    href = new SPFilterReference(style->object);
                }
            }
            if( href ) {
                // If we now have an href, try to attach parent filter
                try {
                    href->attach(*p->href->getURI());
                } catch (Inkscape::BadURIException &e) {
                    std::cerr << "SPIFilter::merge: " << e.what() << std::endl;
                    href->detach();
                }
            }
        }
    }
}

// FIXME
bool
SPIFilter::operator==(const SPIBase& rhs) const {
    if( const SPIFilter* r = dynamic_cast<const SPIFilter*>(&rhs) ) {
        (void)r;
        return true;
    } else {
        return false;
    }
}



// SPIDashArray ---------------------------------------------------------

void
SPIDashArray::read( gchar const *str ) {

    if( !str ) return;

    set = true;

    if( strcmp( str, "inherit") == 0 ) {
        inherit = true;
        return;
    }

    values.clear();

    if( strcmp(str, "none") == 0) {
        return;
    }

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[(,\\s|\\s)]+", str);

    bool LineSolid = true;

    for (auto token : tokens) {
        SPILength spilength;
        spilength.read(token.c_str());
        if (spilength.value > 0.00000001)
            LineSolid = false;
        values.push_back(spilength);
    }

    if (LineSolid) {
        values.clear();
    }
    return;
}

const Glib::ustring SPIDashArray::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->values.empty()) return Glib::ustring("none");
    auto ret = Glib::ustring("");
    for(auto value: this->values) {
        if (!ret.empty()) ret += ", ";
        ret += value.toString();
    }
    return ret;
}

void
SPIDashArray::cascade( const SPIBase* const parent ) {
    if( const SPIDashArray* p = dynamic_cast<const SPIDashArray*>(parent) ) {
        if( !set || inherit ) values = p->values;  // Always inherits
    } else {
        std::cerr << "SPIDashArray::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPIDashArray::merge( const SPIBase* const parent ) {
    if( const SPIDashArray* p = dynamic_cast<const SPIDashArray*>(parent) ) {
        if( inherits ) {
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set     = p->set;
                inherit = p->inherit;
                values  = p->values;
            }
        }
    } else {
        std::cerr << "SPIDashArray::merge(): Incorrect parent type" << std::endl;
    }
}

bool
SPIDashArray::operator==(const SPIBase& rhs) const {
    if (const SPIDashArray *r = dynamic_cast<const SPIDashArray *>(&rhs)) {
        if (values.size() != r->values.size()) {
            return false;
        }
        for (int i = 0; i < values.size(); i++) {
            if (values[i] != r->values[i]) {
                return false;
            }
        }
    }
    return SPIBase::operator==(rhs);
}


// SPIFontSize ----------------------------------------------------------

/** Indexed by SP_CSS_FONT_SIZE_blah.   These seem a bit small */
float const SPIFontSize::font_size_table[] = {6.0, 8.0, 10.0, 12.0, 14.0, 18.0, 24.0};
float const SPIFontSize::font_size_default = 12.0;

void
SPIFontSize::read( gchar const *str ) {

    if( !str ) return;

    if (!strcmp(str, "inherit")) {
        set = true;
        inherit = true;
    } else if ((*str == 'x') || (*str == 's') || (*str == 'm') || (*str == 'l')) {
        // xx-small, x-small, etc.
        for (unsigned i = 0; enum_font_size[i].key; i++) {
            if (!strcmp(str, enum_font_size[i].key)) {
                set = true;
                inherit = false;
                type = SP_FONT_SIZE_LITERAL;
                literal = enum_font_size[i].value;
                return;
            }
        }
        /* Invalid */
        return;
    } else {
        SPILength length;
        length.set = false;
        length.read( str );
        if( length.set ) {
            set      = true;
            inherit  = length.inherit;
            unit     = length.unit;
            value    = length.value;
            computed = length.computed;
            /*  Set a minimum font size to something much smaller than should ever (ever!) be encountered in a real file.
                If a bad SVG file is encountered and this is zero odd things
                might happen because the inverse is used in some scaling actions.
            */
            if ( computed <= 1.0e-32 ) { computed = 1.0e-32; }
            if( unit == SP_CSS_UNIT_PERCENT ) {
                type = SP_FONT_SIZE_PERCENTAGE;
            } else {
                type = SP_FONT_SIZE_LENGTH;
            }
        }
        return;
    }
}

const Glib::ustring SPIFontSize::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
    auto ret = Glib::ustring("");
    switch (this->type) {
        case SP_FONT_SIZE_LITERAL:
            for (unsigned i = 0; enum_font_size[i].key; i++) {
                if (enum_font_size[i].value == static_cast< gint > (this->literal) ) {
                    if (!ret.empty()) ret += " ";
                    ret += enum_font_size[i].key;
                }
            }
            break;
        case SP_FONT_SIZE_LENGTH:
            if (prefs->getBool("/options/font/textOutputPx", true)) {
                unit = SP_CSS_UNIT_PX;
            }
            ret += Glib::ustring::format(sp_style_css_size_px_to_units(this->computed, unit));
            ret += sp_style_get_css_unit_string(unit);
            break;
        case SP_FONT_SIZE_PERCENTAGE:
            return Glib::ustring::format(this->value * 100.0) + "%";
        default:
            g_error("Invalid FontSize value, not writing it.");
    }
    return ret;
}

void
SPIFontSize::cascade( const SPIBase* const parent ) {
    if( const SPIFontSize* p = dynamic_cast<const SPIFontSize*>(parent) ) {
        if( !set || inherit ) { // Always inherits
            computed = p->computed;value = p->value;


        // Calculate computed based on parent as needed
        } else if( type == SP_FONT_SIZE_LITERAL ) {
            if( literal < SP_CSS_FONT_SIZE_SMALLER ) {
                computed = font_size_table[ literal ];
            } else if( literal == SP_CSS_FONT_SIZE_SMALLER ) {
                computed = p->computed / 1.2;
            } else if( literal == SP_CSS_FONT_SIZE_LARGER ) {
                computed = p->computed * 1.2;
            } else {
                std::cerr << "SPIFontSize::cascade: Illegal literal value" << std::endl;
            }
        } else if( type == SP_FONT_SIZE_PERCENTAGE ) {
            // Percentage for font size is relative to parent computed (rather than viewport)
            computed = p->computed * value;
        } else if( type == SP_FONT_SIZE_LENGTH ) {
            switch ( unit ) {
                case SP_CSS_UNIT_EM:
                    /* Relative to parent font size */
                    computed = p->computed * value;
                    break;
                case SP_CSS_UNIT_EX:
                    /* Relative to parent font size */
                    computed = p->computed * value * 0.5; /* Hack FIXME */
                    break;
                default:
                    /* No change */
                    break;
            }
        }
        /*  Set a minimum font size to something much smaller than should ever (ever!) be encountered in a real file.
            If a bad SVG file is encountered and this is zero odd things
            might happen because the inverse is used in some scaling actions.
        */
        if ( computed <= 1.0e-32 ) { computed = 1.0e-32; }
    } else {
        std::cerr << "SPIFontSize::cascade(): Incorrect parent type" << std::endl;
    }
}

double
SPIFontSize::relative_fraction() const {

    switch (type) {
        case SP_FONT_SIZE_LITERAL: {
            switch (literal) {
                case SP_CSS_FONT_SIZE_SMALLER:
                    return 5.0 / 6.0;

                case SP_CSS_FONT_SIZE_LARGER:
                    return 6.0 / 5.0;

                default:
                    g_assert_not_reached();
            }
        }

        case SP_FONT_SIZE_PERCENTAGE:
            return value;

        case SP_FONT_SIZE_LENGTH: {
            switch (unit ) {
                case SP_CSS_UNIT_EM:
                    return value;

                case SP_CSS_UNIT_EX:
                    return value * 0.5;

                default:
                    g_assert_not_reached();
            }
        }
    }
    g_assert_not_reached();
    return 1; // Make -Wreturn-type happy
}

void
SPIFontSize::merge( const SPIBase* const parent ) {
    if( const SPIFontSize* p = dynamic_cast<const SPIFontSize*>(parent) ) {
        if( p->set && !(p->inherit) ) {
            // Parent has definined font-size
            if( (!set || inherit) ) {
                // Computed value same as parent
                set      = p->set;
                inherit  = p->inherit;
                type     = p->type;
                unit     = p->unit;
                literal  = p->literal;
                value    = p->value;
                computed = p->computed; // Just to be sure
            } else if ( type == SP_FONT_SIZE_LENGTH  &&
                        unit != SP_CSS_UNIT_EM &&
                        unit != SP_CSS_UNIT_EX ) {
                // Absolute size, computed value already set
            } else if ( type == SP_FONT_SIZE_LITERAL &&
                        literal < SP_CSS_FONT_SIZE_SMALLER ) {
                // Absolute size, computed value already set
                //g_assert( literal < G_N_ELEMENTS(font_size_table) );
                g_assert( computed == font_size_table[literal] );
            } else {
                // Relative size
                double const child_frac( relative_fraction() );
                set = true;
                inherit = false;
                computed = p->computed * child_frac;

                if ( ( p->type == SP_FONT_SIZE_LITERAL &&
                       p->literal < SP_CSS_FONT_SIZE_SMALLER ) ||
                     ( p->type == SP_FONT_SIZE_LENGTH &&
                       p->unit != SP_CSS_UNIT_EM &&
                       p->unit != SP_CSS_UNIT_EX ) ) {
                    // Parent absolute size
                    type = SP_FONT_SIZE_LENGTH;

                } else {
                    // Parent relative size
                    double const parent_frac( p->relative_fraction() );
                    if( type == SP_FONT_SIZE_LENGTH ) {
                        // ex/em
                        value *= parent_frac;
                    } else {
                        value = parent_frac * child_frac;
                        type = SP_FONT_SIZE_PERCENTAGE;
                    }
                }
            } // Relative size
            /*  Set a minimum font size to something much smaller than should ever (ever!) be encountered in a real file.
                If a bad SVG file is encountered and this is zero odd things
                might happen because the inverse is used in some scaling actions.
            */
            if ( computed <= 1.0e-32 ) { computed = 1.0e-32; }
        } // Parent set and not inherit
    } else {
        std::cerr << "SPIFontSize::merge(): Incorrect parent type" << std::endl;
    }
}

// What about different SVG units?
bool
SPIFontSize::operator==(const SPIBase& rhs) const {
    if( const SPIFontSize* r = dynamic_cast<const SPIFontSize*>(&rhs) ) {
        if( type != r->type ) { return false;}
        if( type == SP_FONT_SIZE_LENGTH ) {
            if( computed != r->computed ) { return false;}
        } else if (type == SP_FONT_SIZE_LITERAL ) {
            if( literal != r->literal ) { return false;}
        } else {
            if( value != r->value ) { return false;}
        }
        return SPIBase::operator==(rhs);
    } else {
        return false;
    }
}



// SPIFont ----------------------------------------------------------

void
SPIFont::read( gchar const *str ) {

    if( !str ) return;

    if( !style ) {
        std::cerr << "SPIFont::read(): style is void" << std::endl;
        return;
    }

    if ( !strcmp(str, "inherit") ) {
        set = true;
        inherit = true;
    } else {

        // Break string into white space separated tokens
        std::stringstream os( str );
        Glib::ustring param;

        while (os >> param) {

            // CSS is case insensitive but we're comparing against lowercase strings
            Glib::ustring lparam = param.lowercase();

            if (lparam == "/" ) {
                // line_height follows... note: font-size already read

                os >> param;
                lparam = param.lowercase();
                style->line_height.readIfUnset( lparam.c_str() );

            } else {
                // Try to parse each property in turn

                decltype(style->font_style) test_style;
                test_style.read( lparam.c_str() );
                if( test_style.set ) {
                    style->font_style = test_style;
                    continue;
                }

                // font-variant (Note: only CSS2.1 value small-caps is valid in shortcut.)
                decltype(style->font_variant) test_variant;
                test_variant.read( lparam.c_str() );
                if( test_variant.set ) {
                    style->font_variant = test_variant;
                    continue;
                }

                // font-weight
                decltype(style->font_weight) test_weight;
                test_weight.read( lparam.c_str() );
                if( test_weight.set ) {
                    style->font_weight = test_weight;
                    continue;
                }

                // font-stretch (added in CSS 3 Fonts)
                decltype(style->font_stretch) test_stretch;
                test_stretch.read( lparam.c_str() );
                if( test_stretch.set ) {
                    style->font_stretch = test_stretch;
                    continue;
                }

                // font-size
                decltype(style->font_size) test_size;
                test_size.read( lparam.c_str() );
                if( test_size.set ) {
                    style->font_size = test_size;
                    continue;
                }

                // No valid property value found.
                break;
            }
        } // params

        // The rest must be font-family...
        std::string str_s = str; // Why this extra step?
        std::string family = str_s.substr( str_s.find( param ) );

        style->font_family.readIfUnset( family.c_str() );

        // Everything in shorthand is set per CSS rules, this works since
        // properties are read backwards from end to start.
        style->font_style.set = true;
        style->font_variant.set = true;
        style->font_weight.set = true;
        style->font_stretch.set = true;
        style->font_size.set = true;
        style->line_height.set = true;
        style->font_family.set = true;
        // style->font_size_adjust.set = true;
        // style->font_kerning.set = true;
        // style->font_language_override.set = true;;
    }
}

const Glib::ustring SPIFont::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    // At the moment, do nothing. We could add a preference to write out
    // 'font' shorthand rather than longhand properties.
    /* SPIFontSize const *const my_base = dynamic_cast<const SPIFontSize*>(base);
    if ( (flags & SP_STYLE_FLAG_ALWAYS) ||
          ((flags & SP_STYLE_FLAG_IFSET) && this->set) ||
          ((flags & SP_STYLE_FLAG_IFDIFF) && this->set
          && (!my_base->set || this != my_base )))
    {
    }*/

    return Glib::ustring("");
}

// void
// SPIFont::cascade( const SPIBase* const parent ) {
// }

// void
// SPIFont::merge( const SPIBase* const parent ) {
// }

// Does nothing...
bool
SPIFont::operator==(const SPIBase& rhs) const {
    if( /* const SPIFont* r = */ dynamic_cast<const SPIFont*>(&rhs) ) {
        return SPIBase::operator==(rhs);
    } else {
        return false;
    }
}



// SPIBaselineShift -----------------------------------------------------

void
SPIBaselineShift::read( gchar const *str ) {

    if( !str ) return;

    if (!strcmp(str, "inherit")) {
        set = true;
        inherit = true;
    } else if ((*str == 'b') || (*str == 's')) {
        // baseline or sub or super
        for (unsigned i = 0; enum_baseline_shift[i].key; i++) {
            if (!strcmp(str, enum_baseline_shift[i].key)) {
                set = true;
                inherit = false;
                type = SP_BASELINE_SHIFT_LITERAL;
                literal = enum_baseline_shift[i].value;
                return;
            }
        }
        /* Invalid */
        return;
    } else {
        SPILength length;
        length.read( str );
        set      = length.set;
        inherit  = length.inherit;
        unit     = length.unit;
        value    = length.value;
        computed = length.computed;
        if( unit == SP_CSS_UNIT_PERCENT ) {
            type = SP_BASELINE_SHIFT_PERCENTAGE;
        } else {
            type = SP_BASELINE_SHIFT_LENGTH;
        }
        return;
    }
}

const Glib::ustring SPIBaselineShift::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    auto ret = Glib::ustring("");
    switch (this->type) {
        case SP_BASELINE_SHIFT_LITERAL:
            for (unsigned i = 0; enum_baseline_shift[i].key; i++) {
                if (enum_baseline_shift[i].value == static_cast< gint > (this->literal) ) {
                    if (!ret.empty()) ret += " ";
                    ret += enum_baseline_shift[i].key;
                }
            }
            break;
        case SP_BASELINE_SHIFT_LENGTH:
            if( this->unit == SP_CSS_UNIT_EM || this->unit == SP_CSS_UNIT_EX ) {
                ret += Glib::ustring::format(this->value);
                ret += (this->unit == SP_CSS_UNIT_EM ? "em" : "ex");
            } else {
                // must specify px, see inkscape bug 1221626, mozilla bug 234789
                ret += Glib::ustring::format(this->computed) + "px";
            }
            break;
        case SP_BASELINE_SHIFT_PERCENTAGE:
            return Glib::ustring::format(this->value * 100.0) + "%";
    }
    return ret;
}

void
SPIBaselineShift::cascade( const SPIBase* const parent ) {
    if( const SPIBaselineShift* p = dynamic_cast<const SPIBaselineShift*>(parent) ) {
        SPIFontSize *pfont_size = &(p->style->font_size);
        g_assert( pfont_size != nullptr );

        if( !set || inherit ) {
            computed = p->computed;  // Shift relative to parent shift, corrected below
        } else if (type == SP_BASELINE_SHIFT_LITERAL) {
            if( literal == SP_CSS_BASELINE_SHIFT_BASELINE ) {
                computed = 0; // No change
            } else if (literal == SP_CSS_BASELINE_SHIFT_SUB ) {
                // Should use subscript position from font relative to alphabetic baseline
                // OpenOffice, Adobe: -0.33, Word -0.14, LaTex about -0.2.
                computed = -0.2 * pfont_size->computed;
            } else if (literal == SP_CSS_BASELINE_SHIFT_SUPER ) {
                // Should use superscript position from font relative to alphabetic baseline
                // OpenOffice, Adobe: 0.33, Word 0.35, LaTex about 0.45.
                computed =  0.4 * pfont_size->computed;
            } else {
                /* Illegal value */
            }
        } else if (type == SP_BASELINE_SHIFT_PERCENTAGE) {
            // Percentage for baseline shift is relative to computed "line-height"
            // which is just font-size (see SVG1.1 'font').
            computed = pfont_size->computed * value;
        } else if (type == SP_BASELINE_SHIFT_LENGTH) {
            switch (unit) {
                case SP_CSS_UNIT_EM:
                    computed = value * pfont_size->computed;
                    break;
                case SP_CSS_UNIT_EX:
                    computed = value * 0.5 * pfont_size->computed;
                    break;
                default:
                    /* No change */
                    break;
            }
        }
        // baseline-shifts are relative to parent baseline
        computed += p->computed;

    } else {
        std::cerr << "SPIBaselineShift::cascade(): Incorrect parent type" << std::endl;
    }
}

// This was not defined in the legacy C code, it needs some serious thinking (but is low priority).
// FIX ME
void
SPIBaselineShift::merge( const SPIBase* const parent ) {
    if( const SPIBaselineShift* p = dynamic_cast<const SPIBaselineShift*>(parent) ) {
        if( (!set || inherit) && p->set && !(p->inherit) ) {
            set     = p->set;
            inherit = p->inherit;
            value   = p->value;
        }
    } else {
        std::cerr << "SPIBaselineShift::merge(): Incorrect parent type" << std::endl;
    }
}

// This is not used but we have it for completeness, it has not been tested.
bool
SPIBaselineShift::operator==(const SPIBase& rhs) const {
    if( const SPIBaselineShift* r = dynamic_cast<const SPIBaselineShift*>(&rhs) ) {
        if( type != r->type ) return false;
        if( type == SP_BASELINE_SHIFT_LENGTH ) {
            if( computed != r->computed ) return false;
        } else if ( type == SP_BASELINE_SHIFT_LITERAL ) {
            if( literal != r->literal ) return false;
        } else {
            if( value != r->value ) return false;
        }
        return SPIBase::operator==(rhs);
    } else {
        return false;
    }
}

bool
SPIBaselineShift::isZero() const {
    if( type == SP_BASELINE_SHIFT_LITERAL ) {
        if( literal == SP_CSS_BASELINE_SHIFT_BASELINE ) return true;
    } else {
        if( value == 0.0 ) return true;
    }
    return false;
}



// SPITextDecorationLine ------------------------------------------------

void
SPITextDecorationLine::read( gchar const *str ) {

    if( !str ) return;

    if (!strcmp(str, "inherit")) {
        set          = true;
        inherit      = true;
    } else if (!strcmp(str, "none")) {
        set          = true;
        inherit      = false;
        underline    = false;
        overline     = false;
        line_through = false;
        blink        = false;
    } else {
        bool found_one          = false;
        bool hit_one            = false;

        // CSS 2 keywords
        bool found_underline    = false;
        bool found_overline     = false;
        bool found_line_through = false;
        bool found_blink        = false;

        // This method ignores inlineid keys and extra delimiters, so " ,,, blink hello" will set
        // blink and ignore hello
        const gchar *hstr = str;
        while (true) {
            if (*str == ' ' || *str == ',' || *str == '\0'){
                int slen = str - hstr;
                // CSS 2 keywords
                while(true){ // not really a loop, used to avoid a goto
                    hit_one = true; // most likely we will
                    if ((slen ==  9) && strneq(hstr, "underline",    slen)){  found_underline    = true; break; }
                    if ((slen ==  8) && strneq(hstr, "overline",     slen)){  found_overline     = true; break; }
                    if ((slen == 12) && strneq(hstr, "line-through", slen)){  found_line_through = true; break; }
                    if ((slen ==  5) && strneq(hstr, "blink",        slen)){  found_blink        = true; break; }
                    if ((slen ==  4) && strneq(hstr, "none",         slen)){                             break; }

                    hit_one = false; // whatever this thing is, we do not recognize it
                    break;
                }
                found_one |= hit_one;
                if(*str == '\0')break;
                hstr = str + 1;
            }
            str++;
        }
        if (found_one) {
            set          = true;
            inherit      = false;
            underline    = found_underline;
            overline     = found_overline;
            line_through = found_line_through;
            blink        = found_blink;
        }
        else {
            set          = false;
            inherit      = false;
        }
    }
}

const Glib::ustring SPITextDecorationLine::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    auto ret = Glib::ustring("");
    if (underline) ret += "underline ";
    if (overline) ret += "overline ";
    if (line_through) ret += "line-through ";
    if (blink) ret += "blink "; // Deprecated
    if (ret.empty()) {
        ret = "none";
    } else {
        assert(ret.raw().back() == ' ');
        ret.resize(ret.size() - 1);
    }
    return ret;
}

void
SPITextDecorationLine::cascade( const SPIBase* const parent ) {
    if( const SPITextDecorationLine* p = dynamic_cast<const SPITextDecorationLine*>(parent) ) {
        if( inherits && (!set || inherit) ) {
            underline    = p->underline;
            overline     = p->overline;
            line_through = p->line_through;
            blink        = p->blink;
        }
    } else {
        std::cerr << "SPITextDecorationLine::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPITextDecorationLine::merge( const SPIBase* const parent ) {
    if( const SPITextDecorationLine* p = dynamic_cast<const SPITextDecorationLine*>(parent) ) {
        if( inherits ) { // Always inherits... but special rules?
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set      = p->set;
                inherit  = p->inherit;
                underline    = p->underline;
                overline     = p->overline;
                line_through = p->line_through;
                blink        = p->blink;
            }
        }
    }
}

bool
SPITextDecorationLine::operator==(const SPIBase& rhs) const {
    if( const SPITextDecorationLine* r = dynamic_cast<const SPITextDecorationLine*>(&rhs) ) {
        return
            (underline    == r->underline    ) &&
            (overline     == r->overline     ) &&
            (line_through == r->line_through ) &&
            (blink        == r->blink        ) &&
            SPIBase::operator==(rhs);
    } else {
        return false;
    }
}



// SPITextDecorationStyle -----------------------------------------------

void
SPITextDecorationStyle::read( gchar const *str ) {

    if( !str ) return;

    set         = false;
    inherit     = false;

    solid       = true; // Default
    isdouble    = false;
    dotted      = false;
    dashed      = false;
    wavy        = false;

    if (!strcmp(str, "inherit")) {
        set         = true;
        inherit     = true;
        solid       = false;
    } else {
        // note, these are CSS 3 keywords
        bool found_solid        = false;
        bool found_double       = false;
        bool found_dotted       = false;
        bool found_dashed       = false;
        bool found_wavy         = false;
        bool found_one          = false;

        // this method ignores inlineid keys and extra delimiters, so " ,,, style hello" will set style and ignore hello
        // if more than one style is present, the first is used
        const gchar *hstr = str;
        while (true) {
            if (*str == ' ' || *str == ',' || *str == '\0'){
                int slen = str - hstr;
                if (     (slen ==  5) && strneq(hstr, "solid",        slen)){  found_solid  = true; found_one = true; break; }
                else if ((slen ==  6) && strneq(hstr, "double",       slen)){  found_double = true; found_one = true; break; }
                else if ((slen ==  6) && strneq(hstr, "dotted",       slen)){  found_dotted = true; found_one = true; break; }
                else if ((slen ==  6) && strneq(hstr, "dashed",       slen)){  found_dashed = true; found_one = true; break; }
                else if ((slen ==  4) && strneq(hstr, "wavy",         slen)){  found_wavy   = true; found_one = true; break; }
                if(*str == '\0')break; // nothing more to test
                hstr = str + 1;
            }
            str++;
        }
        if(found_one){
            set         = true;
            solid       = found_solid;
            isdouble    = found_double;
            dotted      = found_dotted;
            dashed      = found_dashed;
            wavy        = found_wavy;
        }
        else {
            set         = false;
            inherit     = false;
        }
    }
}

const Glib::ustring SPITextDecorationStyle::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->solid) return Glib::ustring("solid");
    if (this->isdouble) return Glib::ustring("double");
    if (this->dotted) return Glib::ustring("dotted");
    if (this->dashed) return Glib::ustring("dashed");
    if (this->wavy) return Glib::ustring("wavy");
    g_error("SPITextDecorationStyle::write(): No valid value for property");
    return Glib::ustring("");
}

void
SPITextDecorationStyle::cascade( const SPIBase* const parent ) {
    if( const SPITextDecorationStyle* p = dynamic_cast<const SPITextDecorationStyle*>(parent) ) {
        if( inherits && (!set || inherit) ) {
            solid     = p->solid;
            isdouble  = p->isdouble;
            dotted    = p->dotted;
            dashed    = p->dashed;
            wavy      = p->wavy;
        }
    } else {
        std::cerr << "SPITextDecorationStyle::cascade(): Incorrect parent type" << std::endl;
    }
}

void
SPITextDecorationStyle::merge( const SPIBase* const parent ) {
    if( const SPITextDecorationStyle* p = dynamic_cast<const SPITextDecorationStyle*>(parent) ) {
        if( inherits ) { // Always inherits... but special rules?
            if( (!set || inherit) && p->set && !(p->inherit) ) {
                set      = p->set;
                inherit  = p->inherit;
                solid     = p->solid;
                isdouble  = p->isdouble;
                dotted    = p->dotted;
                dashed    = p->dashed;
                wavy      = p->wavy;
            }
        }
    }
}

bool
SPITextDecorationStyle::operator==(const SPIBase& rhs) const {
    if( const SPITextDecorationStyle* r = dynamic_cast<const SPITextDecorationStyle*>(&rhs) ) {
        return
            (solid    == r->solid    ) &&
            (isdouble == r->isdouble ) &&
            (dotted   == r->dotted   ) &&
            (dashed   == r->dashed   ) &&
            (wavy     == r->wavy     ) &&
            SPIBase::operator==(rhs);
    } else {
        return false;
    }
}



// TextDecorationColor is handled by SPIPaint (should be SPIColor), default value is "currentColor"
// FIXME



// SPITextDecoration ----------------------------------------------------

void
SPITextDecoration::read( gchar const *str ) {

    if( !str ) return;

    bool is_css3 = false;

    decltype(style->text_decoration_line) test_line;
    test_line.read( str );
    if( test_line.set ) {
        if (!style->text_decoration_line.set) {
            style->text_decoration_line = test_line;
        }
        set = true;
    }

    decltype(style->text_decoration_style) test_style;
    test_style.read( str );
    if( test_style.set ) {
        style->text_decoration_style = test_style;
        is_css3 = true;
    }

    // the color routine must be fed one token at a time - if multiple colors are found the LAST
    // one is used  ???? then why break on set?

    // This could certainly be designed better
    decltype(style->text_decoration_color) test_color;
    test_color.setStylePointer( style );
    test_color.read( "currentColor" );  // Default value
    test_color.set = false;
    const gchar *hstr = str;
    while (true) {
        if (*str == ' ' || *str == ',' || *str == '\0'){
            int slen = str - hstr;
            gchar *frag = g_strndup(hstr,slen+1); // only send one piece at a time, since keywords may be intermixed

            if( strcmp( frag, "none" ) != 0 ) { // 'none' not allowed
                test_color.read( frag );
            }

            free(frag);
            if( test_color.set ) {
                style->text_decoration_color = test_color;
                is_css3 = true;
                break;
            }
            test_color.read( "currentColor" );  // Default value
            test_color.set = false;
            if( *str == '\0' )break;
            hstr = str + 1;
        }
        str++;
    }

    // If we read a style or color then we have CSS3 which require any non-set values to be
    // set to their default values.
    if( is_css3 ) {
        style->text_decoration_line.set = true;
        style->text_decoration_style.set = true;
        style->text_decoration_color.set = true;
        set = true;
    }

    // If we set text_decoration_line, then update style_td (for CSS2 text-decoration)
    if( style->text_decoration_line.set ) {
        style_td = style;
    }
}

// Returns CSS2 'text-decoration' (using settings in SPTextDecorationLine)
// This is required until all SVG renderers support CSS3 'text-decoration'
const Glib::ustring SPITextDecoration::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    return style->text_decoration_line.get_value();
}
const Glib::ustring
SPITextDecoration::write( guint const flags, SPStyleSrc const &style_src_req, SPIBase const *const base) const {
    SPITextDecoration const *const my_base = dynamic_cast<const SPITextDecoration*>(base);
    assert(!base || my_base);
    // proxy for text-decoration-line, but only if set
    if (set && style &&
        style->text_decoration_line.shall_write(flags, style_src_req,
                                                my_base ? &my_base->style->text_decoration_line : nullptr)) {
        return (name() + ":" + this->get_value() + important_str() + ";");
    }
    return Glib::ustring("");
}

void
SPITextDecoration::cascade( const SPIBase* const parent ) {
    if( const SPITextDecoration* p = dynamic_cast<const SPITextDecoration*>(parent) ) {
        if( style_td == nullptr ) {
            style_td = p->style_td;
        }
    } else {
        std::cerr << "SPITextDecoration::cascade(): Incorrect parent type" << std::endl;
    }

}

void
SPITextDecoration::merge( const SPIBase* const parent ) {
    if( const SPITextDecoration* p = dynamic_cast<const SPITextDecoration*>(parent) ) {
        if( style_td == nullptr ) {
            style_td = p->style_td;
        }
    } else {
        std::cerr << "SPITextDecoration::merge(): Incorrect parent type" << std::endl;
    }

}

// Use CSS2 value
bool
SPITextDecoration::operator==(const SPIBase& rhs) const {
    if( const SPITextDecoration* r = dynamic_cast<const SPITextDecoration*>(&rhs) ) {
        return (style->text_decoration_line == r->style->text_decoration_line &&
                SPIBase::operator==(rhs));
    } else {
        return false;
    }
}

// SPIVectorEffect ------------------------------------------------

void
SPIVectorEffect::read( gchar const *str ) {

    if( !str ) return;

    if (!strcmp(str, "none")) {
        set          = true;
        stroke       = false;
        size         = false;
        rotate       = false;
        fixed        = false;
    } else {
        bool found_one          = false;
        bool hit_one            = false;

        bool found_stroke       = false;
        bool found_size         = false;
        bool found_rotate       = false;
        bool found_fixed        = false;

        const gchar *hstr = str;
        while (true) {
            if (*str == ' ' || *str == ',' || *str == '\0'){
                int slen = str - hstr;

                while(true){ // not really a loop, used to avoid a goto
                    hit_one = true; // most likely we will
                    if ((slen == 18) && strneq(hstr, "non-scaling-stroke", slen)){  found_stroke = true; break; }
                    if ((slen == 16) && strneq(hstr, "non-scaling-size",   slen)){  found_size   = true; break; }
                    if ((slen == 12) && strneq(hstr, "non-rotation",       slen)){  found_rotate = true; break; }
                    if ((slen == 14) && strneq(hstr, "fixed-position",     slen)){  found_fixed  = true; break; }
                    if ((slen ==  4) && strneq(hstr, "none",               slen)){                       break; }

                    hit_one = false; // whatever this thing is, we do not recognize it
                    break;
                }
                found_one |= hit_one;
                if(*str == '\0')break;
                hstr = str + 1;
            }
            str++;
        }
        if (found_one) {
            set          = true;
            stroke       = found_stroke;
            size         = found_size;
            rotate       = found_rotate;
            fixed        = found_fixed;
        }
        else {
            set          = false;
        }
    }

    // std::cout << "  stroke: " << stroke
    //           << "  size: " << size
    //           << "  rotate: " << rotate
    //           << "  fixed: " << fixed
    //           << std::endl;
}

const Glib::ustring SPIVectorEffect::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    auto ret = Glib::ustring("");
    if (this->stroke) ret += " non-scaling-stroke";
    if (this->size)   ret += " non-scaling-size";
    if (this->rotate) ret += " non-rotation";
    if (this->fixed)  ret += " fixed-position";
    if (ret.empty()) {
        ret += "none";
    } else {
        ret.erase(0, 1);
    }
    return ret;
}

// Does not inherit!
// void
// SPIVectorEffect::cascade( const SPIBase* const parent ) {
// }

// void
// SPIVectorEffect::merge( const SPIBase* const parent ) {
// }

bool
SPIVectorEffect::operator==(const SPIBase& rhs) const {
    if( const SPIVectorEffect* r = dynamic_cast<const SPIVectorEffect*>(&rhs) ) {
        return
            (stroke    == r->stroke) &&
            (size      == r->size  ) &&
            (rotate    == r->rotate) &&
            (fixed     == r->fixed ) &&
            SPIBase::operator==(rhs);
    } else {
        return false;
    }
}

// SPIStrokeExtensions ------------------------------------------------

void
SPIStrokeExtensions::read( gchar const *str ) {

    if( !str ) return;

    set = false;
    hairline = false;
    if (!strcmp(str, "none")) {
        set = true;
    } else if (!strcmp(str, "hairline")) {
        set = true;
        hairline = true;
    }
}

const Glib::ustring SPIStrokeExtensions::get_value() const
{
    if (this->inherit) return Glib::ustring("inherit");
    if (this->hairline) return Glib::ustring("hairline");
    return Glib::ustring("none");
}

// Does not inherit!
// void
// SPIStrokeExtensions::cascade( const SPIBase* const parent ) {
// }

// void
// SPIStrokeExtensions::merge( const SPIBase* const parent ) {
// }

bool
SPIStrokeExtensions::operator==(const SPIBase& rhs) const {
    if( const SPIStrokeExtensions* r = dynamic_cast<const SPIStrokeExtensions*>(&rhs) ) {
        return
            (hairline == r->hairline  ) &&
            SPIBase::operator==(rhs);
    } else {
        return false;
    }
}

// template instantiation
template class SPIEnum<SPBlendMode>;
template class SPIEnum<SPColorInterpolation>;
template class SPIEnum<SPColorRendering>;
template class SPIEnum<SPCSSBaseline>;
template class SPIEnum<SPCSSDirection>;
template class SPIEnum<SPCSSDisplay>;
template class SPIEnum<SPCSSFontVariantAlternates>;
template class SPIEnum<SPCSSTextAlign>;
template class SPIEnum<SPCSSTextOrientation>;
template class SPIEnum<SPCSSTextTransform>;
template class SPIEnum<SPCSSWritingMode>;
template class SPIEnum<SPEnableBackground>;
template class SPIEnum<SPImageRendering>;
template class SPIEnum<SPIsolation>;
template class SPIEnum<SPOverflow>;
template class SPIEnum<SPShapeRendering>;
template class SPIEnum<SPStrokeCapType>;
template class SPIEnum<SPStrokeJoinType>;
template class SPIEnum<SPTextAnchor>;
template class SPIEnum<SPTextRendering>;
template class SPIEnum<SPVisibility>;
template class SPIEnum<SPWhiteSpace>;
template class SPIEnum<SPWindRule>;
template class SPIEnum<SPCSSFontStretch>;
template class SPIEnum<SPCSSFontStyle>;
template class SPIEnum<SPCSSFontVariant>;
template class SPIEnum<SPCSSFontVariantPosition>;
template class SPIEnum<SPCSSFontVariantCaps>;
template class SPIEnum<SPCSSFontWeight>;
template class SPIEnum<uint_least16_t>;
template class SPIEnum<uint_least8_t>;



/* ----------------------------  NOTES  ----------------------------- */

/*
 * opacity's effect is cumulative; we set the new value to the combined effect.  The default value
 * for opacity is 1.0, not inherit. stop-opacity also does not inherit. (Note that stroke-opacity
 * and fill-opacity are quite different from opacity, and don't need any special handling.)
 *
 * Cases:
 * - parent & child were each previously unset, in which case the effective
 *   opacity value is 1.0, and style should remain unset.
 * - parent was previously unset (so computed opacity value of 1.0)
 *   and child was set to inherit.  The merged child should
 *   get a value of 1.0, and shouldn't inherit (lest the new parent
 *   has a different opacity value).  Given that opacity's default
 *   value is 1.0 (rather than inherit), we might as well have the
 *   merged child's opacity be unset.
 * - parent was previously unset (so opacity 1.0), and child was set to a number.
 *   The merged child should retain its existing settings (though it doesn't matter
 *   if we make it unset if that number was 1.0).
 * - parent was inherit and child was unset.  Merged child should be set to inherit.
 * - parent was inherit and child was inherit.  (We can't in general reproduce this
 *   effect (short of introducing a new group), but setting opacity to inherit is rare.)
 *   If the inherited value was strictly between 0.0 and 1.0 (exclusive) then the merged
 *   child's value should be set to the product of the two, i.e. the square of the
 *   inherited value, and should not be marked as inherit.  (This decision assumes that it
 *   is more important to retain the effective opacity than to retain the inheriting
 *   effect, and assumes that the inheriting effect either isn't important enough to create
 *   a group or isn't common enough to bother maintaining the code to create a group.)  If
 *   the inherited value was 0.0 or 1.0, then marking the merged child as inherit comes
 *   closer to maintaining the effect.
 * - parent was inherit and child was set to a numerical value.  If the child's value
 *   was 1.0, then the merged child should have the same settings as the parent.
 *   If the child's value was 0, then the merged child should also be set to 0.
 *   If the child's value was anything else, then we do the same as for the inherit/inherit
 *   case above: have the merged child set to the product of the two opacities and not
 *   marked as inherit, for the same reasons as for that case.
 * - parent was set to a value, and child was unset.  The merged child should have
 *   parent's settings.
 * - parent was set to a value, and child was inherit.  The merged child should
 *   be set to the product, i.e. the square of the parent's value.
 * - parent & child are each set to a value.  The merged child should be set to the
 *   product.
 */


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
