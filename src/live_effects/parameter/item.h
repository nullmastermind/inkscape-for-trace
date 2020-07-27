// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ITEM_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ITEM_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>


#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/item-reference.h"
#include <cstddef>
#include <sigc++/sigc++.h>

namespace Inkscape {

namespace LivePathEffect {

class ItemParam : public Parameter {
public:
    ItemParam ( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect,
                const gchar * default_value = "");
    ~ItemParam() override;
    Gtk::Widget * param_newWidget() override;

    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    void param_set_default() override;
    void param_update_default(const gchar * default_value) override;
    void param_set_and_write_default();
    SPObject *param_fork();
    void addCanvasIndicators(SPLPEItem const* lpeitem, std::vector<Geom::PathVector> &hp_vec) override;
    sigc::signal <void> signal_item_pasted;
    sigc::signal <void> signal_item_changed;
    void linkitem(Glib::ustring itemid);
    Geom::Affine last_transform;
    friend class LPEBool;
    bool changed; /* this gets set whenever the path is changed (this is set to true, and then the signal_item_changed signal is emitted).
                   * the user must set it back to false if she wants to use it sensibly */
protected:

    gchar * href;     // contains link to other object, e.g. "#path2428", NULL if ItemParam contains pathdata itself
    ItemReference ref;
    sigc::connection ref_changed_connection;
    sigc::connection linked_delete_connection;
    sigc::connection linked_modified_connection;
    sigc::connection linked_transformed_connection;
    void ref_changed(SPObject *old_ref, SPObject *new_ref);
    void remove_link();
    void start_listening(SPObject * to);
    void quit_listening();
    void linked_delete(SPObject *deleted);
    void linked_modified(SPObject *linked_obj, guint flags);
    void linked_transformed(Geom::Affine const *rel_transf, SPItem *moved_item);
    virtual void linked_modified_callback(SPObject *linked_obj, guint flags);
    virtual void linked_transformed_callback(Geom::Affine const *rel_transf, SPItem */*moved_item*/);
    void on_link_button_click();
    void emit_changed();

    gchar * defvalue;

private:
    ItemParam(const ItemParam&) = delete;
    ItemParam& operator=(const ItemParam&) = delete;
};


} //namespace LivePathEffect

} //namespace Inkscape

#endif
