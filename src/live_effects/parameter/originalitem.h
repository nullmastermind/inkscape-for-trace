// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINAL_ITEM_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINAL_ITEM_H

/*
 * Inkscape::LiveItemEffectParameters
 *
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/item.h"

namespace Inkscape {

namespace LivePathEffect {

class OriginalItemParam: public ItemParam {
public:
    OriginalItemParam ( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect);
    ~OriginalItemParam() override;
    bool linksToItem() const { return (href != nullptr); }
    SPItem * getObject() const { return ref.getObject(); }

    Gtk::Widget * param_newWidget() override;

protected:
    void linked_modified_callback(SPObject *linked_obj, guint flags) override;
    void linked_transformed_callback(Geom::Affine const *rel_transf, SPItem *moved_item) override;

    void on_select_original_button_click();

private:
    OriginalItemParam(const OriginalItemParam&) = delete;
    OriginalItemParam& operator=(const OriginalItemParam&) = delete;
};


} //namespace LivePathEffect

} //namespace Inkscape

#endif
