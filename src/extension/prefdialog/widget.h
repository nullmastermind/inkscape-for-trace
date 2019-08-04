// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Base class for extension widgets.
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INK_EXTENSION_WIDGET_H
#define SEEN_INK_EXTENSION_WIDGET_H

#include <string>
#include <vector>

#include <sigc++/sigc++.h>
#include <glibmm/ustring.h>

class SPDocument;

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace XML {
class Node;
}

namespace Extension {

class Extension;


/**
 * Base class to represent all widgets of an extension (including parameters)
 */
class InxWidget {
public:
    InxWidget(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *in_ext);

    virtual ~InxWidget();

    /**
     * Creates a new extension widget for usage in a prefdialog.
     *
     * The type of widget created is parsed from the XML representation passed in,
     * and the suitable subclass constructor is called.
     *
     * For specialized widget types (like parameters) we defer to the subclass function of the same name.
     *
     * @param  in_repr The XML representation describing the widget.
     * @param  in_ex t The extension the widget belongs to.
     * @return a pointer to a new Widget if applicable, null otherwise..
     */
    static InxWidget *make(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *in_ext);

    /** Checks if name is a valid widget name, i.e. a widget can be constructed from it using make() */
    static bool is_valid_widget_name(const char *name);


    virtual Gtk::Widget *get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal);

    virtual const gchar *get_tooltip() const { return nullptr; } // tool-tips are exclusive to InxParameters for now

    /** Indicates if the widget is hidden or not */
    bool get_hidden() const { return _hidden; }

    /** Indentation level of the widget */
    int get_indent() const { return _indent; }



    /** Recommended margin of boxes containing multiple Widgets (in px) */
    const static int GUI_BOX_MARGIN = 10;
    /** Recommended spacing between multiple Widgets packed into a box (in px) */
    const static int GUI_BOX_SPACING = 4;
    /** Recommended indentation width of Widgets(in px) */
    const static int GUI_INDENTATION = 12;
    /** Recommended maximum line length for wrapping textual Widgets (in chars) */
    const static int GUI_MAX_LINE_LENGTH = 60;

protected:
    enum Translatable {
        UNSET, YES, NO
    };

    /** Which extension is this Widget attached to. */
    Inkscape::Extension::Extension *_extension = nullptr;

    /** Whether the Widget is visible. */
    bool _hidden = false;

    /** Indentation level of the Widget. */
    int _indent = 0;

    /** Appearance of the Widget (not used by all Widgets). */
    gchar *_appearance = nullptr;

    /** Is Widget translatable? */
    Translatable _translatable = UNSET;

    /** context for translation of translatable strings. */
    gchar *_context = nullptr;


    /* **** member functions **** */

    /** gets the gettext translation for msgid
      *
      * Handles translation domain of the extension and message context of the Widget internally
      *
      * @param msgid String to translate
      * @return      Translated string
      */
    const char *get_translation(const char* msgid);
};

}  // namespace Extension
}  // namespace Inkscape

#endif // SEEN_INK_EXTENSION_WIDGET_H

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
