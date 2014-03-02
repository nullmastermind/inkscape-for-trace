#pragma once

#include <2geom/path.h>
#include <2geom/circle.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/shape.h>
#include <2geom/transforms.h>
#include <2geom/path-sink.h>

#include <livarot/Path.h>
#include <livarot/LivarotDefs.h>

enum LineJoinType {
    LINEJOIN_STRAIGHT,
    LINEJOIN_ROUND,
    LINEJOIN_POINTY,
    LINEJOIN_REFLECTED,
    LINEJOIN_EXTRAPOLATED
};

namespace Geom
{
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
		if (d > (r0 + r1))
		{
			/* no solution. circles do not intersect. */
			return 0;
		}
		if (d <= fabs(r0 - r1))
		{
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

		for (unsigned path_descr = 0; path_descr < in.size(); path_descr++)
		{
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
		if (!Geom::are_near(denom,0.)){
			Geom::Coord t = (cross(origin_a,vector_b) + cross(vector_b,origin_b)) / denom;
			return origin_a + t * vector_a;
		}
		return boost::none;
	}
}

namespace Outline
{

	typedef Geom::D2<Geom::SBasis> D2SB;
	typedef Geom::Piecewise<D2SB> PWD2;

	static void extrapolate_curves(Geom::Path& path_builder, Geom::Curve* cbc1, Geom::Curve*cbc2, Geom::Point endPt, double miter_limit)
{
	Geom::Crossings cross = Geom::crossings(*cbc1, *cbc2);
	if (cross.empty())
	{
		Geom::Path pth;
		pth.append(*cbc1);
				
		Geom::Point tang1 = Geom::unitTangentAt(pth.toPwSb()[0], 1);

		pth = Geom::Path();
		pth.append( *cbc2 );
		Geom::Point tang2 = Geom::unitTangentAt(pth.toPwSb()[0], 0);


		Geom::Circle circle1 = Geom::touching_circle(Geom::reverse(cbc1->toSBasis()), 0.);
		Geom::Circle circle2 = Geom::touching_circle(cbc2->toSBasis(), 0);

		Geom::Point points[2];
		int solutions = Geom::circle_circle_intersection(circle1, circle2, points[0], points[1]);
		if (solutions == 2)
		{
			Geom::Point sol(0,0);
			if ( dot(tang2,points[0]-endPt) > 0 ) 
			{
				// points[0] is bad, choose points[1]
				sol = points[1];
			}
			else if ( dot(tang2,points[1]-endPt) > 0 ) { // points[0] could be good, now check points[1]
				// points[1] is bad, choose points[0]
				sol = points[0];
			} 
			else 
			{
				// both points are good, choose nearest
				sol = ( distanceSq(endPt, points[0]) < distanceSq(endPt, points[1]) ) ?
						points[0] : points[1];
			}
			Geom::EllipticalArc *arc0 = circle1.arc(cbc1->finalPoint(), 0.5*(cbc1->finalPoint()+sol), sol, true);
			Geom::EllipticalArc *arc1 = circle2.arc(sol, 0.5*(sol+endPt), endPt, true);

			if (arc0)
			{
				path_builder.append (arc0->toSBasis());
				delete arc0;
				arc0 = NULL;
			}

			if (arc1)
			{
				path_builder.append (arc1->toSBasis());
				delete arc1;
				arc1 = NULL;
			}
		}
		else
		{
			path_builder.appendNew<Geom::LineSegment> (endPt);
		}
	}
	else
	{
		path_builder.appendNew<Geom::LineSegment> (endPt);
	}
}
	static Geom::Path half_outline_extrp(const Geom::Path& path_in, double line_width, ButtType linecap_type, double miter_limit)
	{
			Geom::PathVector pv = split_at_cusps(path_in);
			unsigned m;
			Path path_outline = Path();
			Path path_tangent = Path();

			Geom::Point initialPoint;
			Geom::Point endPoint;

			Geom::Path path_builder = Geom::Path();
			Geom::PathVector * pathvec;
		
			//load the first portion in before the loop starts
			{
				path_outline = Path();
				path_outline.LoadPath(pv[0], Geom::Affine(), false, false);
				path_outline.OutsideOutline(&path_tangent, line_width / 2, join_straight, linecap_type, 10);
				//now half of first cusp has been loaded

				pathvec = path_tangent.MakePathVector();
				path_tangent = Path();
			
				//instead of array accessing twice, dereferencing used for clarity
				initialPoint = (*pathvec)[0].initialPoint();

				path_builder.start(initialPoint);
				path_builder.append( (*pathvec)[0] );

				path_outline = Path();
				path_outline.LoadPath(pv[1], Geom::Affine(), false, false);
				path_outline.OutsideOutline(&path_tangent, line_width / 2, join_straight, linecap_type, 10);

				delete pathvec; pathvec = NULL;
				pathvec = path_tangent.MakePathVector();
				path_tangent = Path();

				Geom::Curve *cbc1 = path_builder[path_builder.size() - 1].duplicate();
				Geom::Curve *cbc2 = (*pathvec)[0][0].duplicate();

				extrapolate_curves(path_builder, cbc1, cbc2, (*pathvec)[0].initialPoint(), miter_limit );
			
				path_builder.append( (*pathvec)[0] );

				//always set pointers null after deleting
				delete pathvec; pathvec = NULL;
				delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;
			}

			for (m = 2; m < pv.size(); m++)
			{
				path_outline = Path();
				path_outline.LoadPath(pv[m], Geom::Affine(), false, false);
				path_outline.OutsideOutline(&path_tangent, line_width / 2, join_straight, linecap_type, 10);

				delete pathvec; pathvec = NULL;
				pathvec = path_tangent.MakePathVector();

				Geom::Curve *cbc1 = path_builder[path_builder.size() - 1].duplicate();
				Geom::Curve *cbc2 = (*pathvec)[0][0].duplicate();

				extrapolate_curves(path_builder, cbc1, cbc2, (*pathvec)[0].initialPoint(), miter_limit );
				path_builder.append( (*pathvec)[0] );
			
				delete pathvec; pathvec = NULL;
				delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;
			}
		
			return path_builder;
	}

	//Create a reflected outline join.
	//Note: it is generally recommended to let half_outline do this for you!
	//path_builder: the path to append the curves to
	//cbc1: the curve before the join
	//cbc2: the curve after the join
	//endPt: the point to end at
	//miter_limit: the miter parameter
	static void reflect_curves(Geom::Path& path_builder, Geom::Curve* cbc1, Geom::Curve* cbc2, Geom::Point endPt, double miter_limit)
	{
		//the most important work for the reflected join is done here

		//determine where we are in the path. If we're on the inside, ignore
		//and just lineTo. On the outside, we'll do a little reflection magic :)
		Geom::Crossings cross = Geom::crossings(*cbc1, *cbc2);
		if (cross.empty())
		{
				//probably on the outside of the corner
				Geom::Path pth;
				pth.append(*cbc1);
				
				Geom::Point tang1 = Geom::unitTangentAt(pth.toPwSb()[0], 1);

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
				if ( cross.empty() )
				{
					//std::cout << "Oops, no crossings!" << std::endl;
					//curves didn't cross; default to miter
					/*boost::optional <Geom::Point> p = intersection_point (cbc1->finalPoint(), tang1,
																		cbc2->initialPoint(), tang2);
					if (p)
					{
						path_builder.appendNew<Geom::LineSegment> (*p);
					}*/
					//bevel
					path_builder.appendNew<Geom::LineSegment>( endPt );
				}
				else
				{
					//join
					std::pair<Geom::CubicBezier, Geom::CubicBezier> sub1 = bzr1.subdivide(cross[0].ta);
					std::pair<Geom::CubicBezier, Geom::CubicBezier> sub2 = bzr2.subdivide(cross[0].tb);

					//@TODO joins have a strange tendency to cross themselves twice. Check this.

					//sections commented out are for general stability
					path_builder.appendNew <Geom::CubicBezier> (sub1.first[1], sub1.first[2], /*sub1.first[3]*/ sub2.second[0] );
					path_builder.appendNew <Geom::CubicBezier> (sub2.second[1], sub2.second[2], /*sub2.second[3]*/ endPt );
				}
		}
		else // cross.empty()
		{
			//probably on the inside of the corner
			path_builder.appendNew<Geom::LineSegment> ( endPt );
		}
	}

	/** @brief Converts a path to one half of an outline.
	* path_in: The input path to use. (To create the other side use path_in.reverse() )
	* line_width: the line width to use (usually you want to divide this by 2)
	* linecap_type: (not used here) the cap to apply. Passed to libvarot.
	* miter_limit: the miter parameter
	*/
	static Geom::Path half_outline(const Geom::Path& path_in, double line_width, ButtType linecap_type, double miter_limit)
	{
			Geom::PathVector pv = split_at_cusps(path_in);
			unsigned m;
			Path path_outline = Path();
			Path path_tangent = Path();
			//needed for closing the path
			Geom::Point initialPoint;
			Geom::Point endPoint;

			//some issues prevented me from using a PathBuilder here
			//it seems like PathBuilder::peek() gave me a null reference exception
			//and I was unable to get a stack trace on Windows, so had to switch to Linux
			//to see what the hell was wrong. :( 
			//I wasted five hours opening it in IDAPro, VS2012, and GDB Windows

			/*Program received signal SIGSEGV, Segmentation fault.
				0x00000000006539ac in get_curves (this=0x0)
					at /usr/include/c++/4.6/bits/locale_facets.h:1077
				1077	      { return __c; }
			*/

			Geom::Path path_builder = Geom::Path();
			Geom::PathVector * pathvec;
		
			//load the first portion in before the loop starts
			{
				path_outline = Path();
				path_outline.LoadPath(pv[0], Geom::Affine(), false, false);
				path_outline.OutsideOutline(&path_tangent, line_width / 2, join_straight, linecap_type, 10);
				//now half of first cusp has been loaded

				pathvec = path_tangent.MakePathVector();
				path_tangent = Path();
			
				//instead of array accessing twice, dereferencing used for clarity
				initialPoint = (*pathvec)[0].initialPoint();

				path_builder.start(initialPoint);
				path_builder.append( (*pathvec)[0] );

				path_outline = Path();
				path_outline.LoadPath(pv[1], Geom::Affine(), false, false);
				path_outline.OutsideOutline(&path_tangent, line_width / 2, join_straight, linecap_type, 10);

				delete pathvec; pathvec = NULL;
				pathvec = path_tangent.MakePathVector();
				path_tangent = Path();

				Geom::Curve *cbc1 = path_builder[path_builder.size() - 1].duplicate();
				Geom::Curve *cbc2 = (*pathvec)[0][0].duplicate();

				reflect_curves(path_builder, cbc1, cbc2, (*pathvec)[0].initialPoint(), miter_limit );
			
				path_builder.append( (*pathvec)[0] );

				//always set pointers null after deleting
				delete pathvec; pathvec = NULL;
				delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;
			}

			for (m = 2; m < pv.size(); m++)
			{
				path_outline = Path();
				path_outline.LoadPath(pv[m], Geom::Affine(), false, false);
				path_outline.OutsideOutline(&path_tangent, line_width / 2, join_straight, linecap_type, 10);

				delete pathvec; pathvec = NULL;
				pathvec = path_tangent.MakePathVector();

				Geom::Curve *cbc1 = path_builder[path_builder.size() - 1].duplicate();
				Geom::Curve *cbc2 = (*pathvec)[0][0].duplicate();

				reflect_curves(path_builder, cbc1, cbc2, (*pathvec)[0].initialPoint(), miter_limit );
				path_builder.append( (*pathvec)[0] );
			
				delete pathvec; pathvec = NULL;
				delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;
			}
		
			return path_builder;
	}

	static Geom::PathVector outlinePath(const Geom::PathVector& path_in, double line_width, JoinType join, ButtType butt, double miter_lim)
	{
		Path p = Path();
		Path outlinepath = Path();
		for (unsigned i = 0; i < path_in.size(); i++)
		{
			p.LoadPath(path_in[i], Geom::Affine(), false, ( (i==0) ? false : true));
		}

		Geom::PathVector path_out;
		for (unsigned lmnop = 0; lmnop < path_in.size(); lmnop++)
		{
			if (path_in[lmnop].size() > 1)
			{
				Geom::Path p_init;
				Geom::Path p_rev;
				Geom::PathBuilder pb = Geom::PathBuilder();
			
				if ( !path_in[lmnop].closed() )
				{
					p_init = Outline::half_outline( path_in[lmnop], -line_width, butt,
						miter_lim );
					p_rev = Outline::half_outline( path_in[lmnop].reverse(), -line_width, butt,
						miter_lim );

					pb.moveTo(p_init.initialPoint() );
					pb.append(p_init);

					//cap
					if (butt == butt_straight) {
						pb.lineTo(p_rev.initialPoint() );
					} else if (butt == butt_round) {
						pb.arcTo((-line_width) / 2, (-line_width) / 2, 0., true, true, p_rev.initialPoint() );
					} else if (butt == butt_square) {
						//don't know what to do
						pb.lineTo(p_rev.initialPoint() );
					} else if (butt == butt_pointy) {
						//don't know what to do
						pb.lineTo(p_rev.initialPoint() );
					}

					pb.append(p_rev);

					if (butt == butt_straight) {
						pb.lineTo(p_init.initialPoint() );
					} else if (butt == butt_round) {
						pb.arcTo((-line_width) / 2, (-line_width) / 2, 0., true, true, p_init.initialPoint() );
					} else if (butt == butt_square) {
						//don't know what to do
						pb.lineTo(p_init.initialPoint() );
					} else if (butt == butt_pointy) {
						//don't know what to do
						//Geom::Point end_deriv = Geom::unitTangentAt( Geom::reverse(path_in[lmnop].toPwSb()[path_in[lmnop].size()]), 0);
						//double radius = 0.5 * Geom::distance(path_in[lmnop].finalPoint(), p_rev.initialPoint());

						pb.lineTo(p_init.initialPoint() );
					}
				}
				else
				{
					//final join
					//refer to half_outline for documentation
					Geom::Path p_almost = path_in[lmnop];
					p_almost.appendNew<Geom::LineSegment> ( path_in[lmnop].initialPoint() );
					p_init = Outline::half_outline( p_almost, -line_width, butt,
						miter_lim );
					p_rev = Outline::half_outline( p_almost.reverse(), -line_width, butt,
						miter_lim );
					p.LoadPath(path_in[lmnop], Geom::Affine(), false, false);

					//this is a kludge, because I can't find how to make this work properly
					bool lastIsLinear = ( (Geom::distance(path_in[lmnop].finalPoint(), 
						path_in[lmnop] [path_in[lmnop].size() - 1].finalPoint())) == 
						(path_in[lmnop] [path_in[lmnop].size()].length()));

					p_almost = p_init;
					if (lastIsLinear)
					{
						p_almost.erase_last(); p_almost.erase_last();
					}

					//outside test
					Geom::Curve* cbc1 = p_almost[p_almost.size() - 1].duplicate();
					Geom::Curve* cbc2 = p_almost[0].duplicate();

					Geom::Crossings cross = Geom::crossings(*cbc1, *cbc2);

					if (cross.empty())
					{
						//this is the outside path
						
						//reuse the old one
						p_init = p_almost;
						Outline::reflect_curves(p_init, cbc1, cbc2, p_almost.initialPoint(), miter_lim );
						pb.moveTo(p_init.initialPoint()); pb.append(p_init);
					}
					else
					{
						//inside, carry on :-)
						pb.moveTo(p_almost.initialPoint()); pb.append(p_almost);
					}

					p_almost = p_rev;
					if (lastIsLinear)
					{			
						p_almost.erase(p_almost.begin() );
						p_almost.erase(p_almost.begin() );
					}

					delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;

					cbc1 = p_almost[p_almost.size() - 1].duplicate();
					cbc2 = p_almost[0].duplicate();

					cross = Geom::crossings(*cbc1, *cbc2);

					if (cross.empty())
					{
						//outside path
					
						p_init = p_almost;
						reflect_curves(p_init, cbc1, cbc2, p_almost.initialPoint(), miter_lim );
						pb.moveTo(p_init.initialPoint()); pb.append(p_init);
					}
					else
					{
						//inside
						pb.moveTo(p_almost.initialPoint()); pb.append(p_almost);
					}
					delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;
				}
				//pb.closePath();
				pb.flush();
				Geom::PathVector pv_np = pb.peek();
				//hack
				for (unsigned abcd = 0; abcd < pv_np.size(); abcd++)
				{
					path_out.push_back( pv_np[abcd] );
				}
			} 
			else
			{
				p.LoadPath(path_in[lmnop], Geom::Affine(), false, false);
				p.Outline(&outlinepath, line_width / 2, join, butt, miter_lim);
				std::vector<Geom::Path> *pv_p = outlinepath.MakePathVector();
				//hack
				path_out.push_back( (*pv_p)[0].reverse() );
				delete pv_p;
			}
		}
		return path_out;
	}
	static Geom::PathVector outlinePath_extr(const Geom::PathVector& path_in, double line_width, LineJoinType join, ButtType butt, double miter_lim)
	{
		Path p = Path();
		Path outlinepath = Path();
		for (unsigned i = 0; i < path_in.size(); i++)
		{
			p.LoadPath(path_in[i], Geom::Affine(), false, ( (i==0) ? false : true));
		}

		Geom::PathVector path_out;
		for (unsigned lmnop = 0; lmnop < path_in.size(); lmnop++)
		{
			if (path_in[lmnop].size() > 1)
			{
				Geom::Path p_init;
				Geom::Path p_rev;
				Geom::PathBuilder pb = Geom::PathBuilder();
			
				if ( !path_in[lmnop].closed() )
				{
					p_init = Outline::half_outline_extrp( path_in[lmnop], -line_width, butt,
						miter_lim );
					p_rev = Outline::half_outline_extrp( path_in[lmnop].reverse(), -line_width, butt,
						miter_lim );

					pb.moveTo(p_init.initialPoint() );
					pb.append(p_init);

					//cap
					if (butt == butt_straight) {
						pb.lineTo(p_rev.initialPoint() );
					} else if (butt == butt_round) {
						pb.arcTo((-line_width) / 2, (-line_width) / 2, 0., true, true, p_rev.initialPoint() );
					} else if (butt == butt_square) {
						//don't know what to do
						pb.lineTo(p_rev.initialPoint() );
					} else if (butt == butt_pointy) {
						//don't know what to do
						pb.lineTo(p_rev.initialPoint() );
					}

					pb.append(p_rev);

					if (butt == butt_straight) {
						pb.lineTo(p_init.initialPoint() );
					} else if (butt == butt_round) {
						pb.arcTo((-line_width) / 2, (-line_width) / 2, 0., true, true, p_init.initialPoint() );
					} else if (butt == butt_square) {
						//don't know what to do
						pb.lineTo(p_init.initialPoint() );
					} else if (butt == butt_pointy) {
						//don't know what to do
						//Geom::Point end_deriv = Geom::unitTangentAt( Geom::reverse(path_in[lmnop].toPwSb()[path_in[lmnop].size()]), 0);
						//double radius = 0.5 * Geom::distance(path_in[lmnop].finalPoint(), p_rev.initialPoint());

						pb.lineTo(p_init.initialPoint() );
					}
				}
				else
				{
					//final join
					//refer to half_outline for documentation
					Geom::Path p_almost = path_in[lmnop];
					p_almost.appendNew<Geom::LineSegment> ( path_in[lmnop].initialPoint() );
					p_init = Outline::half_outline_extrp( p_almost, -line_width, butt,
						miter_lim );
					p_rev = Outline::half_outline_extrp( p_almost.reverse(), -line_width, butt,
						miter_lim );
					p.LoadPath(path_in[lmnop], Geom::Affine(), false, false);

					//this is a kludge, because I can't find how to make this work properly
					bool lastIsLinear = ( (Geom::distance(path_in[lmnop].finalPoint(), 
						path_in[lmnop] [path_in[lmnop].size() - 1].finalPoint())) == 
						(path_in[lmnop] [path_in[lmnop].size()].length()));

					p_almost = p_init;
					if (lastIsLinear)
					{
						p_almost.erase_last(); p_almost.erase_last();
					}

					//outside test
					Geom::Curve* cbc1 = p_almost[p_almost.size() - 1].duplicate();
					Geom::Curve* cbc2 = p_almost[0].duplicate();

					Geom::Crossings cross = Geom::crossings(*cbc1, *cbc2);

					if (cross.empty())
					{
						//this is the outside path
						
						//reuse the old one
						p_init = p_almost;
						Outline::extrapolate_curves(p_init, cbc1, cbc2, p_almost.initialPoint(), miter_lim );
						pb.moveTo(p_init.initialPoint()); pb.append(p_init);
					}
					else
					{
						//inside, carry on :-)
						pb.moveTo(p_almost.initialPoint()); pb.append(p_almost);
					}

					p_almost = p_rev;
					if (lastIsLinear)
					{			
						p_almost.erase(p_almost.begin() );
						p_almost.erase(p_almost.begin() );
					}

					delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;

					cbc1 = p_almost[p_almost.size() - 1].duplicate();
					cbc2 = p_almost[0].duplicate();

					cross = Geom::crossings(*cbc1, *cbc2);

					if (cross.empty())
					{
						//outside path
					
						p_init = p_almost;
						extrapolate_curves(p_init, cbc1, cbc2, p_almost.initialPoint(), miter_lim );
						pb.moveTo(p_init.initialPoint()); pb.append(p_init);
					}
					else
					{
						//inside
						pb.moveTo(p_almost.initialPoint()); pb.append(p_almost);
					}
					delete cbc1; delete cbc2; cbc1 = cbc2 = NULL;
				}
				//pb.closePath();
				pb.flush();
				Geom::PathVector pv_np = pb.peek();
				//hack
				for (unsigned abcd = 0; abcd < pv_np.size(); abcd++)
				{
					path_out.push_back( pv_np[abcd] );
				}
			} 
			else
			{
				p.LoadPath(path_in[lmnop], Geom::Affine(), false, false);
				p.Outline(&outlinepath, line_width / 2, join_pointy, butt, miter_lim);
				std::vector<Geom::Path> *pv_p = outlinepath.MakePathVector();
				//hack
				path_out.push_back( (*pv_p)[0].reverse() );
				delete pv_p;
			}
		}
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
