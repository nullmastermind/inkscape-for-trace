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


using Inkscape::ObjectSet;

bool
ObjectSet::strokesToPaths(bool legacy, bool skip_undo)
{
  if (desktop() && isEmpty()) {
    desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>stroked path(s)</b> to convert stroke to path."));
    return false;
  }

  // Need to turn on stroke scaling to ensure stroke is scaled when transformed!
  Inkscape::Preferences *prefs = Inkscape::Preferences::get();
  bool scale_stroke = prefs->getBool("/options/transform/stroke", true);
  prefs->setBool("/options/transform/stroke", true);

  bool did = false;

  std::vector<SPItem *> my_items(items().begin(), items().end());

  for (auto item : my_items) {
    Inkscape::XML::Node *new_node = item_to_paths(item, legacy);
    if (new_node) {
      remove (item); // Remove from selection.
      SPObject* new_item = document()->getObjectByRepr(new_node);

      // Markers don't inherit properties from outside the
      // marker. When converted to paths objects they need to be
      // protected from inheritance. This is why (probably) the stroke
      // to path code uses SP_STYLE_FLAG_ALWAYS when defining the
      // style of the fill and stroke during the conversion. This
      // means the style contains every possible property. Once we've
      // finished the stroke to path conversion, we can eliminate
      // unneeded properties from the style element.
      sp_attribute_clean_recursive(new_node, SP_ATTR_CLEAN_STYLE_REMOVE | SP_ATTR_CLEAN_DEFAULT_REMOVE);

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
