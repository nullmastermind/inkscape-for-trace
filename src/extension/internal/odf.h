// SPDX-License-Identifier: LGPL-2.1-or-later
/** @file
 * OpenDocument (drawing) input and output
 *//*
 * Authors:
 *   Bob Jamison
 *   Abhishek Sharma
 *
 * Copyright (C) 2018 Authors
 * Released under GNU LGPL v2.1+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_ODG_OUT_H
#define EXTENSION_INTERNAL_ODG_OUT_H

#include <util/ziptool.h>

#include "extension/implementation/implementation.h"

#include <xml/repr.h>
#include <string>
#include <map>

#include "object/uri.h"
class SPItem;

#include <glibmm/ustring.h>

namespace Inkscape
{
namespace Extension
{
namespace Internal
{

typedef Inkscape::IO::Writer Writer;

class StyleInfo
{
public:

    StyleInfo()
        {
        init();
        }

    StyleInfo(const StyleInfo &other)
        {
        assign(other);
        }

    StyleInfo &operator=(const StyleInfo &other)
        {
        assign(other);
        return *this;
        }

    void assign(const StyleInfo &other)
        {
        name          = other.name;
        stroke        = other.stroke;
        strokeColor   = other.strokeColor;
        strokeWidth   = other.strokeWidth;
        strokeOpacity = other.strokeOpacity;
        fill          = other.fill;
        fillColor     = other.fillColor;
        fillOpacity   = other.fillOpacity;
        }

    void init()
        {
        name          = "none";
        stroke        = "none";
        strokeColor   = "none";
        strokeWidth   = "none";
        strokeOpacity = "none";
        fill          = "none";
        fillColor     = "none";
        fillOpacity   = "none";
        }

    virtual ~StyleInfo()
        = default;

    //used for eliminating duplicates in the styleTable
    bool equals(const StyleInfo &other)
        {
        if (
            stroke        != other.stroke        ||
            strokeColor   != other.strokeColor   ||
            strokeWidth   != other.strokeWidth   ||
            strokeOpacity != other.strokeOpacity ||
            fill          != other.fill          ||
            fillColor     != other.fillColor     ||
            fillOpacity   != other.fillOpacity
           )
            return false;
        return true;
        }

    Glib::ustring name;
    Glib::ustring stroke;
    Glib::ustring strokeColor;
    Glib::ustring strokeWidth;
    Glib::ustring strokeOpacity;
    Glib::ustring fill;
    Glib::ustring fillColor;
    Glib::ustring fillOpacity;

};




class GradientStop
{
public:
    GradientStop() : rgb(0), opacity(0)
        {}
    GradientStop(unsigned long rgbArg, double opacityArg)
        { rgb = rgbArg; opacity = opacityArg; }
    virtual ~GradientStop()
        = default;
    GradientStop(const GradientStop &other)
        {  assign(other); }
    virtual GradientStop& operator=(const GradientStop &other)
        {  assign(other); return *this; }
    void assign(const GradientStop &other)
        {
        rgb = other.rgb;
        opacity = other.opacity;
        }
    unsigned long rgb;
    double opacity;
};



class GradientInfo
{
public:

    GradientInfo()
        {
        init();
        }

    GradientInfo(const GradientInfo &other)
        {
        assign(other);
        }

    GradientInfo &operator=(const GradientInfo &other)
        {
        assign(other);
        return *this;
        }

    void assign(const GradientInfo &other)
        {
        name          = other.name;
        style         = other.style;
        cx            = other.cx;
        cy            = other.cy;
        fx            = other.fx;
        fy            = other.fy;
        r             = other.r;
        x1            = other.x1;
        y1            = other.y1;
        x2            = other.x2;
        y2            = other.y2;
        stops         = other.stops;
        }

    void init()
        {
        name          = "none";
        style         = "none";
        cx            = 0.0;
        cy            = 0.0;
        fx            = 0.0;
        fy            = 0.0;
        r             = 0.0;
        x1            = 0.0;
        y1            = 0.0;
        x2            = 0.0;
        y2            = 0.0;
        stops.clear();
        }

    virtual ~GradientInfo()
        = default;

    //used for eliminating duplicates in the styleTable
    bool equals(const GradientInfo &other)
        {
        if (
            name        != other.name   ||
            style       != other.style  ||
            cx          != other.cx     ||
            cy          != other.cy     ||
            fx          != other.fx     ||
            fy          != other.fy     ||
            r           != other.r      ||
            x1          != other.x1     ||
            y1          != other.y1     ||
            x2          != other.x2     ||
            y2          != other.y2
           )
            return false;
        if (stops.size() != other.stops.size())
            return false;
        for (unsigned int i=0 ; i<stops.size() ; i++)
            {
            GradientStop g1 = stops[i];
            GradientStop g2 = other.stops[i];
            if (g1.rgb != g2.rgb)
                return false;
            if (g1.opacity != g2.opacity)
                return false;
            }
        return true;
        }

    Glib::ustring name;
    Glib::ustring style;
    double cx;
    double cy;
    double fx;
    double fy;
    double r;
    double x1;
    double y1;
    double x2;
    double y2;
    std::vector<GradientStop> stops;

};



/**
 * OpenDocument <drawing> input and output
 *
 * This is an an entry in the extensions mechanism to begin to enable
 * the inputting and outputting of OpenDocument Format (ODF) files from
 * within Inkscape.  Although the initial implementations will be very lossy
 * do to the differences in the models of SVG and ODF, they will hopefully
 * improve greatly with time.
 *
 * http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/idl-definitions.html
 */
class OdfOutput : public Inkscape::Extension::Implementation::Implementation
{

public:

    bool check (Inkscape::Extension::Extension * module) override;

    void save  (Inkscape::Extension::Output *mod,
	        SPDocument *doc,
	        gchar const *filename) override;

    static void   init  ();

private:

    std::string docBaseUri;

    void reset();

    //cc or dc metadata name/value pairs
    std::map<Glib::ustring, Glib::ustring> metadata;

    /* Style table
       Uses a two-stage lookup to avoid style duplication.
       Use like:
       StyleInfo si = styleTable[styleLookupTable[id]];
       but check for errors, of course
    */
    //element id -> style entry name
    std::map<Glib::ustring, Glib::ustring> styleLookupTable;
    //style entry name -> style info
    std::vector<StyleInfo> styleTable;

    //element id -> gradient entry name
    std::map<Glib::ustring, Glib::ustring> gradientLookupTable;
    //gradient entry name -> gradient info
    std::vector<GradientInfo> gradientTable;

    //for renaming image file names
    std::map<Glib::ustring, Glib::ustring> imageTable;

    void preprocess(ZipFile &zf, Inkscape::XML::Node *node);

    bool writeManifest(ZipFile &zf);

    bool writeMeta(ZipFile &zf);

    bool writeStyle(ZipFile &zf);

    bool processStyle(SPItem *item, const Glib::ustring &id, const Glib::ustring &gradientNameFill, const Glib::ustring &gradientNameStroke, Glib::ustring& output);

    bool processGradient(SPItem *item,
                    const Glib::ustring &id, Geom::Affine &tf, Glib::ustring& gradientName, Glib::ustring& output, bool checkFillGradient = true);

    bool writeStyleHeader(Writer &outs);

    bool writeStyleFooter(Writer &outs);

    bool writeContentHeader(Writer &outs);

    bool writeContentFooter(Writer &outs);

    bool writeTree(Writer &couts, Writer &souts, Inkscape::XML::Node *node);

    bool writeContent(ZipFile &zf, Inkscape::XML::Node *node);

};


}  //namespace Internal
}  //namespace Extension
}  //namespace Inkscape



#endif /* EXTENSION_INTERNAL_ODG_OUT_H */

