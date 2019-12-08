// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <image> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Edward Flick (EAF)
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2005 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <cstring>
#include <algorithm>
#include <string>
#include <glibmm.h>
#include <glib/gstdio.h>
#include <2geom/rect.h>
#include <2geom/transforms.h>
#include <glibmm/i18n.h>
#include <giomm/error.h>

#include "display/drawing-image.h"
#include "display/cairo-utils.h"
#include "display/curve.h"
// Added for preserveAspectRatio support -- EAF
#include "attributes.h"
#include "print.h"
#include "document.h"
#include "sp-image.h"
#include "sp-clippath.h"
#include "xml/quote.h"
#include "preferences.h"
#include "io/sys.h"

#if defined(HAVE_LIBLCMS2)
#include "cms-system.h"
#include "color-profile.h"
#include <lcms2.h>

//#define DEBUG_LCMS
#ifdef DEBUG_LCMS
#define DEBUG_MESSAGE(key, ...)\
{\
    g_message( __VA_ARGS__ );\
}
#include <gtk/gtk.h>
#else
#define DEBUG_MESSAGE(key, ...)
#endif // DEBUG_LCMS
#endif // defined(HAVE_LIBLCMS2)
/*
 * SPImage
 */

// TODO: give these constants better names:
#define MAGIC_EPSILON 1e-9
#define MAGIC_EPSILON_TOO 1e-18
// TODO: also check if it is correct to be using two different epsilon values

static void sp_image_set_curve(SPImage *image);

static Inkscape::Pixbuf *sp_image_repr_read_image(gchar const *href, gchar const *absref, gchar const *base,
                                                  double svgdpi = 0);
static void sp_image_update_arenaitem (SPImage *img, Inkscape::DrawingImage *ai);
static void sp_image_update_canvas_image (SPImage *image);

#ifdef DEBUG_LCMS
extern guint update_in_progress;
#define DEBUG_MESSAGE_SCISLAC(key, ...) \
{\
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();\
    bool dump = prefs->getBool("/options/scislac/" #key);\
    bool dumpD = prefs->getBool("/options/scislac/" #key "D");\
    bool dumpD2 = prefs->getBool("/options/scislac/" #key "D2");\
    dumpD &&= ( (update_in_progress == 0) || dumpD2 );\
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
#else // DEBUG_LCMS
#define DEBUG_MESSAGE_SCISLAC(key, ...)
#endif // DEBUG_LCMS

SPImage::SPImage() : SPItem(), SPViewBox() {

    this->x.unset();
    this->y.unset();
    this->width.unset();
    this->height.unset();
    this->clipbox = Geom::Rect();
    this->sx = this->sy = 1.0;
    this->ox = this->oy = 0.0;
    this->dpi = 96.00;
    this->prev_width = 0.0;
    this->prev_height = 0.0;
    this->curve = nullptr;

    this->href = nullptr;
#if defined(HAVE_LIBLCMS2)
    this->color_profile = nullptr;
#endif // defined(HAVE_LIBLCMS2)
    this->pixbuf = nullptr;
}

SPImage::~SPImage() = default;

void SPImage::build(SPDocument *document, Inkscape::XML::Node *repr) {
    SPItem::build(document, repr);

    this->readAttr( "xlink:href" );
    this->readAttr( "x" );
    this->readAttr( "y" );
    this->readAttr( "width" );
    this->readAttr( "height" );
    this->readAttr("inkscape:svg-dpi");
    this->readAttr( "preserveAspectRatio" );
    this->readAttr( "color-profile" );

    /* Register */
    document->addResource("image", this);
}

void SPImage::release() {
    if (this->document) {
        // Unregister ourselves
        this->document->removeResource("image", this);
    }

    if (this->href) {
        g_free (this->href);
        this->href = nullptr;
    }

    delete this->pixbuf;
    this->pixbuf = nullptr;

#if defined(HAVE_LIBLCMS2)
    if (this->color_profile) {
        g_free (this->color_profile);
        this->color_profile = nullptr;
    }
#endif // defined(HAVE_LIBLCMS2)

    if (this->curve) {
        this->curve = this->curve->unref();
    }

    SPItem::release();
}

void SPImage::set(SPAttributeEnum key, const gchar* value) {
    switch (key) {
        case SP_ATTR_XLINK_HREF:
            g_free (this->href);
            this->href = (value) ? g_strdup (value) : nullptr;
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
            break;

        case SP_ATTR_X:
            /* ex, em not handled correctly. */
            if (!this->x.read(value)) {
                this->x.unset();
            }

            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SP_ATTR_Y:
            /* ex, em not handled correctly. */
            if (!this->y.read(value)) {
                this->y.unset();
            }

            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SP_ATTR_WIDTH:
            /* ex, em not handled correctly. */
            if (!this->width.read(value)) {
                this->width.unset();
            }

            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SP_ATTR_HEIGHT:
            /* ex, em not handled correctly. */
            if (!this->height.read(value)) {
                this->height.unset();
            }

            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SP_ATTR_SVG_DPI:
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
            break;

        case SP_ATTR_PRESERVEASPECTRATIO:
            set_preserveAspectRatio( value );
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
            break;

#if defined(HAVE_LIBLCMS2)
        case SP_PROP_COLOR_PROFILE:
            if ( this->color_profile ) {
                g_free (this->color_profile);
            }

            this->color_profile = (value) ? g_strdup (value) : nullptr;

            if ( value ) {
                DEBUG_MESSAGE( lcmsFour, "<this> color-profile set to '%s'", value );
            } else {
                DEBUG_MESSAGE( lcmsFour, "<this> color-profile cleared" );
            }

            // TODO check on this HREF_MODIFIED flag
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
            break;

#endif // defined(HAVE_LIBLCMS2)

        default:
            SPItem::set(key, value);
            break;
    }

    sp_image_set_curve(this); //creates a curve at the image's boundary for snapping
}

// BLIP
#if defined(HAVE_LIBLCMS2)
void SPImage::apply_profile(Inkscape::Pixbuf *pixbuf) {

    // TODO: this will prevent using MIME data when exporting.
    // Integrate color correction into loading.
    pixbuf->ensurePixelFormat(Inkscape::Pixbuf::PF_GDK);
    int imagewidth = pixbuf->width();
    int imageheight = pixbuf->height();
    int rowstride = pixbuf->rowstride();;
    guchar* px = pixbuf->pixels();

    if ( px ) {
        DEBUG_MESSAGE( lcmsFive, "in <image>'s sp_image_update. About to call colorprofile_get_handle()" );

        guint profIntent = Inkscape::RENDERING_INTENT_UNKNOWN;
        cmsHPROFILE prof = Inkscape::CMSSystem::getHandle( this->document,
                                                           &profIntent,
                                                           this->color_profile );
        if ( prof ) {
            cmsProfileClassSignature profileClass = cmsGetDeviceClass( prof );
            if ( profileClass != cmsSigNamedColorClass ) {
                int intent = INTENT_PERCEPTUAL;
                                
                switch ( profIntent ) {
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
                                
                cmsHPROFILE destProf = cmsCreate_sRGBProfile();
                cmsHTRANSFORM transf = cmsCreateTransform( prof,
                                                           TYPE_RGBA_8,
                                                           destProf,
                                                           TYPE_RGBA_8,
                                                           intent, 0 );
                if ( transf ) {
                    guchar* currLine = px;
                    for ( int y = 0; y < imageheight; y++ ) {
                        // Since the types are the same size, we can do the transformation in-place
                        cmsDoTransform( transf, currLine, currLine, imagewidth );
                        currLine += rowstride;
                    }

                    cmsDeleteTransform( transf );
                } else {
                    DEBUG_MESSAGE( lcmsSix, "in <image>'s sp_image_update. Unable to create LCMS transform." );
                }

                cmsCloseProfile( destProf );
            } else {
                DEBUG_MESSAGE( lcmsSeven, "in <image>'s sp_image_update. Profile type is named color. Can't transform." );
            }
        } else {
            DEBUG_MESSAGE( lcmsEight, "in <image>'s sp_image_update. No profile found." );
        }
    }
}
#endif // defined(HAVE_LIBLCMS2)

void SPImage::update(SPCtx *ctx, unsigned int flags) {

    SPDocument *doc = this->document;

    SPItem::update(ctx, flags);
    if (flags & SP_IMAGE_HREF_MODIFIED_FLAG) {
        delete this->pixbuf;
        this->pixbuf = nullptr;
        if (this->href) {
            Inkscape::Pixbuf *pixbuf = nullptr;
            double svgdpi = 96;
            if (this->getRepr()->attribute("inkscape:svg-dpi")) {
                svgdpi = atof(this->getRepr()->attribute("inkscape:svg-dpi"));
            }
            this->dpi = svgdpi;
            pixbuf = sp_image_repr_read_image(this->getRepr()->attribute("xlink:href"),
                                              this->getRepr()->attribute("sodipodi:absref"), doc->getDocumentBase(), svgdpi);

            if (pixbuf) {
#if defined(HAVE_LIBLCMS2)
                if ( this->color_profile ) apply_profile( pixbuf );
#endif
                this->pixbuf = pixbuf;
            }
        }
    }

    SPItemCtx *ictx = (SPItemCtx *) ctx;

    // Why continue without a pixbuf? So we can display "Missing Image" png.
    // Eventually, we should properly support SVG image type (i.e. render it ourselves).
    if (this->pixbuf) {
        if (!this->x._set) {
            this->x.unit = SVGLength::PX;
            this->x.computed = 0;
        }

        if (!this->y._set) {
            this->y.unit = SVGLength::PX;
            this->y.computed = 0;
        }

        if (!this->width._set) {
            this->width.unit = SVGLength::PX;
            this->width.computed = this->pixbuf->width();
        }

        if (!this->height._set) {
            this->height.unit = SVGLength::PX;
            this->height.computed = this->pixbuf->height();
        }
    }

    // Calculate x, y, width, height from parent/initial viewport, see sp-root.cpp
    this->calcDimsFromParentViewport(ictx);

    // Image creates a new viewport
    ictx->viewport= Geom::Rect::from_xywh( this->x.computed, this->y.computed,
                                           this->width.computed, this->height.computed);
 
    this->clipbox = ictx->viewport;

    this->ox = this->x.computed;
    this->oy = this->y.computed;

    if (this->pixbuf) {

        // Viewbox is either from SVG (not supported) or dimensions of pixbuf (PNG, JPG)
        this->viewBox = Geom::Rect::from_xywh(0, 0, this->pixbuf->width(), this->pixbuf->height()); 
        this->viewBox_set = true;

        // SPItemCtx rctx =
        get_rctx( ictx );

        this->ox = c2p[4];
        this->oy = c2p[5];
        this->sx = c2p[0];
        this->sy = c2p[3];
    }



    // TODO: eliminate ox, oy, sx, sy

    sp_image_update_canvas_image ((SPImage *) this);

    // don't crash with missing xlink:href attribute
    if (!this->pixbuf) {
        return;
    }

    double proportion_pixbuf = this->pixbuf->height() / (double)this->pixbuf->width();
    double proportion_image = this->height.computed / (double)this->width.computed;
    if (this->prev_width &&
        (this->prev_width != this->pixbuf->width() || this->prev_height != this->pixbuf->height())) {
        if (std::abs(this->prev_width - this->pixbuf->width()) > std::abs(this->prev_height - this->pixbuf->height())) {
            proportion_pixbuf = this->pixbuf->width() / (double)this->pixbuf->height();
            proportion_image = this->width.computed / (double)this->height.computed;
            if (proportion_pixbuf != proportion_image) {
                double new_height = this->height.computed * proportion_pixbuf;
                sp_repr_set_svg_double(this->getRepr(), "width", new_height);
            }
        }
        else {
            if (proportion_pixbuf != proportion_image) {
                double new_width = this->width.computed * proportion_pixbuf;
                sp_repr_set_svg_double(this->getRepr(), "height", new_width);
            }
        }
    }
    this->prev_width = this->pixbuf->width();
    this->prev_height = this->pixbuf->height();
}

void SPImage::modified(unsigned int flags) {
//  SPItem::onModified(flags);

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (SPItemView *v = this->display; v != nullptr; v = v->next) {
            Inkscape::DrawingImage *img = dynamic_cast<Inkscape::DrawingImage *>(v->arenaitem);
            img->setStyle(this->style);
        }
    }
}


Inkscape::XML::Node *SPImage::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags ) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:image");
    }

    repr->setAttribute("xlink:href", this->href);

    /* fixme: Reset attribute if needed (Lauris) */
    if (this->x._set) {
        sp_repr_set_svg_double(repr, "x", this->x.computed);
    }

    if (this->y._set) {
        sp_repr_set_svg_double(repr, "y", this->y.computed);
    }

    if (this->width._set) {
        sp_repr_set_svg_double(repr, "width", this->width.computed);
    }

    if (this->height._set) {
        sp_repr_set_svg_double(repr, "height", this->height.computed);
    }
    repr->setAttribute("inkscape:svg-dpi", this->getRepr()->attribute("inkscape:svg-dpi"));
    //XML Tree being used directly here while it shouldn't be...
    repr->setAttribute("preserveAspectRatio", this->getRepr()->attribute("preserveAspectRatio"));
#if defined(HAVE_LIBLCMS2)
    if (this->color_profile) {
        repr->setAttribute("color-profile", this->color_profile);
    }
#endif // defined(HAVE_LIBLCMS2)

    SPItem::write(xml_doc, repr, flags);

    return repr;
}

Geom::OptRect SPImage::bbox(Geom::Affine const &transform, SPItem::BBoxType /*type*/) const {
    Geom::OptRect bbox;

    if ((this->width.computed > 0.0) && (this->height.computed > 0.0)) {
        bbox = Geom::Rect::from_xywh(this->x.computed, this->y.computed, this->width.computed, this->height.computed);
        *bbox *= transform;
    }

    return bbox;
}

void SPImage::print(SPPrintContext *ctx) {
    if (this->pixbuf && (this->width.computed > 0.0) && (this->height.computed > 0.0) ) {
        Inkscape::Pixbuf *pb = new Inkscape::Pixbuf(*this->pixbuf);
        pb->ensurePixelFormat(Inkscape::Pixbuf::PF_GDK);

        guchar *px = pb->pixels();
        int w = pb->width();
        int h = pb->height();
        int rs = pb->rowstride();

        double vx = this->ox;
        double vy = this->oy;

        Geom::Affine t;
        Geom::Translate tp(vx, vy);
        Geom::Scale s(this->sx, this->sy);
        t = s * tp;
        ctx->image_R8G8B8A8_N(px, w, h, rs, t, this->style);
        delete pb;
    }
}

const char* SPImage::displayName() const {
    return _("Image");
}

gchar* SPImage::description() const {
    char *href_desc;

    if (this->href) {
        href_desc = (strncmp(this->href, "data:", 5) == 0)
            ? g_strdup(_("embedded"))
            : xml_quote_strdup(this->href);
    } else {
        g_warning("Attempting to call strncmp() with a null pointer.");
        href_desc = g_strdup("(null_pointer)"); // we call g_free() on href_desc
    }

    char *ret = ( this->pixbuf == nullptr
                  ? g_strdup_printf(_("[bad reference]: %s"), href_desc)
                  : g_strdup_printf(_("%d &#215; %d: %s"),
                                    this->pixbuf->width(),
                                    this->pixbuf->height(),
                                    href_desc) );
                                    
    if (this->pixbuf == nullptr && 
        this->document) 
    {
        Inkscape::Pixbuf * pb = nullptr;
        double svgdpi = 96;
        if (this->getRepr()->attribute("inkscape:svg-dpi")) {
            svgdpi = atof(this->getRepr()->attribute("inkscape:svg-dpi"));
        }
        pb = sp_image_repr_read_image(this->getRepr()->attribute("xlink:href"),
                                      this->getRepr()->attribute("sodipodi:absref"), this->document->getDocumentBase(), svgdpi);

        if (pb) {
            ret = g_strdup_printf(_("%d &#215; %d: %s"),
                                        pb->width(),
                                        pb->height(),
                                        href_desc);
            delete pb;
        }
    }

    g_free(href_desc);
    return ret;
}

Inkscape::DrawingItem* SPImage::show(Inkscape::Drawing &drawing, unsigned int /*key*/, unsigned int /*flags*/) {
    Inkscape::DrawingImage *ai = new Inkscape::DrawingImage(drawing);

    sp_image_update_arenaitem(this, ai);

    return ai;
}

static std::string broken_image_svg = R"A(
<svg xmlns="http://www.w3.org/2000/svg" width="640" height="640">
  <rect width="100%" height="100%" style="fill:white;stroke:red;stroke-width:20px"/>
  <rect x="35%" y="10%" width="30%" height="30%" style="fill:red"/>
  <path d="m 280,120  80,80" style="fill:none;stroke:white;stroke-width:20px"/>
  <path d="m 360,120 -80,80" style="fill:none;stroke:white;stroke-width:20px"/>
  <g style="font-family:sans-serif;font-size:100px;font-weight:bold;text-anchor:middle">
    <text x="50%" y="380">Linked</text>
    <text x="50%" y="490">Image</text>
    <text x="50%" y="600">Not Found</text>
  </g>
</svg>
)A";

Inkscape::Pixbuf *sp_image_repr_read_image(gchar const *href, gchar const *absref, gchar const *base, double svgdpi)
{
    Inkscape::Pixbuf *inkpb = nullptr;

    gchar const *filename = href;
    
    if (filename != nullptr) {
        if (g_ascii_strncasecmp(filename, "data:", 5) == 0) {
            /* data URI - embedded image */
            filename += 5;
            inkpb = Inkscape::Pixbuf::create_from_data_uri(filename, svgdpi);
        } else {
            auto url = Inkscape::URI::from_href_and_basedir(href, base);

            if (url.hasScheme("file")) {
                auto native = url.toNativeFilename();
                inkpb = Inkscape::Pixbuf::create_from_file(native.c_str(), svgdpi);
            } else {
                try {
                    auto contents = url.getContents();
                    inkpb = Inkscape::Pixbuf::create_from_buffer(contents, svgdpi);
                } catch (const Gio::Error &e) {
                    g_warning("URI::getContents failed for '%.100s'", href);
                }
            }
        }

        if (inkpb != nullptr) {
            return inkpb;
        }
    }

    /* at last try to load from sp absolute path name */
    filename = absref;
    if (filename != nullptr) {
        // using absref is outside of SVG rules, so we must at least warn the user
        if ( base != nullptr && href != nullptr ) {
            g_warning ("<image xlink:href=\"%s\"> did not resolve to a valid image file (base dir is %s), now trying sodipodi:absref=\"%s\"", href, base, absref);
        } else {
            g_warning ("xlink:href did not resolve to a valid image file, now trying sodipodi:absref=\"%s\"", absref);
        }

        inkpb = Inkscape::Pixbuf::create_from_file(filename, svgdpi);
        if (inkpb != nullptr) {
            return inkpb;
        }
    }

    /* Nope: We do not find any valid pixmap file :-( */
    // Need a "fake" filename to trigger svg mode.
    inkpb = Inkscape::Pixbuf::create_from_buffer(broken_image_svg, 0, "brokenimage.svg");

    /* It's included here so if it still does not does load, */
    /* our libraries are broken! */
    g_assert (inkpb != nullptr);

    return inkpb;
}

/* We assert that realpixbuf is either NULL or identical size to pixbuf */
static void
sp_image_update_arenaitem (SPImage *image, Inkscape::DrawingImage *ai)
{
    ai->setStyle(SP_OBJECT(image)->style);
    ai->setPixbuf(image->pixbuf);
    ai->setOrigin(Geom::Point(image->ox, image->oy));
    ai->setScale(image->sx, image->sy);
    ai->setClipbox(image->clipbox);
}

static void sp_image_update_canvas_image(SPImage *image)
{
    SPItem *item = SP_ITEM(image);
    for (SPItemView *v = item->display; v != nullptr; v = v->next) {
        sp_image_update_arenaitem(image, dynamic_cast<Inkscape::DrawingImage *>(v->arenaitem));
    }
}

void SPImage::snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const {
    /* An image doesn't have any nodes to snap, but still we want to be able snap one image
    to another. Therefore we will create some snappoints at the corner, similar to a rect. If
    the image is rotated, then the snappoints will rotate with it. Again, just like a rect.
    */

    if (this->getClipObject()) {
        //We are looking at a clipped image: do not return any snappoints, as these might be
        //far far away from the visible part from the clipped image
        //TODO Do return snappoints, but only when within visual bounding box
    } else {
        if (snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_IMG_CORNER)) {
            // The image has not been clipped: return its corners, which might be rotated for example
            double const x0 = this->x.computed;
            double const y0 = this->y.computed;
            double const x1 = x0 + this->width.computed;
            double const y1 = y0 + this->height.computed;

            Geom::Affine const i2d (this->i2dt_affine ());

            p.emplace_back(Geom::Point(x0, y0) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER);
            p.emplace_back(Geom::Point(x0, y1) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER);
            p.emplace_back(Geom::Point(x1, y1) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER);
            p.emplace_back(Geom::Point(x1, y0) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER);
        }
    }
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

Geom::Affine SPImage::set_transform(Geom::Affine const &xform) {
    /* Calculate position in parent coords. */
    Geom::Point pos( Geom::Point(this->x.computed, this->y.computed) * xform );

    /* This function takes care of translation and scaling, we return whatever parts we can't
       handle. */
    Geom::Affine ret(Geom::Affine(xform).withoutTranslation());
    Geom::Point const scale(hypot(ret[0], ret[1]),
                            hypot(ret[2], ret[3]));

    if ( scale[Geom::X] > MAGIC_EPSILON ) {
        ret[0] /= scale[Geom::X];
        ret[1] /= scale[Geom::X];
    } else {
        ret[0] = 1.0;
        ret[1] = 0.0;
    }

    if ( scale[Geom::Y] > MAGIC_EPSILON ) {
        ret[2] /= scale[Geom::Y];
        ret[3] /= scale[Geom::Y];
    } else {
        ret[2] = 0.0;
        ret[3] = 1.0;
    }

    this->width = this->width.computed * scale[Geom::X];
    this->height = this->height.computed * scale[Geom::Y];

    /* Find position in item coords */
    pos = pos * ret.inverse();
    this->x = pos[Geom::X];
    this->y = pos[Geom::Y];

    return ret;
}

static void sp_image_set_curve( SPImage *image )
{
    //create a curve at the image's boundary for snapping
    if ((image->height.computed < MAGIC_EPSILON_TOO) || (image->width.computed < MAGIC_EPSILON_TOO) || (image->getClipObject())) {
        if (image->curve) {
            image->curve = image->curve->unref();
        }
    } else {
        Geom::OptRect rect = image->bbox(Geom::identity(), SPItem::VISUAL_BBOX);
        SPCurve *c = nullptr;
        
        if (rect->isFinite()) {
            c = SPCurve::new_from_rect(*rect, true);
        }

        if (image->curve) {
            image->curve = image->curve->unref();
        }

        if (c) {
            image->curve = c->ref();

            c->unref();
        }
    }
}

/**
 * Return duplicate of curve (if any exists) or NULL if there is no curve
 */
SPCurve *SPImage::get_curve() const
{
    SPCurve *result = nullptr;
    if (curve) {
        result = curve->copy();
    }
    return result;
}

void sp_embed_image(Inkscape::XML::Node *image_node, Inkscape::Pixbuf *pb)
{
    bool free_data = false;

    // check whether the pixbuf has MIME data
    guchar *data = nullptr;
    gsize len = 0;
    std::string data_mimetype;

    data = const_cast<guchar *>(pb->getMimeData(len, data_mimetype));

    if (data == nullptr) {
        // if there is no supported MIME data, embed as PNG
        data_mimetype = "image/png";
        gdk_pixbuf_save_to_buffer(pb->getPixbufRaw(), reinterpret_cast<gchar**>(&data), &len, "png", nullptr, NULL);
        free_data = true;
    }

    // Save base64 encoded data in image node
    // this formula taken from Glib docs
    gsize needed_size = len * 4 / 3 + len * 4 / (3 * 72) + 7;
    needed_size += 5 + 8 + data_mimetype.size(); // 5 bytes for data: + 8 for ;base64,

    gchar *buffer = (gchar *) g_malloc(needed_size);
    gchar *buf_work = buffer;
    buf_work += g_sprintf(buffer, "data:%s;base64,", data_mimetype.c_str());

    gint state = 0;
    gint save = 0;
    gsize written = 0;
    written += g_base64_encode_step(data, len, TRUE, buf_work, &state, &save);
    written += g_base64_encode_close(TRUE, buf_work + written, &state, &save);
    buf_work[written] = 0; // null terminate

    // TODO: this is very wasteful memory-wise.
    // It would be better to only keep the binary data around,
    // and base64 encode on the fly when saving the XML.
    image_node->setAttribute("xlink:href", buffer);

    g_free(buffer);
    if (free_data) g_free(data);
}

void sp_embed_svg(Inkscape::XML::Node *image_node, std::string const &fn)
{
    if (!g_file_test(fn.c_str(), G_FILE_TEST_EXISTS)) { 
        return;
    }
    GStatBuf stdir;
    int val = g_stat(fn.c_str(), &stdir);
    if (val == 0 && stdir.st_mode & S_IFDIR){
        return;
    }

    // we need to load the entire file into memory,
    // since we'll store it as MIME data
    gchar *data = nullptr;
    gsize len = 0;
    GError *error = nullptr;

    if (g_file_get_contents(fn.c_str(), &data, &len, &error)) {

        if (error != nullptr) {
            std::cerr << "Pixbuf::create_from_file: " << error->message << std::endl;
            std::cerr << "   (" << fn << ")" << std::endl;
            return;
        }

        std::string data_mimetype = "image/svg+xml";


        // Save base64 encoded data in image node
        // this formula taken from Glib docs
        gsize needed_size = len * 4 / 3 + len * 4 / (3 * 72) + 7;
        needed_size += 5 + 8 + data_mimetype.size(); // 5 bytes for data: + 8 for ;base64,

        gchar *buffer = (gchar *) g_malloc(needed_size);
        gchar *buf_work = buffer;
        buf_work += g_sprintf(buffer, "data:%s;base64,", data_mimetype.c_str());

        gint state = 0;
        gint save = 0;
        gsize written = 0;
        written += g_base64_encode_step(reinterpret_cast<guchar *>(data), len, TRUE, buf_work, &state, &save);
        written += g_base64_encode_close(TRUE, buf_work + written, &state, &save);
        buf_work[written] = 0; // null terminate

        // TODO: this is very wasteful memory-wise.
        // It would be better to only keep the binary data around,
        // and base64 encode on the fly when saving the XML.
        image_node->setAttribute("xlink:href", buffer);

        g_free(buffer);
        g_free(data);
    }
}

void SPImage::refresh_if_outdated()
{
    if ( href && pixbuf && pixbuf->modificationTime()) {
        // It *might* change

        GStatBuf st;
        memset(&st, 0, sizeof(st));
        int val = 0;
        if (g_file_test (pixbuf->originalPath().c_str(), G_FILE_TEST_EXISTS)){ 
            val = g_stat(pixbuf->originalPath().c_str(), &st);
        }
        if ( !val ) {
            // stat call worked. Check time now
            if ( st.st_mtime != pixbuf->modificationTime() ) {
                requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
            }
        }
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
