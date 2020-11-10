// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for aligning and distrbuting objects without GUI.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Some code and ideas from src/ui/dialogs/align-and-distribute.cpp
 *   Authors: Bryce Harrington
 *            Martin Owens
 *            John Smith
 *            Patrick Storz
 *            Jabier Arraiza
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-object-align.h"

#include "actions-helper.h"
#include "document-undo.h"
#include "enums.h"                // Clones
#include "inkscape-application.h"
#include "inkscape.h"             // Inkscape::Application
#include "selection.h"            // Selection

#include "live_effects/effect-enum.h"
#include "object/object-set.h"    // Selection enum
#include "object/sp-root.h"       // "Desktop Bounds"
#include "path/path-simplify.h"

enum class ObjectAlignTarget {
    LAST,
    FIRST,
    BIGGEST,
    SMALLEST,
    PAGE,
    DRAWING,
    SELECTION
};

void
object_align(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(" ", s.get());

    // Find out if we are using an anchor.
    bool anchor = std::find(tokens.begin(), tokens.end(), "anchor") != tokens.end();

    // Default values:
    auto target = ObjectAlignTarget::SELECTION;
    bool group = false;
    double mx0 = 0;
    double mx1 = 0;
    double my0 = 0;
    double my1 = 0;
    double sx0 = 0;
    double sx1 = 0;
    double sy0 = 0;
    double sy1 = 0;

    // clang-format off
    for (auto token : tokens) {

        // Target
        if      (token == "last"     ) target = ObjectAlignTarget::LAST;
        else if (token == "first"    ) target = ObjectAlignTarget::FIRST;
        else if (token == "biggest"  ) target = ObjectAlignTarget::BIGGEST;
        else if (token == "smallest" ) target = ObjectAlignTarget::SMALLEST;
        else if (token == "page"     ) target = ObjectAlignTarget::PAGE;
        else if (token == "drawing"  ) target = ObjectAlignTarget::DRAWING;
        else if (token == "selection") target = ObjectAlignTarget::SELECTION;

        // Group
        else if (token == "group")     group = true;

        // Position
        if (!anchor) {
            if      (token == "left"    ) { mx0 = 1.0; mx1 = 0.0; sx0 = 1.0; sx1 = 0.0; }
            else if (token == "hcenter" ) { mx0 = 0.5; mx1 = 0.5; sx0 = 0.5; sx1 = 0.5; }
            else if (token == "right"   ) { mx0 = 0.0; mx1 = 1.0; sx0 = 0.0; sx1 = 1.0; }

            else if (token == "top"     ) { my0 = 1.0; my1 = 0.0; sy0 = 1.0; sy1 = 0.0; }
            else if (token == "vcenter" ) { my0 = 0.5; my1 = 0.5; sy0 = 0.5; sy1 = 0.5; }
            else if (token == "bottom"  ) { my0 = 0.0; my1 = 1.0; sy0 = 0.0; sy1 = 1.0; }
        } else {
            if      (token == "left"    ) { mx0 = 0.0; mx1 = 1.0; sx0 = 1.0; sx1 = 0.0; }
            else if (token == "hcenter" ) std::cerr << "'anchor' cannot be used with 'hcenter'" << std::endl;
            else if (token == "right"   ) { mx0 = 1.0; mx1 = 0.0; sx0 = 0.0; sx1 = 1.0; }

            else if (token == "top"     ) { my0 = 0.0; my1 = 1.0; sy0 = 1.0; sy1 = 0.0; }
            else if (token == "vcenter" ) std::cerr << "'anchor' cannot be used with 'vcenter'" << std::endl;
            else if (token == "bottom"  ) { my0 = 1.0; my1 = 0.0; sy0 = 0.0; sy1 = 1.0; }
        }
    }
    // clang-format on

    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document  = app->get_active_document();
    selection->setDocument(document);

    // We force unselect operand in bool LPE. TODO: See if we can use "selected" from below.
    auto list = selection->items();
    for (auto itemlist = list.begin(); itemlist != list.end(); ++itemlist) {
        SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(*itemlist);
        if (lpeitem && lpeitem->hasPathEffectOfType(Inkscape::LivePathEffect::EffectType::BOOL_OP)) {
            sp_lpe_item_update_patheffect(lpeitem, false, false);
        }
    }

    std::vector<SPItem*> selected(selection->items().begin(), selection->items().end());
    if (selected.empty()) return;

    // Find alignment rectangle. This can come from:
    // - The bounding box of an object
    // - The bounding box of a group of objects
    // - The bounding box of the page, drawing, or selection.
    SPItem *focus = nullptr;
    Geom::OptRect b = Geom::OptRect();
    Inkscape::Selection::CompareSize direction = (mx0 != 0.0 || mx1 != 0.0) ? Inkscape::Selection::VERTICAL : Inkscape::Selection::HORIZONTAL;

    switch (target) {
        case ObjectAlignTarget::LAST:
            focus = SP_ITEM(selected.back());
            break;
        case ObjectAlignTarget::FIRST:
            focus = SP_ITEM(selected.front());
            break;
        case ObjectAlignTarget::BIGGEST:
            focus = selection->largestItem(direction);
            break;
        case ObjectAlignTarget::SMALLEST:
            focus = selection->smallestItem(direction);
            break;
        case ObjectAlignTarget::PAGE:
            b = document->preferredBounds();
            break;
        case ObjectAlignTarget::DRAWING:
            b = document->getRoot()->desktopPreferredBounds();
            break;
        case ObjectAlignTarget::SELECTION:
            b = selection->preferredBounds();
            break;
        default:
            g_assert_not_reached ();
            break;
    };

    if (focus) {
        b = focus->desktopPreferredBounds();
    }

    g_return_if_fail(b);

    // if (desktop->is_yaxisdown()) {
    //     std::swap(a.my0, a.my1);
    //     std::swap(a.sy0, a.sy1);
    // }

    // Generate the move point from the selected bounding box
    Geom::Point mp = Geom::Point(mx0 * b->min()[Geom::X] + mx1 * b->max()[Geom::X],
                                 my0 * b->min()[Geom::Y] + my1 * b->max()[Geom::Y]);

    if (group) {
        if (focus) {
            // Use bounding box of all selected elements except the "focused" element.
            Inkscape::ObjectSet copy(document);
            copy.add(selection->objects().begin(), selection->objects().end());
            copy.remove(focus);
            b = copy.preferredBounds();
        } else {
            // Use bounding box of all selected elements.
            b = selection->preferredBounds();
        }
    }

    // Move each item in the selected list separately.
    bool changed = false;
    for (auto item : selected) {
    	document->ensureUpToDate();

        if (!group) {
            b = (item)->desktopPreferredBounds();
        }

        if (b && (!focus || (item) != focus)) {
            Geom::Point const sp(sx0 * b->min()[Geom::X] + sx1 * b->max()[Geom::X],
                                 sy0 * b->min()[Geom::Y] + sy1 * b->max()[Geom::Y]);
            Geom::Point const mp_rel( mp - sp );
            if (LInfty(mp_rel) > 1e-9) {
                item->move_rel(Geom::Translate(mp_rel));
                changed = true;
            }
        }
    }

    if (changed) {
        Inkscape::DocumentUndo::done(document, 0, _("Align"));
    }
}

class BBoxSort {

public:
    BBoxSort(SPItem *item, Geom::Rect const &bounds, Geom::Dim2 orientation, double begin, double end)
        : item(item)
        , bbox(bounds)
    {
        anchor = begin * bbox.min()[orientation] + end * bbox.max()[orientation];
    }

    BBoxSort(const BBoxSort &rhs) = default; // Should really be vector of pointers to avoid copying class when sorting.
    ~BBoxSort() = default;

    double anchor = 0.0;
    SPItem* item = nullptr;
    Geom::Rect bbox;
};

bool operator< (const BBoxSort &a, const BBoxSort &b) {
    return a.anchor < b.anchor;
}


void
object_distribute(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);
    auto token = s.get();

    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document  = app->get_active_document();
    selection->setDocument(document);

    std::vector<SPItem*> selected(selection->items().begin(), selection->items().end());
    if (selected.size() < 2) {
        return;
    }

    // clang-format off
    double a = 0.0;
    double b = 0.0;
    bool gap = false;
    auto orientation = Geom::X;
    if      (token == "hgap"    ) { gap = true;  orientation = Geom::X; a = 0.5, b = 0.5; }
    else if (token == "left"    ) { gap = false; orientation = Geom::X; a = 1.0, b = 0.0; }
    else if (token == "hcenter" ) { gap = false; orientation = Geom::X; a = 0.5, b = 0.5; }
    else if (token == "right"   ) { gap = false; orientation = Geom::X; a = 0.0, b = 1.0; }
    else if (token == "vgap"    ) { gap = true;  orientation = Geom::Y; a = 0.5, b = 0.5; }
    else if (token == "top"     ) { gap = false; orientation = Geom::Y; a = 1.0, b = 0.0; }
    else if (token == "vcenter" ) { gap = false; orientation = Geom::Y; a = 0.5, b = 0.5; }
    else if (token == "bottom"  ) { gap = false; orientation = Geom::Y; a = 0.0, b = 1.0; }
    // clang-format on


    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int prefs_bbox = prefs->getBool("/tools/bounding_box");

    // Make a list of objects, sorted by anchors.
    std::vector<BBoxSort> sorted;
    for (auto item : selected) {
        Geom::OptRect bbox = !prefs_bbox ? (item)->desktopVisualBounds() : (item)->desktopGeometricBounds();
        if (bbox) {
            sorted.emplace_back(item, *bbox, orientation, a, b);
        }
    }
    std::stable_sort(sorted.begin(), sorted.end());

    // See comment in ActionAlign above (MISSING).
    int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

    bool changed = false;
    if (gap) {
        // Evenly spaced.

        // Overall bboxes span.
        double dist = (sorted.back().bbox.max()[orientation] - sorted.front().bbox.min()[orientation]);

        // Space eaten by bboxes.
        double span = 0.0;
        for (auto bbox : sorted) {
            span += bbox.bbox[orientation].extent();
        }

        // New distance between each bbox.
        double step = (dist - span) / (sorted.size() - 1);
        double pos = sorted.front().bbox.min()[orientation];
        for (auto bbox : sorted) {

            // Don't move if we are really close.
            if (!Geom::are_near(pos, bbox.bbox.min()[orientation], 1e-6)) {

                // Compute translation.
                Geom::Point t(0.0, 0.0);
                t[orientation] = pos - bbox.bbox.min()[orientation];

                // Translate
                bbox.item->move_rel(Geom::Translate(t));
                changed = true;
            }

            pos += bbox.bbox[orientation].extent();
            pos += step;
        }

    } else  {

        // Overall anchor span.
        double dist = sorted.back().anchor - sorted.front().anchor;

        // Distance between anchors.
        double step = dist / (sorted.size() - 1);

        for (unsigned int i = 0; i < sorted.size() ; i++) {
            BBoxSort & it(sorted[i]);

            // New anchor position.
            double pos = sorted.front().anchor + i * step;

            // Don't move if we are really close.
            if (!Geom::are_near(pos, it.anchor, 1e-6)) {

                // Compute translation.
                Geom::Point t(0.0, 0.0);
                t[orientation] = pos - it.anchor;

                // Translate
                it.item->move_rel(Geom::Translate(t));
                changed = true;
            }
        }
    }

    // Restore compensation setting.
    prefs->setInt("/options/clonecompensation/value", saved_compensation);

    if (changed) {
        Inkscape::DocumentUndo::done( document, 0, _("Distribute"));
    }
}

std::vector<std::vector<Glib::ustring>> raw_data_object_align =
{
    // clang-format off
    {"app.object-align",      N_("Align objects"),      "Object", N_("Align selected objects: Usage: [[left|hcenter|right] || [top|vcenter|bottom]] [last|first|biggest|smallest|page|drawing|selection]? group? anchor?")},
    {"app.object-distribute", N_("Distribute objects"), "Object", N_("Distribute selected objects: Usage: [hgap | left | hcenter | right | vgap | top | vcenter | bottom]"                                               )}
    // clang-format on
};

void
add_actions_object_align(InkscapeApplication* app)
{
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)

    // clang-format off
    gapp->add_action_with_parameter( "object-align",      String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_align),      app));
    gapp->add_action_with_parameter( "object-distribute", String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_distribute), app));
    // clang-format on

#endif

    app->get_action_extra_data().add_data(raw_data_object_align);
}


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
