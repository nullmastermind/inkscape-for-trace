// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#define noDEBUG_LCMS

#include <gdkmm/rgba.h>

#include <glib/gstdio.h>
#include <fcntl.h>
#include <glib/gi18n.h>

#ifdef DEBUG_LCMS
#include <gtk/gtk.h>
#endif // DEBUG_LCMS

#include <unistd.h>
#include <cstring>
#include <utility>
#include <io/sys.h>
#include <io/resource.h>

#ifdef _WIN32
#include <windows.h>
#endif

#if HAVE_LIBLCMS2
#  include <lcms2.h>
#endif // HAVE_LIBLCMS2

#include "xml/repr.h"
#include "color.h"
#include "color-profile.h"
#include "cms-system.h"
#include "color-profile-cms-fns.h"
#include "attributes.h"
#include "inkscape.h"
#include "document.h"
#include "preferences.h"
#include <glibmm/checksum.h>
#include <glibmm/convert.h>
#include "uri.h"

#ifdef _WIN32
#include <icm.h>
#endif // _WIN32

using Inkscape::ColorProfile;
using Inkscape::ColorProfileImpl;

namespace
{
#if defined(HAVE_LIBLCMS2)
cmsHPROFILE getSystemProfileHandle();
cmsHPROFILE getProofProfileHandle();
void loadProfiles();
Glib::ustring getNameFromProfile(cmsHPROFILE profile);
#endif // defined(HAVE_LIBLCMS2)
}

#ifdef DEBUG_LCMS
extern guint update_in_progress;
#define DEBUG_MESSAGE_SCISLAC(key, ...) \
{\
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();\
    bool dump = prefs->getBool(Glib::ustring("/options/scislac/") + #key);\
    bool dumpD = prefs->getBool(Glib::ustring("/options/scislac/") + #key"D");\
    bool dumpD2 = prefs->getBool(Glib::ustring("/options/scislac/") + #key"D2");\
    dumpD &= ( (update_in_progress == 0) || dumpD2 );\
    if ( dump )\
    {\
        g_message( __VA_ARGS__ );\
\
    }\
    if ( dumpD )\
    {\
        GtkWidget *dialog = gtk_message_dialog_new(NULL,\
                                                   GTK_DIALOG_DESTROY_WITH_PARENT, \
                                                   GTK_MESSAGE_INFO,    \
                                                   GTK_BUTTONS_OK,      \
                                                   __VA_ARGS__          \
                                                   );\
        g_signal_connect_swapped(dialog, "response",\
                                 G_CALLBACK(gtk_widget_destroy),        \
                                 dialog);                               \
        gtk_widget_show_all( dialog );\
    }\
}


#define DEBUG_MESSAGE(key, ...)\
{\
    g_message( __VA_ARGS__ );\
}

#else
#define DEBUG_MESSAGE_SCISLAC(key, ...)
#define DEBUG_MESSAGE(key, ...)
#endif // DEBUG_LCMS

namespace Inkscape {

class ColorProfileImpl {
public:
#if defined(HAVE_LIBLCMS2)
    static cmsHPROFILE _sRGBProf;
    static cmsHPROFILE _NullProf;
#endif // defined(HAVE_LIBLCMS2)

    ColorProfileImpl();

#if defined(HAVE_LIBLCMS2)
    static cmsUInt32Number _getInputFormat( cmsColorSpaceSignature space );

    static cmsHPROFILE getNULLProfile();
    static cmsHPROFILE getSRGBProfile();

    void _clearProfile();

    cmsHPROFILE _profHandle;
    cmsProfileClassSignature _profileClass;
    cmsColorSpaceSignature _profileSpace;
    cmsHTRANSFORM _transf;
    cmsHTRANSFORM _revTransf;
    cmsHTRANSFORM _gamutTransf;
#endif // defined(HAVE_LIBLCMS2)
};

#if defined(HAVE_LIBLCMS2)
cmsColorSpaceSignature asICColorSpaceSig(ColorSpaceSig const & sig)
{
    return ColorSpaceSigWrapper(sig);
}

cmsProfileClassSignature asICColorProfileClassSig(ColorProfileClassSig const & sig)
{
    return ColorProfileClassSigWrapper(sig);
}
#endif //  defined(HAVE_LIBLCMS2)

} // namespace Inkscape

ColorProfileImpl::ColorProfileImpl()
#if defined(HAVE_LIBLCMS2)
	:
    _profHandle(nullptr),
    _profileClass(cmsSigInputClass),
    _profileSpace(cmsSigRgbData),
    _transf(nullptr),
    _revTransf(nullptr),
    _gamutTransf(nullptr)
#endif // defined(HAVE_LIBLCMS2)
{
}

#if defined(HAVE_LIBLCMS2)

cmsHPROFILE ColorProfileImpl::_sRGBProf = nullptr;

cmsHPROFILE ColorProfileImpl::getSRGBProfile() {
    if ( !_sRGBProf ) {
        _sRGBProf = cmsCreate_sRGBProfile();
    }
    return ColorProfileImpl::_sRGBProf;
}

cmsHPROFILE ColorProfileImpl::_NullProf = nullptr;

cmsHPROFILE ColorProfileImpl::getNULLProfile() {
    if ( !_NullProf ) {
        _NullProf = cmsCreateNULLProfile();
    }
    return _NullProf;
}

#endif // defined(HAVE_LIBLCMS2)

ColorProfile::FilePlusHome::FilePlusHome(Glib::ustring filename, bool isInHome) : filename(std::move(filename)), isInHome(isInHome) {
}

ColorProfile::FilePlusHome::FilePlusHome(const ColorProfile::FilePlusHome &filePlusHome) : FilePlusHome(filePlusHome.filename, filePlusHome.isInHome) {
}

bool ColorProfile::FilePlusHome::operator<(FilePlusHome const &other) const {
    // if one is from home folder, other from global folder, sort home folder first. cf bug 1457126
    bool result;
    if (this->isInHome != other.isInHome) result = this->isInHome;
    else                                  result = this->filename < other.filename;
    return result;
}

ColorProfile::FilePlusHomeAndName::FilePlusHomeAndName(ColorProfile::FilePlusHome filePlusHome, Glib::ustring name)
                                                      : FilePlusHome(filePlusHome), name(std::move(name)) {
}

bool ColorProfile::FilePlusHomeAndName::operator<(ColorProfile::FilePlusHomeAndName const &other) const {
    bool result;
    if (this->isInHome != other.isInHome) result = this->isInHome;
    else                                  result = this->name < other.name;
    return result;
}


ColorProfile::ColorProfile() : SPObject() {
    this->impl = new ColorProfileImpl();

    this->href = nullptr;
    this->local = nullptr;
    this->name = nullptr;
    this->intentStr = nullptr;
    this->rendering_intent = Inkscape::RENDERING_INTENT_UNKNOWN;
}

ColorProfile::~ColorProfile() = default;

bool ColorProfile::operator<(ColorProfile const &other) const {
    gchar *a_name_casefold = g_utf8_casefold(this->name, -1 );
    gchar *b_name_casefold = g_utf8_casefold(other.name, -1 );
    int result = g_strcmp0(a_name_casefold, b_name_casefold);
    g_free(a_name_casefold);
    g_free(b_name_casefold);
    return result < 0;
}

/**
 * Callback: free object
 */
void ColorProfile::release() {
    // Unregister ourselves
    if ( this->document ) {
        this->document->removeResource("iccprofile", this);
    }

    if ( this->href ) {
        g_free( this->href );
        this->href = nullptr;
    }

    if ( this->local ) {
        g_free( this->local );
        this->local = nullptr;
    }

    if ( this->name ) {
        g_free( this->name );
        this->name = nullptr;
    }

    if ( this->intentStr ) {
        g_free( this->intentStr );
        this->intentStr = nullptr;
    }

#if defined(HAVE_LIBLCMS2)
    this->impl->_clearProfile();
#endif // defined(HAVE_LIBLCMS2)

    delete this->impl;
    this->impl = nullptr;
}

#if defined(HAVE_LIBLCMS2)
void ColorProfileImpl::_clearProfile()
{
    _profileSpace = cmsSigRgbData;

    if ( _transf ) {
        cmsDeleteTransform( _transf );
        _transf = nullptr;
    }
    if ( _revTransf ) {
        cmsDeleteTransform( _revTransf );
        _revTransf = nullptr;
    }
    if ( _gamutTransf ) {
        cmsDeleteTransform( _gamutTransf );
        _gamutTransf = nullptr;
    }
    if ( _profHandle ) {
        cmsCloseProfile( _profHandle );
        _profHandle = nullptr;
    }
}
#endif // defined(HAVE_LIBLCMS2)

/**
 * Callback: set attributes from associated repr.
 */
void ColorProfile::build(SPDocument *document, Inkscape::XML::Node *repr) {
    g_assert(this->href == nullptr);
    g_assert(this->local == nullptr);
    g_assert(this->name == nullptr);
    g_assert(this->intentStr == nullptr);

    SPObject::build(document, repr);

    this->readAttr( "xlink:href" );
    this->readAttr( "id" );
    this->readAttr( "local" );
    this->readAttr( "name" );
    this->readAttr( "rendering-intent" );

    // Register
    if ( document ) {
        document->addResource( "iccprofile", this );
    }
}


/**
 * Callback: set attribute.
 */
void ColorProfile::set(SPAttributeEnum key, gchar const *value) {
    switch (key) {
        case SP_ATTR_XLINK_HREF:
            if ( this->href ) {
                g_free( this->href );
                this->href = nullptr;
            }
            if ( value ) {
                this->href = g_strdup( value );
                if ( *this->href ) {
#if defined(HAVE_LIBLCMS2)

                    // TODO open filename and URIs properly
                    //FILE* fp = fopen_utf8name( filename, "r" );
                    //LCMSAPI cmsHPROFILE   LCMSEXPORT cmsOpenProfileFromMem(LPVOID MemPtr, cmsUInt32Number dwSize);

                    // Try to open relative
                    SPDocument *doc = this->document;
                    if (!doc) {
                        doc = SP_ACTIVE_DOCUMENT;
                        g_warning("this has no document.  using active");
                    }
                    //# 1.  Get complete URI of document
                    gchar const *docbase = doc->getDocumentURI();

                    Inkscape::URI docUri("");
                    if (docbase) { // The file has already been saved
                        docUri = Inkscape::URI::from_native_filename(docbase);
                    }

                    this->impl->_clearProfile();

                    try {
                        auto hrefUri = Inkscape::URI(this->href, docUri);
                        auto contents = hrefUri.getContents();
                        this->impl->_profHandle = cmsOpenProfileFromMem(contents.data(), contents.size());
                    } catch (...) {
                        g_warning("Failed to open CMS profile URI '%.100s'", this->href);
                    }

                    if ( this->impl->_profHandle ) {
                        this->impl->_profileSpace = cmsGetColorSpace( this->impl->_profHandle );
                        this->impl->_profileClass = cmsGetDeviceClass( this->impl->_profHandle );
                    }
                    DEBUG_MESSAGE( lcmsOne, "cmsOpenProfileFromFile( '%s'...) = %p", fullname, (void*)this->impl->_profHandle );
#endif // defined(HAVE_LIBLCMS2)
                }
            }
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SP_ATTR_LOCAL:
            if ( this->local ) {
                g_free( this->local );
                this->local = nullptr;
            }
            this->local = g_strdup( value );
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SP_ATTR_NAME:
            if ( this->name ) {
                g_free( this->name );
                this->name = nullptr;
            }
            this->name = g_strdup( value );
            DEBUG_MESSAGE( lcmsTwo, "<color-profile> name set to '%s'", this->name );
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SP_ATTR_RENDERING_INTENT:
            if ( this->intentStr ) {
                g_free( this->intentStr );
                this->intentStr = nullptr;
            }
            this->intentStr = g_strdup( value );

            if ( value ) {
                if ( strcmp( value, "auto" ) == 0 ) {
                    this->rendering_intent = RENDERING_INTENT_AUTO;
                } else if ( strcmp( value, "perceptual" ) == 0 ) {
                    this->rendering_intent = RENDERING_INTENT_PERCEPTUAL;
                } else if ( strcmp( value, "relative-colorimetric" ) == 0 ) {
                    this->rendering_intent = RENDERING_INTENT_RELATIVE_COLORIMETRIC;
                } else if ( strcmp( value, "saturation" ) == 0 ) {
                    this->rendering_intent = RENDERING_INTENT_SATURATION;
                } else if ( strcmp( value, "absolute-colorimetric" ) == 0 ) {
                    this->rendering_intent = RENDERING_INTENT_ABSOLUTE_COLORIMETRIC;
                } else {
                    this->rendering_intent = RENDERING_INTENT_UNKNOWN;
                }
            } else {
                this->rendering_intent = RENDERING_INTENT_UNKNOWN;
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

        default:
        	SPObject::set(key, value);
            break;
    }
}

/**
 * Callback: write attributes to associated repr.
 */
Inkscape::XML::Node* ColorProfile::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:color-profile");
    }

    if ( (flags & SP_OBJECT_WRITE_ALL) || this->href ) {
        repr->setAttribute( "xlink:href", this->href );
    }

    if ( (flags & SP_OBJECT_WRITE_ALL) || this->local ) {
        repr->setAttribute( "local", this->local );
    }

    if ( (flags & SP_OBJECT_WRITE_ALL) || this->name ) {
        repr->setAttribute( "name", this->name );
    }

    if ( (flags & SP_OBJECT_WRITE_ALL) || this->intentStr ) {
        repr->setAttribute( "rendering-intent", this->intentStr );
    }

    SPObject::write(xml_doc, repr, flags);

    return repr;
}


#if defined(HAVE_LIBLCMS2)

struct MapMap {
    cmsColorSpaceSignature space;
    cmsUInt32Number inForm;
};

cmsUInt32Number ColorProfileImpl::_getInputFormat( cmsColorSpaceSignature space )
{
    MapMap possible[] = {
        {cmsSigXYZData,   TYPE_XYZ_16},
        {cmsSigLabData,   TYPE_Lab_16},
        //cmsSigLuvData
        {cmsSigYCbCrData, TYPE_YCbCr_16},
        {cmsSigYxyData,   TYPE_Yxy_16},
        {cmsSigRgbData,   TYPE_RGB_16},
        {cmsSigGrayData,  TYPE_GRAY_16},
        {cmsSigHsvData,   TYPE_HSV_16},
        {cmsSigHlsData,   TYPE_HLS_16},
        {cmsSigCmykData,  TYPE_CMYK_16},
        {cmsSigCmyData,   TYPE_CMY_16},
    };

    int index = 0;
    for ( guint i = 0; i < G_N_ELEMENTS(possible); i++ ) {
        if ( possible[i].space == space ) {
            index = i;
            break;
        }
    }

    return possible[index].inForm;
}

static int getLcmsIntent( guint svgIntent )
{
    int intent = INTENT_PERCEPTUAL;
    switch ( svgIntent ) {
        case Inkscape::RENDERING_INTENT_RELATIVE_COLORIMETRIC:
            intent = INTENT_RELATIVE_COLORIMETRIC;
            break;
        case Inkscape::RENDERING_INTENT_SATURATION:
            intent = INTENT_SATURATION;
            break;
        case Inkscape::RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
            intent = INTENT_ABSOLUTE_COLORIMETRIC;
            break;
        case Inkscape::RENDERING_INTENT_PERCEPTUAL:
        case Inkscape::RENDERING_INTENT_UNKNOWN:
        case Inkscape::RENDERING_INTENT_AUTO:
        default:
            intent = INTENT_PERCEPTUAL;
    }
    return intent;
}

static SPObject* bruteFind( SPDocument* document, gchar const* name )
{
    SPObject* result = nullptr;
    std::vector<SPObject *> current = document->getResourceList("iccprofile");
    for (std::vector<SPObject *>::const_iterator it = current.begin(); (!result) && (it != current.end()); ++it) {
        if ( IS_COLORPROFILE(*it) ) {
            ColorProfile* prof = COLORPROFILE(*it);
            if ( prof ) {
                if ( prof->name && (strcmp(prof->name, name) == 0) ) {
                    result = SP_OBJECT(*it);
                    break;
                }
            }
        }
    }

    return result;
}

cmsHPROFILE Inkscape::CMSSystem::getHandle( SPDocument* document, guint* intent, gchar const* name )
{
    cmsHPROFILE prof = nullptr;

    SPObject* thing = bruteFind( document, name );
    if ( thing ) {
        prof = COLORPROFILE(thing)->impl->_profHandle;
    }

    if ( intent ) {
        *intent = thing ? COLORPROFILE(thing)->rendering_intent : (guint)RENDERING_INTENT_UNKNOWN;
    }

    DEBUG_MESSAGE( lcmsThree, "<color-profile> queried for profile of '%s'. Returning %p with intent of %d", name, prof, (intent? *intent:0) );

    return prof;
}

Inkscape::ColorSpaceSig ColorProfile::getColorSpace() const {
    return ColorSpaceSigWrapper(impl->_profileSpace);
}

Inkscape::ColorProfileClassSig ColorProfile::getProfileClass() const {
    return ColorProfileClassSigWrapper(impl->_profileClass);
}

cmsHTRANSFORM ColorProfile::getTransfToSRGB8()
{
    if ( !impl->_transf && impl->_profHandle ) {
        int intent = getLcmsIntent(rendering_intent);
        impl->_transf = cmsCreateTransform( impl->_profHandle, ColorProfileImpl::_getInputFormat(impl->_profileSpace), ColorProfileImpl::getSRGBProfile(), TYPE_RGBA_8, intent, 0 );
    }
    return impl->_transf;
}

cmsHTRANSFORM ColorProfile::getTransfFromSRGB8()
{
    if ( !impl->_revTransf && impl->_profHandle ) {
        int intent = getLcmsIntent(rendering_intent);
        impl->_revTransf = cmsCreateTransform( ColorProfileImpl::getSRGBProfile(), TYPE_RGBA_8, impl->_profHandle, ColorProfileImpl::_getInputFormat(impl->_profileSpace), intent, 0 );
    }
    return impl->_revTransf;
}

cmsHTRANSFORM ColorProfile::getTransfGamutCheck()
{
    if ( !impl->_gamutTransf ) {
        impl->_gamutTransf = cmsCreateProofingTransform(ColorProfileImpl::getSRGBProfile(),
                                                        TYPE_BGRA_8,
                                                        ColorProfileImpl::getNULLProfile(),
                                                        TYPE_GRAY_8,
                                                        impl->_profHandle,
                                                        INTENT_RELATIVE_COLORIMETRIC,
                                                        INTENT_RELATIVE_COLORIMETRIC,
                                                        (cmsFLAGS_GAMUTCHECK | cmsFLAGS_SOFTPROOFING));
    }
    return impl->_gamutTransf;
}

bool ColorProfile::GamutCheck(SPColor color)
{
    guint32 val = color.toRGBA32(0);

#if HAVE_LIBLCMS2
    cmsUInt16Number oldAlarmCodes[cmsMAXCHANNELS] = {0};
    cmsGetAlarmCodes(oldAlarmCodes);
    cmsUInt16Number newAlarmCodes[cmsMAXCHANNELS] = {0};
    newAlarmCodes[0] = ~0;
    cmsSetAlarmCodes(newAlarmCodes);
#endif

    cmsUInt8Number outofgamut = 0;
    guchar check_color[4] = {
        static_cast<guchar>(SP_RGBA32_R_U(val)),
        static_cast<guchar>(SP_RGBA32_G_U(val)),
        static_cast<guchar>(SP_RGBA32_B_U(val)),
        255};

    cmsHTRANSFORM gamutCheck = ColorProfile::getTransfGamutCheck();
    if (gamutCheck) {
        cmsDoTransform(gamutCheck, &check_color, &outofgamut, 1);
    }

#if HAVE_LIBLCMS2
    cmsSetAlarmCodes(oldAlarmCodes);
#endif

    return (outofgamut != 0);
}

class ProfileInfo
{
public:
    ProfileInfo( cmsHPROFILE prof, Glib::ustring  path );

    Glib::ustring const& getName() {return _name;}
    Glib::ustring const& getPath() {return _path;}
    cmsColorSpaceSignature getSpace() {return _profileSpace;}
    cmsProfileClassSignature getClass() {return _profileClass;}

private:
    Glib::ustring _path;
    Glib::ustring _name;
    cmsColorSpaceSignature _profileSpace;
    cmsProfileClassSignature _profileClass;
};

ProfileInfo::ProfileInfo( cmsHPROFILE prof, Glib::ustring  path ) :
    _path(std::move( path )),
    _name( getNameFromProfile(prof) ),
    _profileSpace( cmsGetColorSpace( prof ) ),
    _profileClass( cmsGetDeviceClass( prof ) )
{
}



static std::vector<ProfileInfo> knownProfiles;

std::vector<Glib::ustring> Inkscape::CMSSystem::getDisplayNames()
{
    loadProfiles();
    std::vector<Glib::ustring> result;

    for (auto & knownProfile : knownProfiles) {
        if ( knownProfile.getClass() == cmsSigDisplayClass && knownProfile.getSpace() == cmsSigRgbData ) {
            result.push_back( knownProfile.getName() );
        }
    }
    std::sort(result.begin(), result.end());

    return result;
}

std::vector<Glib::ustring> Inkscape::CMSSystem::getSoftproofNames()
{
    loadProfiles();
    std::vector<Glib::ustring> result;

    for (auto & knownProfile : knownProfiles) {
        if ( knownProfile.getClass() == cmsSigOutputClass ) {
            result.push_back( knownProfile.getName() );
        }
    }
    std::sort(result.begin(), result.end());

    return result;
}

Glib::ustring Inkscape::CMSSystem::getPathForProfile(Glib::ustring const& name)
{
    loadProfiles();
    Glib::ustring result;

    for (auto & knownProfile : knownProfiles) {
        if ( name == knownProfile.getName() ) {
            result = knownProfile.getPath();
            break;
        }
    }

    return result;
}

void Inkscape::CMSSystem::doTransform(cmsHTRANSFORM transform, void *inBuf, void *outBuf, unsigned int size)
{
    cmsDoTransform(transform, inBuf, outBuf, size);
}

bool Inkscape::CMSSystem::isPrintColorSpace(ColorProfile const *profile)
{
    bool isPrint = false;
    if ( profile ) {
        ColorSpaceSigWrapper colorspace = profile->getColorSpace();
        isPrint = (colorspace == cmsSigCmykData) || (colorspace == cmsSigCmyData);
    }
    return isPrint;
}

gint Inkscape::CMSSystem::getChannelCount(ColorProfile const *profile)
{
    gint count = 0;
    if ( profile ) {
#if HAVE_LIBLCMS2
        count = cmsChannelsOf( asICColorSpaceSig(profile->getColorSpace()) );
#endif
    }
    return count;
}

#endif // defined(HAVE_LIBLCMS2)

// the bool return value tells if it's a user's directory or a system location
// note that this will treat places under $HOME as system directories when they are found via $XDG_DATA_DIRS
std::set<ColorProfile::FilePlusHome> ColorProfile::getBaseProfileDirs() {
#if defined(HAVE_LIBLCMS2)
    static bool warnSet = false;
    if (!warnSet) {
        warnSet = true;
    }
#endif // defined(HAVE_LIBLCMS2)
    std::set<ColorProfile::FilePlusHome> sources;

    // first try user's local dir
    gchar* path = g_build_filename(g_get_user_data_dir(), "color", "icc", NULL);
    sources.insert(FilePlusHome(path, true));
    g_free(path);

    // search colord ICC store paths
    // (see https://github.com/hughsie/colord/blob/fe10f76536bb27614ced04e0ff944dc6fb4625c0/lib/colord/cd-icc-store.c#L590)

    // user store
    path = g_build_filename(g_get_user_data_dir(), "icc", NULL);
    sources.insert(FilePlusHome(path, true));
    g_free(path);

    path = g_build_filename(g_get_home_dir(), ".color", "icc", NULL);
    sources.insert(FilePlusHome(path, true));
    g_free(path);

    // machine store
    sources.insert(FilePlusHome("/var/lib/color/icc", false));
    sources.insert(FilePlusHome("/var/lib/colord/icc", false));

    const gchar* const * dataDirs = g_get_system_data_dirs();
    for ( int i = 0; dataDirs[i]; i++ ) {
        gchar* path = g_build_filename(dataDirs[i], "color", "icc", NULL);
        sources.insert(FilePlusHome(path, false));
        g_free(path);
    }

    // On OS X:
    {
        sources.insert(FilePlusHome("/System/Library/ColorSync/Profiles", false));
        sources.insert(FilePlusHome("/Library/ColorSync/Profiles", false));

        gchar *path = g_build_filename(g_get_home_dir(), "Library", "ColorSync", "Profiles", NULL);
        sources.insert(FilePlusHome(path, true));
        g_free(path);
    }

#ifdef _WIN32
    wchar_t pathBuf[MAX_PATH + 1];
    pathBuf[0] = 0;
    DWORD pathSize = sizeof(pathBuf);
    g_assert(sizeof(wchar_t) == sizeof(gunichar2));
    if ( GetColorDirectoryW( NULL, pathBuf, &pathSize ) ) {
        gchar * utf8Path = g_utf16_to_utf8( (gunichar2*)(&pathBuf[0]), -1, NULL, NULL, NULL );
        if ( !g_utf8_validate(utf8Path, -1, NULL) ) {
            g_warning( "GetColorDirectoryW() resulted in invalid UTF-8" );
        } else {
            sources.insert(FilePlusHome(utf8Path, false));
        }
        g_free( utf8Path );
    }
#endif // _WIN32

    return sources;
}

static bool isIccFile( gchar const *filepath )
{
    bool isIccFile = false;
    GStatBuf st;
    if ( g_stat(filepath, &st) == 0 && (st.st_size > 128) ) {
        //0-3 == size
        //36-39 == 'acsp' 0x61637370
        int fd = g_open( filepath, O_RDONLY, S_IRWXU);
        if ( fd != -1 ) {
            guchar scratch[40] = {0};
            size_t len = sizeof(scratch);

            //size_t left = 40;
            ssize_t got = read(fd, scratch, len);
            if ( got != -1 ) {
                size_t calcSize = (scratch[0] << 24) | (scratch[1] << 16) | (scratch[2] << 8) | scratch[3];
                if ( calcSize > 128 && calcSize <= static_cast<size_t>(st.st_size) ) {
                    isIccFile = (scratch[36] == 'a') && (scratch[37] == 'c') && (scratch[38] == 's') && (scratch[39] == 'p');
                }
            }

            close(fd);
#if defined(HAVE_LIBLCMS2)
            if (isIccFile) {
                cmsHPROFILE prof = cmsOpenProfileFromFile( filepath, "r" );
                if ( prof ) {
                    cmsProfileClassSignature profClass = cmsGetDeviceClass(prof);
                    if ( profClass == cmsSigNamedColorClass ) {
                        isIccFile = false; // Ignore named color profiles for now.
                    }
                    cmsCloseProfile( prof );
                }
            }
#endif // defined(HAVE_LIBLCMS2)
        }
    }
    return isIccFile;
}

std::set<ColorProfile::FilePlusHome > ColorProfile::getProfileFiles()
{
    std::set<FilePlusHome> files;
    using Inkscape::IO::Resource::get_filenames;

    for (auto &path: ColorProfile::getBaseProfileDirs()) {
        for(auto &filename: get_filenames(path.filename, {".icc", ".icm"})) {
            if ( isIccFile(filename.c_str()) ) {
                files.insert(FilePlusHome(filename, path.isInHome));
            }
        }
    }

    return files;
}

std::set<ColorProfile::FilePlusHomeAndName> ColorProfile::getProfileFilesWithNames()
{
    std::set<FilePlusHomeAndName> result;

#if defined(HAVE_LIBLCMS2)
    for (auto &profile: getProfileFiles()) {
        cmsHPROFILE hProfile = cmsOpenProfileFromFile(profile.filename.c_str(), "r");
        if ( hProfile ) {
            Glib::ustring name = getNameFromProfile(hProfile);
            result.insert( FilePlusHomeAndName(profile, name) );
            cmsCloseProfile(hProfile);
        }
    }
#endif // defined(HAVE_LIBLCMS2)

    return result;
}

#if defined(HAVE_LIBLCMS2)
#if HAVE_LIBLCMS2
void errorHandlerCB(cmsContext /*contextID*/, cmsUInt32Number errorCode, char const *errorText)
{
    g_message("lcms: Error %d", errorCode);
    g_message("                 %p", errorText);
    //g_message("lcms: Error %d; %s", errorCode, errorText);
}
#endif

namespace
{

Glib::ustring getNameFromProfile(cmsHPROFILE profile)
{
    Glib::ustring nameStr;
    if ( profile ) {
#if HAVE_LIBLCMS2
    cmsUInt32Number byteLen = cmsGetProfileInfo(profile, cmsInfoDescription, "en", "US", nullptr, 0);
    if (byteLen > 0) {
        // TODO investigate wchar_t and cmsGetProfileInfo()
        std::vector<char> data(byteLen);
        cmsUInt32Number readLen = cmsGetProfileInfoASCII(profile, cmsInfoDescription,
                                                         "en", "US",
                                                         data.data(), data.size());
        if (readLen < data.size()) {
            data.resize(readLen);
        }
        nameStr = Glib::ustring(data.begin(), data.end());
    }
    if (nameStr.empty() || !g_utf8_validate(nameStr.c_str(), -1, nullptr)) {
        nameStr = _("(invalid UTF-8 string)");
    }
#endif
    }
    return nameStr;
}

/**
 * This function loads or refreshes data in knownProfiles.
 * Call it at the start of every call that requires this data.
 */
void loadProfiles()
{
    static bool error_handler_set = false;
    if (!error_handler_set) {
#if HAVE_LIBLCMS2
        //cmsSetLogErrorHandler(errorHandlerCB);
        //g_message("LCMS error handler set");
#endif
        error_handler_set = true;
    }

    static bool profiles_searched = false;
    if ( !profiles_searched ) {
        knownProfiles.clear();

        for (auto &profile: ColorProfile::getProfileFiles()) {
            cmsHPROFILE prof = cmsOpenProfileFromFile( profile.filename.c_str(), "r" );
            if ( prof ) {
                ProfileInfo info( prof, Glib::filename_to_utf8( profile.filename.c_str() ) );
                cmsCloseProfile( prof );
                prof = nullptr;

                bool sameName = false;
                for(auto &knownProfile: knownProfiles) {
                    if ( knownProfile.getName() == info.getName() ) {
                        sameName = true;
                        break;
                    }
                }

                if ( !sameName ) {
                    knownProfiles.push_back(info);
                }
            }
        }
        profiles_searched = true;
    }
}
} // namespace

static bool gamutWarn = false;

static Gdk::RGBA lastGamutColor("#808080");

static bool lastBPC = false;
#if defined(cmsFLAGS_PRESERVEBLACK)
static bool lastPreserveBlack = false;
#endif // defined(cmsFLAGS_PRESERVEBLACK)
static int lastIntent = INTENT_PERCEPTUAL;
static int lastProofIntent = INTENT_PERCEPTUAL;
static cmsHTRANSFORM transf = nullptr;

namespace {
cmsHPROFILE getSystemProfileHandle()
{
    static cmsHPROFILE theOne = nullptr;
    static Glib::ustring lastURI;

    loadProfiles();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring uri = prefs->getString("/options/displayprofile/uri");

    if ( !uri.empty() ) {
        if ( uri != lastURI ) {
            lastURI.clear();
            if ( theOne ) {
                cmsCloseProfile( theOne );
            }
            if ( transf ) {
                cmsDeleteTransform( transf );
                transf = nullptr;
            }
            theOne = cmsOpenProfileFromFile( uri.data(), "r" );
            if ( theOne ) {
                // a display profile must have the proper stuff
                cmsColorSpaceSignature space = cmsGetColorSpace(theOne);
                cmsProfileClassSignature profClass = cmsGetDeviceClass(theOne);

                if ( profClass != cmsSigDisplayClass ) {
                    g_warning("Not a display profile");
                    cmsCloseProfile( theOne );
                    theOne = nullptr;
                } else if ( space != cmsSigRgbData ) {
                    g_warning("Not an RGB profile");
                    cmsCloseProfile( theOne );
                    theOne = nullptr;
                } else {
                    lastURI = uri;
                }
            }
        }
    } else if ( theOne ) {
        cmsCloseProfile( theOne );
        theOne = nullptr;
        lastURI.clear();
        if ( transf ) {
            cmsDeleteTransform( transf );
            transf = nullptr;
        }
    }

    return theOne;
}


cmsHPROFILE getProofProfileHandle()
{
    static cmsHPROFILE theOne = nullptr;
    static Glib::ustring lastURI;

    loadProfiles();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool which = prefs->getBool( "/options/softproof/enable");
    Glib::ustring uri = prefs->getString("/options/softproof/uri");

    if ( which && !uri.empty() ) {
        if ( lastURI != uri ) {
            lastURI.clear();
            if ( theOne ) {
                cmsCloseProfile( theOne );
            }
            if ( transf ) {
                cmsDeleteTransform( transf );
                transf = nullptr;
            }
            theOne = cmsOpenProfileFromFile( uri.data(), "r" );
            if ( theOne ) {
                // a display profile must have the proper stuff
                cmsColorSpaceSignature space = cmsGetColorSpace(theOne);
                cmsProfileClassSignature profClass = cmsGetDeviceClass(theOne);

                (void)space;
                (void)profClass;
/*
                if ( profClass != cmsSigDisplayClass ) {
                    g_warning("Not a display profile");
                    cmsCloseProfile( theOne );
                    theOne = 0;
                } else if ( space != cmsSigRgbData ) {
                    g_warning("Not an RGB profile");
                    cmsCloseProfile( theOne );
                    theOne = 0;
                } else {
*/
                    lastURI = uri;
/*
                }
*/
            }
        }
    } else if ( theOne ) {
        cmsCloseProfile( theOne );
        theOne = nullptr;
        lastURI.clear();
        if ( transf ) {
            cmsDeleteTransform( transf );
            transf = nullptr;
        }
    }

    return theOne;
}
} // namespace

static void free_transforms();

cmsHTRANSFORM Inkscape::CMSSystem::getDisplayTransform()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool fromDisplay = prefs->getBool( "/options/displayprofile/from_display");
    if ( fromDisplay ) {
        if ( transf ) {
            cmsDeleteTransform(transf);
            transf = nullptr;
        }
        return nullptr;
    }

    bool warn = prefs->getBool( "/options/softproof/gamutwarn");
    int intent = prefs->getIntLimited( "/options/displayprofile/intent", 0, 0, 3 );
    int proofIntent = prefs->getIntLimited( "/options/softproof/intent", 0, 0, 3 );
    bool bpc = prefs->getBool( "/options/softproof/bpc");
#if defined(cmsFLAGS_PRESERVEBLACK)
    bool preserveBlack = prefs->getBool( "/options/softproof/preserveblack");
#endif //defined(cmsFLAGS_PRESERVEBLACK)
    Glib::ustring colorStr = prefs->getString("/options/softproof/gamutcolor");
    Gdk::RGBA gamutColor( colorStr.empty() ? "#808080" : colorStr );

    if ( (warn != gamutWarn)
         || (lastIntent != intent)
         || (lastProofIntent != proofIntent)
         || (bpc != lastBPC)
#if defined(cmsFLAGS_PRESERVEBLACK)
         || (preserveBlack != lastPreserveBlack)
#endif // defined(cmsFLAGS_PRESERVEBLACK)
         || (gamutColor != lastGamutColor)
        ) {
        gamutWarn = warn;
        free_transforms();
        lastIntent = intent;
        lastProofIntent = proofIntent;
        lastBPC = bpc;
#if defined(cmsFLAGS_PRESERVEBLACK)
        lastPreserveBlack = preserveBlack;
#endif // defined(cmsFLAGS_PRESERVEBLACK)
        lastGamutColor = gamutColor;
    }

    // Fetch these now, as they might clear the transform as a side effect.
    cmsHPROFILE hprof = getSystemProfileHandle();
    cmsHPROFILE proofProf = hprof ? getProofProfileHandle() : nullptr;

    if ( !transf ) {
        if ( hprof && proofProf ) {
            cmsUInt32Number dwFlags = cmsFLAGS_SOFTPROOFING;
            if ( gamutWarn ) {
                dwFlags |= cmsFLAGS_GAMUTCHECK;

                auto gamutColor_r = gamutColor.get_red_u();
                auto gamutColor_g = gamutColor.get_green_u();
                auto gamutColor_b = gamutColor.get_blue_u();

#if HAVE_LIBLCMS2
                cmsUInt16Number newAlarmCodes[cmsMAXCHANNELS] = {0};
                newAlarmCodes[0] = gamutColor_r;
                newAlarmCodes[1] = gamutColor_g;
                newAlarmCodes[2] = gamutColor_b;
                newAlarmCodes[3] = ~0;
                cmsSetAlarmCodes(newAlarmCodes);
#endif
            }
            if ( bpc ) {
                dwFlags |= cmsFLAGS_BLACKPOINTCOMPENSATION;
            }
#if defined(cmsFLAGS_PRESERVEBLACK)
            if ( preserveBlack ) {
                dwFlags |= cmsFLAGS_PRESERVEBLACK;
            }
#endif // defined(cmsFLAGS_PRESERVEBLACK)
            transf = cmsCreateProofingTransform( ColorProfileImpl::getSRGBProfile(), TYPE_BGRA_8, hprof, TYPE_BGRA_8, proofProf, intent, proofIntent, dwFlags );
        } else if ( hprof ) {
            transf = cmsCreateTransform( ColorProfileImpl::getSRGBProfile(), TYPE_BGRA_8, hprof, TYPE_BGRA_8, intent, 0 );
        }
    }

    return transf;
}


class MemProfile {
public:
    MemProfile();
    ~MemProfile();

    std::string id;
    cmsHPROFILE hprof;
    cmsHTRANSFORM transf;
};

MemProfile::MemProfile() :
    id(),
    hprof(nullptr),
    transf(nullptr)
{
}

MemProfile::~MemProfile()
= default;

static std::vector<MemProfile> perMonitorProfiles;

void free_transforms()
{
    if ( transf ) {
        cmsDeleteTransform(transf);
        transf = nullptr;
    }

    for ( auto profile : perMonitorProfiles ) {
        if ( profile.transf ) {
            cmsDeleteTransform(profile.transf);
            profile.transf = nullptr;
        }
    }
}

Glib::ustring Inkscape::CMSSystem::getDisplayId( int monitor )
{
    Glib::ustring id;

    if ( monitor >= 0 && monitor < static_cast<int>(perMonitorProfiles.size()) ) {
        MemProfile& item = perMonitorProfiles[monitor];
        id = item.id;
    }

    return id;
}

Glib::ustring Inkscape::CMSSystem::setDisplayPer( gpointer buf, guint bufLen, int monitor )
{
    while ( static_cast<int>(perMonitorProfiles.size()) <= monitor ) {
        MemProfile tmp;
        perMonitorProfiles.push_back(tmp);
    }
    MemProfile& item = perMonitorProfiles[monitor];

    if ( item.hprof ) {
        cmsCloseProfile( item.hprof );
        item.hprof = nullptr;
    }

    Glib::ustring id;

    if ( buf && bufLen ) {
        gsize len = bufLen; // len is an inout parameter
        id = Glib::Checksum::compute_checksum(Glib::Checksum::CHECKSUM_MD5,
            reinterpret_cast<guchar*>(buf), len);

        // Note: if this is not a valid profile, item.hprof will be set to null.
        item.hprof = cmsOpenProfileFromMem(buf, bufLen);
    }
    item.id = id;

    return id;
}

cmsHTRANSFORM Inkscape::CMSSystem::getDisplayPer( Glib::ustring const& id )
{
    cmsHTRANSFORM result = nullptr;
    if ( id.empty() ) {
        return nullptr;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool found = false;

    for ( auto it2 = perMonitorProfiles.begin(); it2 != perMonitorProfiles.end() && !found; ++it2 ) {
        if ( id == it2->id ) {
            MemProfile& item = *it2;

            bool warn = prefs->getBool( "/options/softproof/gamutwarn");
            int intent = prefs->getIntLimited( "/options/displayprofile/intent", 0, 0, 3 );
            int proofIntent = prefs->getIntLimited( "/options/softproof/intent", 0, 0, 3 );
            bool bpc = prefs->getBool( "/options/softproof/bpc");
#if defined(cmsFLAGS_PRESERVEBLACK)
            bool preserveBlack = prefs->getBool( "/options/softproof/preserveblack");
#endif //defined(cmsFLAGS_PRESERVEBLACK)
            Glib::ustring colorStr = prefs->getString("/options/softproof/gamutcolor");
            Gdk::RGBA gamutColor( colorStr.empty() ? "#808080" : colorStr );

            if ( (warn != gamutWarn)
                    || (lastIntent != intent)
                    || (lastProofIntent != proofIntent)
                    || (bpc != lastBPC)
#if defined(cmsFLAGS_PRESERVEBLACK)
                    || (preserveBlack != lastPreserveBlack)
#endif // defined(cmsFLAGS_PRESERVEBLACK)
                    || (gamutColor != lastGamutColor)
               ) {
                gamutWarn = warn;
                free_transforms();
                lastIntent = intent;
                lastProofIntent = proofIntent;
                lastBPC = bpc;
#if defined(cmsFLAGS_PRESERVEBLACK)
                lastPreserveBlack = preserveBlack;
#endif // defined(cmsFLAGS_PRESERVEBLACK)
                lastGamutColor = gamutColor;
            }

            // Fetch these now, as they might clear the transform as a side effect.
            cmsHPROFILE proofProf = item.hprof ? getProofProfileHandle() : nullptr;

            if ( !item.transf ) {
                if ( item.hprof && proofProf ) {
                    cmsUInt32Number dwFlags = cmsFLAGS_SOFTPROOFING;
                    if ( gamutWarn ) {
                        dwFlags |= cmsFLAGS_GAMUTCHECK;
                        auto gamutColor_r = gamutColor.get_red_u();
                        auto gamutColor_g = gamutColor.get_green_u();
                        auto gamutColor_b = gamutColor.get_blue_u();

#if HAVE_LIBLCMS2
                        cmsUInt16Number newAlarmCodes[cmsMAXCHANNELS] = {0};
                        newAlarmCodes[0] = gamutColor_r;
                        newAlarmCodes[1] = gamutColor_g;
                        newAlarmCodes[2] = gamutColor_b;
                        newAlarmCodes[3] = ~0;
                        cmsSetAlarmCodes(newAlarmCodes);
#endif
                    }
                    if ( bpc ) {
                        dwFlags |= cmsFLAGS_BLACKPOINTCOMPENSATION;
                    }
#if defined(cmsFLAGS_PRESERVEBLACK)
                    if ( preserveBlack ) {
                        dwFlags |= cmsFLAGS_PRESERVEBLACK;
                    }
#endif // defined(cmsFLAGS_PRESERVEBLACK)
                    item.transf = cmsCreateProofingTransform( ColorProfileImpl::getSRGBProfile(), TYPE_BGRA_8, item.hprof, TYPE_BGRA_8, proofProf, intent, proofIntent, dwFlags );
                } else if ( item.hprof ) {
                    item.transf = cmsCreateTransform( ColorProfileImpl::getSRGBProfile(), TYPE_BGRA_8, item.hprof, TYPE_BGRA_8, intent, 0 );
                }
            }

            result = item.transf;
            found = true;
        }
    }

    return result;
}



#endif // defined(HAVE_LIBLCMS2)

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
