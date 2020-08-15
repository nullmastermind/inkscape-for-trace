// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_CLONE_ORIGINAL_H
#define INKSCAPE_LPE_CLONE_ORIGINAL_H

/*
 * Inkscape::LPECloneOriginal
 *
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/effect.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/originalitem.h"
#include "live_effects/parameter/text.h"
#include "live_effects/lpegroupbbox.h"

#include <sigc++/sigc++.h>

namespace Inkscape {
namespace LivePathEffect {

enum Clonelpemethod { CLM_NONE, CLM_D, CLM_ORIGINALD, CLM_BSPLINESPIRO, CLM_END };

class LPECloneOriginal : public Effect, GroupBBoxEffect {
public:
    LPECloneOriginal(LivePathEffectObject *lpeobject);
    ~LPECloneOriginal() override;
    void doEffect (SPCurve * curve) override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    void cloneAttrbutes(SPObject *origin, SPObject *dest, const gchar * attributes, const gchar * css_properties, bool init);
    void modified(SPObject */*obj*/, guint /*flags*/);
    Gtk::Widget *newWidget() override;
    void syncOriginal();
    void start_listening();
    void quit_listening();
    void lpeitem_deleted(SPObject */*deleted*/);

private:
    OriginalItemParam linkeditem;
    EnumParam<Clonelpemethod> method;
    TextParam attributes;
    Glib::ustring old_attributes;
    TextParam css_properties;
    Glib::ustring old_css_properties;
    BoolParam allow_transforms;
    const gchar * linked;
    bool listening;
    bool is_updating;
    bool sync;
    sigc::connection modified_connection;
    sigc::connection deleted_connection;
    LPECloneOriginal(const LPECloneOriginal&) = delete;
    LPECloneOriginal& operator=(const LPECloneOriginal&) = delete;
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
