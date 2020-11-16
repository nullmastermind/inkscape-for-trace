// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_STYLE_INTERNAL_H
#define SEEN_SP_STYLE_INTERNAL_H

/** \file
 * SPStyle internal: classes that are internal to SPStyle
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2014, 2018 Tavmjong Bah
 * Copyright (C) 2010 Jon A. Cruz
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <utility>
#include <vector>
#include <map>

#include "attributes.h"
#include "style-enums.h"

#include "color.h"

#include "object/sp-marker-loc.h"
#include "object/sp-filter.h"
#include "object/sp-filter-reference.h"
#include "object/sp-paint-server-reference.h"
#include "object/sp-shape-reference.h"

#include "object/uri.h"

#include "svg/svg-icc-color.h"

#include "xml/repr.h"


static const unsigned SP_STYLE_FLAG_ALWAYS (1 << 2);
static const unsigned SP_STYLE_FLAG_IFSET  (1 << 0);
static const unsigned SP_STYLE_FLAG_IFDIFF (1 << 1);
static const unsigned SP_STYLE_FLAG_IFSRC  (1 << 3); // If source matches

// for the bitfield in SPStyle::style_src this must be an unsigned type
enum class SPStyleSrc : unsigned
{
    UNSET,
    ATTRIBUTE,   // fill="red"
    STYLE_PROP,  // style="fill:red"
    STYLE_SHEET, // .red { fill:red; }
};

/* General comments:
 *
 * This code is derived from the original C style code in style.cpp.
 *
 * Overview:
 *   Style can be obtained (in order of precedence) [CHECK]
 *     1. "style" property in an element (style="fill:red").
 *     2. Style sheet, internal or external (<style> rect {fill:red;}</style>). 
 *     3. Attributes in an element (fill="red").
 *     4. Parent's style.
 *   A later property overrides an earlier property. This is implemented by
 *   reading in the properties backwards. If a property is already set, it
 *   prevents an earlier property from being read.
 *
 *   A declaration with an "!important" rule overrides any other declarations (except those that
 *   also have an "!important" rule). Attributes can not use the "!important" rule and the rule
 *   is not inherited.
 *
 *   In order for cascading to work, each element in the tree must be read in from top to bottom
 *   (parent before child). At each step, if a style property is not explicitly set, the property
 *   value is taken from the parent. Some properties have "computed" values that depend on:
 *      the parent's value (e.g. "font-size:larger"),
 *      another property value ("stroke-width":1em"), or
 *      an external value ("stroke-width:5%").
 *
 * To summarize:
 *
 *   An explicitly set value (including 'inherit') has a 'true' "set" flag.
 *   The "value" is either explicitly set or inherited.
 *   The "computed" value (if present) is calculated from "value" and some other input. 
 *
 * Functions:
 *   write():    Write a property and its value to a string.
 *     Flags:
 *       ALWAYS: Always write out property.
 *       IFSET:  Write a property if 'set' flag is true, otherwise return empty string.
 *       IFDIFF: Write a property if computed values are different, otherwise return empty string,
 *               This is only used for text!!
 *       IFSRC   Write a property if the source matches the requested source (style sheet, etc.).
 *
 *   read():     Set a property value from a string.
 *   clear():    Set a property to its default value and set the 'set' flag to false.
 *   cascade():  Cascade the parent's property values to the child if the child's property
 *               is unset (and it allows inheriting) or the value is 'inherit'.
 *               Calculate computed values that depend on parent.
 *               This requires that the parent already be updated.
 *   merge():    Merge the property values of a child and a parent that is being deleted,
 *               attempting to preserve the style of the child.
 *   operator=:  Assignment operator required due to use of templates (in original C code).
 *   operator==: True if computed values are equal.  TO DO: DEFINE EXACTLY WHAT THIS MEANS
 *   operator!=: Inverse of operator==.
 *
 *
 * Outside dependencies:
 *
 *   The C structures that these classes are evolved from were designed to be embedded in to the
 *   style structure (i.e they are "internal" and thus have an "I" in the SPI prefix). However,
 *   they should be reasonably stand-alone and can provide some functionality outside of the style
 *   structure (i.e. reading and writing style strings). Some properties do need access to other
 *   properties from the same object (e.g. SPILength sometimes needs to know font size) to
 *   calculate 'computed' values. Inheritance, of course, requires access to the parent object's
 *   style class.
 *
 *   The only real outside dependency is SPObject... which is needed in the cases of SPIPaint and
 *   SPIFilter for setting up the "href". (Currently, SPDocument is needed but this dependency
 *   should be removed as an "href" only needs the SPDocument for attaching an external document to
 *   the XML tree [see uri-references.cpp]. If SPDocument is really needed, it can be obtained from
 *   SPObject.)
 *
 */

/// Virtual base class for all SPStyle internal classes
class SPIBase
{

public:
    SPIBase(bool inherits_ = true)
        : inherits(inherits_),
          set(false),
          inherit(false),
          important(false),
          style_src(SPStyleSrc::STYLE_PROP), // Default to property, see bug 1662285.
          style(nullptr)
    {}

    virtual ~SPIBase()
    = default;

    virtual void read( gchar const *str ) = 0;
    void readIfUnset(gchar const *str, SPStyleSrc source = SPStyleSrc::STYLE_PROP);

protected:
    char const *important_str() const { return important ? " !important" : ""; }

public:
    void readAttribute(Inkscape::XML::Node *repr)
    {
        readIfUnset(repr->attribute(name().c_str()), SPStyleSrc::ATTRIBUTE);
    }

    virtual const Glib::ustring get_value() const = 0;
    bool shall_write( guint const flags = SP_STYLE_FLAG_IFSET,
                      SPStyleSrc const &style_src_req = SPStyleSrc::STYLE_PROP,
                      SPIBase const *const base = nullptr ) const;
    virtual const Glib::ustring write( guint const flags = SP_STYLE_FLAG_IFSET,
                                       SPStyleSrc const &style_src_req = SPStyleSrc::STYLE_PROP,
                                       SPIBase const *const base = nullptr ) const;
    virtual void clear() {
        set = false, inherit = false, important = false;
    }

    virtual void cascade( const SPIBase* const parent ) = 0;
    virtual void merge(   const SPIBase* const parent ) = 0;

    void setStylePointer(SPStyle *style_in) { style = style_in; }

    // Explicit assignment operator required due to templates.
    SPIBase& operator=(const SPIBase& rhs) = default;

    // Check apples being compared to apples
    virtual bool operator==(const SPIBase& rhs) const {
        return id() == rhs.id();
    }

    bool operator!=(const SPIBase& rhs) const {
        return !(*this == rhs);
    }

    virtual SPAttr id() const { return SPAttr::INVALID; }
    Glib::ustring const &name() const;

  // To do: make private
public:
    bool inherits : 1;    // Property inherits by default from parent.
    bool set : 1;         // Property has been explicitly set (vs. inherited).
    bool inherit : 1;     // Property value set to 'inherit'.
    bool important : 1;   // Property rule 'important' has been explicitly set.
    SPStyleSrc style_src : 2; // Source (attribute, style attribute, style-sheet).

protected:
    SPStyle* style;       // Used by SPIPaint, SPIFilter... to find values of other properties
};


/**
 * Decorator which overrides SPIBase::id()
 */
template <SPAttr Id, class Base>
class TypedSPI : public Base {
  public:
    using Base::Base;

    /**
     * Get the attribute enum
     */
    SPAttr id() const override { return Id; }
    static SPAttr static_id() { return Id; }

    /**
     * Upcast to the base class
     */
    Base *upcast() { return static_cast<Base *>(this); }
    Base const *upcast() const { return static_cast<Base const *>(this); }
};


/// Float type internal to SPStyle. (Only 'stroke-miterlimit')
class SPIFloat : public SPIBase
{

public:
    SPIFloat(float value_default = 0.0 )
        : value(value_default),
          value_default(value_default)
    {}

    ~SPIFloat() override = default;
    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        value = value_default;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIFloat& operator=(const SPIFloat& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    float value = 0.0;

private:
    float value_default = 0.0;
};

/*
 * One might think that the best value for SP_SCALE24_MAX would be ((1<<24)-1), which allows the
 * greatest possible precision for fitting [0, 1] fractions into 24 bits.
 *
 * However, in practice, that gives a problem with 0.5, which falls half way between two fractions
 * of ((1<<24)-1).  What's worse is that casting double(1<<23) / ((1<<24)-1) to float on x86
 * produces wrong rounding behaviour, resulting in a fraction of ((1<<23)+2.0f) / (1<<24) rather
 * than ((1<<23)+1.0f) / (1<<24) as one would expect, let alone ((1<<23)+0.0f) / (1<<24) as one
 * would ideally like for this example.
 *
 * The value (1<<23) is thus best if one considers float conversions alone.
 *
 * The value 0xff0000 can exactly represent all 8-bit alpha channel values,
 * and can exactly represent all multiples of 0.1.  I haven't yet tested whether
 * rounding bugs still get in the way of conversions to & from float, but my instinct is that
 * it's fairly safe because 0xff fits three times inside float's significand.
 *
 * We should probably use the value 0xffff00 once we support 16 bits per channel and/or LittleCMS,
 * though that might need to be accompanied by greater use of double instead of float for
 * colours and opacities, to be safe from rounding bugs.
 */
static const unsigned SP_SCALE24_MAX = 0xff0000;
#define SP_SCALE24_TO_FLOAT(v) ((double) (v) / SP_SCALE24_MAX)
#define SP_SCALE24_FROM_FLOAT(v) unsigned(((v) * SP_SCALE24_MAX) + .5)

/** Returns a scale24 for the product of two scale24 values. */
#define SP_SCALE24_MUL(_v1, _v2) unsigned((double)(_v1) * (_v2) / SP_SCALE24_MAX + .5)


/// 24 bit data type internal to SPStyle.
// Used only for opacity, fill-opacity, stroke-opacity.
// Opacity does not inherit but stroke-opacity and fill-opacity do. 
class SPIScale24 : public SPIBase
{
    static unsigned get_default() { return SP_SCALE24_MAX; }

public:
    SPIScale24(bool inherits = true )
        : SPIBase(inherits),
          value(get_default())
    {}

    ~SPIScale24() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        value = get_default();
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIScale24& operator=(const SPIScale24& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;


  // To do: make private
public:
    unsigned value : 24;
};


enum SPCSSUnit {
    SP_CSS_UNIT_NONE,
    SP_CSS_UNIT_PX,
    SP_CSS_UNIT_PT,
    SP_CSS_UNIT_PC,
    SP_CSS_UNIT_MM,
    SP_CSS_UNIT_CM,
    SP_CSS_UNIT_IN,
    SP_CSS_UNIT_EM,
    SP_CSS_UNIT_EX,
    SP_CSS_UNIT_PERCENT
};


/// Length type internal to SPStyle.
// Needs access to 'font-size' and 'font-family' for computed values.
// Used for 'stroke-width' 'stroke-dash-offset' ('none' not handled), text-indent
class SPILength : public SPIBase
{

public:
    SPILength(float value = 0)
        : unit(SP_CSS_UNIT_NONE),
          value(value),
          computed(value),
          value_default(value)
    {}

    ~SPILength() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        unit = SP_CSS_UNIT_NONE, value = value_default;
        computed = value_default;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPILength& operator=(const SPILength& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;
    void setDouble(double v);
    virtual const Glib::ustring toString(bool wname = false) const;

    // To do: make private
  public:
    unsigned unit : 4;
    float value = 0.f;
    float computed = 0.f;

private:
    float value_default = 0.f;
};


/// Extended length type internal to SPStyle.
// Used for: line-height, letter-spacing, word-spacing
class SPILengthOrNormal : public SPILength
{

public:
    SPILengthOrNormal(float value = 0)
        : SPILength(value),
          normal(true)
    {}

    ~SPILengthOrNormal() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPILength::clear();
        normal = true;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPILengthOrNormal& operator=(const SPILengthOrNormal& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    bool normal : 1;
};


/// Extended length type internal to SPStyle.
// Used for: font-variation-settings
class SPIFontVariationSettings : public SPIBase
{

public:
    SPIFontVariationSettings()
        : normal(true)
    {}

    ~SPIFontVariationSettings() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        axes.clear();
        normal = true;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIFontVariationSettings& operator=(const SPIFontVariationSettings& rhs) {
        SPIBase::operator=(rhs);
        axes = rhs.axes;
        normal = rhs.normal;
        return *this;
    }

    bool operator==(const SPIBase& rhs) const override;

    virtual const Glib::ustring toString() const;

  // To do: make private
public:
    bool normal : 1;
    bool inherit : 1;
    std::map<Glib::ustring, float> axes;
};


/// Enum type internal to SPStyle.
// Used for many properties. 'font-stretch' and 'font-weight' must be special cased.
template <typename T>
class SPIEnum : public SPIBase
{

public:
    SPIEnum(T value = T(), bool inherits = true) :
        SPIBase(inherits),
        value(value),
        value_default(value)
    {
        update_computed();
    }

    ~SPIEnum() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        value = value_default;
        update_computed();
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIEnum& operator=(const SPIEnum& rhs) {
        SPIBase::operator=(rhs);
        value            = rhs.value;
        computed         = rhs.computed;
        value_default    = rhs.value_default;
        return *this;
    }

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    T value{};
    T computed{};

private:
    T value_default{};

    //! Update computed from value
    void update_computed();
    //! Update computed from parent computed
    void update_computed_cascade(T const &parent_computed) {}
    //! Update value from parent
    //! @pre computed is up to date
    void update_value_merge(SPIEnum<T> const &) {}
    void update_value_merge(SPIEnum<T> const &, T, T);
};


#if 0
/// SPIEnum w/ bits, allows values with multiple key words.
class SPIEnumBits : public SPIEnum
{

public:
    SPIEnumBits( Glib::ustring const &name, SPStyleEnum const *enums, unsigned value = 0, bool inherits = true ) :
        SPIEnum( name, enums, value, inherits )
    {}

    ~SPIEnumBits() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
};
#endif


/// SPIEnum w/ extra bits. The 'font-variants-ligatures' property is a complete mess that needs
/// special handling. For OpenType fonts the values 'common-ligatures', 'contextual',
/// 'no-discretionary-ligatures', and 'no-historical-ligatures' are not useful but we still must be
/// able to parse them.
using _SPCSSFontVariantLigatures_int = typename std::underlying_type<SPCSSFontVariantLigatures>::type;
class SPILigatures : public SPIEnum<_SPCSSFontVariantLigatures_int>
{

public:
    SPILigatures() :
        SPIEnum<_SPCSSFontVariantLigatures_int>(SP_CSS_FONT_VARIANT_LIGATURES_NORMAL)
    {}

    ~SPILigatures() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
};


/// SPIEnum w/ extra bits. The 'font-variants-numeric' property is a complete mess that needs
/// special handling. Multiple key words can be specified, some exclusive of others.
using _SPCSSFontVariantNumeric_int = typename std::underlying_type<SPCSSFontVariantNumeric>::type;
class SPINumeric : public SPIEnum<_SPCSSFontVariantNumeric_int>
{

public:
    SPINumeric() :
        SPIEnum<_SPCSSFontVariantNumeric_int>(SP_CSS_FONT_VARIANT_NUMERIC_NORMAL)
    {}

    ~SPINumeric() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
};


/// SPIEnum w/ extra bits. The 'font-variants-east-asian' property is a complete mess that needs
/// special handling. Multiple key words can be specified, some exclusive of others.
using _SPCSSFontVariantEastAsian_int = typename std::underlying_type<SPCSSFontVariantEastAsian>::type;
class SPIEastAsian : public SPIEnum<_SPCSSFontVariantEastAsian_int>
{

public:
    SPIEastAsian() :
        SPIEnum<_SPCSSFontVariantEastAsian_int>(SP_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL)
    {}

    ~SPIEastAsian() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
};


/// String type internal to SPStyle.
// Used for 'marker', ..., 'font', 'font-family', 'inkscape-font-specification'
class SPIString : public SPIBase
{

public:
    SPIString(bool inherits = true)
        : SPIBase(inherits)
    {}

    SPIString(const SPIString &rhs) { *this = rhs; }

    ~SPIString() override {
        g_free(_value);
    }

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override; // TODO check about value and value_default
    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIString& operator=(const SPIString& rhs) {
        if (this == &rhs) {
            return *this;
        }
        SPIBase::operator=(rhs);
        g_free(_value);
        _value = g_strdup(rhs._value);
        return *this;
    }

    bool operator==(const SPIBase& rhs) const override;

    //! Get value if set, or inherited value, or default value (may be NULL)
    char const *value() const;

  private:
    char const *get_default_value() const;

    gchar *_value = nullptr;
};

/// Shapes type internal to SPStyle.
// Used for 'shape-inside', shape-subtract'
// Differs from SPIString by creating/deleting listeners on referenced shapes.
class SPIShapes : public SPIString
{
    void hrefs_clear();

public:
    ~SPIShapes() override;
    SPIShapes();
    SPIShapes(const SPIShapes &) = delete; // Copying causes problems with hrefs.
    void read( gchar const *str ) override;
    void clear() override;

public:
    std::vector<SPShapeReference *> hrefs;
};

/// Color type internal to SPStyle, FIXME Add string value to store SVG named color.
class SPIColor : public SPIBase
{

public:
    SPIColor(bool inherits = true)
        : SPIBase(inherits)
        , currentcolor(false)
    {
        value.color.set(0);
    }

    ~SPIColor() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        value.color.set(0);
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIColor& operator=(const SPIColor& rhs) {
        SPIBase::operator=(rhs);
        currentcolor = rhs.currentcolor;
        value.color  = rhs.value.color;
        return *this;
    }

    bool operator==(const SPIBase& rhs) const override;

    void setColor( float r, float g, float b ) {
        value.color.set( r, g, b );
    }

    void setColor( guint32 val ) {
        value.color.set( val );
    }

    void setColor( SPColor const& color ) {
        value.color = color;
    }

public:
    bool currentcolor : 1;
    // FIXME: remove structure and derive SPIPaint from this class.
    struct {
         SPColor color;
    } value;
};



#define SP_STYLE_FILL_SERVER(s) ((const_cast<SPStyle *> (s))->getFillPaintServer())
#define SP_STYLE_STROKE_SERVER(s) ((const_cast<SPStyle *> (s))->getStrokePaintServer())

// SVG 2
enum SPPaintOrigin {
    SP_CSS_PAINT_ORIGIN_NORMAL,
    SP_CSS_PAINT_ORIGIN_CURRENT_COLOR,
    SP_CSS_PAINT_ORIGIN_CONTEXT_FILL,
    SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE
};


/// Paint type internal to SPStyle.
class SPIPaint : public SPIBase
{

public:
    SPIPaint()
        : paintOrigin(SP_CSS_PAINT_ORIGIN_NORMAL),
          colorSet(false),
          noneSet(false) {
        value.href = nullptr;
        clear();
    }

    ~SPIPaint() override;  // Clear and delete href.
    void read( gchar const *str ) override;
    virtual void read( gchar const *str, SPStyle &style, SPDocument *document = nullptr);
    const Glib::ustring get_value() const override;
    void clear() override;
    virtual void reset( bool init ); // Used internally when reading or cascading
    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIPaint& operator=(const SPIPaint& rhs) {
        SPIBase::operator=(rhs);
        paintOrigin     = rhs.paintOrigin;
        colorSet        = rhs.colorSet;
        noneSet         = rhs.noneSet;
        value.color     = rhs.value.color;
        value.href      = rhs.value.href;
        return *this;
    }

    bool operator==(const SPIBase& rhs) const override;

    bool isSameType( SPIPaint const & other ) const {
        return (isPaintserver() == other.isPaintserver()) && (colorSet == other.colorSet) && (paintOrigin == other.paintOrigin);
    }

    bool isNoneSet() const {
        return noneSet;
    }

    bool isNone() const {
        return !colorSet && !isPaintserver() && (paintOrigin == SP_CSS_PAINT_ORIGIN_NORMAL);
    } // TODO refine

    bool isColor() const {
        return colorSet && !isPaintserver();
    }

    bool isPaintserver() const {
        return value.href && value.href->getObject() != nullptr;
    }

    void setColor( float r, float g, float b ) {
        value.color.set( r, g, b ); colorSet = true;
    }

    void setColor( guint32 val ) {
        value.color.set( val ); colorSet = true;
    }

    void setColor( SPColor const& color ) {
        value.color = color; colorSet = true;
    }

    void setNone() {noneSet = true; colorSet=false;}

  // To do: make private
public:
    SPPaintOrigin paintOrigin : 2;
    bool colorSet : 1;
    bool noneSet : 1;
    struct {
         SPPaintServerReference *href;
         SPColor color;
    } value;
};


// SVG 2
enum SPPaintOrderLayer {
    SP_CSS_PAINT_ORDER_NORMAL,
    SP_CSS_PAINT_ORDER_FILL,
    SP_CSS_PAINT_ORDER_STROKE,
    SP_CSS_PAINT_ORDER_MARKER
};

// Normal maybe should be moved out as is done in other classes.
// This could be replaced by a generic enum class where multiple keywords are allowed and
// where order matters (in contrast to 'text-decoration-line' where order does not matter).

// Each layer represents a layer of paint which can be a fill, a stroke, or markers.
const size_t PAINT_ORDER_LAYERS = 3;

/// Paint order type internal to SPStyle
class SPIPaintOrder : public SPIBase
{

public:
    SPIPaintOrder() {
        this->clear();
    }

    SPIPaintOrder(const SPIPaintOrder &rhs) { *this = rhs; }

    ~SPIPaintOrder() override {
        g_free( value );
    }

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        for( unsigned i = 0; i < PAINT_ORDER_LAYERS; ++i ) {
            layer[i]     = SP_CSS_PAINT_ORDER_NORMAL;
            layer_set[i] = false;
        }
        g_free(value);
        value = nullptr;
    }
    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIPaintOrder& operator=(const SPIPaintOrder& rhs) {
        if (this == &rhs) {
            return *this;
        }
        SPIBase::operator=(rhs);
        for( unsigned i = 0; i < PAINT_ORDER_LAYERS; ++i ) {
            layer[i]     = rhs.layer[i];
            layer_set[i] = rhs.layer_set[i];
        }
        g_free(value);
        value            = g_strdup(rhs.value);
        return *this;
    }

    bool operator==(const SPIBase& rhs) const override;


  // To do: make private
public:
    SPPaintOrderLayer layer[PAINT_ORDER_LAYERS];
    bool layer_set[PAINT_ORDER_LAYERS];
    gchar *value = nullptr; // Raw string
};


/// Filter type internal to SPStyle
class SPIDashArray : public SPIBase
{

public:
    SPIDashArray() = default;

    ~SPIDashArray() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        values.clear();
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIDashArray& operator=(const SPIDashArray& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private, change double to SVGLength
public:
  std::vector<SPILength> values;
};

/// Filter type internal to SPStyle
class SPIFilter : public SPIBase
{

public:
    SPIFilter()
        : SPIBase(false)
    {}

    ~SPIFilter() override;
    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override;
    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIFilter& operator=(const SPIFilter& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    SPFilterReference *href = nullptr;
};



enum {
    SP_FONT_SIZE_LITERAL,
    SP_FONT_SIZE_LENGTH,
    SP_FONT_SIZE_PERCENTAGE
};

/// Fontsize type internal to SPStyle (also used by libnrtype/Layout-TNG-Input.cpp).
class SPIFontSize : public SPIBase
{

public:
    SPIFontSize() {
        this->clear();
    }

    ~SPIFontSize() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        type = SP_FONT_SIZE_LITERAL, unit = SP_CSS_UNIT_NONE,
            literal = SP_CSS_FONT_SIZE_MEDIUM, value = 12.0, computed = 12.0;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIFontSize& operator=(const SPIFontSize& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

public:
    static float const font_size_default;

  // To do: make private
public:
    unsigned type : 2;
    unsigned unit : 4;
    unsigned literal : 4;
    float value;
    float computed;

private:
    double relative_fraction() const;
    static float const font_size_table[];
};


/// Font type internal to SPStyle ('font' shorthand)
class SPIFont : public SPIBase
{

public:
    SPIFont() = default;

    ~SPIFont() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
    }

    void cascade( const SPIBase* const /*parent*/ ) override
    {} // Done in dependent properties

    void merge(   const SPIBase* const /*parent*/ ) override
    {}

    SPIFont& operator=(const SPIFont& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;
};


enum {
    SP_BASELINE_SHIFT_LITERAL,
    SP_BASELINE_SHIFT_LENGTH,
    SP_BASELINE_SHIFT_PERCENTAGE
};

/// Baseline shift type internal to SPStyle. (This is actually just like SPIFontSize)
class SPIBaselineShift : public SPIBase
{

public:
    SPIBaselineShift()
        : SPIBase(false) {
        this->clear();
    }

    ~SPIBaselineShift() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        type=SP_BASELINE_SHIFT_LITERAL, unit=SP_CSS_UNIT_NONE,
            literal = SP_CSS_BASELINE_SHIFT_BASELINE, value = 0.0, computed = 0.0;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPIBaselineShift& operator=(const SPIBaselineShift& rhs) = default;

    // This is not used but we have it for completeness, it has not been tested.
    bool operator==(const SPIBase& rhs) const override;

    bool isZero() const;

  // To do: make private
public:
    unsigned type : 2;
    unsigned unit : 4;
    unsigned literal: 2;
    float value; // Can be negative
    float computed;
};

// CSS 2.  Changes in CSS 3, where description is for TextDecorationLine, NOT TextDecoration
// See http://www.w3.org/TR/css-text-decor-3/

// CSS3 2.2
/// Text decoration line type internal to SPStyle.  THIS SHOULD BE A GENERIC CLASS
class SPITextDecorationLine : public SPIBase
{

public:
    SPITextDecorationLine() {
        this->clear();
    }

    ~SPITextDecorationLine() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        underline = false, overline = false, line_through = false, blink = false;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPITextDecorationLine& operator=(const SPITextDecorationLine& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    bool underline : 1;
    bool overline : 1;
    bool line_through : 1;
    bool blink : 1;    // "Conforming user agents are not required to support this value." yay!
};

// CSS3 2.2
/// Text decoration style type internal to SPStyle.  THIS SHOULD JUST BE SPIEnum!
class SPITextDecorationStyle : public SPIBase
{

public:
    SPITextDecorationStyle() {
        this->clear();
    }

    ~SPITextDecorationStyle() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        solid = true, isdouble = false, dotted = false, dashed = false, wavy = false;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPITextDecorationStyle& operator=(const SPITextDecorationStyle& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    bool solid : 1;
    bool isdouble : 1;  // cannot use "double" as it is a reserved keyword
    bool dotted : 1;
    bool dashed : 1;
    bool wavy : 1;
};



// This class reads in both CSS2 and CSS3 'text-decoration' property. It passes the line, style,
// and color parts to the appropriate CSS3 long-hand classes for reading and storing values.  When
// writing out data, we write all four properties, with 'text-decoration' being written out with
// the CSS2 format. This allows CSS1/CSS2 renderers to at least render lines, even if they are not
// the right style. (See http://www.w3.org/TR/css-text-decor-3/#text-decoration-property )

/// Text decoration type internal to SPStyle.
class SPITextDecoration : public SPIBase
{

public:
    SPITextDecoration() = default;

    ~SPITextDecoration() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    const Glib::ustring write( guint const flags = SP_STYLE_FLAG_IFSET,
                                       SPStyleSrc const &style_src_req = SPStyleSrc::STYLE_PROP,
                                       SPIBase const *const base = nullptr ) const override;
    void clear() override {
        SPIBase::clear();
        style_td = nullptr;
    }

    void cascade( const SPIBase* const parent ) override;
    void merge(   const SPIBase* const parent ) override;

    SPITextDecoration& operator=(const SPITextDecoration& rhs) {
        SPIBase::operator=(rhs);
        return *this;
    }

    // Use CSS2 value
    bool operator==(const SPIBase& rhs) const override;

public:
    SPStyle* style_td = nullptr;   // Style to be used for drawing CSS2 text decorations
};


// These are used to implement text_decoration. The values are not saved to or read from SVG file
struct SPITextDecorationData {
    float   phase_length;          // length along text line,used for phase for dot/dash/wavy
    bool    tspan_line_start;      // is first  span on a line
    bool    tspan_line_end;        // is last span on a line
    float   tspan_width;           // from libnrtype, when it calculates spans
    float   ascender;              // the rest from tspan's font
    float   descender;
    float   underline_thickness;
    float   underline_position; 
    float   line_through_thickness;
    float   line_through_position;
};

/// Vector Effects.  THIS SHOULD BE A GENERIC CLASS
class SPIVectorEffect : public SPIBase
{

public:
    SPIVectorEffect()
        : SPIBase(false)
    {
        this->clear();
    }

    ~SPIVectorEffect() override
    = default;

    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        stroke = false;
        size   = false;
        rotate = false;
        fixed  = false;
    }

    // Does not inherit
    void cascade( const SPIBase* const parent ) override {};
    void merge(   const SPIBase* const parent ) override {};

    SPIVectorEffect& operator=(const SPIVectorEffect& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    bool stroke : 1;
    bool size   : 1;
    bool rotate : 1;
    bool fixed  : 1;
};

/// Custom stroke properties. Only used for -inkscape-stroke: hairline.
class SPIStrokeExtensions : public SPIBase
{

public:
    SPIStrokeExtensions()
        : hairline(false)
    {}

    ~SPIStrokeExtensions() override = default;
    void read( gchar const *str ) override;
    const Glib::ustring get_value() const override;
    void clear() override {
        SPIBase::clear();
        hairline = false;
    }

    // Does not inherit
    void cascade( const SPIBase* const parent ) override {};
    void merge(   const SPIBase* const parent ) override {};

    SPIStrokeExtensions& operator=(const SPIStrokeExtensions& rhs) = default;

    bool operator==(const SPIBase& rhs) const override;

  // To do: make private
public:
    bool hairline : 1;
};

#endif // SEEN_SP_STYLE_INTERNAL_H


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
