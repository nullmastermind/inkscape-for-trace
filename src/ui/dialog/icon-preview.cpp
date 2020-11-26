// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * A simple dialog for previewing icon representation.
 */
/* Authors:
 *   Jon A. Cruz
 *   Bob Jamison
 *   Other dudes from The Inkscape Organization
 *   Abhishek Sharma
 *
 * Copyright (C) 2004 Bob Jamison
 * Copyright (C) 2005,2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <glibmm/timer.h>
#include <glibmm/main.h>

#include <gtkmm/buttonbox.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/frame.h>

#include "desktop.h"
#include "document.h"
#include "inkscape.h"
#include "verbs.h"

#include "display/cairo-utils.h"
#include "display/drawing.h"
#include "display/drawing-context.h"

#include "object/sp-namedview.h"
#include "object/sp-root.h"

#include "icon-preview.h"

#include "ui/widget/frame.h"

extern "C" {
// takes doc, drawing, icon, and icon name to produce pixels
guchar *
sp_icon_doc_icon( SPDocument *doc, Inkscape::Drawing &drawing,
                  const gchar *name, unsigned int psize, unsigned &stride);
}

#define noICON_VERBOSE 1

namespace Inkscape {
namespace UI {
namespace Dialog {


IconPreviewPanel &IconPreviewPanel::getInstance()
{
    IconPreviewPanel *instance = new IconPreviewPanel();

    instance->refreshPreview();

    return *instance;
}

//#########################################################################
//## E V E N T S
//#########################################################################

void IconPreviewPanel::on_button_clicked(int which)
{
    if ( hot != which ) {
        buttons[hot]->set_active( false );

        hot = which;
        updateMagnify();
        queue_draw();
    }
}




//#########################################################################
//## C O N S T R U C T O R    /    D E S T R U C T O R
//#########################################################################
/**
 * Constructor
 */
IconPreviewPanel::IconPreviewPanel()
    : DialogBase("/dialogs/iconpreview", SP_VERB_VIEW_ICON_PREVIEW)
    , desktop(nullptr)
    , document(nullptr)
    , drawing(nullptr)
    , visionkey(0)
    , timer(nullptr)
    , renderTimer(nullptr)
    , pending(false)
    , minDelay(0.1)
    , targetId()
    , hot(1)
    , selectionButton(nullptr)
    , docModConn()
    , iconBox(Gtk::ORIENTATION_VERTICAL)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    numEntries = 0;

    bool pack = prefs->getBool("/iconpreview/pack", true);

    std::vector<Glib::ustring> pref_sizes = prefs->getAllDirs("/iconpreview/sizes/default");
    std::vector<int> rawSizes;

    for (auto & pref_size : pref_sizes) {
        if (prefs->getBool(pref_size + "/show", true)) {
            int sizeVal = prefs->getInt(pref_size + "/value", -1);
            if (sizeVal > 0) {
                rawSizes.push_back(sizeVal);
            }
        }
    }

    if ( !rawSizes.empty() ) {
        numEntries = rawSizes.size();
        sizes = new int[numEntries];
        int i = 0;
        for ( std::vector<int>::iterator it = rawSizes.begin(); it != rawSizes.end(); ++it, ++i ) {
            sizes[i] = *it;
        }
    }

    if ( numEntries < 1 )
    {
        numEntries = 5;
        sizes = new int[numEntries];
        sizes[0] = 16;
        sizes[1] = 24;
        sizes[2] = 32;
        sizes[3] = 48;
        sizes[4] = 128;
    }

    pixMem = new guchar*[numEntries];
    images = new Gtk::Image*[numEntries];
    labels = new Glib::ustring*[numEntries];
    buttons = new Gtk::ToggleToolButton*[numEntries];


    for ( int i = 0; i < numEntries; i++ ) {
        char *label = g_strdup_printf(_("%d x %d"), sizes[i], sizes[i]);
        labels[i] = new Glib::ustring(label);
        g_free(label);
        pixMem[i] = nullptr;
        images[i] = nullptr;
    }


    magLabel.set_label( *labels[hot] );

    Gtk::Box* magBox = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);

    UI::Widget::Frame *magFrame = Gtk::manage(new UI::Widget::Frame(_("Magnified:")));
    magFrame->add( magnified );

    magBox->pack_start( *magFrame, Gtk::PACK_EXPAND_WIDGET );
    magBox->pack_start( magLabel, Gtk::PACK_SHRINK );


    Gtk::Box *verts = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    Gtk::Box *horiz = nullptr;
    int previous = 0;
    int avail = 0;
    for ( int i = numEntries - 1; i >= 0; --i ) {
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, sizes[i]);
        pixMem[i] = new guchar[sizes[i] * stride];
        memset( pixMem[i], 0x00, sizes[i] * stride );

        auto pb = Gdk::Pixbuf::create_from_data(pixMem[i], Gdk::COLORSPACE_RGB, true, 8, sizes[i], sizes[i], stride);
        images[i] = Gtk::make_managed<Gtk::Image>(pb);
        Glib::ustring label(*labels[i]);
        buttons[i] = new Gtk::ToggleToolButton(label);
        buttons[i]->set_active( i == hot );
        if ( prefs->getBool("/iconpreview/showFrames", true) ) {
            Gtk::Frame *frame = new Gtk::Frame();
            frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
            frame->add(*images[i]);
            buttons[i]->set_icon_widget(*Gtk::manage(frame));
        } else {
            buttons[i]->set_icon_widget(*images[i]);
        }

        buttons[i]->set_tooltip_text(label);

        buttons[i]->signal_clicked().connect( sigc::bind<int>( sigc::mem_fun(*this, &IconPreviewPanel::on_button_clicked), i) );

        buttons[i]->set_halign(Gtk::ALIGN_CENTER);
        buttons[i]->set_valign(Gtk::ALIGN_CENTER);

        if ( !pack || ( (avail == 0) && (previous == 0) ) ) {
            verts->pack_end(*(buttons[i]), Gtk::PACK_SHRINK);
            previous = sizes[i];
            avail = sizes[i];
        } else {
            int pad = 12;
            if ((avail < pad) || ((sizes[i] > avail) && (sizes[i] < previous))) {
                horiz = nullptr;
            }
            if ((horiz == nullptr) && (sizes[i] <= previous)) {
                avail = previous;
            }
            if (sizes[i] <= avail) {
                if (!horiz) {
                    horiz = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
                    avail = previous;
                    verts->pack_end(*horiz, Gtk::PACK_SHRINK);
                }
                horiz->pack_start(*(buttons[i]), Gtk::PACK_EXPAND_WIDGET);
                avail -= sizes[i];
                avail -= pad; // a little extra for padding
            } else {
                horiz = nullptr;
                verts->pack_end(*(buttons[i]), Gtk::PACK_SHRINK);
            }
        }
    }

    iconBox.pack_start(splitter);
    splitter.pack1( *magBox, true, false );
    UI::Widget::Frame *actuals = Gtk::manage(new UI::Widget::Frame (_("Actual Size:")));
    actuals->set_border_width(4);
    actuals->add(*verts);
    splitter.pack2( *actuals, false, false );


    selectionButton = new Gtk::CheckButton(C_("Icon preview window", "Sele_ction"), true);//selectionButton = (Gtk::ToggleButton*) gtk_check_button_new_with_mnemonic(_("_Selection")); // , GTK_RESPONSE_APPLY
    magBox->pack_start( *selectionButton, Gtk::PACK_SHRINK );
    selectionButton->set_tooltip_text(_("Selection only or whole document"));
    selectionButton->signal_clicked().connect( sigc::mem_fun(*this, &IconPreviewPanel::modeToggled) );

    gint val = prefs->getBool("/iconpreview/selectionOnly");
    selectionButton->set_active( val != 0 );

    pack_start(iconBox, Gtk::PACK_SHRINK);

    show_all_children();
}

IconPreviewPanel::~IconPreviewPanel()
{
    if (timer) {
        timer->stop();
        delete timer;
        timer = nullptr;
    }
    if ( renderTimer ) {
        renderTimer->stop();
        delete renderTimer;
        renderTimer = nullptr;
    }

    docModConn.disconnect();
}

//#########################################################################
//## M E T H O D S
//#########################################################################


#if ICON_VERBOSE
static Glib::ustring getTimestr()
{
    Glib::ustring str;
    gint64 micr = g_get_monotonic_time();
    gint64 mins = ((int)round(micr / 60000000)) % 60;
    gdouble dsecs = micr / 1000000;
    gchar *ptr = g_strdup_printf(":%02u:%f", mins, dsecs);
    str = ptr;
    g_free(ptr);
    ptr = 0;
    return str;
}
#endif // ICON_VERBOSE

void IconPreviewPanel::update()
{
    if (!_app) {
        std::cerr << "IconPreviewPanel::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();

    if (desktop) {
        this->desktop = desktop;

        if (this->desktop->selection && Inkscape::Preferences::get()->getBool("/iconpreview/autoRefresh", true)) {
            queueRefresh();
        }
    }

    SPDocument *document = _app->get_active_document();
    setDocument(document);
}

void IconPreviewPanel::setDocument( SPDocument *document )
{
    if (this->document != document) {
        docModConn.disconnect();
        if (drawing) {
            this->document->getRoot()->invoke_hide(visionkey);
            delete drawing;
            drawing = nullptr;
        }
        this->document = document;
        if (this->document) {
            drawing = new Inkscape::Drawing();
            visionkey = SPItem::display_key_new(1);
            drawing->setRoot(this->document->getRoot()->invoke_show(*drawing, visionkey, SP_ITEM_SHOW_DISPLAY));

            if ( Inkscape::Preferences::get()->getBool("/iconpreview/autoRefresh", true) ) {
                docModConn = this->document->connectModified(sigc::hide(sigc::mem_fun(this, &IconPreviewPanel::queueRefresh)));
            }
            queueRefresh();
        }
    }
}

void IconPreviewPanel::refreshPreview()
{
    SPDesktop *desktop = getDesktop();
    if (!timer) {
        timer = new Glib::Timer();
    }
    if (timer->elapsed() < minDelay) {
#if ICON_VERBOSE
        g_message( "%s Deferring refresh as too soon. calling queueRefresh()", getTimestr().c_str() );
#endif //ICON_VERBOSE
        // Do not refresh too quickly
        queueRefresh();
    } else if ( desktop && desktop->doc() ) {
#if ICON_VERBOSE
        g_message( "%s Refreshing preview.", getTimestr().c_str() );
#endif // ICON_VERBOSE
        bool hold = Inkscape::Preferences::get()->getBool("/iconpreview/selectionHold", true);
        SPObject *target = nullptr;
        if ( selectionButton && selectionButton->get_active() )
        {
            target = (hold && !targetId.empty()) ? desktop->doc()->getObjectById( targetId.c_str() ) : nullptr;
            if ( !target ) {
                targetId.clear();
                Inkscape::Selection * sel = desktop->getSelection();
                if ( sel ) {
                    //g_message("found a selection to play with");

                	auto items = sel->items();
                    for(auto i=items.begin();!target && i!=items.end();++i){
                        SPItem* item = *i;
                        gchar const *id = item->getId();
                        if ( id ) {
                            targetId = id;
                            target = item;
                        }
                    }
                }
            }
        } else {
            target = desktop->currentRoot();
        }
        if ( target ) {
            renderPreview(target);
        }
#if ICON_VERBOSE
        g_message( "%s  resetting timer", getTimestr().c_str() );
#endif // ICON_VERBOSE
        timer->reset();
    }
}

bool IconPreviewPanel::refreshCB()
{
    bool callAgain = true;
    if (!timer) {
        timer = new Glib::Timer();
    }
    if ( timer->elapsed() > minDelay ) {
#if ICON_VERBOSE
        g_message( "%s refreshCB() timer has progressed", getTimestr().c_str() );
#endif // ICON_VERBOSE
        callAgain = false;
        refreshPreview();
#if ICON_VERBOSE
        g_message( "%s refreshCB() setting pending false", getTimestr().c_str() );
#endif // ICON_VERBOSE
        pending = false;
    }
    return callAgain;
}

void IconPreviewPanel::queueRefresh()
{
    if (!pending) {
        pending = true;
#if ICON_VERBOSE
        g_message( "%s queueRefresh() Setting pending true", getTimestr().c_str() );
#endif // ICON_VERBOSE
        if (!timer) {
            timer = new Glib::Timer();
        }
        Glib::signal_idle().connect( sigc::mem_fun(this, &IconPreviewPanel::refreshCB), Glib::PRIORITY_DEFAULT_IDLE );
    }
}

void IconPreviewPanel::modeToggled()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool selectionOnly = (selectionButton && selectionButton->get_active());
    prefs->setBool("/iconpreview/selectionOnly", selectionOnly);
    if ( !selectionOnly ) {
        targetId.clear();
    }

    refreshPreview();
}

void overlayPixels(guchar *px, int width, int height, int stride,
                            unsigned r, unsigned g, unsigned b)
{
    int bytesPerPixel = 4;
    int spacing = 4;
    for ( int y = 0; y < height; y += spacing ) {
        guchar *ptr = px + y * stride;
        for ( int x = 0; x < width; x += spacing ) {
            *(ptr++) = r;
            *(ptr++) = g;
            *(ptr++) = b;
            *(ptr++) = 0xff;

            ptr += bytesPerPixel * (spacing - 1);
        }
    }

    if ( width > 1 && height > 1 ) {
        // point at the last pixel
        guchar *ptr = px + ((height-1) * stride) + ((width - 1) * bytesPerPixel);

        if ( width > 2 ) {
            px[4] = r;
            px[5] = g;
            px[6] = b;
            px[7] = 0xff;

            ptr[-12] = r;
            ptr[-11] = g;
            ptr[-10] = b;
            ptr[-9] = 0xff;
        }

        ptr[-4] = r;
        ptr[-3] = g;
        ptr[-2] = b;
        ptr[-1] = 0xff;

        px[0 + stride] = r;
        px[1 + stride] = g;
        px[2 + stride] = b;
        px[3 + stride] = 0xff;

        ptr[0 - stride] = r;
        ptr[1 - stride] = g;
        ptr[2 - stride] = b;
        ptr[3 - stride] = 0xff;

        if ( height > 2 ) {
            ptr[0 - stride * 3] = r;
            ptr[1 - stride * 3] = g;
            ptr[2 - stride * 3] = b;
            ptr[3 - stride * 3] = 0xff;
        }
    }
}

// takes doc, drawing, icon, and icon name to produce pixels
extern "C" guchar *
sp_icon_doc_icon( SPDocument *doc, Inkscape::Drawing &drawing,
                  gchar const *name, unsigned psize,
                  unsigned &stride)
{
    bool const dump = Inkscape::Preferences::get()->getBool("/debug/icons/dumpSvg");
    guchar *px = nullptr;

    if (doc) {
        SPObject *object = doc->getObjectById(name);
        if (object && SP_IS_ITEM(object)) {
            SPItem *item = SP_ITEM(object);
            // Find bbox in document
            Geom::OptRect dbox = item->documentVisualBounds();

            if ( object->parent == nullptr )
            {
                dbox = Geom::Rect(Geom::Point(0, 0),
                                Geom::Point(doc->getWidth().value("px"), doc->getHeight().value("px")));
            }

            /* This is in document coordinates, i.e. pixels */
            if ( dbox ) {
                /* Update to renderable state */
                double sf = 1.0;
                drawing.root()->setTransform(Geom::Scale(sf));
                drawing.update();
                /* Item integer bbox in points */
                // NOTE: previously, each rect coordinate was rounded using floor(c + 0.5)
                Geom::IntRect ibox = dbox->roundOutwards();

                if ( dump ) {
                    g_message( "   box    --'%s'  (%f,%f)-(%f,%f)", name, (double)ibox.left(), (double)ibox.top(), (double)ibox.right(), (double)ibox.bottom() );
                }

                /* Find button visible area */
                int width = ibox.width();
                int height = ibox.height();

                if ( dump ) {
                    g_message( "   vis    --'%s'  (%d,%d)", name, width, height );
                }

                {
                    int block = std::max(width, height);
                    if (block != static_cast<int>(psize) ) {
                        if ( dump ) {
                            g_message("      resizing" );
                        }
                        sf = (double)psize / (double)block;

                        drawing.root()->setTransform(Geom::Scale(sf));
                        drawing.update();

                        auto scaled_box = *dbox * Geom::Scale(sf);
                        ibox = scaled_box.roundOutwards();
                        if ( dump ) {
                            g_message( "   box2   --'%s'  (%f,%f)-(%f,%f)", name, (double)ibox.left(), (double)ibox.top(), (double)ibox.right(), (double)ibox.bottom() );
                        }

                        /* Find button visible area */
                        width = ibox.width();
                        height = ibox.height();
                        if ( dump ) {
                            g_message( "   vis2   --'%s'  (%d,%d)", name, width, height );
                        }
                    }
                }

                Geom::IntPoint pdim(psize, psize);
                int dx, dy;
                //dx = (psize - width) / 2;
                //dy = (psize - height) / 2;
                dx=dy=psize;
                dx=(dx-width)/2; // watch out for psize, since 'unsigned'-'signed' can cause problems if the result is negative
                dy=(dy-height)/2;
                Geom::IntRect area = Geom::IntRect::from_xywh(ibox.min() - Geom::IntPoint(dx,dy), pdim);
                /* Actual renderable area */
                Geom::IntRect ua = *Geom::intersect(ibox, area);

                if ( dump ) {
                    g_message( "   area   --'%s'  (%f,%f)-(%f,%f)", name, (double)area.left(), (double)area.top(), (double)area.right(), (double)area.bottom() );
                    g_message( "   ua     --'%s'  (%f,%f)-(%f,%f)", name, (double)ua.left(), (double)ua.top(), (double)ua.right(), (double)ua.bottom() );
                }

                stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, psize);

                /* Set up pixblock */
                px = g_new(guchar, stride * psize);
                memset(px, 0x00, stride * psize);

                /* Render */
                cairo_surface_t *s = cairo_image_surface_create_for_data(px,
                    CAIRO_FORMAT_ARGB32, psize, psize, stride);
                Inkscape::DrawingContext dc(s, ua.min());

                SPNamedView *nv = sp_document_namedview(doc, nullptr);
                float bg_r = SP_RGBA32_R_F(nv->pagecolor);
                float bg_g = SP_RGBA32_G_F(nv->pagecolor);
                float bg_b = SP_RGBA32_B_F(nv->pagecolor);
                float bg_a = SP_RGBA32_A_F(nv->pagecolor);

                cairo_t *cr = cairo_create(s);
                cairo_set_source_rgba(cr, bg_r, bg_g, bg_b, bg_a);
                cairo_rectangle(cr, 0, 0, psize, psize);
                cairo_fill(cr);
                cairo_save(cr);
                cairo_destroy(cr);

                drawing.render(dc, ua);
                cairo_surface_destroy(s);

                // convert to GdkPixbuf format
                convert_pixels_argb32_to_pixbuf(px, psize, psize, stride);

                if ( Inkscape::Preferences::get()->getBool("/debug/icons/overlaySvg") ) {
                    overlayPixels( px, psize, psize, stride, 0x00, 0x00, 0xff );
                }
            }
        }
    }

    return px;
} // end of sp_icon_doc_icon()


void IconPreviewPanel::renderPreview( SPObject* obj )
{
    SPDocument * doc = obj->document;
    gchar const * id = obj->getId();
    if ( !renderTimer ) {
        renderTimer = new Glib::Timer();
    }
    renderTimer->reset();

#if ICON_VERBOSE
    g_message("%s setting up to render '%s' as the icon", getTimestr().c_str(), id );
#endif // ICON_VERBOSE

    for ( int i = 0; i < numEntries; i++ ) {
        unsigned unused;
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, sizes[i]);
        guchar *px = sp_icon_doc_icon(doc, *drawing, id, sizes[i], unused);
//         g_message( " size %d %s", sizes[i], (px ? "worked" : "failed") );
        if ( px ) {
            memcpy( pixMem[i], px, sizes[i] * stride );
            g_free( px );
            px = nullptr;
        } else {
            memset( pixMem[i], 0, sizes[i] * stride );
        }
        images[i]->set(images[i]->get_pixbuf());
        // images[i]->queue_draw();
    }
    updateMagnify();

    renderTimer->stop();
    minDelay = std::max( 0.1, renderTimer->elapsed() * 3.0 );
#if ICON_VERBOSE
    g_message("  render took %f seconds.", renderTimer->elapsed());
#endif // ICON_VERBOSE
}

void IconPreviewPanel::updateMagnify()
{
    Glib::RefPtr<Gdk::Pixbuf> buf = images[hot]->get_pixbuf()->scale_simple( 128, 128, Gdk::INTERP_NEAREST );
    magLabel.set_label( *labels[hot] );
    magnified.set( buf );
    // magnified.queue_draw();
    // magnified.get_parent()->queue_draw();
}

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape

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
