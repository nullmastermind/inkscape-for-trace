// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/point.h"
#include "live_effects/effect.h"

#include "inkscape.h"
#include "verbs.h"

#include "svg/stringstream.h"
#include "svg/svg.h"
#include "ui/knot/knot-holder.h"
#include "ui/knot/knot-holder-entity.h"
#include "ui/widget/point.h"

#include <glibmm/i18n.h>

namespace Inkscape {

namespace LivePathEffect {

PointParam::PointParam( const Glib::ustring& label, const Glib::ustring& tip,
                        const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                        Effect* effect, const gchar *htip, Geom::Point default_value,
                        bool live_update )
    : Parameter(label, tip, key, wr, effect)
    , defvalue(default_value)
    , liveupdate(live_update)
{
    handle_tip = g_strdup(htip);
}

PointParam::~PointParam()
{
    if (handle_tip)
        g_free(handle_tip);
}

void
PointParam::param_set_default()
{
    param_setValue(defvalue,true);
}

void
PointParam::param_set_liveupdate( bool live_update)
{
    liveupdate = live_update;
}

Geom::Point 
PointParam::param_get_default() const{
    return defvalue;
}

void
PointParam::param_update_default(Geom::Point default_point)
{
    defvalue = default_point;
}

void
PointParam::param_update_default(const gchar * default_point)
{
    gchar ** strarray = g_strsplit(default_point, ",", 2);
    double newx, newy;
    unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
    success += sp_svg_number_read_d(strarray[1], &newy);
    g_strfreev (strarray);
    if (success == 2) {
        param_update_default( Geom::Point(newx, newy) );
    }
}

void 
PointParam::param_hide_knot(bool hide) {
    if (_knot_entity) {
        bool update = false;
        if (hide && _knot_entity->knot->flags & SP_KNOT_VISIBLE) {
            update = true;
            _knot_entity->knot->hide();
        } else if(!hide && !(_knot_entity->knot->flags & SP_KNOT_VISIBLE)) {
            update = true;
            _knot_entity->knot->show();
        }
        if (update) {
            _knot_entity->update_knot();
        }
    }
}

void
PointParam::param_setValue(Geom::Point newpoint, bool write)
{
    *dynamic_cast<Geom::Point *>( this ) = newpoint;
    if(write){
        Inkscape::SVGOStringStream os;
        os << newpoint;
        gchar * str = g_strdup(os.str().c_str());
        param_write_to_repr(str);
        g_free(str);
    }
    if(_knot_entity && liveupdate){
        _knot_entity->update_knot();
    }
}

bool
PointParam::param_readSVGValue(const gchar * strvalue)
{
    gchar ** strarray = g_strsplit(strvalue, ",", 2);
    double newx, newy;
    unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
    success += sp_svg_number_read_d(strarray[1], &newy);
    g_strfreev (strarray);
    if (success == 2) {
        param_setValue( Geom::Point(newx, newy) );
        return true;
    }
    return false;
}

Glib::ustring
PointParam::param_getSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << *dynamic_cast<Geom::Point const *>( this );
    return os.str();
}

Glib::ustring
PointParam::param_getDefaultSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << defvalue;
    return os.str();
}

void
PointParam::param_transform_multiply(Geom::Affine const& postmul, bool /*set*/)
{
    param_setValue( (*this) * postmul, true);
}

Gtk::Widget *
PointParam::param_newWidget()
{
    Inkscape::UI::Widget::RegisteredTransformedPoint * pointwdg = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredTransformedPoint( param_label,
                                                              param_tooltip,
                                                              param_key,
                                                              *param_wr,
                                                              param_effect->getRepr(),
                                                              param_effect->getSPDoc() ) );
    Geom::Affine transf = SP_ACTIVE_DESKTOP->doc2dt();
    pointwdg->setTransform(transf);
    pointwdg->setValue( *this );
    pointwdg->clearProgrammatically();
    pointwdg->set_undo_parameters(SP_VERB_DIALOG_LIVE_PATH_EFFECT, _("Change point parameter"));
    pointwdg->signal_button_release_event().connect(sigc::mem_fun (*this, &PointParam::on_button_release));

    Gtk::Box * hbox = Gtk::manage( new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL) );
    hbox->pack_start(*pointwdg, true, true);
    hbox->show_all_children();
    return dynamic_cast<Gtk::Widget *> (hbox);
}

bool PointParam::on_button_release(GdkEventButton* button_event) {
    param_effect->refresh_widgets = true;
    return false;
}

void
PointParam::set_oncanvas_looks(Inkscape::CanvasItemCtrlShape shape,
                               Inkscape::CanvasItemCtrlMode mode,
                               guint32 color)
{
    knot_shape = shape;
    knot_mode  = mode;
    knot_color = color;
}

class PointParamKnotHolderEntity : public KnotHolderEntity {
public:
    PointParamKnotHolderEntity(PointParam *p) { this->pparam = p; }
    ~PointParamKnotHolderEntity() override { this->pparam->_knot_entity = nullptr;}

    void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    void knot_click(guint state) override;

private:
    PointParam *pparam;
};

void
PointParamKnotHolderEntity::knot_set(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    Geom::Point s = snap_knot_position(p, state);
    if (state & GDK_CONTROL_MASK) {
        Geom::Point A(origin[Geom::X],p[Geom::Y]);
        Geom::Point B(p[Geom::X],origin[Geom::Y]);
        double distanceA = Geom::distance(A,p);
        double distanceB = Geom::distance(B,p);
        if(distanceA > distanceB){
            s = B;
        } else {
            s = A;
        }
    }
    if(this->pparam->liveupdate){
        pparam->param_setValue(s, true);
    } else {
        pparam->param_setValue(s);
    }
}

void
PointParamKnotHolderEntity::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    pparam->param_effect->refresh_widgets = true;
    pparam->write_to_SVG();
}

Geom::Point
PointParamKnotHolderEntity::knot_get() const
{
    return *pparam;
}

void
PointParamKnotHolderEntity::knot_click(guint state)
{
    if (state & GDK_CONTROL_MASK) {
        if (state & GDK_MOD1_MASK) {
            this->pparam->param_set_default();
            pparam->param_setValue(*pparam,true);
        }
    }
}

void
PointParam::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    _knot_entity = new PointParamKnotHolderEntity(this);
    // TODO: can we ditch handleTip() etc. because we have access to handle_tip etc. itself???
    _knot_entity->create(nullptr, item, knotholder, Inkscape::CANVAS_ITEM_CTRL_TYPE_LPE, "LPE:Point",
                         handleTip(), knot_color);
    knotholder->add(_knot_entity);
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
