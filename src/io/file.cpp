// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * File operations (independent of GUI)
 *
 * Copyright (C) 2018, 2019 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "file.h"

#include <iostream>
#include <gtkmm.h>

#include "document.h"
#include "document-undo.h"

#include "extension/system.h"     // Extension::open()
#include "extension/extension.h"
#include "extension/db.h"
#include "extension/output.h"
#include "extension/input.h"

#include "object/sp-root.h"

#include "xml/repr.h"


/**
 * Create a blank document, remove any template data.
 * Input: Empty string or template file name.
 */
SPDocument*
ink_file_new(const std::string &Template)
{
    SPDocument *doc = SPDocument::createNewDoc ((Template.empty() ? nullptr : Template.c_str()), true, true );

    if (doc) {
        // Remove all the template info from xml tree
        Inkscape::XML::Node *myRoot = doc->getReprRoot();
        Inkscape::XML::Node *nodeToRemove;

        nodeToRemove = sp_repr_lookup_name(myRoot, "inkscape:templateinfo");
        if (nodeToRemove != nullptr) {
            Inkscape::DocumentUndo::ScopedInsensitive no_undo(doc);
            sp_repr_unparent(nodeToRemove);
            delete nodeToRemove;
        }
        nodeToRemove = sp_repr_lookup_name(myRoot, "inkscape:_templateinfo"); // backwards-compatibility
        if (nodeToRemove != nullptr) {
            Inkscape::DocumentUndo::ScopedInsensitive no_undo(doc);
            sp_repr_unparent(nodeToRemove);
            delete nodeToRemove;
        }
    } else {
        std::cout << "ink_file_new: Did not create new document!" << std::endl;
    }

    return doc;
}

/**
 * Open a document from memory.
 */
SPDocument*
ink_file_open(const Glib::ustring& data)
{
    SPDocument *doc = SPDocument::createNewDocFromMem (data.c_str(), data.length(), true);

    if (doc == nullptr) {
        std::cerr << "ink_file_open: cannot open file in memory (pipe?)" << std::endl;
    } else {

        // This is the only place original values should be set.
        SPRoot *root = doc->getRoot();
        root->original.inkscape = root->version.inkscape;
        root->original.svg      = root->version.svg;
    }

    return doc;
}

/**
 * Open a document.
 */
SPDocument*
ink_file_open(const Glib::RefPtr<Gio::File>& file, bool *cancelled_param)
{
    bool cancelled = false;

    SPDocument *doc = nullptr;

    std::string path = file->get_path();

    // TODO: It's useless to catch these exceptions here (and below) unless we do something with them.
    //       If we can't properly handle them (e.g. by showing a user-visible message) don't catch them!
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
    // TODO: Why is this necessary? Shouldn't this be handled by the first call already?
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

    if (doc != nullptr) {
        // This is the only place original values should be set.
        SPRoot *root = doc->getRoot();
        root->original.inkscape = root->version.inkscape;
        root->original.svg      = root->version.svg;
    } else if (!cancelled) {
        std::cerr << "ink_file_open: '" << path << "' cannot be opened!" << std::endl;
    }

    if (cancelled_param) {
        *cancelled_param = cancelled;
    }
    return doc;
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
