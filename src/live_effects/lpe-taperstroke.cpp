/**
 * @file
 * Taper Stroke path effect, provided as an alternative to Power Strokes
 * for otherwise constant-width paths.
 *
 * Authors:
 *   Liam P White <inkscapebrony@gmail.com>
 *
 * Copyright (C) 2014 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-taperstroke.h"

// You might need to include other 2geom files. You can add them here:
#include <2geom/path.h>
#include <2geom/shape.h>
#include "pathoutlineprovider.h"
#include "display/curve.h"
#include "svg/svg.h"

//#include <glibmm/i18n.h>

#include "knot-holder-entity.h"
#include "knotholder.h"

namespace Inkscape {
namespace LivePathEffect {

namespace TpS {
	class KnotHolderEntityAttachBegin : public LPEKnotHolderEntity {
	public:
		KnotHolderEntityAttachBegin(LPETaperStroke * effect) : LPEKnotHolderEntity(effect) {}
		virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
		virtual Geom::Point knot_get() const;
	};
	class KnotHolderEntityAttachEnd : public LPEKnotHolderEntity {
	public:
		KnotHolderEntityAttachEnd(LPETaperStroke * effect) : LPEKnotHolderEntity(effect) {}
		virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
		virtual Geom::Point knot_get() const;
	};
} // TpS

static const Util::EnumData<unsigned> JoinType[] = {
	{LINEJOIN_STRAIGHT,		N_("Beveled"),		"bevel"},
	{LINEJOIN_ROUND,			N_("Rounded"),		"round"},
	{LINEJOIN_REFLECTED,		N_("Reflected"),	"reflected"},
	{LINEJOIN_POINTY,			N_("Miter"),		"miter"},
	{LINEJOIN_EXTRAPOLATED,	N_("Extrapolated"), "extrapolated"}
};

static const Util::EnumDataConverter<unsigned> JoinTypeConverter(JoinType, sizeof (JoinType)/sizeof(*JoinType));

LPETaperStroke::LPETaperStroke(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    line_width(_("Stroke width"), _("The (non-tapered) width of the path"), "stroke_width", &wr, this, 3),
	attach_start(_("Start offset"), _("Taper distance from path start"), "attach_start", &wr, this, 0.2),
	attach_end(_("End offset"), _("The ending position of the taper"), "end_offset", &wr, this, 0.2),
	smoothing(_("Taper smoothing"), _("Amount of smoothing to apply to the tapers"), "smoothing", &wr, this, 0.2),
	join_type(_("Join type"), _("Join type for non-smooth nodes"), "jointype", JoinTypeConverter, &wr, this, LINEJOIN_EXTRAPOLATED),
	miter_limit(_("Miter limit"), _("Limit for miter joins"), "miter_limit", &wr, this, 30.)
{
    /* uncomment the following line to have the original path displayed while the item is selected */
    show_orig_path = true;
	_provides_knotholder_entities = true;

	attach_start.param_set_digits(3);
	attach_end.param_set_digits(3);
	
	
	registerParameter( dynamic_cast<Parameter *>(&line_width) );
	registerParameter( dynamic_cast<Parameter *>(&attach_start) );
	registerParameter( dynamic_cast<Parameter *>(&attach_end) );
	registerParameter( dynamic_cast<Parameter *>(&smoothing) );
	registerParameter( dynamic_cast<Parameter *>(&join_type) );
	registerParameter( dynamic_cast<Parameter *>(&miter_limit) );
}

LPETaperStroke::~LPETaperStroke()
{
	
}

unsigned curveOrder (const Geom::Curve* curve_in)
{
	using namespace Geom;
	//cast it
	const CubicBezier *cbc = dynamic_cast<const CubicBezier*>(curve_in);
	if (cbc) return 3;
	const QuadraticBezier * qbc = dynamic_cast<const QuadraticBezier*>(curve_in);
	if (qbc) return 2;
	const BezierCurveN<1U> * lbc = dynamic_cast<const BezierCurveN<1U> *>(curve_in);
	if (lbc) return 1;
	//BezierCurveN<0> * dbc = dynamic_cast<BezierCurveN<0> *> (curve_in);
	return 0;
}

Geom::Path return_at_first_cusp (Geom::Path const & path_in, double smooth_tolerance = 0.01)
{
	Geom::Path path_out = Geom::Path();

	for (unsigned i = 0; i < path_in.size(); i++)
	{
		path_out.append(path_in[i]);
		if (path_in.size() == 1)
			break;

		//determine order of curve
		int order = curveOrder(&path_in[i]);
		
		Geom::Point start_point;
		Geom::Point cross_point = path_in[i].finalPoint();
		Geom::Point end_point;
	
		g_assert(path_in[i].finalPoint() == path_in[i+1].initialPoint());

		//can you tell that the following expressions have been shaped by
		//repeated compiler errors? ;)
		switch (order)
		{
		case 3:
			start_point = (dynamic_cast<const Geom::CubicBezier*>(&path_in[i]))->operator[] (2);
			break;
		case 2:
			start_point = (dynamic_cast<const Geom::QuadraticBezier*>(&path_in[i]))->operator[] (1);
			break;
		case 1:
		default:
			start_point = path_in[i].initialPoint();
		}

		order = curveOrder(&path_in[i+1]);

		switch (order)
		{
		case 3:
			end_point = (dynamic_cast<const Geom::CubicBezier*>(&path_in[i+1]))->operator[] (1);
			break;
		case 2:
			end_point = (dynamic_cast<const Geom::QuadraticBezier*>(&path_in[i+1]))->operator[] (1);
			break;
		case 1:
		default:
			end_point = path_in[i+1].finalPoint();
		}
		if (!are_collinear(start_point, cross_point, end_point, smooth_tolerance))
			break;
	}
	return path_out;
}

Geom::Curve * subdivide_at(const Geom::Curve* curve_in, Geom::Coord time, bool first)
{
	//the only reason for this function is the lack of a subdivide function in the Curve class.
	//you have to cast to Beziers to be able to use subdivide(t)
	unsigned order = curveOrder(curve_in);
	Geom::Curve* curve_out = curve_in->duplicate();
	switch (order)
	{
	//these need to be scoped because of the variable 'c'
	case 3:
	{
		Geom::CubicBezier c = first ? (dynamic_cast<Geom::CubicBezier*> (curve_out))->subdivide(time).first :
									  (dynamic_cast<Geom::CubicBezier*> (curve_out))->subdivide(time).second;
		if (curve_out) delete curve_out;
		curve_out = c.duplicate();
		break;
	}
	case 2:
	{
		Geom::QuadraticBezier c = first ? (dynamic_cast<Geom::QuadraticBezier*>(curve_out))->subdivide(time).first :
										  (dynamic_cast<Geom::QuadraticBezier*>(curve_out))->subdivide(time).second;
		if (curve_out) delete curve_out;
		curve_out = c.duplicate();
		break;
	}
	case 1:
	{
		Geom::BezierCurveN<1> c = first ? (dynamic_cast<Geom::BezierCurveN<1>* >(curve_out))->subdivide(time).first :
										  (dynamic_cast<Geom::BezierCurveN<1>* >(curve_out))->subdivide(time).second;
		if (curve_out) delete curve_out;
		curve_out = c.duplicate();
		break;
	}
	}
	return curve_out;
}

Geom::Piecewise<Geom::D2<Geom::SBasis> > stretch_along(Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2_in, Geom::Path pattern, double width);

Geom::PathVector LPETaperStroke::doEffect_path(Geom::PathVector const& path_in)
{
	//there is a pretty good chance that people will try to drag the knots
	//on top of each other, so block it

	unsigned size = path_in[0].size();
	if (size == return_at_first_cusp(path_in[0]).size()) {
		//check to see if the knots were dragged over each other
		//if so, reset the end offset
		if ( attach_start >= (size - attach_end) ) {
			attach_end.param_set_value( size - attach_start );
		}
	}

	//don't ever let it be zero
	if (attach_start <= 0) {
		attach_start.param_set_value( 0.0001 );
	}
	if (attach_end <= 0) {
		attach_end.param_set_value( 0.0001 );
	}

	/*if (size != return_at_first_cusp(path_in[0]).size()) { //will  get to this in a bit
		//check to see if either knot was dragged past their allowed amount
		volatile unsigned size_p_start = (unsigned)attach_start;
		volatile unsigned size_p_end = (unsigned)attach_end;
		
		//maximum allowed value in either direction is return_at_first_cusp(path_in[0]).size
		volatile unsigned allowed_p_start = return_at_first_cusp(path_in[0]).size();
		volatile unsigned allowed_p_end = return_at_first_cusp(path_in[0].reverse()).size();
		
		if (size_p_start > allowed_p_start) {
			attach_start.param_set_value(allowed_p_start - 0.0001);
		} else if (size_p_end > allowed_p_end) {
			attach_end.param_set_value(allowed_p_end - 0.0001);
		}
	}*/
	
	//Path::operator () means get point at time t
	start_attach_point = return_at_first_cusp(path_in[0])(attach_start);
	end_attach_point = return_at_first_cusp(path_in[0].reverse())(attach_end);
	Geom::PathVector pathv_out;
	
	pathv_out = doEffect_simplePath(path_in);
	
	
	//now for the fun stuff. Right? RIGHT?
	
	if (true) {
		Geom::PathVector real_pathv;
		
		//Construct the pattern (pat_str stands for pattern string)
		char* pat_str = new char[200];
		sprintf(pat_str, "M 1,0 1,1 C %5.5f,1 0,0.5 0,0.5 0,0.5 %5.5f,0 1,0 Z", (double)smoothing, (double)smoothing);
		Geom::PathVector pat_vec = sp_svg_read_pathv(pat_str);
		
		Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2;
		pwd2.concat(stretch_along(pathv_out[0].toPwSb(), pat_vec[0], line_width));
		
		real_pathv.push_back(path_from_piecewise(pwd2, 0.001)[0]);
		
		Geom::PathVector sht_path;
		sht_path.push_back(pathv_out[1]);
		sht_path = Outline::outlinePath_extr(sht_path, line_width, LINEJOIN_STRAIGHT, butt_straight, miter_limit);
		
		real_pathv.push_back(sht_path[0]);
		
		sprintf(pat_str, "M 0,0 0,1 C %5.5f,1 1,0.5 1,0.5 1,0.5 %5.5f,0 0,0 Z", (double)smoothing, (double)smoothing);
		pat_vec = sp_svg_read_pathv(pat_str);
		
		pwd2 = Geom::Piecewise<Geom::D2<Geom::SBasis> > ();
		pwd2.concat(stretch_along(pathv_out[2].toPwSb(), pat_vec[0], line_width));
		real_pathv.push_back(path_from_piecewise(pwd2, 0.001)[0].reverse());
		
		//clever union
		//Geom::Shape shape1 = Geom::sanitize(Geom::PathVector(1, real_pathv[0]));
		//Geom::Shape shape2 = Geom::sanitize(Geom::PathVector(1, real_pathv[1]));
		//Geom::Shape shape3 = Geom::boolop(shape1, shape2, Geom::BOOLOP_UNION);
		
		//shape2 = Geom::sanitize(Geom::PathVector(1, real_pathv[2]));
		//shape1 = Geom::boolop(shape3, shape2, Geom::BOOLOP_UNION);
		
		//real_pathv = Geom::desanitize(shape1);
		return real_pathv;
	}
	
	return pathv_out;
}

//in all cases, this should return a PathVector with three elements.
Geom::PathVector LPETaperStroke::doEffect_simplePath(Geom::PathVector const & path_in)
{
		unsigned size = path_in[0].size();
		
		//do subdivision and get out
		unsigned loc = (unsigned)attach_start;
		Geom::Curve * curve_start = path_in[0] [loc].duplicate();
		
		std::vector<Geom::Path> pathv_out;
		Geom::Path path_out = Geom::Path();
		
		Geom::Path trimmed_start = Geom::Path();
		Geom::Path trimmed_end = Geom::Path();
		
		for (unsigned i = 0; i < loc; i++) {
			trimmed_start.append(path_in[0] [i]);
		}
		
		#define OVERLAP 0.001
		
		trimmed_start.append(*subdivide_at(curve_start, (attach_start - loc) + OVERLAP, true));
		curve_start = subdivide_at(curve_start, attach_start - loc, false);
		
		//special case: path is one segment long
		//special case: what if the two knots occupy the same segment?
		if ((size == 1) || ( size - unsigned(attach_end) - 1 == loc ))
		{
			Geom::Coord t = Geom::nearest_point(end_attach_point, *curve_start);
			//it is just a dumb segment
			//we have to do some shifting here because the value changed when we reduced the length
			//of the previous segment.
			trimmed_end.append(*subdivide_at(curve_start, t - OVERLAP, false));
			for (unsigned j = (size - attach_end) + 1; j < size; j++) {
				trimmed_end.append(path_in[0] [j]);
			}
			
			curve_start = subdivide_at(curve_start, t, true);
			path_out.append(*curve_start);
			pathv_out.push_back(trimmed_start);
			pathv_out.push_back(path_out);
			pathv_out.push_back(trimmed_end);
			return pathv_out;
		}
		
		pathv_out.push_back(trimmed_start);
		
		//append almost all of the rest of the path, ignore the curves that the knot is past (we'll get to it in a minute)
		path_out.append(*curve_start);

		for (unsigned k = loc + 1; k < (size - unsigned(attach_end)) - 1; k++) {
			path_out.append(path_in[0] [k]);
		}
		
		//deal with the last segment in a very similar fashion to the first
		loc = size - attach_end;
		
		Geom::Curve * curve_end = path_in[0] [loc].duplicate();

		Geom::Coord t = Geom::nearest_point(end_attach_point, *curve_end);
		
		trimmed_end.append(*subdivide_at(curve_end, t - OVERLAP, false));
		curve_end = subdivide_at(curve_end, t, true);
		
		for (unsigned j = (size - attach_end) + 1; j < size; j++) {
			trimmed_end.append(path_in[0] [j]);
		}
		
		path_out.append(*curve_end);
		pathv_out.push_back(path_out);
		
		pathv_out.push_back(trimmed_end);
		
		if (curve_end) delete curve_end;
		if (curve_start) delete curve_start;
		return pathv_out;
}


//most of the below code is verbatim from Pattern Along Path. However, it needed a little
//tweaking to get it to work right in this case.
Geom::Piecewise<Geom::D2<Geom::SBasis> > stretch_along(Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2_in, Geom::Path pattern, double prop_scale)
{
	using namespace Geom;

    // Don't allow empty path parameter:
    if ( pattern.empty() ) {
        return pwd2_in;
    }

/* Much credit should go to jfb and mgsloan of lib2geom development for the code below! */
    Piecewise<D2<SBasis> > output;
    std::vector<Geom::Piecewise<Geom::D2<Geom::SBasis> > > pre_output;

    D2<Piecewise<SBasis> > patternd2 = make_cuts_independent(pattern.toPwSb());
    Piecewise<SBasis> x0 = Piecewise<SBasis>(patternd2[0]);
    Piecewise<SBasis> y0 = Piecewise<SBasis>(patternd2[1]);
    OptInterval pattBndsX = bounds_exact(x0);
    OptInterval pattBndsY = bounds_exact(y0);
    if (pattBndsX && pattBndsY) {
        x0 -= pattBndsX->min();
        y0 -= pattBndsY->middle();

        double xspace  = 0;
        double noffset = 0;
        double toffset = 0;
        /*if (prop_units.get_value() && pattBndsY){
            xspace  *= pattBndsX->extent();
            noffset *= pattBndsY->extent();
            toffset *= pattBndsX->extent();
        }*/

        //Prevent more than 90% overlap...
        if (xspace < -pattBndsX->extent()*.9) {
            xspace = -pattBndsX->extent()*.9;
        }

        y0+=noffset;

        std::vector<Geom::Piecewise<Geom::D2<Geom::SBasis> > > paths_in;
        paths_in = split_at_discontinuities(pwd2_in);

        for (unsigned idx = 0; idx < paths_in.size(); idx++){
            Geom::Piecewise<Geom::D2<Geom::SBasis> > path_i = paths_in[idx];
            Piecewise<SBasis> x = x0;
            Piecewise<SBasis> y = y0;
            Piecewise<D2<SBasis> > uskeleton = arc_length_parametrization(path_i,2,.1);
            uskeleton = remove_short_cuts(uskeleton,.01);
            Piecewise<D2<SBasis> > n = rot90(derivative(uskeleton));
            n = force_continuity(remove_short_cuts(n,.1));
            
            int nbCopies = 0;
            double scaling = 1;
            nbCopies = 1;
            scaling = (uskeleton.domain().extent() - toffset)/pattBndsX->extent();

            double pattWidth = pattBndsX->extent() * scaling;
            
            if (scaling != 1.0) {
                x*=scaling;
            }
            if ( false ) {
                y*=(scaling*prop_scale);
            } else {
                if (prop_scale != 1.0) y *= prop_scale;
            }
            x += toffset;
            
            double offs = 0;
            for (int i=0; i<nbCopies; i++){
                if (false){        
                    Geom::Piecewise<Geom::D2<Geom::SBasis> > output_piece = compose(uskeleton,x+offs)+y*compose(n,x+offs);
                    std::vector<Geom::Piecewise<Geom::D2<Geom::SBasis> > > splited_output_piece = split_at_discontinuities(output_piece);
                    pre_output.insert(pre_output.end(), splited_output_piece.begin(), splited_output_piece.end() );
                }else{
                    output.concat(compose(uskeleton,x+offs)+y*compose(n,x+offs));
                }
                offs+=pattWidth;
            }
        }
        /*if (false){        
            pre_output = fuse_nearby_ends(pre_output, fuse_tolerance);
            for (unsigned i=0; i<pre_output.size(); i++){
                output.concat(pre_output[i]);
            }
        }*/
        return output;
    } else {
        return pwd2_in;
    }
}

void LPETaperStroke::addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop, SPItem *item) 
{
	{
		KnotHolderEntity *e = new TpS::KnotHolderEntityAttachBegin(this);
		e->create(  desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN,
				_("Start point of the taper"), SP_KNOT_SHAPE_CIRCLE );
		knotholder->add(e);
	}
	{
		KnotHolderEntity *e = new TpS::KnotHolderEntityAttachEnd(this);
		e->create(	desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN,
					_("End point of the taper"), SP_KNOT_SHAPE_CIRCLE );
		knotholder->add(e);
	}
}

namespace TpS {
	void KnotHolderEntityAttachBegin::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, guint state)
	{
		using namespace Geom;

		LPETaperStroke* lpe = dynamic_cast<LPETaperStroke *>(_effect);

		Geom::Point const s = snap_knot_position(p, state);

		SPCurve *curve = SP_PATH(item)->get_curve_for_edit();
		Geom::PathVector pathv = curve->get_pathvector();
		Piecewise<D2<SBasis> > pwd2;
		Geom::Path p_in = return_at_first_cusp(pathv[0]);
		pwd2.concat(p_in.toPwSb());

		double t0 = nearest_point(s, pwd2);
		lpe->attach_start.param_set_value(t0);

		// FIXME: this should not directly ask for updating the item. It should write to SVG, which triggers updating.
		sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, true);
	}
	void KnotHolderEntityAttachEnd::knot_set(Geom::Point const &p, Geom::Point const& /*origin*/, guint state)
	{
		using namespace Geom;

		LPETaperStroke* lpe = dynamic_cast<LPETaperStroke *>(_effect);

		Geom::Point const s = snap_knot_position(p, state);

		SPCurve *curve = SP_PATH(item)->get_curve_for_edit();
		Geom::PathVector pathv = curve->get_pathvector();
		Piecewise<D2<SBasis> > pwd2;
		Geom::Path p_in = return_at_first_cusp(pathv[0].reverse());
		pwd2.concat(p_in.toPwSb());
		
		double t0 = nearest_point(s, pwd2);
		lpe->attach_end.param_set_value(t0);

		// FIXME: this should not directly ask for updating the item. It should write to SVG, which triggers updating.
		sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, true);
	}
	Geom::Point KnotHolderEntityAttachBegin::knot_get() const
	{
		LPETaperStroke const * lpe = dynamic_cast<LPETaperStroke const*> (_effect);
		return lpe->start_attach_point;
	}
	Geom::Point KnotHolderEntityAttachEnd::knot_get() const
	{
		LPETaperStroke const * lpe = dynamic_cast<LPETaperStroke const*> (_effect);
		return lpe->end_attach_point;
	}
}


/* ######################## */

} //namespace LivePathEffect
} /* namespace Inkscape */

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
