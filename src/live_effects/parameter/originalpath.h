// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINAL_PATH_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINAL_PATH_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/path.h"

namespace Inkscape {

namespace LivePathEffect {

class OriginalPathParam: public PathParam {
public:
    OriginalPathParam ( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect);
    ~OriginalPathParam() override;

    bool linksToPath() const { return (href != nullptr); }
    SPItem * getObject() const { return ref.getObject(); }

    Gtk::Widget * param_newWidget() override;
    /** Disable the canvas indicators of parent class by overriding this method */
    void param_editOncanvas(SPItem * /*item*/, SPDesktop * /*dt*/) override {};
    /** Disable the canvas indicators of parent class by overriding this method */
    void addCanvasIndicators(SPLPEItem const* /*lpeitem*/, std::vector<Geom::PathVector> & /*hp_vec*/) override {};
    void setFromOriginalD(bool from_original_d){ _from_original_d = from_original_d; };

protected:
    void linked_modified_callback(SPObject *linked_obj, guint flags) override;
    void linked_transformed_callback(Geom::Affine const *rel_transf, SPItem *moved_item) override;

    void on_select_original_button_click();

private:
    bool _from_original_d;
    OriginalPathParam(const OriginalPathParam&) = delete;
    OriginalPathParam& operator=(const OriginalPathParam&) = delete;
};


} //namespace LivePathEffect

} //namespace Inkscape

#endif
