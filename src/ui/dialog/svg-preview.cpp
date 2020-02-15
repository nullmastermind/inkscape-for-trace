// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Implementation of the file dialog interfaces defined in filedialogimpl.h.
 */
/* Authors:
 *   Bob Jamison
 *   Joel Holdsworth
 *   Bruno Dilly
 *   Other dudes from The Inkscape Organization
 *   Abhishek Sharma
 *
 * Copyright (C) 2004-2007 Bob Jamison
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2007-2008 Joel Holdsworth
 * Copyright (C) 2004-2007 The Inkscape Organization
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>
#include <fstream>

#include <glibmm/i18n.h>
#include <glib/gstdio.h>  // GStatBuf

#include "svg-preview.h"

#include "document.h"
#include "ui/view/svg-view-widget.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/*#########################################################################
### SVG Preview Widget
#########################################################################*/

bool SVGPreview::setDocument(SPDocument *doc)
{
    if (viewer) {
        viewer->setDocument(doc);
    } else {
        viewer = Gtk::manage(new Inkscape::UI::View::SVGViewWidget(doc));
        pack_start(*viewer, true, true);
    }

    if (document) {
        delete document;
    }
    document = doc;

    show_all();

    return true;
}


bool SVGPreview::setFileName(Glib::ustring &theFileName)
{
    Glib::ustring fileName = theFileName;

    fileName = Glib::filename_to_utf8(fileName);

    /**
     * I don't know why passing false to keepalive is bad.  But it
     * prevents the display of an svg with a non-ascii filename
     */
    SPDocument *doc = SPDocument::createNewDoc(fileName.c_str(), true);
    if (!doc) {
        g_warning("SVGView: error loading document '%s'\n", fileName.c_str());
        return false;
    }

    setDocument(doc);

    return true;
}



bool SVGPreview::setFromMem(char const *xmlBuffer)
{
    if (!xmlBuffer)
        return false;

    gint len = (gint)strlen(xmlBuffer);
    SPDocument *doc = SPDocument::createNewDocFromMem(xmlBuffer, len, false);
    if (!doc) {
        g_warning("SVGView: error loading buffer '%s'\n", xmlBuffer);
        return false;
    }

    setDocument(doc);

    return true;
}



void SVGPreview::showImage(Glib::ustring &theFileName)
{
    Glib::ustring fileName = theFileName;

    // Let's get real width and height from SVG file. These are template
    // files so we assume they are well formed.

    // std::cout << "SVGPreview::showImage: " << theFileName << std::endl;
    std::string width;
    std::string height;

    /*#####################################
    # LET'S HAVE SOME FUN WITH SVG!
    # Instead of just loading an image, why
    # don't we make a lovely little svg and
    # display it nicely?
    #####################################*/

    // Arbitrary size of svg doc -- rather 'portrait' shaped
    gint previewWidth = 400;
    gint previewHeight = 600;

    // Get some image info. Smart pointer does not need to be deleted
    Glib::RefPtr<Gdk::Pixbuf> img(nullptr);
    try
    {
        img = Gdk::Pixbuf::create_from_file(fileName);
    }
    catch (const Glib::FileError &e)
    {
        g_message("caught Glib::FileError in SVGPreview::showImage");
        return;
    }
    catch (const Gdk::PixbufError &e)
    {
        g_message("Gdk::PixbufError in SVGPreview::showImage");
        return;
    }
    catch (...)
    {
        g_message("Caught ... in SVGPreview::showImage");
        return;
    }

    gint imgWidth = img->get_width();
    gint imgHeight = img->get_height();
    
    Glib::ustring svg = ".svg";
    if (hasSuffix(fileName, svg)) {
        std::ifstream input(theFileName.c_str());
        if( !input ) {
            std::cerr << "SVGPreview::showImage: Failed to open file: " << theFileName << std::endl;
        } else {

            std::string token;

            Glib::MatchInfo match_info;
            Glib::RefPtr<Glib::Regex> regex1 = Glib::Regex::create("width=\"(.*)\"");
            Glib::RefPtr<Glib::Regex> regex2 = Glib::Regex::create("height=\"(.*)\"");
     
            while( !input.eof() && (height.empty() || width.empty()) ) {

                input >> token;
                // std::cout << "|" << token << "|" << std::endl;

                if (regex1->match(token, match_info)) {
                    width = match_info.fetch(1).raw();
                }

                if (regex2->match(token, match_info)) {
                    height = match_info.fetch(1).raw();
                }

            }
        }
    }
    
    // TODO: replace int to string conversion with std::to_string when fully C++11 compliant
    if (height.empty() || width.empty()) {
        std::ostringstream s_width;
        std::ostringstream s_height;
        s_width << imgWidth;
        s_height << imgHeight;
        width = s_width.str();
        height = s_height.str();
    }

    // Find the minimum scale to fit the image inside the preview area
    double scaleFactorX = (0.9 * (double)previewWidth) / ((double)imgWidth);
    double scaleFactorY = (0.9 * (double)previewHeight) / ((double)imgHeight);
    double scaleFactor = scaleFactorX;
    if (scaleFactorX > scaleFactorY)
        scaleFactor = scaleFactorY;

    // Now get the resized values
    gint scaledImgWidth = (int)(scaleFactor * (double)imgWidth);
    gint scaledImgHeight = (int)(scaleFactor * (double)imgHeight);

    // center the image on the area
    gint imgX = (previewWidth - scaledImgWidth) / 2;
    gint imgY = (previewHeight - scaledImgHeight) / 2;

    // wrap a rectangle around the image
    gint rectX = imgX - 1;
    gint rectY = imgY - 1;
    gint rectWidth = scaledImgWidth + 2;
    gint rectHeight = scaledImgHeight + 2;

    // Our template.  Modify to taste
    gchar const *xformat = R"A(
<svg width="%d" height="%d"
  xmlns="http://www.w3.org/2000/svg"
  xmlns:xlink="http://www.w3.org/1999/xlink">
  <rect width="100%" height="100%" style="fill:#eeeeee"/>
  <image x="%d" y="%d" width="%d" height="%d" xlink:href="%s"/>
  <rect  x="%d" y="%d" width="%d" height="%d" style="fill:none;stroke:black"/>
  <text  x="50%" y="55%" style="font-family:sans-serif;font-size:24px;text-anchor:middle">%s x %s</text>
</svg>
)A";

    // if (!Glib::get_charset()) //If we are not utf8
    fileName = Glib::filename_to_utf8(fileName);
    // Filenames in xlinks are decoded, so any % will break without this.
    auto encodedName = Glib::uri_escape_string(fileName);

    // Fill in the template
    /* FIXME: Do proper XML quoting for fileName. */
    gchar *xmlBuffer =
        g_strdup_printf(xformat, previewWidth, previewHeight, imgX, imgY, scaledImgWidth, scaledImgHeight,
                        encodedName.c_str(), rectX, rectY, rectWidth, rectHeight, width.c_str(), height.c_str() );

    // g_message("%s\n", xmlBuffer);

    // now show it!
    setFromMem(xmlBuffer);
    g_free(xmlBuffer);
}



void SVGPreview::showNoPreview()
{
    // Are we already showing it?
    if (showingNoPreview)
        return;

    // Our template.  Modify to taste
    gchar const *xformat = R"B(
<svg width="400" height="600"
     xmlns="http://www.w3.org/2000/svg"
     xmlns:xlink="http://www.w3.org/1999/xlink">
  <g transform="translate(-160,90)" style="opacity:0.10">
    <path style="fill:white"
          d="M 397.64309 320.25301 L 280.39197 282.517 L 250.74227 124.83447 L 345.08225
             29.146783 L 393.59996 46.667064 L 483.89679 135.61619 L 397.64309 320.25301 z"/>
    <path d="M 476.95792 339.17168 C 495.78197 342.93607 499.54842 356.11361 495.78197 359.87802
             C 492.01856 363.6434 482.6065 367.40781 475.07663 361.76014 C 467.54478
             356.11361 467.54478 342.93607 476.95792 339.17168 z"
             id="droplet01" />
    <path d="M 286.46194 340.42914 C 284.6277 340.91835 269.30405 327.71337 257.16909 333.8338
             C 245.03722 339.95336 236.89276 353.65666 248.22676 359.27982 C 259.56184 364.90298
             267.66433 358.41867 277.60113 351.44119 C 287.53903 344.46477
             287.18046 343.1206 286.46194 340.42914 z"
             id= "droplet02"/>
    <path d="M 510.35756 306.92856 C 520.59494 304.36879 544.24333 306.92856 540.47688 321.98634
             C 536.71354 337.04806 504.71297 331.39827 484.00371 323.87156 C 482.12141
             308.81083 505.53237 308.13423 510.35756 306.92856 z"
             id="droplet03"/>
    <path d="M 359.2403 21.362537 C 347.92693 21.362537 336.6347 25.683095 327.96556 34.35223 
             L 173.87387 188.41466 C 165.37697 196.9114 161.1116 207.95813 160.94269 219.04577
             L 160.88418 219.04577 C 160.88418 219.08524 160.94076 219.12322 160.94269 219.16279
             C 160.94033 219.34888 160.88418 219.53256 160.88418 219.71865 L 161.14748 219.71865
             C 164.0966 230.93917 240.29699 245.24198 248.79866 253.74346 C 261.63771 266.58263
             199.5652 276.01151 212.4041 288.85074 C 225.24316 301.68979 289.99433 313.6933 302.8346
             326.53254 C 315.67368 339.37161 276.5961 353.04289 289.43532 365.88196 C 302.27439
             378.72118 345.40201 362.67257 337.5908 396.16198 C 354.92909 413.50026 391.10302
             405.2208 415.32417 387.88252 C 428.16323 375.04345 390.6948 376.17577 403.53397
             363.33668 C 416.37304 350.49745 448.78128 350.4282 476.08902 319.71589 C 465.09739
             302.62116 429.10801 295.34136 441.94719 282.50217 C 454.78625 269.66311 479.74708
             276.18423 533.60644 251.72479 C 559.89837 239.78398 557.72636 230.71459 557.62567
             219.71865 C 557.62356 219.48727 557.62567 219.27892 557.62567 219.04577 L 557.56716
             219.04577 C 557.3983 207.95812 553.10345 196.9114 544.60673 188.41466 L 390.54428
             34.35223 C 381.87515 25.683095 370.55366 21.362537 359.2403 21.362537 z M 357.92378
             41.402939 C 362.95327 41.533963 367.01541 45.368018 374.98006 50.530832 L 447.76915
             104.50827 C 448.56596 105.02498 449.32484 105.564 450.02187 106.11735 C 450.7189 106.67062
             451.3556 107.25745 451.95277 107.84347 C 452.54997 108.42842 453.09281 109.01553 453.59111
             109.62808 C 454.08837 110.24052 454.53956 110.86661 454.93688 111.50048 C 455.33532 112.13538
             455.69164 112.78029 455.9901 113.43137 C 456.28877 114.08363 456.52291 114.75639 456.7215
             115.42078 C 456.92126 116.08419 457.08982 116.73973 457.18961 117.41019 C 457.28949
             118.08184 457.33588 118.75535 457.33588 119.42886 L 414.21245 98.598549 L 409.9118
             131.16055 L 386.18512 120.04324 L 349.55654 144.50131 L 335.54288 96.1703 L 317.4919
             138.4453 L 267.08369 143.47735 L 267.63956 121.03795 C 267.63956 115.64823 296.69685
             77.915899 314.39075 68.932902 L 346.77721 45.674327 C 351.55594 42.576634 354.90608
             41.324327 357.92378 41.402939 z M 290.92738 261.61333 C 313.87149 267.56365 339.40299
             275.37038 359.88393 275.50997 L 360.76161 284.72563 C 343.2235 282.91785 306.11346
             274.45012 297.36372 269.98057 L 290.92738 261.61333 z"
             id="mountainDroplet"/>
  </g>
  <text xml:space="preserve" x="200" y="320"
        style="font-size:32px;font-weight:bold;text-anchor:middle">%s</text>
</svg>
)B";

    // Fill in the template
    gchar *xmlBuffer = g_strdup_printf(xformat, _("No preview"));

    // g_message("%s\n", xmlBuffer);

    // Now show it!
    setFromMem(xmlBuffer);
    g_free(xmlBuffer);
    showingNoPreview = true;
}


/**
 * Inform the user that the svg file is too large to be displayed.
 * This does not check for sizes of embedded images (yet)
 */
void SVGPreview::showTooLarge(long fileLength)
{
    // Our template.  Modify to taste
    gchar const *xformat = R"C(
<svg width="400" height="600"
     xmlns="http://www.w3.org/2000/svg"
     xmlns:xlink="http://www.w3.org/1999/xlink">
  <g transform="translate(-160,90)" style="opacity:0.10">
    <path style="fill:white"
          d="M 397.64309 320.25301 L 280.39197 282.517 L 250.74227 124.83447 L 345.08225
             29.146783 L 393.59996 46.667064 L 483.89679 135.61619 L 397.64309 320.25301 z"/>
    <path d="M 476.95792 339.17168 C 495.78197 342.93607 499.54842 356.11361 495.78197 359.87802
             C 492.01856 363.6434 482.6065 367.40781 475.07663 361.76014 C 467.54478
             356.11361 467.54478 342.93607 476.95792 339.17168 z"
             id="droplet01" />
    <path d="M 286.46194 340.42914 C 284.6277 340.91835 269.30405 327.71337 257.16909 333.8338
             C 245.03722 339.95336 236.89276 353.65666 248.22676 359.27982 C 259.56184 364.90298
             267.66433 358.41867 277.60113 351.44119 C 287.53903 344.46477
             287.18046 343.1206 286.46194 340.42914 z"
             id= "droplet02"/>
    <path d="M 510.35756 306.92856 C 520.59494 304.36879 544.24333 306.92856 540.47688 321.98634
             C 536.71354 337.04806 504.71297 331.39827 484.00371 323.87156 C 482.12141
             308.81083 505.53237 308.13423 510.35756 306.92856 z"
             id="droplet03"/>
    <path d="M 359.2403 21.362537 C 347.92693 21.362537 336.6347 25.683095 327.96556 34.35223 
             L 173.87387 188.41466 C 165.37697 196.9114 161.1116 207.95813 160.94269 219.04577
             L 160.88418 219.04577 C 160.88418 219.08524 160.94076 219.12322 160.94269 219.16279
             C 160.94033 219.34888 160.88418 219.53256 160.88418 219.71865 L 161.14748 219.71865
             C 164.0966 230.93917 240.29699 245.24198 248.79866 253.74346 C 261.63771 266.58263
             199.5652 276.01151 212.4041 288.85074 C 225.24316 301.68979 289.99433 313.6933 302.8346
             326.53254 C 315.67368 339.37161 276.5961 353.04289 289.43532 365.88196 C 302.27439
             378.72118 345.40201 362.67257 337.5908 396.16198 C 354.92909 413.50026 391.10302
             405.2208 415.32417 387.88252 C 428.16323 375.04345 390.6948 376.17577 403.53397
             363.33668 C 416.37304 350.49745 448.78128 350.4282 476.08902 319.71589 C 465.09739
             302.62116 429.10801 295.34136 441.94719 282.50217 C 454.78625 269.66311 479.74708
             276.18423 533.60644 251.72479 C 559.89837 239.78398 557.72636 230.71459 557.62567
             219.71865 C 557.62356 219.48727 557.62567 219.27892 557.62567 219.04577 L 557.56716
             219.04577 C 557.3983 207.95812 553.10345 196.9114 544.60673 188.41466 L 390.54428
             34.35223 C 381.87515 25.683095 370.55366 21.362537 359.2403 21.362537 z M 357.92378
             41.402939 C 362.95327 41.533963 367.01541 45.368018 374.98006 50.530832 L 447.76915
             104.50827 C 448.56596 105.02498 449.32484 105.564 450.02187 106.11735 C 450.7189 106.67062
             451.3556 107.25745 451.95277 107.84347 C 452.54997 108.42842 453.09281 109.01553 453.59111
             109.62808 C 454.08837 110.24052 454.53956 110.86661 454.93688 111.50048 C 455.33532 112.13538
             455.69164 112.78029 455.9901 113.43137 C 456.28877 114.08363 456.52291 114.75639 456.7215
             115.42078 C 456.92126 116.08419 457.08982 116.73973 457.18961 117.41019 C 457.28949
             118.08184 457.33588 118.75535 457.33588 119.42886 L 414.21245 98.598549 L 409.9118
             131.16055 L 386.18512 120.04324 L 349.55654 144.50131 L 335.54288 96.1703 L 317.4919
             138.4453 L 267.08369 143.47735 L 267.63956 121.03795 C 267.63956 115.64823 296.69685
             77.915899 314.39075 68.932902 L 346.77721 45.674327 C 351.55594 42.576634 354.90608
             41.324327 357.92378 41.402939 z M 290.92738 261.61333 C 313.87149 267.56365 339.40299
             275.37038 359.88393 275.50997 L 360.76161 284.72563 C 343.2235 282.91785 306.11346
             274.45012 297.36372 269.98057 L 290.92738 261.61333 z"
             id="mountainDroplet"/>
  </g>
  <text xml:space="preserve" x="200" y="280"
        style="font-size:20px;font-weight:bold;text-anchor:middle">%.1f MB</text>
  <text xml:space="preserve" x="200" y="360"
        style="font-size:20px;font-weight:bold;text-anchor:middle">%s</text>
</svg>
)C";


    // Fill in the template
    double floatFileLength = ((double)fileLength) / 1048576.0;
    // printf("%ld %f\n", fileLength, floatFileLength);

    gchar *xmlBuffer =
        g_strdup_printf(xformat, floatFileLength, _("Too large for preview"));

    // g_message("%s\n", xmlBuffer);

    // now show it!
    setFromMem(xmlBuffer);
    g_free(xmlBuffer);
}

bool SVGPreview::set(Glib::ustring &fileName, int dialogType)
{

    if (!Glib::file_test(fileName, Glib::FILE_TEST_EXISTS)) {
        showNoPreview();
        return false;
    }

    if (Glib::file_test(fileName, Glib::FILE_TEST_IS_DIR)) {
        showNoPreview();
        return false;
    }

    if (Glib::file_test(fileName, Glib::FILE_TEST_IS_REGULAR)) {
        Glib::ustring fileNameUtf8 = Glib::filename_to_utf8(fileName);
        gchar *fName = const_cast<gchar *>(
            fileNameUtf8.c_str()); // const-cast probably not necessary? (not necessary on Windows version of stat())
        GStatBuf info;
        if (g_stat(fName, &info)) // stat returns 0 upon success
        {
            g_warning("SVGPreview::set() : %s : %s", fName, strerror(errno));
            return false;
        }
        if (info.st_size > 0xA00000L) {
            showingNoPreview = false;
            showTooLarge(info.st_size);
            return false;
        }
    }

    Glib::ustring svg = ".svg";
    Glib::ustring svgz = ".svgz";

    if ((dialogType == SVG_TYPES || dialogType == IMPORT_TYPES) &&
        (hasSuffix(fileName, svg) || hasSuffix(fileName, svgz))) {
        bool retval = setFileName(fileName);
        showingNoPreview = false;
        return retval;
    } else if (isValidImageFile(fileName)) {
        showImage(fileName);
        showingNoPreview = false;
        return true;
    } else {
        showNoPreview();
        return false;
    }
}


SVGPreview::SVGPreview()
    : document(nullptr)
    , viewer(nullptr)
    , showingNoPreview(false)
{
    set_size_request(200, 300);
}

SVGPreview::~SVGPreview()
{
    if (viewer) {
        viewer->setDocument(nullptr);
    }  
    delete document; 
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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

