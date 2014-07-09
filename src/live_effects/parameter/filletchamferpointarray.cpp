/*
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * and finaly to Liam P. White for his big help on coding, that save me a lot of
 * hours
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <2geom/piecewise.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/sbasis-geometric.h>
#include <gtkmm.h>

#include "ui/dialog/lpe-fillet-chamfer-properties.h"
#include "live_effects/parameter/filletchamferpointarray.h"
#include "live_effects/effect.h"
#include "svg/svg.h"
#include "svg/stringstream.h"
#include "knotholder.h"
#include "sp-lpe-item.h"
#include "selection.h"

// needed for on-canvas editting:
#include "desktop.h"
#include "live_effects/lpeobject.h"
#include "helper/geom-nodetype.h"
#include "helper/geom-curves.h"
#include "ui/tools/node-tool.h"

// TODO due to internal breakage in glibmm headers,
// this has to be included last.
#include <glibmm/i18n.h>

using namespace Geom;

namespace Inkscape {

namespace LivePathEffect {

FilletChamferPointArrayParam::FilletChamferPointArrayParam(
    const Glib::ustring &label, const Glib::ustring &tip,
    const Glib::ustring &key, Inkscape::UI::Widget::Registry *wr,
    Effect *effect)
    : ArrayParam<Point>(label, tip, key, wr, effect, 0)
{
    knot_shape = SP_KNOT_SHAPE_DIAMOND;
    knot_mode = SP_KNOT_MODE_XOR;
    knot_color = 0x00ff0000;
}

FilletChamferPointArrayParam::~FilletChamferPointArrayParam() {}

Gtk::Widget *FilletChamferPointArrayParam::param_newWidget()
{
    return NULL;
    /*
          Inkscape::UI::Widget::RegisteredTransformedPoint * pointwdg =
        Gtk::manage(
              new Inkscape::UI::Widget::RegisteredTransformedPoint(
        param_label,
                                                                    param_tooltip,
                                                                    param_key,
                                                                    *param_wr,
                                                                    param_effect->getRepr(),
                                                                    param_effect->getSPDoc()
        ) );
          // TODO: fix to get correct desktop (don't use SP_ACTIVE_DESKTOP)
          SPDesktop *desktop = SP_ACTIVE_DESKTOP;
          Affine transf = desktop->doc2dt();
          pointwdg->setTransform(transf);
          pointwdg->setValue( *this );
          pointwdg->clearProgrammatically();
          pointwdg->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT,
        _("Change point parameter"));

          Gtk::HBox * hbox = Gtk::manage( new Gtk::HBox() );
          static_cast<Gtk::HBox*>(hbox)->pack_start(*pointwdg, true, true);
          static_cast<Gtk::HBox*>(hbox)->show_all_children();

          return dynamic_cast<Gtk::Widget *> (hbox);
      */
}

void
FilletChamferPointArrayParam::param_transform_multiply(Affine const &postmul,
        bool /*set*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/options/transform/rectcorners", true) &&
            _vector[1][X] <= 0) {
        std::vector<Geom::Point> result;
        for (std::vector<Point>::const_iterator point_it = _vector.begin();
                point_it != _vector.end(); ++point_it) {
            Coord A =
                (*point_it)[X] * ((postmul.expansionX() + postmul.expansionY()) / 2);
            result.push_back(Point(A, (*point_it)[Y]));
        }
        param_set_and_write_new_value(result);
    }

    //    param_set_and_write_new_value( (*this) * postmul );
}

/** call this method to recalculate the controlpoints such that they stay at the
 * same location relative to the new path. Useful after adding/deleting nodes to
 * the path.*/
void FilletChamferPointArrayParam::recalculate_controlpoints_for_new_pwd2(
    Piecewise<D2<SBasis> > const &pwd2_in)
{
    if (!last_pwd2.empty()) {
        PathVector const pathv =
            path_from_piecewise(remove_short_cuts(pwd2_in, 0.1), 0.001);
        PathVector last_pathv =
            path_from_piecewise(remove_short_cuts(last_pwd2, 0.1), 0.001);
        std::vector<Point> result;
        unsigned long counter = 0;
        unsigned long counterPaths = 0;
        unsigned long counterCurves = 0;
        long offset = 0;
        long offsetPaths = 0;
        Geom::NodeType nodetype;
        for (PathVector::const_iterator path_it = pathv.begin();
                path_it != pathv.end(); ++path_it) {
            if (path_it->empty()) {
                counterPaths++;
                counter++;
                continue;
            }
            Geom::Path::const_iterator curve_it1 = path_it->begin();
            Geom::Path::const_iterator curve_it2 = ++(path_it->begin());
            Geom::Path::const_iterator curve_endit = path_it->end_default();
            if (path_it->closed() && path_it->back_closed().isDegenerate()) {
                const Curve &closingline = path_it->back_closed();
                if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                    curve_endit = path_it->end_open();
                }
            }
            counterCurves = 0;
            while (curve_it1 != curve_endit) {
                //if start a path get node type
                if (counterCurves == 0) {
                    if (path_it->closed()) {
                        if (path_it->back_closed().isDegenerate()) {
                            nodetype = get_nodetype(path_it->back_open(), *curve_it1);
                        } else {
                            nodetype = get_nodetype(path_it->back_closed(), *curve_it1);
                        }
                    } else {
                        nodetype = NODE_NONE;
                    }
                } else {
                    //check node type also whith straight lines because get_nodetype
                    //return non cusp node in a node inserted inside a straight line
                    //todo: if the path remove some nodes whith the result of a straight
                    //line but with handles, the node inserted into dont fire the knot
                    // because is not handle as cusp node by  get_nodetype function
                    bool this_is_line = true;
                    bool next_is_line = is_straight_curve(*curve_it1);
                    this_is_line = is_straight_curve((*path_it)[counterCurves - 1]);
                    nodetype = get_nodetype((*path_it)[counterCurves - 1], *curve_it1);
                    if (this_is_line || next_is_line) {
                        nodetype = NODE_CUSP;
                    }
                }
                if (last_pathv.size() > pathv.size() ||
                        (last_pathv[counterPaths].size() > counter - offset &&
                         !are_near(curve_it1->initialPoint(),
                                   last_pathv[counterPaths][counter - offset].initialPoint(),
                                   0.1))) {
                    if (last_pathv.size() > counterPaths && curve_it2 == curve_endit) {
                        if (last_pathv[counterPaths].size() < pathv[counterPaths].size()) {
                            offset = abs(last_pathv[counterPaths].size() -
                                         pathv[counterPaths].size());
                        } else if (last_pathv[counterPaths].size() >
                                   pathv[counterPaths].size()) {
                            offset = (abs(last_pathv[counterPaths].size() -
                                          pathv[counterPaths].size())) * -1;
                        } else {
                            offset = 0;
                        }
                        offsetPaths += offset;
                        offset = offsetPaths;
                    } else if (counterCurves == 0 && last_pathv.size() <= pathv.size() &&
                            counter - offset <= last_pathv[counterPaths].size() &&
                            are_near(curve_it1->initialPoint(),
                                     last_pathv[counterPaths].finalPoint(), 0.1) &&
                                     !last_pathv[counterPaths].closed()) {
                        long e = counter - offset + 1;
                        std::vector<Point> tmp = _vector;
                        for (unsigned long i =
                                     last_pathv[counterPaths].size() + counter - offset;
                                i > counterCurves - offset + 1; i--) {

                            if (tmp[i - 1][X] > 0) {
                                double fractpart, intpart;
                                fractpart = modf(tmp[i - 1][X], &intpart);
                                _vector[e] = Point(e + fractpart, tmp[i - 1][Y]);
                            } else {
                                _vector[e] = Point(tmp[i - 1][X], tmp[i - 1][Y]);
                            }
                            e++;
                        }
                        //delete temp vector
                        std::vector<Point>().swap(tmp);
                        if (last_pathv.size() > counterPaths) {
                            last_pathv[counterPaths] = last_pathv[counterPaths].reverse();
                        }
                    } else {
                        if (last_pathv.size() > counterPaths) {
                            if (last_pathv[counterPaths].size() <
                                    pathv[counterPaths].size()) {
                                offset++;
                            } else if (last_pathv[counterPaths].size() >
                                       pathv[counterPaths].size()) {
                                offset--;
                                continue;
                            }
                        } else {
                            offset++;
                        }
                    }
                    double xPos = 0;
                    if (_vector[1][X] > 0) {
                        xPos = nearest_point(curve_it1->initialPoint(), pwd2_in);
                    }
                    if (nodetype == NODE_CUSP) {
                        result.push_back(Point(xPos, 1));
                    } else {
                        result.push_back(Point(xPos, 0));
                    }
                } else {
                    double xPos = _vector[counter - offset][X];
                    if (_vector.size() <= (unsigned)(counter - offset)) {
                        if (_vector[1][X] > 0) {
                            xPos = nearest_point(curve_it1->initialPoint(), pwd2_in);
                        } else {
                            xPos = 0;
                        }
                    }
                    if (nodetype == NODE_CUSP) {
                        double vectorY = _vector[counter - offset][Y];
                        if (_vector.size() <= (unsigned)(counter - offset) || vectorY == 0) {
                            vectorY = 1;
                        }
                        result.push_back(Point(xPos, vectorY));
                    } else {
                        if (_vector[1][X] < 0) {
                            xPos = 0;
                        }
                        result.push_back(Point(floor(xPos), 0));
                    }
                }
                ++curve_it1;
                if (curve_it2 != curve_endit) {
                    ++curve_it2;
                }
                counter++;
                counterCurves++;
            }
            counterPaths++;
        }
        _vector = result;
        write_to_SVG();
    }
}

void FilletChamferPointArrayParam::recalculate_knots(
    Piecewise<D2<SBasis> > const &pwd2_in)
{
    bool change = false;
    PathVector pathv = path_from_piecewise(pwd2_in, 0.001);
    if (!pathv.empty()) {
        std::vector<Point> result;
        int counter = 0;
        int counterCurves = 0;
        Geom::NodeType nodetype;
        for (PathVector::const_iterator path_it = pathv.begin();
                path_it != pathv.end(); ++path_it) {
            if (path_it->empty()) {
                counter++;
                continue;
            }
            Geom::Path::const_iterator curve_it1 = path_it->begin();
            Geom::Path::const_iterator curve_it2 = ++(path_it->begin());
            Geom::Path::const_iterator curve_endit = path_it->end_default();
            if (path_it->closed() && path_it->back_closed().isDegenerate()) {
                const Curve &closingline = path_it->back_closed();
                if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                    curve_endit = path_it->end_open();
                }
            }
            counterCurves = 0;
            while (curve_it1 != curve_endit) {
                //if start a path get node type
                if (counterCurves == 0) {
                    if (path_it->closed()) {
                        if (path_it->back_closed().isDegenerate()) {
                            nodetype = get_nodetype(path_it->back_open(), *curve_it1);
                        } else {
                            nodetype = get_nodetype(path_it->back_closed(), *curve_it1);
                        }
                    } else {
                        nodetype = NODE_NONE;
                    }
                } else {
                    bool this_is_line = true;
                    bool next_is_line = is_straight_curve(*curve_it1);
                    this_is_line = is_straight_curve((*path_it)[counterCurves - 1]);
                    nodetype = get_nodetype((*path_it)[counterCurves - 1], *curve_it1);
                    if (this_is_line || next_is_line) {
                        nodetype = NODE_CUSP;
                    }
                }
                if (nodetype == NODE_CUSP) {
                    double vectorY = _vector[counter][Y];
                    if (vectorY == 0) {
                        vectorY = 1;
                        change = true;
                    }
                    result.push_back(Point(_vector[counter][X], vectorY));
                } else {
                    double xPos = floor(_vector[counter][X]);
                    if (_vector[1][X] < 0) {
                        xPos = 0;
                    }
                    double vectorY = _vector[counter][Y];
                    if (vectorY != 0) {
                        change = true;
                    }
                    result.push_back(Point(xPos, 0));
                }
                ++curve_it1;
                ++curve_it2;
                if (curve_it2 != curve_endit) {
                    counter++;
                }
                counterCurves++;
            }
        }
        if (change) {
            _vector = result;
            write_to_SVG();
        }
    }
}

void FilletChamferPointArrayParam::set_pwd2(
    Piecewise<D2<SBasis> > const &pwd2_in,
    Piecewise<D2<SBasis> > const &pwd2_normal_in)
{
    last_pwd2 = pwd2_in;
    last_pwd2_normal = pwd2_normal_in;
}

void FilletChamferPointArrayParam::set_helper_size(int hs)
{
    helper_size = hs;
}

void FilletChamferPointArrayParam::set_unit(const gchar *abbr)
{
    unit = abbr;
}

void FilletChamferPointArrayParam::updateCanvasIndicators()
{
    std::vector<Point> ts = data();
    hp.clear();
    unsigned int i = 0;
    Piecewise<D2<SBasis> > const &n = get_pwd2_normal();
    for (std::vector<Point>::const_iterator point_it = ts.begin();
            point_it != ts.end(); ++point_it) {
        double Xvalue = to_time(i, (*point_it)[X]);
        double XPlusValue = to_time(i, -helper_size) - i;
        if (Xvalue == i) {
            i++;
            continue;
        }
        Point canvas_point = last_pwd2.valueAt(Xvalue) + 0 * n.valueAt(Xvalue);
        Point start_point = last_pwd2.valueAt(Xvalue + XPlusValue) +
                            helper_size * n.valueAt(Xvalue + XPlusValue);
        Point end_point = last_pwd2.valueAt(Xvalue + XPlusValue) -
                          helper_size * n.valueAt(Xvalue + XPlusValue);
        Geom::Path arrow;
        arrow.start(start_point);
        arrow.appendNew<Geom::LineSegment>(canvas_point);
        arrow.appendNew<Geom::LineSegment>(end_point);
        hp.push_back(arrow);
        i++;
    }
}

void FilletChamferPointArrayParam::addCanvasIndicators(
    SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(hp);
}

double FilletChamferPointArrayParam::len_to_time(int index, double len)
{
    double t = 0;
    if (last_pwd2.size() > (unsigned) index) {
        if (len != 0) {
            if (last_pwd2[index][0].degreesOfFreedom() != 2) {
                Piecewise<D2<SBasis> > u;
                u.push_cut(0);
                u.push(last_pwd2[index], 1);
                std::vector<double> t_roots = roots(arcLengthSb(u) - std::abs(len));
                if (t_roots.size() > 0) {
                    t = t_roots[0];
                }
            } else {
                double lenghtPart = 0;
                if (last_pwd2.size() > (unsigned) index) {
                    lenghtPart = length(last_pwd2[index], EPSILON);
                }
                if (std::abs(len) < lenghtPart && lenghtPart != 0) {
                    t = std::abs(len) / lenghtPart;
                }
            }
        }
        t = double(index) + t;
    } else {
        t = double(last_pwd2.size() - 1);
    }

    return t;
}

double FilletChamferPointArrayParam::time_to_len(int index, double time)
{
    double intpart;
    double len = 0;
    time = modf(time, &intpart);
    double lenghtPart = 0;
    if (last_pwd2.size() <= (unsigned) index || time == 0) {
        return len;
    }
    if (last_pwd2[index][0].degreesOfFreedom() != 2) {
        Piecewise<D2<SBasis> > u;
        u.push_cut(0);
        u.push(last_pwd2[index], 1);
        u = portion(u, 0, time);
        return length(u, 0.001) * -1;
    }
    lenghtPart = length(last_pwd2[index], EPSILON);
    return (time * lenghtPart) * -1;
}

double FilletChamferPointArrayParam::to_time(int index, double A)
{
    if (A > 0) {
        return A;
    } else {
        return len_to_time(index, A);
    }
}

double FilletChamferPointArrayParam::to_len(int index, double A)
{
    if (A > 0) {
        return time_to_len(index, A);
    } else {
        return A;
    }
}

void FilletChamferPointArrayParam::set_oncanvas_looks(SPKnotShapeType shape,
        SPKnotModeType mode,
        guint32 color)
{
    knot_shape = shape;
    knot_mode = mode;
    knot_color = color;
}
/*
class FilletChamferPointArrayParamKnotHolderEntity : public KnotHolderEntity {
public:
    FilletChamferPointArrayParamKnotHolderEntity(FilletChamferPointArrayParam
*p, unsigned int index);
    virtual ~FilletChamferPointArrayParamKnotHolderEntity() {}

    virtual void knot_set(Point const &p, Point const &origin, guint state);
    virtual Point knot_get() const;
    virtual void knot_click(guint state);
    virtual void knot_doubleclicked(guint state);

    /Checks whether the index falls within the size of the parameter's vector/
    bool valid_index(unsigned int index) const {
        return (_pparam->_vector.size() > index);
    };

private:
    FilletChamferPointArrayParam *_pparam;
    unsigned int _index;
};
/*/

FilletChamferPointArrayParamKnotHolderEntity::
FilletChamferPointArrayParamKnotHolderEntity(
    FilletChamferPointArrayParam *p, unsigned int index)
    : _pparam(p), _index(index) {}

void FilletChamferPointArrayParamKnotHolderEntity::knot_set(Point const &p,
        Point const &origin,
        guint state)
{
    using namespace Geom;

    if (!valid_index(_index)) {
        return;
    }
    /// @todo how about item transforms???
    Piecewise<D2<SBasis> > const &pwd2 = _pparam->get_pwd2();
    //todo: add snapping
    //Geom::Point const s = snap_knot_position(p, state);
    double t = nearest_point(p, pwd2[_index]);
    if (t == 1) {
        t = 0.9999;
    }
    t += _index;

    if (_pparam->_vector.at(_index)[X] <= 0) {
        _pparam->_vector.at(_index) =
            Point(_pparam->time_to_len(_index, t), _pparam->_vector.at(_index)[Y]);
    } else {
        _pparam->_vector.at(_index) = Point(t, _pparam->_vector.at(_index)[Y]);
    }
    sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
}

Point FilletChamferPointArrayParamKnotHolderEntity::knot_get() const
{
    using namespace Geom;

    if (!valid_index(_index)) {
        return Point(infinity(), infinity());
    }

    Piecewise<D2<SBasis> > const &pwd2 = _pparam->get_pwd2();

    double time_it = _pparam->to_time(_index, _pparam->_vector.at(_index)[X]);
    Point canvas_point = pwd2.valueAt(time_it);

    _pparam->updateCanvasIndicators();
    return canvas_point;

}

void FilletChamferPointArrayParamKnotHolderEntity::knot_click(guint state)
{
    if (state & GDK_CONTROL_MASK) {
        using namespace Geom;
        double type = _pparam->_vector.at(_index)[Y] + 1;
        if (type > 4) {
            type = 1;
        }
        _pparam->_vector.at(_index) = Point(_pparam->_vector.at(_index)[X], type);
        _pparam->param_set_and_write_new_value(_pparam->_vector);
        sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
        const gchar *tip;
        if (type == 3) {
            tip = _("<b>Chamfer</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        } else if (type == 2) {
            tip = _("<b>Inverse Fillet</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        } else if (type == 1) {
            tip = _("<b>Fillet</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        } else {
            tip = _("<b>Double Chamfer</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        }
        this->knot->tip = g_strdup(tip);
        this->knot->show();
        //}
    } else if ((state & GDK_MOD1_MASK) || (state & GDK_SHIFT_MASK)) {
        Geom::Point offset = Geom::Point(_pparam->_vector.at(_index).x(),
                                         _pparam->_vector.at(_index).y());
        Inkscape::UI::Dialogs::FilletChamferPropertiesDialog::showDialog(
            this->desktop, offset, this, _pparam->unit);
    }

}

void
FilletChamferPointArrayParamKnotHolderEntity::knot_doubleclicked(guint state)
{
    //todo: fill the double click dialog whith this parameters in the added file
    //src/ui/dialog/lpe-fillet-chamfer-properties.cpp(.h)
    //My idea for when have enought time is:
    //label whith radius in percent
    //label whith radius in size -maybe handle units-
    //entry whith actual radius -in flexible % mode or fixed -?px-
    //2 radio options to switch entry from fixed or flexible, also update the
    //entry
    //Checkbox or two radios to swith fillet or chamfer

}

void FilletChamferPointArrayParamKnotHolderEntity::knot_set_offset(
    Geom::Point offset)
{
    _pparam->_vector.at(_index) = Geom::Point(offset.x(), offset.y());
    this->parent_holder->knot_ungrabbed_handler(this->knot, 0);
}

void FilletChamferPointArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item)
{
    recalculate_knots(get_pwd2());
    for (unsigned int i = 0; i < _vector.size(); ++i) {
        if (_vector[i][Y] <= 0) {
            continue;
        }
        const gchar *tip;
        if (_vector[i][Y] == 3) {
            tip = _("<b>Chamfer</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        } else if (_vector[i][Y] == 2) {
            tip = _("<b>Inverse Fillet</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        } else if (_vector[i][Y] == 1) {
            tip = _("<b>Fillet</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        } else {
            tip = _("<b>Double Chamfer</b>: <b>Ctrl+click</b> toogle type, "
                    "<b>Shift+click</b> open dialog");
        }
        FilletChamferPointArrayParamKnotHolderEntity *e =
            new FilletChamferPointArrayParamKnotHolderEntity(this, i);
        e->create(desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _(tip),
                  knot_shape, knot_mode, knot_color);
        knotholder->add(e);
    }
    updateCanvasIndicators();
}

} /* namespace LivePathEffect */

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
