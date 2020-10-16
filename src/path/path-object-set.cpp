// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 *
 * Path related functions for ObjectSet.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * TODO: Move related code from path-chemistry.cpp
 *
 */

#include <glibmm/i18n.h>

#include "document-undo.h"
#include "message-stack.h"

#include "attribute-rel-util.h"

#include "object/object-set.h"
#include "path/path-outline.h"
#include "path/path-simplify.h"

using Inkscape::ObjectSet;

bool
ObjectSet::strokesToPaths(bool legacy, bool skip_undo)
{
  if (desktop() && isEmpty()) {
    desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>stroked path(s)</b> to convert stroke to path."));
    return false;
  }

  Inkscape::Preferences *prefs = Inkscape::Preferences::get();
  if (prefs->getBool("/options/pathoperationsunlink/value", true)) {
      unlinkRecursive(true);
  }

  // Need to turn on stroke scaling to ensure stroke is scaled when transformed!
  bool scale_stroke = prefs->getBool("/options/transform/stroke", true);
  prefs->setBool("/options/transform/stroke", true);

  bool did = false;

  std::vector<SPItem *> my_items(items().begin(), items().end());

  for (auto item : my_items) {
    // Do not remove the object from the selection here 
    // as we want to keep it selected if the whole operation fails
    Inkscape::XML::Node *new_node = item_to_paths(item, legacy);
    if (new_node) {
      SPObject* new_item = document()->getObjectByRepr(new_node);

      // Markers don't inherit properties from outside the
      // marker. When converted to paths objects they need to be
      // protected from inheritance. This is why (probably) the stroke
      // to path code uses SP_STYLE_FLAG_ALWAYS when defining the
      // style of the fill and stroke during the conversion. This
      // means the style contains every possible property. Once we've
      // finished the stroke to path conversion, we can eliminate
      // unneeded properties from the style element.
      sp_attribute_clean_recursive(new_node, SP_ATTRCLEAN_STYLE_REMOVE | SP_ATTRCLEAN_DEFAULT_REMOVE);

      add(new_item); // Add to selection.
      did = true;
    }
  }

  // Reset
  prefs->setBool("/options/transform/stroke", scale_stroke);

  if (desktop() && !did) {
    desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No stroked paths</b> in the selection."));
  }

  if (did && !skip_undo) {
    Inkscape::DocumentUndo::done(document(), SP_VERB_NONE, _("Convert stroke to path"));
  }

  return did;
}

bool
ObjectSet::simplifyPaths(bool skip_undo)
{
    if (desktop() && isEmpty()) {
        desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>path(s)</b> to simplify."));
        return false;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double threshold = prefs->getDouble("/options/simplifythreshold/value", 0.003);
    bool justCoalesce = prefs->getBool(  "/options/simplifyjustcoalesce/value", false);

    // Keep track of accelerated simplify
    static gint64 previous_time = 0;
    static gdouble multiply = 1.0;

    // Get the current time
    gint64 current_time = g_get_monotonic_time();

    // Was the previous call to this function recent? (<0.5 sec)
    if (previous_time > 0 && current_time - previous_time < 500000) {

        // add to the threshold 1/2 of its original value
        multiply  += 0.5;
        threshold *= multiply;

    } else {
        // reset to the default
        multiply = 1;
    }

    // Remember time for next call
    previous_time = current_time;

    // set "busy" cursor
    if (desktop()) {
        desktop()->setWaitingCursor();
    }

    Geom::OptRect selectionBbox = visualBounds();
    if (!selectionBbox) {
        std::cerr << "ObjectSet::: selection has no visual bounding box!" << std::endl;
        return false;
    }
    double size = L2(selectionBbox->dimensions());

    int pathsSimplified = 0;
    std::vector<SPItem *> my_items(items().begin(), items().end());
    for (auto item : my_items) {
        pathsSimplified += path_simplify(item, threshold, justCoalesce, size);
    }

    if (pathsSimplified > 0 && !skip_undo) {
        DocumentUndo::done(document(), SP_VERB_SELECTION_SIMPLIFY,  _("Simplify"));
    }

    if (desktop()) {
        desktop()->clearWaitingCursor();
        if (pathsSimplified > 0) {
            desktop()->messageStack()->flashF(Inkscape::NORMAL_MESSAGE, _("<b>%d</b> paths simplified."), pathsSimplified);
        } else {
            desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No paths</b> to simplify in the selection."));
        }
    }

    return (pathsSimplified > 0);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
