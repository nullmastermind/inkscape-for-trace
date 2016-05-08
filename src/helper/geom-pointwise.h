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
typedef Geom::Piecewise<Geom::D2<Geom::SBasis> > PwD2SBasisasis;
typedef std::vector< Satelites > Satelites;
class Pointwise {
public:
    PwD2SBasis getPwd2() const;
    Geom::Pathvector getPV() const;
    void setPwd2(PwD2SBasis const &pwd2_in);
    Satelites getSatellites();
    size_t getTotalSatellites();
    void setSatellites(Satelites const &sats);
    void recalculateForNewPwd2(PwD2SBasis const &A, Geom::PathVector const &B, Satellite const &S);
    //Fired when a path is modified.
    void recalculatePwD2(PwD2SBasis const &A, Satellite const &S);
    //Recalculate satellites
    void insertDegenerateSatellites(PwD2SBasis const &A, Geom::PathVector const &B, Satellite const &S);
    //Fired when a path is modified duplicating a node. Piecewise ignore degenerated curves.

private:
    PwD2SBasis _pwd2;
    Geom::PathVector _pathvector;
    Satelites _satellites;
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
