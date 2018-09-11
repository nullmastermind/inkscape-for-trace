// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_SPIRO_CONVERTERS_H
#define INKSCAPE_SPIRO_CONVERTERS_H

#include <2geom/forward.h>
class SPCurve;

namespace Spiro {

class ConverterBase {
public:
    ConverterBase() = default;;
    virtual ~ConverterBase() = default;;

    virtual void moveto(double x, double y) = 0;
    virtual void lineto(double x, double y, bool close_last) = 0;
    virtual void quadto(double x1, double y1, double x2, double y2, bool close_last) = 0;
    virtual void curveto(double x1, double y1, double x2, double y2, double x3, double y3, bool close_last) = 0;
};


/**
 * Converts Spiro to Inkscape's SPCurve
 */
class ConverterSPCurve : public ConverterBase {
public:
    ConverterSPCurve(SPCurve &curve)
        : _curve(curve)
    {}

    void moveto(double x, double y) override;
    void lineto(double x, double y, bool close_last) override;
    void quadto(double x1, double y1, double x2, double y2, bool close_last) override;
    void curveto(double x1, double y1, double x2, double y2, double x3, double y3, bool close_last) override;

private:
    SPCurve &_curve;

    ConverterSPCurve(const ConverterSPCurve&) = delete;
    ConverterSPCurve& operator=(const ConverterSPCurve&) = delete;
};


/**
 * Converts Spiro to 2Geom's Path
 */
class ConverterPath : public ConverterBase {
public:
    ConverterPath(Geom::Path &path);

    void moveto(double x, double y) override;
    void lineto(double x, double y, bool close_last) override;
    void quadto(double x1, double y1, double x2, double y2, bool close_last) override;
    void curveto(double x1, double y1, double x2, double y2, double x3, double y3, bool close_last) override;

private:
    Geom::Path &_path;

    ConverterPath(const ConverterPath&) = delete;
    ConverterPath& operator=(const ConverterPath&) = delete;
};


} // namespace Spiro

#endif
