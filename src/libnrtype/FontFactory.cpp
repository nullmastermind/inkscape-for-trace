// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 *     fred
 *     bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#ifndef PANGO_ENABLE_ENGINE
#define PANGO_ENABLE_ENGINE
#endif

#include <unordered_map>

#include <glibmm/i18n.h>

#include <fontconfig/fontconfig.h>

#include <pango/pangofc-fontmap.h>
#include <pango/pangoft2.h>
#include <pango/pango-ot.h>

#include "io/sys.h"

#include "libnrtype/FontFactory.h"
#include "libnrtype/font-instance.h"
#include "libnrtype/OpenTypeUtil.h"

typedef std::unordered_map<PangoFontDescription*, font_instance*, font_descr_hash, font_descr_equal> FaceMapType;

// need to avoid using the size field
size_t font_descr_hash::operator()( PangoFontDescription *const &x) const {
    int h = 0;
    char const *theF = sp_font_description_get_family(x);
    h += (theF)?g_str_hash(theF):0;
    h *= 1128467;
    h += (int)pango_font_description_get_style(x);
    h *= 1128467;
    h += (int)pango_font_description_get_variant(x);
    h *= 1128467;
    h += (int)pango_font_description_get_weight(x);
    h *= 1128467;
    h += (int)pango_font_description_get_stretch(x);
#if PANGO_VERSION_CHECK(1,41,1)
    char const *theV = pango_font_description_get_variations(x);
    h *= 1128467;
    h += (theV)?g_str_hash(theV):0;
#endif
    return h;
}

bool  font_descr_equal::operator()( PangoFontDescription *const&a, PangoFontDescription *const &b) const {
    //if ( pango_font_description_equal(a,b) ) return true;
    char const *fa = sp_font_description_get_family(a);
    char const *fb = sp_font_description_get_family(b);
    if ( ( fa && fb == nullptr ) || ( fb && fa == nullptr ) ) return false;
    if ( fa && fb && strcmp(fa,fb) != 0 ) return false;
    if ( pango_font_description_get_style(a) != pango_font_description_get_style(b) ) return false;
    if ( pango_font_description_get_variant(a) != pango_font_description_get_variant(b) ) return false;
    if ( pango_font_description_get_weight(a) != pango_font_description_get_weight(b) ) return false;
    if ( pango_font_description_get_stretch(a) != pango_font_description_get_stretch(b) ) return false;
#if PANGO_VERSION_CHECK(1,41,1)
    if ( g_strcmp0( pango_font_description_get_variations(a),
                    pango_font_description_get_variations(b) ) != 0 ) return false;
#endif
    return true;
}

// User must free return value.
PangoFontDescription* ink_font_description_from_style(SPStyle const *style)
{
    PangoFontDescription *descr = pango_font_description_new();

    pango_font_description_set_family(descr, style->font_family.value());

    // This duplicates Layout::EnumConversionItem... perhaps we can share code?
    switch ( style->font_style.computed ) {
        case SP_CSS_FONT_STYLE_ITALIC:
            pango_font_description_set_style(descr, PANGO_STYLE_ITALIC);
            break;

        case SP_CSS_FONT_STYLE_OBLIQUE:
            pango_font_description_set_style(descr, PANGO_STYLE_OBLIQUE);
            break;

        case SP_CSS_FONT_STYLE_NORMAL:
        default:
            pango_font_description_set_style(descr, PANGO_STYLE_NORMAL);
            break;
    }

    switch( style->font_weight.computed ) {
        case SP_CSS_FONT_WEIGHT_100:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_THIN);
            break;

        case SP_CSS_FONT_WEIGHT_200:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_ULTRALIGHT);
            break;

        case SP_CSS_FONT_WEIGHT_300:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_LIGHT);
            break;

        case SP_CSS_FONT_WEIGHT_400:
        case SP_CSS_FONT_WEIGHT_NORMAL:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_NORMAL);
            break;

        case SP_CSS_FONT_WEIGHT_500:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_MEDIUM);
            break;

        case SP_CSS_FONT_WEIGHT_600:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_SEMIBOLD);
            break;

        case SP_CSS_FONT_WEIGHT_700:
        case SP_CSS_FONT_WEIGHT_BOLD:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_BOLD);
            break;

        case SP_CSS_FONT_WEIGHT_800:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_ULTRABOLD);
            break;

        case SP_CSS_FONT_WEIGHT_900:
            pango_font_description_set_weight(descr, PANGO_WEIGHT_HEAVY);
            break;

        case SP_CSS_FONT_WEIGHT_LIGHTER:
        case SP_CSS_FONT_WEIGHT_BOLDER:
        default:
            g_warning("FaceFromStyle: Unrecognized font_weight.computed value");
            pango_font_description_set_weight(descr, PANGO_WEIGHT_NORMAL);
            break;
    }
    // PANGO_WIEGHT_ULTRAHEAVY not used (not CSS2)

    switch (style->font_stretch.computed) {
        case SP_CSS_FONT_STRETCH_ULTRA_CONDENSED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_ULTRA_CONDENSED);
            break;

        case SP_CSS_FONT_STRETCH_EXTRA_CONDENSED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_EXTRA_CONDENSED);
            break;

        case SP_CSS_FONT_STRETCH_CONDENSED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_CONDENSED);
            break;

        case SP_CSS_FONT_STRETCH_SEMI_CONDENSED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_SEMI_CONDENSED);
            break;

        case SP_CSS_FONT_STRETCH_NORMAL:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_NORMAL);
            break;

        case SP_CSS_FONT_STRETCH_SEMI_EXPANDED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_SEMI_EXPANDED);
            break;

        case SP_CSS_FONT_STRETCH_EXPANDED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_EXPANDED);
            break;

        case SP_CSS_FONT_STRETCH_EXTRA_EXPANDED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_EXTRA_EXPANDED);
            break;

        case SP_CSS_FONT_STRETCH_ULTRA_EXPANDED:
            pango_font_description_set_stretch(descr, PANGO_STRETCH_ULTRA_EXPANDED);

        case SP_CSS_FONT_STRETCH_WIDER:
        case SP_CSS_FONT_STRETCH_NARROWER:
        default:
            g_warning("FaceFromStyle: Unrecognized font_stretch.computed value");
            pango_font_description_set_stretch(descr, PANGO_STRETCH_NORMAL);
            break;
    }

    switch ( style->font_variant.computed ) {
        case SP_CSS_FONT_VARIANT_SMALL_CAPS:
            pango_font_description_set_variant(descr, PANGO_VARIANT_SMALL_CAPS);
            break;

        case SP_CSS_FONT_VARIANT_NORMAL:
        default:
            pango_font_description_set_variant(descr, PANGO_VARIANT_NORMAL);
            break;
    }

#if PANGO_VERSION_CHECK(1,41,1)
    // Check if not empty as Pango will add @ to string even if empty (bug in Pango?).
    if (!style->font_variation_settings.axes.empty()) {
        pango_font_description_set_variations(descr, style->font_variation_settings.toString().c_str());
    }
#endif

    return descr;
}

/////////////////// helper functions

static void noop(...) {}
//#define PANGO_DEBUG g_print
#define PANGO_DEBUG noop


///////////////////// FontFactory
#ifndef USE_PANGO_WIN32
// the substitute function to tell fontconfig to enforce outline fonts
static void FactorySubstituteFunc(FcPattern *pattern,gpointer /*data*/)
{
    FcPatternAddBool(pattern, "FC_OUTLINE",FcTrue);
    //char *fam = NULL;
    //FcPatternGetString(pattern, "FC_FAMILY",0, &fam);
    //printf("subst_f on %s\n",fam);
}
#endif


font_factory *font_factory::lUsine = nullptr;

font_factory *font_factory::Default()
{
    if ( lUsine == nullptr ) lUsine = new font_factory;
    return lUsine;
}

font_factory::font_factory() :
    nbEnt(0), // Note: this "ents" cache only keeps fonts from being unreffed, does not speed up access
    maxEnt(32),
    ents(static_cast<font_entry*>(g_malloc(maxEnt*sizeof(font_entry)))),
#ifdef USE_PANGO_WIN32
    fontServer(pango_win32_font_map_for_display()),
    pangoFontCache(pango_win32_font_map_get_font_cache(fontServer)),
    hScreenDC(pango_win32_get_dc()),
#else
    fontServer(pango_ft2_font_map_new()),
#endif
    fontContext(pango_font_map_create_context(fontServer)),
    fontSize(512),
    loadedPtr(new FaceMapType())
{
#ifndef USE_PANGO_WIN32
    pango_ft2_font_map_set_resolution(PANGO_FT2_FONT_MAP(fontServer),
                                      72, 72);
    pango_ft2_font_map_set_default_substitute(PANGO_FT2_FONT_MAP(fontServer),
                                              FactorySubstituteFunc,
                                              this,
                                              nullptr);
#endif

}

font_factory::~font_factory()
{
    for (int i = 0;i < nbEnt;i++) ents[i].f->Unref();
    if ( ents ) g_free(ents);

    g_object_unref(fontServer);
#ifdef USE_PANGO_WIN32
    pango_win32_shutdown_display();
#else
    //pango_ft2_shutdown_display();
#endif
    //g_object_unref(fontContext);

    if (loadedPtr) {
        FaceMapType* tmp = static_cast<FaceMapType*>(loadedPtr);
        delete tmp;
        loadedPtr = nullptr;
    }
}


Glib::ustring font_factory::ConstructFontSpecification(PangoFontDescription *font)
{
    Glib::ustring pangoString;

    g_assert(font);

    if (font) {
        // Once the format for the font specification is decided, it must be
        // kept.. if it is absolutely necessary to change it, the attribute
        // it is written to needs to have a new version so the legacy files
        // can be read.

        PangoFontDescription *copy = pango_font_description_copy(font);

        pango_font_description_unset_fields (copy, PANGO_FONT_MASK_SIZE);
        char * copyAsString = pango_font_description_to_string(copy);
        pangoString = copyAsString;
        g_free(copyAsString);
        copyAsString = nullptr;

        pango_font_description_free(copy);

    }

    return pangoString;
}

Glib::ustring font_factory::ConstructFontSpecification(font_instance *font)
{
    Glib::ustring pangoString;

    g_assert(font);

    if (font) {
        pangoString = ConstructFontSpecification(font->descr);
    }

    return pangoString;
}

/*
 * Wrap calls to pango_font_description_get_family
 * and replace some of the pango font names with generic css names
 * http://www.w3.org/TR/2008/REC-CSS2-20080411/fonts.html#generic-font-families
 *
 * This function should be called in place of pango_font_description_get_family()
 */
const char *sp_font_description_get_family(PangoFontDescription const *fontDescr) {

    static std::map<Glib::ustring, Glib::ustring> fontNameMap;
    std::map<Glib::ustring, Glib::ustring>::iterator it;

    if (fontNameMap.empty()) {
        fontNameMap.insert(std::make_pair("Sans", "sans-serif"));
        fontNameMap.insert(std::make_pair("Serif", "serif"));
        fontNameMap.insert(std::make_pair("Monospace", "monospace"));
    }

    const char *pangoFamily = pango_font_description_get_family(fontDescr);

    if (pangoFamily && ((it = fontNameMap.find(pangoFamily)) != fontNameMap.end())) {
        return (it->second).c_str();
    }

    return pangoFamily;
}

Glib::ustring font_factory::GetUIFamilyString(PangoFontDescription const *fontDescr)
{
    Glib::ustring family;

    g_assert(fontDescr);

    if (fontDescr) {
        // For now, keep it as family name taken from pango
        const char *pangoFamily = sp_font_description_get_family(fontDescr);

        if( pangoFamily ) {
            family = pangoFamily;
        }
    }

    return family;
}

Glib::ustring font_factory::GetUIStyleString(PangoFontDescription const *fontDescr)
{
    Glib::ustring style;

    g_assert(fontDescr);

    if (fontDescr) {
        PangoFontDescription *fontDescrCopy = pango_font_description_copy(fontDescr);

        pango_font_description_unset_fields(fontDescrCopy, PANGO_FONT_MASK_FAMILY);
        pango_font_description_unset_fields(fontDescrCopy, PANGO_FONT_MASK_SIZE);

        // For now, keep it as style name taken from pango
        char *fontDescrAsString = pango_font_description_to_string(fontDescrCopy);
        style = fontDescrAsString;
        g_free(fontDescrAsString);
        fontDescrAsString = nullptr;
        pango_font_description_free(fontDescrCopy);
    }

    return style;
}


/////

// Calculate a Style "value" based on CSS values for ordering styles.
static int StyleNameValue( const Glib::ustring &style )
{

    PangoFontDescription *pfd = pango_font_description_from_string ( style.c_str() );
    int value =
        pango_font_description_get_weight ( pfd ) * 1000000 +
        pango_font_description_get_style  ( pfd ) *   10000 +
        pango_font_description_get_stretch( pfd ) *     100 +
        pango_font_description_get_variant( pfd );
    pango_font_description_free ( pfd );
    return value;
}

// Determines order in which styles are presented (sorted by CSS style values)
//static bool StyleNameCompareInternal(const StyleNames &style1, const StyleNames &style2)
//{
//   return( StyleNameValue( style1.CssName ) < StyleNameValue( style2.CssName ) );
//}

static gint StyleNameCompareInternalGlib(gconstpointer a, gconstpointer b)
{
    return( StyleNameValue( ((StyleNames *)a)->CssName  ) <
            StyleNameValue( ((StyleNames *)b)->CssName  ) ? -1 : 1 );
}

static bool ustringPairSort(std::pair<PangoFontFamily*, Glib::ustring> const& first, std::pair<PangoFontFamily*, Glib::ustring> const& second)
{
    // well, this looks weird.
    return first.second < second.second;
}

void font_factory::GetUIFamilies(std::vector<PangoFontFamily *>& out)
{
    // Gather the family names as listed by Pango
    PangoFontFamily** families = nullptr;
    int numFamilies = 0;
    pango_font_map_list_families(fontServer, &families, &numFamilies);
    
    std::vector<std::pair<PangoFontFamily *, Glib::ustring> > sorted;

    // not size_t
    for (int currentFamily = 0; currentFamily < numFamilies; ++currentFamily) {
        const char* displayName = pango_font_family_get_name(families[currentFamily]);
        
        if (displayName == nullptr || *displayName == '\0') {
            std::cerr << "font_factory::GetUIFamilies: Missing displayName! " << std::endl;
            continue;
        }
        if (!g_utf8_validate(displayName, -1, nullptr)) {
            // TODO: can can do anything about this or does it always indicate broken fonts that should not be used?
            std::cerr << "font_factory::GetUIFamilies: Illegal characters in displayName. ";
            std::cerr << "Ignoring font '" << displayName << "'" << std::endl;
            continue;
        }
        sorted.emplace_back(families[currentFamily], displayName);
    }

    std::sort(sorted.begin(), sorted.end(), ustringPairSort);
    
    for (auto & i : sorted) {
        out.push_back(i.first);
    }
}

GList* font_factory::GetUIStyles(PangoFontFamily * in)
{
    GList* ret = nullptr;
    // Gather the styles for this family
    PangoFontFace** faces = nullptr;
    int numFaces = 0;
    if (in == nullptr) {
        std::cerr << "font_factory::GetUIStyles(): PangoFontFamily is NULL" << std::endl;
        return ret;
    }

    pango_font_family_list_faces(in, &faces, &numFaces);

    for (int currentFace = 0; currentFace < numFaces; currentFace++) {

        // If the face has a name, describe it, and then use the
        // description to get the UI family and face strings
        const gchar* displayName = pango_font_face_get_face_name(faces[currentFace]);
        // std::cout << "Display Name: " << displayName << std::endl;
        if (displayName == nullptr || *displayName == '\0') {
            std::cerr << "font_factory::GetUIStyles: Missing displayName! " << std::endl;
            continue;
        }

        PangoFontDescription *faceDescr = pango_font_face_describe(faces[currentFace]);
        if (faceDescr) {
            Glib::ustring familyUIName = GetUIFamilyString(faceDescr);
            Glib::ustring styleUIName = GetUIStyleString(faceDescr);
            // std::cout << "  " << familyUIName << "  styleUIName: " << styleUIName << "  displayName: " << displayName << std::endl;

            // Disable synthesized (faux) font faces except for CSS generic faces
            if (pango_font_face_is_synthesized(faces[currentFace]) ) {
                if (familyUIName.compare( "sans-serif" ) != 0 &&
                    familyUIName.compare( "serif"      ) != 0 &&
                    familyUIName.compare( "monospace"  ) != 0 &&
                    familyUIName.compare( "fantasy"    ) != 0 &&
                    familyUIName.compare( "cursive"    ) != 0 ) {
                    continue;
                }
            }

            // Pango breaks the 1 to 1 mapping between Pango weights and CSS weights by
            // adding Semi-Light (as of 1.36.7), Book (as of 1.24), and Ultra-Heavy (as of
            // 1.24). We need to map these weights to CSS weights. Book and Ultra-Heavy
            // are rarely used. Semi-Light (350) is problematic as it is halfway between
            // Light (300) and Normal (400) and if care is not taken it is converted to
            // Normal, rather than Light.
            //
            // Note: The ultimate solution to handling various weight in the same
            // font family is to support the @font rules from CSS.
            //
            // Additional notes, helpful for debugging:
            //   Pango's FC backend:
            //     Weights defined in fontconfig/fontconfig.h
            //     String equivalents in src/fcfreetype.c
            //     Weight set from os2->usWeightClass
            //   Use Fontforge: Element->Font Info...->OS/2->Misc->Weight Class to check font weight
            size_t f = styleUIName.find( "Book" );
            if( f != Glib::ustring::npos ) {
                styleUIName.replace( f, 4, "Normal" );
            }
            f = styleUIName.find( "Semi-Light" );
            if( f != Glib::ustring::npos ) {
                styleUIName.replace( f, 10, "Light" );
            }
            f = styleUIName.find( "Ultra-Heavy" );
            if( f != Glib::ustring::npos ) {
                styleUIName.replace( f, 11, "Heavy" );
            }

            bool exists = false;
            for(GList *temp = ret; temp; temp = temp->next) {
                if( ((StyleNames*)temp->data)->CssName.compare( styleUIName ) == 0 ) {
                    exists = true;
                    std::cerr << "Warning: Font face with same CSS values already added: "
                              << familyUIName << " " << styleUIName
                              << " (" << ((StyleNames*)temp->data)->DisplayName
                              << ", " << displayName << ")" << std::endl;
                    break;
                }
            }

            if (!exists && !familyUIName.empty() && !styleUIName.empty()) {
                // Add the style information
                ret = g_list_append(ret, new StyleNames(styleUIName, displayName));
            }
        }
        pango_font_description_free(faceDescr);
    }
    g_free(faces);

    // Sort the style lists
    ret = g_list_sort( ret, StyleNameCompareInternalGlib );
    return ret;
}


font_instance* font_factory::FaceFromStyle(SPStyle const *style)
{
    font_instance *font = nullptr;

    g_assert(style);

    if (style) {

        //  First try to use the font specification if it is set
        char const *val;
        if (style->font_specification.set
            && (val = style->font_specification.value())
            && val[0]) {

            font = FaceFromFontSpecification(val);
        }

        // If that failed, try using the CSS information in the style
        if (!font) {
            PangoFontDescription* temp_descr =
                ink_font_description_from_style(style);
            font = Face(temp_descr);
            pango_font_description_free(temp_descr);
        }
    }

    return font;
}

font_instance *font_factory::FaceFromDescr(char const *family, char const *style)
{
    PangoFontDescription *temp_descr = pango_font_description_from_string(style);
    pango_font_description_set_family(temp_descr,family);
    font_instance *res = Face(temp_descr);
    pango_font_description_free(temp_descr);
    return res;
}

font_instance* font_factory::FaceFromPangoString(char const *pangoString)
{
    font_instance *fontInstance = nullptr;

    g_assert(pangoString);

    if (pangoString) {

        // Create a font description from the string - this may fail or
        // produce unexpected results if the string does not have a good format
        PangoFontDescription *descr = pango_font_description_from_string(pangoString);

        if (descr) {
            if (sp_font_description_get_family(descr) != nullptr) {
                fontInstance = Face(descr);
            }
            pango_font_description_free(descr);
        }
    }

    return fontInstance;
}

font_instance* font_factory::FaceFromFontSpecification(char const *fontSpecification)
{
    font_instance *font = nullptr;

    g_assert(fontSpecification);

    if (fontSpecification) {
        // How the string is used to reconstruct a font depends on how it
        // was constructed in ConstructFontSpecification.  As it stands,
        // the font specification is a pango-created string
        font = FaceFromPangoString(fontSpecification);
    }

    return font;
}

font_instance *font_factory::Face(PangoFontDescription *descr, bool canFail)
{
#ifdef USE_PANGO_WIN32
    // damn Pango fudges the size, so we need to unfudge. See source of pango_win32_font_map_init()
    pango_font_description_set_size(descr, (int) (fontSize*PANGO_SCALE*72/GetDeviceCaps(pango_win32_get_dc(),LOGPIXELSY))); // mandatory huge size (hinting workaround)
#else
    pango_font_description_set_size(descr, (int) (fontSize*PANGO_SCALE)); // mandatory huge size (hinting workaround)
#endif

    font_instance *res = nullptr;

    FaceMapType& loadedFaces = *static_cast<FaceMapType*>(loadedPtr);
    if ( loadedFaces.find(descr) == loadedFaces.end() ) {
        // not yet loaded
        PangoFont *nFace = nullptr;

        // workaround for bug #1025565.
        // fonts without families blow up Pango.
        if (sp_font_description_get_family(descr) != nullptr) {
            nFace = pango_font_map_load_font(fontServer,fontContext,descr);
        }
        else {
            g_warning("%s", _("Ignoring font without family that will crash Pango"));
        }

        if ( nFace ) {
            // duplicate FcPattern, the hard way
            res = new font_instance();
            // store the descr of the font we asked for, since this is the key where we intend
            // to put the font_instance at in the unordered_map.  the descr of the returned
            // pangofont may differ from what was asked, so we don't know (at this
            // point) whether loadedFaces[that_descr] is free or not (and overwriting
            // an entry will bring deallocation problems)
            res->descr = pango_font_description_copy(descr);
            res->parent = this;
            res->InstallFace(nFace);
            if ( res->pFont == nullptr ) {
                // failed to install face -> bitmap font
                // printf("face failed\n");
                res->parent = nullptr;
                delete res;
                res = nullptr;
                if ( canFail ) {
                    char *tc = pango_font_description_to_string(descr);
                    PANGO_DEBUG("falling back from %s to 'sans-serif' because InstallFace failed\n",tc);
                    g_free(tc);
                    pango_font_description_set_family(descr,"sans-serif");
                    res = Face(descr,false);
                }
            } else {
                loadedFaces[res->descr]=res;
                res->Ref();
                AddInCache(res);
            }
        } else {
            // no match
            if ( canFail ) {
                PANGO_DEBUG("falling back to 'sans-serif'\n");
                PangoFontDescription *new_descr = pango_font_description_new();
                pango_font_description_set_family(new_descr, "sans-serif");
                res = Face(new_descr, false);
                pango_font_description_free(new_descr);
            } else {
                g_critical("Could not load any face for font '%s'.", pango_font_description_to_string(descr));
            }
        }

    } else {
        // already here
        res = loadedFaces[descr];
        res->Ref();
        AddInCache(res);
    }
    if (res) {
        res->InitTheFace();
    }
    return res;
}

// Not used, need to add variations if ever used.
// font_instance *font_factory::Face(char const *family, int variant, int style, int weight, int stretch, int /*size*/, int /*spacing*/)
// {
//     // std::cout << "font_factory::Face(family, variant, style, weight, stretch,)" << std::endl;
//     PangoFontDescription *temp_descr = pango_font_description_new();
//     pango_font_description_set_family(temp_descr,family);
//     pango_font_description_set_weight(temp_descr,(PangoWeight)weight);
//     pango_font_description_set_stretch(temp_descr,(PangoStretch)stretch);
//     pango_font_description_set_style(temp_descr,(PangoStyle)style);
//     pango_font_description_set_variant(temp_descr,(PangoVariant)variant);
//     font_instance *res = Face(temp_descr);
//     pango_font_description_free(temp_descr);
//     return res;
// }

void font_factory::UnrefFace(font_instance *who)
{
    if ( who ) {
        FaceMapType& loadedFaces = *static_cast<FaceMapType*>(loadedPtr);

        if ( loadedFaces.find(who->descr) == loadedFaces.end() ) {
            // not found
            char *tc = pango_font_description_to_string(who->descr);
            g_warning("unrefFace %p=%s: failed\n",who,tc);
            g_free(tc);
        } else {
            loadedFaces.erase(loadedFaces.find(who->descr));
            //            printf("unrefFace %p: success\n",who);
        }
    }
}

void font_factory::AddInCache(font_instance *who)
{
    if ( who == nullptr ) return;
    for (int i = 0;i < nbEnt;i++) ents[i].age *= 0.9;
    for (int i = 0;i < nbEnt;i++) {
        if ( ents[i].f == who ) {
            //            printf("present\n");
            ents[i].age += 1.0;
            return;
        }
    }
    if ( nbEnt > maxEnt ) {
        printf("cache sur-plein?\n");
        return;
    }
    who->Ref();
    if ( nbEnt == maxEnt ) { // cache is filled, unref the oldest-accessed font in it
        int    bi = 0;
        double ba = ents[bi].age;
        for (int i = 1;i < nbEnt;i++) {
            if ( ents[i].age < ba ) {
                bi = i;
                ba = ents[bi].age;
            }
        }
        ents[bi].f->Unref();
        ents[bi]=ents[--nbEnt];
    }
    ents[nbEnt].f = who;
    ents[nbEnt].age = 1.0;
    nbEnt++;
}

void font_factory::AddFontsDir(char const *utf8dir)
{
#ifdef USE_PANGO_WIN32
    g_info("Adding additional font directories only supported for fontconfig backend.");
#else
    if (!Inkscape::IO::file_test(utf8dir, G_FILE_TEST_IS_DIR)) {
        g_warning("Fonts dir '%s' does not exist and will be ignored.", utf8dir);
        return;
    }

    gchar *dir;
# ifdef _WIN32
    dir = g_win32_locale_filename_from_utf8(utf8dir);
# else
    dir = g_filename_from_utf8(utf8dir, -1, nullptr, nullptr, nullptr);
# endif

    FcConfig *conf = nullptr;
# if PANGO_VERSION_CHECK(1,38,0)
    conf = pango_fc_font_map_get_config(PANGO_FC_FONT_MAP(fontServer));
# endif
    FcBool res = FcConfigAppFontAddDir(conf, (FcChar8 const *)dir);
    if (res == FcTrue) {
        g_info("Fonts dir '%s' added successfully.", utf8dir);
# if PANGO_VERSION_CHECK(1,38,0)
        pango_fc_font_map_config_changed(PANGO_FC_FONT_MAP(fontServer));
# endif
    } else {
        g_warning("Could not add fonts dir '%s'.", utf8dir);
    }

    g_free(dir);
#endif
}

void font_factory::AddFontFile(char const *utf8file)
{
#ifdef USE_PANGO_WIN32
    g_info("Adding additional font only supported for fontconfig backend.");
#else
    if (!Inkscape::IO::file_test(utf8file, G_FILE_TEST_IS_REGULAR)) {
        g_warning("Font file '%s' does not exist and will be ignored.", utf8file);
        return;
    }

    gchar *file;
# ifdef _WIN32
    file = g_win32_locale_filename_from_utf8(utf8file);
# else
    file = g_filename_from_utf8(utf8file, -1, nullptr, nullptr, nullptr);
# endif

    FcConfig *conf = nullptr;
# if PANGO_VERSION_CHECK(1,38,0)
    conf = pango_fc_font_map_get_config(PANGO_FC_FONT_MAP(fontServer));
# endif
    FcBool res = FcConfigAppFontAddFile(conf, (FcChar8 const *)file);
    if (res == FcTrue) {
        g_info("Font file '%s' added successfully.", utf8file);
# if PANGO_VERSION_CHECK(1,38,0)
        pango_fc_font_map_config_changed(PANGO_FC_FONT_MAP(fontServer));
# endif
    } else {
        g_warning("Could not add font file '%s'.", utf8file);
    }

    g_free(file);
#endif
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
