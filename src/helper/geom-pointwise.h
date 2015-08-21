/**
 * \file
 * \brief Pointwise a class to manage a vector of satellites per piecewise node
 */ /*
    * Authors:
    * Jabiertxof
    * Nathan Hurst
    * Johan Engelen
    * Josh Andler
    * suv
    * Mc-
    * Liam P. White
    * Krzysztof Kosiński
    * This code is in public domain
    */

#ifndef SEEN_POINTWISE_H
#define SEEN_POINTWISE_H

#include <helper/geom-satellite.h>
#include <2geom/sbasis.h>
#include <2geom/sbasis-2d.h>
#include <2geom/piecewise.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/path.h>
#include <boost/optional.hpp>

/**
 * @brief Pointwise a class to manage a vector of satellites per piecewise curve
 */
typedef Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2sb;
class Pointwise {
public:
    pwd2sb getPwd2() const;
    void setPwd2(pwd2sb const &pwd2_in);

    /**
     * @parameter curve_based allow the use of a satellite on last node of open paths
     * if not curve based
     */
    std::vector<Satellite> getSatellites(bool curve_based = true);
    void setSatellites(std::vector<Satellite> const &sats, bool curve_based = true);
    /** Update the start satellite on open/closed paths.
    */
    void setStart();
    /** Fired when a path is modified.
    */
    void recalculateForNewPwd2(pwd2sb const &A, Geom::PathVector const &B, Satellite const &S);
    /** Some nodes/subpaths are removed.
    */
    void pwd2Subtract(pwd2sb const &A);
    /** Append nodes/subpaths to current pointwise
    */
    void pwd2Append(pwd2sb const &A, Satellite const &S);
    /** Send a subpath to end and update satellites
    */
    void subpathToBack(size_t subpath);
    /** Reverse a subpath and update satellites
    */
    void subpathReverse(size_t start, size_t end);
    /** Fired when a path is modified duplicating a node. Piecewise ignore degenerated curves.
    */
    void insertDegenerateSatellites(pwd2sb const &A, Geom::PathVector const &B, Satellite const &S);

private:
    pwd2sb _pwd2;
    std::vector<Satellite> _satellites;
};

#endif //SEEN_POINTWISE_H
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:
// filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99
// :
