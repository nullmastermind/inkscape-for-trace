#include <glib.h> //g_critical

#include "pathoutlineprovider.h"
#include "livarot/path-description.h"
#include <2geom/angle.h>
#include <2geom/path.h>
#include <2geom/circle.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/shape.h>
#include <2geom/transforms.h>
#include <2geom/path-sink.h>
#include <svg/svg.h>

namespace Geom {
/**
* Refer to: Weisstein, Eric W. "Circle-Circle Intersection."
		 From MathWorld--A Wolfram Web Resource.
		 http://mathworld.wolfram.com/Circle-CircleIntersection.html
*
* @return 0 if no intersection
* @return 1 if one circle is contained in the other
* @return 2 if intersections are found (they are written to p0 and p1)
*/
static int circle_circle_intersection(Circle const &circle0, Circle const &circle1,
                                      Point & p0, Point & p1)
{
    Point X0 = circle0.center();
    double r0 = circle0.ray();
    Point X1 = circle1.center();
    double r1 = circle1.ray();

    /* dx and dy are the vertical and horizontal distances between
    * the circle centers.
    */
    Point D = X1 - X0;

    /* Determine the straight-line distance between the centers. */
    double d = L2(D);

    /* Check for solvability. */
    if (d > (r0 + r1)) {
        /* no solution. circles do not intersect. */
        return 0;
    }
    if (d <= fabs(r0 - r1)) {
        /* no solution. one circle is contained in the other */
        return 1;
    }

    /* 'point 2' is the point where the line through the circle
    * intersection points crosses the line between the circle
    * centers.
    */

    /* Determine the distance from point 0 to point 2. */
    double a = ((r0*r0) - (r1*r1) + (d*d)) / (2.0 * d) ;

    /* Determine the coordinates of point 2. */
    Point p2 = X0 + D * (a/d);

    /* Determine the distance from point 2 to either of the
    * intersection points.
    */
    double h = std::sqrt((r0*r0) - (a*a));

    /* Now determine the offsets of the intersection points from
    * point 2.
    */
    Point r = (h/d)*rot90(D);

    /* Determine the absolute intersection points. */
    p0 = p2 + r;
    p1 = p2 - r;

    return 2;
}
/**
* Find circle that touches inside of the curve, with radius matching the curvature, at time value \c t.
* Because this method internally uses unitTangentAt, t should be smaller than 1.0 (see unitTangentAt).
*/
static Circle touching_circle( D2<SBasis> const &curve, double t, double tol=0.01 )
{
    D2<SBasis> dM=derivative(curve);
    if ( are_near(L2sq(dM(t)),0.) ) {
        dM=derivative(dM);
    }
    if ( are_near(L2sq(dM(t)),0.) ) {   // try second time
        dM=derivative(dM);
    }
    Piecewise<D2<SBasis> > unitv = unitVector(dM,tol);
    Piecewise<SBasis> dMlength = dot(Piecewise<D2<SBasis> >(dM),unitv);
    Piecewise<SBasis> k = cross(derivative(unitv),unitv);
    k = divide(k,dMlength,tol,3);
    double curv = k(t); // note that this value is signed

    Geom::Point normal = unitTangentAt(curve, t).cw();
    double radius = 1/curv;
    Geom::Point center = curve(t) + radius*normal;
    return Geom::Circle(center, fabs(radius));
}

static std::vector<Geom::Path> split_at_cusps(const Geom::Path& in)
{
    Geom::PathVector out = Geom::PathVector();
    Geom::Path temp = Geom::Path();

    for (unsigned path_descr = 0; path_descr < in.size(); path_descr++) {
        temp = Geom::Path();
        temp.append(in[path_descr]);
        out.push_back(temp);
    }

    return out;
}

static Geom::CubicBezier sbasis_to_cubicbezier(Geom::D2<Geom::SBasis> const & sbasis_in)
{
    std::vector<Geom::Point> temp;
    sbasis_to_bezier(temp, sbasis_in, 4);
    return Geom::CubicBezier( temp );
}

static boost::optional<Geom::Point> intersection_point( Geom::Point const & origin_a, Geom::Point const & vector_a,
        Geom::Point const & origin_b, Geom::Point const & vector_b)
{
    Geom::Coord denom = cross(vector_b, vector_a);
    if (!Geom::are_near(denom,0.)) {
        Geom::Coord t = (cross(origin_a,vector_b) + cross(vector_b,origin_b)) / denom;
        return origin_a + t * vector_a;
    }
    return boost::none;
}
}

namespace Outline {

typedef Geom::D2<Geom::SBasis> D2SB;
typedef Geom::Piecewise<D2SB> PWD2;

unsigned bezierOrder (const Geom::Curve* curve_in)
{
    using namespace Geom;
    //cast it
    const CubicBezier *cbc = dynamic_cast<const CubicBezier*>(curve_in);
    if (cbc) return 3;
    const QuadraticBezier * qbc = dynamic_cast<const QuadraticBezier*>(curve_in);
    if (qbc) return 2;
    const BezierCurveN<1U> * lbc = dynamic_cast<const BezierCurveN<1U> *>(curve_in);
    if (lbc) return 1;
    return 0;
}

//returns true if the angle formed by the curves and their handles
//is >180 clockwise, otherwise false.
bool outside_angle (const Geom::Curve& cbc1, const Geom::Curve& cbc2)
{
    unsigned order = bezierOrder(&cbc1);

    Geom::Point start_point;
    Geom::Point cross_point = cbc1.finalPoint();
    Geom::Point end_point;

    //assert(cbc1.finalPoint() == cbc2.initialPoint());
    //short circuiting?
    if (cbc1.finalPoint() != cbc2.initialPoint()) {
        printf("There was an issue when asserting that one curve's end is the start of the other. Line %d, File %s\n"
               "By default we are going to say that this is an inside join, so we cannot make a line join for it.\n", __LINE__, __FILE__);
        return false;
    }
    switch (order) {
    case 3:
        start_point = (static_cast<const Geom::CubicBezier*>(&cbc1))->operator[] (2);
        //major league b***f***ing
        if (are_near(start_point, cross_point, 0.0000001)) {
            start_point = (static_cast<const Geom::CubicBezier*>(&cbc1))->operator[] (1);
        }
        break;
    case 2:
        //this never happens
        start_point = (static_cast<const Geom::QuadraticBezier*>(&cbc1))->operator[] (1);
        break;
    case 1:
    default:
        start_point = cbc1.initialPoint();
    }

    order = Outline::bezierOrder(&cbc2);

    switch (order) {
    case 3:
        end_point = (static_cast<const Geom::CubicBezier*>(&cbc2))->operator[] (1);
        if (are_near(end_point, cross_point, 0.0000001)) {
            end_point = (static_cast<const Geom::CubicBezier*>(&cbc2))->operator[] (2);
        }
        break;
    case 2:
        end_point = (static_cast<const Geom::QuadraticBezier*>(&cbc2))->operator[] (1);
        break;
    case 1:
    default:
        end_point = cbc2.finalPoint();
    }
    //got our three points, now let's see what their clockwise angle is

    //Much credit to Wikipedia for the following ( http://en.wikipedia.org/wiki/Graham_scan )
    /********************************************************************
    # Three points are a counter-clockwise turn if ccw > 0, clockwise if
    # ccw < 0, and collinear if ccw = 0 because ccw is a determinant that
    # gives the signed area of the triangle formed by p1, p2 and p3.
    function ccw(p1, p2, p3):
        return (p2.x - p1.x)*(p3.y - p1.y) - (p2.y - p1.y)*(p3.x - p1.x)
    *********************************************************************/

    double ccw = ( (cross_point.x() - start_point.x()) * (end_point.y() - start_point.y()) ) -
                 ( (cross_point.y() - start_point.y()) * (end_point.x() - start_point.x()) );
    if (ccw > 0) return true;
    return false;
}

void extrapolate_curves(Geom::Path& path_builder, Geom::Curve* cbc1, Geom::Curve* cbc2, Geom::Point endPt,
                        double miter_limit, double line_width, bool outside = false)
{
    bool lineProblem = (dynamic_cast<Geom::BezierCurveN<1u> *>(cbc1)) || (dynamic_cast<Geom::BezierCurveN<1u> *>(cbc2));
    if ( outside && !lineProblem ) {
        Geom::Path pth;
        pth.append(*cbc1);

        Geom::Point tang1 = Geom::unitTangentAt(Geom::reverse(pth.toPwSb()[0]), 0.);

        pth = Geom::Path();
        pth.append( *cbc2 );
        Geom::Point tang2 = Geom::unitTangentAt(pth.toPwSb()[0], 0);


        Geom::Circle circle1 = Geom::touching_circle(Geom::reverse(cbc1->toSBasis()), 0.);
        Geom::Circle circle2 = Geom::touching_circle(cbc2->toSBasis(), 0);

        Geom::Point points[2];
        int solutions = Geom::circle_circle_intersection(circle1, circle2, points[0], points[1]);
        if (solutions == 2) {
            Geom::Point sol(0,0);
            if ( dot(tang2,points[0]-endPt) > 0 ) {
                // points[0] is bad, choose points[1]
                sol = points[1];
            } else if ( dot(tang2,points[1]-endPt) > 0 ) { // points[0] could be good, now check points[1]
                // points[1] is bad, choose points[0]
                sol = points[0];
            } else {
                // both points are good, choose nearest
                sol = ( distanceSq(endPt, points[0]) < distanceSq(endPt, points[1]) ) ?
                      points[0] : points[1];
            }
            Geom::EllipticalArc *arc0 = circle1.arc(cbc1->finalPoint(), 0.5*(cbc1->finalPoint()+sol), sol, true);
            Geom::EllipticalArc *arc1 = circle2.arc(sol, 0.5*(sol+endPt), endPt, true);
            try {
                if (arc0) {
                    path_builder.append (arc0->toSBasis());
                    delete arc0;
                    arc0 = NULL;
                }

                if (arc1) {
                    path_builder.append (arc1->toSBasis());
                    delete arc1;
                    arc1 = NULL;
                }
            } catch (std::exception & ex) {
                printf("Exception occured, probably NaN or infinite valued points: %s\n", ex.what());
                path_builder.appendNew<Geom::LineSegment>(endPt);
            }
        } else {
            boost::optional <Geom::Point> p = intersection_point (cbc1->finalPoint(), tang1,
                                              cbc2->initialPoint(), tang2);
            if (p) {
                //check size of miter
                Geom::Point point_on_path = cbc1->finalPoint() - rot90(tang1) * line_width;
                Geom::Coord len = distance(*p, point_on_path);
                if (len <= miter_limit) {
                    // miter OK
                    path_builder.appendNew<Geom::LineSegment> (*p);
                }
            }
            path_builder.appendNew<Geom::LineSegment> (endPt);
        }
    }
    if ( outside && lineProblem ) {
        Geom::Path pth;
        pth.append(*cbc1);
        Geom::Point tang1 = Geom::unitTangentAt(Geom::reverse(pth.toPwSb()[0]), 0.);
        pth = Geom::Path();
        pth.append( *cbc2 );
        Geom::Point tang2 = Geom::unitTangentAt(pth.toPwSb()[0], 0);

        boost::optional <Geom::Point> p = intersection_point (cbc1->finalPoint(), tang1,
                                          cbc2->initialPoint(), tang2);
        if (p) {
            //check size of miter
            Geom::Point point_on_path = cbc1->finalPoint() - rot90(tang1) * line_width;
            Geom::Coord len = distance(*p, point_on_path);
            if (len <= miter_limit) {
                // miter OK
                path_builder.appendNew<Geom::LineSegment> (*p);
            }
        }
        path_builder.appendNew<Geom::LineSegment> (endPt);
    }
    if ( !outside ) {
        path_builder.appendNew<Geom::LineSegment> (endPt);
    }
}

void reflect_curves(Geom::Path& path_builder, Geom::Curve* cbc1, Geom::Curve* cbc2, Geom::Point endPt,
                    double miter_limit, double line_width, bool outside = false)
{
    //the most important work for the reflected join is done here

    //determine where we are in the path. If we're on the inside, ignore
    //and just lineTo. On the outside, we'll do a little reflection magic :)
    Geom::Crossings cross;

    if (outside) {
        Geom::Path pth;
        pth.append(*cbc1);

        Geom::Point tang1 = Geom::unitTangentAt(Geom::reverse(pth.toPwSb()[0]), 0.);

        //reflect curves along the bevel
        D2SB newcurve1 = pth.toPwSb()[0] *
                         Geom::reflection ( -Geom::rot90(tang1) ,
                                            cbc1->finalPoint() );

        Geom::CubicBezier bzr1 = sbasis_to_cubicbezier(Geom::reverse(newcurve1));

        pth = Geom::Path();
        pth.append( *cbc2 );
        Geom::Point tang2 = Geom::unitTangentAt(pth.toPwSb()[0], 0);

        D2SB newcurve2 = pth.toPwSb()[0] *
                         Geom::reflection ( -Geom::rot90(tang2) ,
                                            cbc2->initialPoint() );
        Geom::CubicBezier bzr2 = sbasis_to_cubicbezier(Geom::reverse(newcurve2));

        cross = Geom::crossings(bzr1, bzr2);
        if ( cross.empty() ) {
            //curves didn't cross; default to miter
            boost::optional <Geom::Point> p = intersection_point (cbc1->finalPoint(), tang1,
                                              cbc2->initialPoint(), tang2);
            if (p) {
                //check size of miter
                Geom::Point point_on_path = cbc1->finalPoint() - rot90(tang1) * line_width;
                Geom::Coord len = distance(*p, point_on_path);
                if (len <= miter_limit) {
                    // miter OK
                    path_builder.appendNew<Geom::LineSegment> (*p);
                }
            }
            //bevel
            path_builder.appendNew<Geom::LineSegment>( endPt );
        } else {
            //join
            std::pair<Geom::CubicBezier, Geom::CubicBezier> sub1 = bzr1.subdivide(cross[0].ta);
            std::pair<Geom::CubicBezier, Geom::CubicBezier> sub2 = bzr2.subdivide(cross[0].tb);

            //@TODO joins have a strange tendency to cross themselves twice. Check this.

            //sections commented out are for general stability
            path_builder.appendNew <Geom::CubicBezier> (sub1.first[1], sub1.first[2], /*sub1.first[3]*/ sub2.second[0] );
            path_builder.appendNew <Geom::CubicBezier> (sub2.second[1], sub2.second[2], /*sub2.second[3]*/ endPt );
        }
    } else { // cross.empty()
        //probably on the inside of the corner
        path_builder.appendNew<Geom::LineSegment> ( endPt );
    }
}

/** @brief Converts a path to one half of an outline.
* path_in: The input path to use. (To create the other side use path_in.reverse() )
* line_width: the line width to use (usually you want to divide this by 2)
* miter_limit: the miter parameter
* extrapolate: whether the join should be extrapolated instead of reflected
*/
Geom::Path doAdvHalfOutline(const Geom::Path& path_in, double line_width, double miter_limit, bool extrapolate = false)
{
    // NOTE: it is important to notice the distinction between a Geom::Path and a livarot Path here!
    // if you do not see "Geom::" there is a different function set!
    Geom::PathVector pv = split_at_cusps(path_in);

    Path to_outline;
    Path outlined_result;

    Geom::Path path_builder = Geom::Path(); //the path to store the result in
    Geom::PathVector * path_vec; //needed because livarot returns a goddamn pointer

    const unsigned k = path_in.size();

    for (unsigned u = 0; u < k; u+=2) {
        to_outline = Path();
        outlined_result = Path();

        to_outline.LoadPath(pv[u], Geom::Affine(), false, false);
        to_outline.OutsideOutline(&outlined_result, line_width / 2, join_straight, butt_straight, 10);
        //now a curve has been outside outlined and loaded into outlined_result

        //get the Geom::Path
        path_vec = outlined_result.MakePathVector();

        //thing to do on the first run through
        if (u == 0) {
            //I could use the pv->operator[] (0) notation but that looks terrible
            path_builder.start( (*path_vec)[0].initialPoint() );
        } else {
            //get the curves ready for the operation
            Geom::Curve * cbc1 = path_builder[path_builder.size() - 1].duplicate();
            Geom::Curve * cbc2 = (*path_vec)[0]  [0].duplicate();

            //do the reflection/extrapolation:
            if (extrapolate) {
                extrapolate_curves(path_builder, cbc1, cbc2, (*path_vec)[0].initialPoint(), miter_limit, line_width,
                                   outside_angle ( pv[u - 1] [pv[u - 1].size() - 1], pv[u] [0] ));
            } else {
                reflect_curves (path_builder, cbc1, cbc2, (*path_vec)[0].initialPoint(), miter_limit, line_width,
                                outside_angle ( pv[u - 1] [pv[u - 1].size() - 1], pv[u] [0] ));
            }
        }

        path_builder.append( (*path_vec)[0] );

        //outline the next segment, but don't store it yet
        if (path_vec) delete path_vec;

        if (u < k - 1) {
            outlined_result = Path();
            to_outline = Path();

            to_outline.LoadPath(pv[u+1], Geom::Affine(), false, false);
            to_outline.OutsideOutline(&outlined_result, line_width / 2, join_straight, butt_straight, 10);

            path_vec = outlined_result.MakePathVector();

            //get the curves ready for the operation
            Geom::Curve * cbc1 = path_builder[path_builder.size() - 1].duplicate();
            Geom::Curve * cbc2 = (*path_vec)[0]  [0].duplicate();

            //do the reflection/extrapolation:
            if (extrapolate) {
                extrapolate_curves(path_builder, cbc1, cbc2, (*path_vec)[0].initialPoint(), miter_limit, line_width,
                                   outside_angle ( pv[u] [pv[u].size()-1], pv[u+1] [0] ));
            } else {
                reflect_curves (path_builder, cbc1, cbc2, (*path_vec)[0].initialPoint(), miter_limit, line_width,
                                outside_angle ( pv[u] [pv[u].size()-1], pv[u+1] [0] ));
            }

            //Now we can store it.
            path_builder.append( (*path_vec)[0] );

            if (cbc1) delete cbc1;
            if (cbc2) delete cbc2;
            if (path_vec) delete path_vec;
        }
    }

    return path_builder;
}

Geom::PathVector outlinePath(const Geom::PathVector& path_in, double line_width, LineJoinType join,
                             ButtType butt, double miter_lim, bool extrapolate)
{
    Geom::PathVector path_out;

    unsigned pv_size = path_in.size();
    for (unsigned i = 0; i < pv_size; i++) {

        if (path_in[i].size() > 1) {
            //since you've made it this far, hopefully all this is obvious :P
            Geom::Path with_direction;
            Geom::Path against_direction;
            if ( !path_in[i].closed() ) {
                with_direction = Outline::doAdvHalfOutline( path_in[i], -line_width, miter_lim, extrapolate );
                against_direction = Outline::doAdvHalfOutline( path_in[i].reverse(), -line_width, miter_lim, extrapolate );
            } else {
                //Geom::Path absolutely refuses to do what I want with these
                Geom::Path newPath = path_in[i];
                newPath.close(false);
                Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = newPath.toPwSb();
                newPath = Geom::path_from_piecewise(pwd2, 0.01)[0];
                //fuk this
                with_direction = Outline::doAdvHalfOutline( newPath, -line_width, miter_lim, extrapolate );
                against_direction = Outline::doAdvHalfOutline( newPath.reverse(), -line_width, miter_lim, extrapolate );

                /*if (dynamic_cast<const Geom::BezierCurveN<1u> *>(&newPath[newPath.size()])) {
                    //delete the 'Z'
                    newPath.erase_last();
                    newPath.append(path_in[i][path_in[i].size() - 1]);
                    newPath.appendNew<Geom::LineSegment>(newPath.initialPoint());
                    newPath.erase_last();
                } else {
                    //delete the 'Z'
                    newPath.erase_last();
                    newPath.append(path_in[i][path_in[i].size() - 1]);
                    newPath.appendNew<Geom::LineSegment>(newPath.initialPoint());
                    newPath.erase_last();
                }*/
            }
            Geom::PathBuilder pb;

            //add in the...do I really need to say this?
            pb.moveTo(with_direction.initialPoint());
            pb.append(with_direction);

            //add in our line caps
            if (!path_in[i].closed()) {
                switch (butt) {
                case butt_straight:
                    pb.lineTo(against_direction.initialPoint());
                    break;
                case butt_round:
                    pb.arcTo((-line_width) / 2, (-line_width) / 2, 0., true, true, against_direction.initialPoint() );
                    break;
                case butt_pointy:
                    //I have ZERO idea what to do here.
                    pb.lineTo(against_direction.initialPoint());
                    break;
                case butt_square:
                    pb.lineTo(against_direction.initialPoint());
                    break;
                }
            } else {
                pb.moveTo(against_direction.initialPoint());
            }

            pb.append(against_direction);

            //cap (if necessary)
            if (!path_in[i].closed()) {
                switch (butt) {
                case butt_straight:
                    pb.lineTo(with_direction.initialPoint());
                    break;
                case butt_round:
                    pb.arcTo((-line_width) / 2, (-line_width) / 2, 0., true, true, with_direction.initialPoint() );
                    break;
                case butt_pointy:
                    //I have ZERO idea what to do here.
                    pb.lineTo(with_direction.initialPoint());
                    break;
                case butt_square:
                    pb.lineTo(with_direction.initialPoint());
                    break;
                }
            }
            pb.flush();
            for (unsigned m = 0; i < pb.peek().size(); i++) {
                path_out.push_back(pb.peek()[m]);
            }
        } else {
            Path p = Path();
            Path outlinepath = Path();

            p.LoadPath(path_in[i], Geom::Affine(), false, false);
            p.Outline(&outlinepath, line_width / 2, static_cast<join_typ>(join), butt, miter_lim);
            Geom::PathVector *pv_p = outlinepath.MakePathVector();
            //somewhat hack-ish
            path_out.push_back( (*pv_p)[0].reverse() );
            if (pv_p) delete pv_p;
        }
    }
    return path_out;
}

Geom::PathVector PathVectorOutline(Geom::PathVector const & path_in, double line_width, ButtType linecap_type,
                                   LineJoinType linejoin_type, double miter_limit)
{
    std::vector<Geom::Path> path_out = std::vector<Geom::Path>();
    if (path_in.empty()) {
        return path_out;
    }
    Path p = Path();
    Path outlinepath = Path();
    for (unsigned i = 0; i < path_in.size(); i++) {
        p.LoadPath(path_in[i], Geom::Affine(), false, ( (i==0) ? false : true));
    }

#define miter_lim fabs(line_width * miter_limit)

    //magic!
    if (linejoin_type <= 2) {
        p.Outline(&outlinepath, line_width / 2, static_cast<join_typ>(linejoin_type),
                  linecap_type, miter_lim);
        //fix memory leak
        std::vector<Geom::Path> *pv_p = outlinepath.MakePathVector();
        path_out = *pv_p;
        delete pv_p;

    } else if (linejoin_type == 3) {
        //reflected arc join
        path_out = outlinePath(path_in, line_width, static_cast<LineJoinType>(linejoin_type),
                               linecap_type , miter_lim, false);

    } else if (linejoin_type == 4) {
        //extrapolated arc join
        path_out = outlinePath(path_in, line_width, LINEJOIN_STRAIGHT, linecap_type, miter_lim, true);

    }

#undef miter_lim
    return path_out;
}

Geom::Path PathOutsideOutline(Geom::Path const & path_in, double line_width, LineJoinType linejoin_type, double miter_limit)
{

#define miter_lim fabs(line_width * miter_limit)

    Geom::Path path_out;

    if (linejoin_type <= LINEJOIN_POINTY || path_in.size() <= 1) {

        Geom::PathVector * pathvec;

        Path path_tangent = Path();
        Path path_outline = Path();
        path_outline.LoadPath(path_in, Geom::Affine(), false, false);
        path_outline.OutsideOutline(&path_tangent, line_width / 2, static_cast<join_typ>(linejoin_type), butt_straight, miter_lim);

        pathvec = path_tangent.MakePathVector();
        path_out = pathvec[0]/* deref pointer */[0]/*actual object ref*/;
        delete pathvec;
        return path_out;
    } else if (linejoin_type == LINEJOIN_REFLECTED) {
        //reflected half outline
        Geom::PathVector pathvec;
        pathvec.push_back(path_in);
        path_out = doAdvHalfOutline(path_in, line_width, miter_lim, false);
        return path_out;
    } else if (linejoin_type == LINEJOIN_EXTRAPOLATED) {
        //what the hell do you think this is? :P
        path_out = doAdvHalfOutline(path_in, line_width, miter_lim, true);
        return path_out;
    }
#undef miter_lim
    return path_out;
}

} // namespace Outline

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
