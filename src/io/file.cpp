// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * File operations (independent of GUI)
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>
#include <gtkmm.h>

#include "file.h"

#include "document.h"

#include "extension/system.h"     // Extension::open()
#include "extension/extension.h"
#include "extension/db.h"
#include "extension/output.h"
#include "extension/input.h"

#include "object/sp-root.h"

// SPDocument*
// ink_file_new(const std::string &Template)
// {
//   SPDocument *doc = SPDocument::createNewDoc( Template, true, true );
//   return doc;
// }


SPDocument*
ink_file_open(const Glib::RefPtr<Gio::File>& file, bool &cancelled)
{
  cancelled = false;

  SPDocument *doc = nullptr;

  std::string path = file->get_path();

  try {
    doc = Inkscape::Extension::open(nullptr, path.c_str());
  } catch (Inkscape::Extension::Input::no_extension_found &e) {
    doc = nullptr;
  } catch (Inkscape::Extension::Input::open_failed &e) {
    doc = nullptr;
  } catch (Inkscape::Extension::Input::open_cancelled &e) {
    cancelled = true;
    doc = nullptr;
  }

  // Try to open explicitly as SVG.
  if (doc == nullptr && !cancelled) {
    try {
      doc = Inkscape::Extension::open(Inkscape::Extension::db.get(SP_MODULE_KEY_INPUT_SVG), path.c_str());
    } catch (Inkscape::Extension::Input::no_extension_found &e) {
      doc = nullptr;
    } catch (Inkscape::Extension::Input::open_failed &e) {
      doc = nullptr;
    } catch (Inkscape::Extension::Input::open_cancelled &e) {
      cancelled = true;
      doc = nullptr;
    }
  }

  if (doc == nullptr) {
    std::cerr << "ink_file_open: '" << path << "' cannot be opened!" << std::endl;
  } else {

    // This is the only place original values should be set.
    SPRoot *root = doc->getRoot();
    root->original.inkscape = root->version.inkscape;
    root->original.svg      = root->version.svg;
  }

  return doc;
}

