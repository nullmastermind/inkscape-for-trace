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
     * @param  in_ext  The extension the widget belongs to.
     * @return a pointer to a new Widget if applicable, null otherwise..
     */
    static InxWidget *make(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *in_ext);

    /** Checks if name is a valid widget name, i.e. a widget can be constructed from it using make() */
    static bool is_valid_widget_name(const char *name);

    /** Return the instance's GTK::Widget representation for usage in a GUI
      *
      * @param changeSignal Can be used to subscribe to parameter changes.
      *                     Will be emitted whenever a parameter value changes.
      *
      * @teturn A Gtk::Widget for the \a InxWidget. \c nullptr if the widget is hidden.
      */
    virtual Gtk::Widget *get_widget(sigc::signal<void> *changeSignal);

    virtual const char *get_tooltip() const { return nullptr; } // tool-tips are exclusive to InxParameters for now

    /** Indicates if the widget is hidden or not */
    bool get_hidden() const { return _hidden; }

    /** Indentation level of the widget */
    int get_indent() const { return _indent; }


    /**
     * Recursively construct a list containing the current widget and all of it's child widgets (if it has any)
     *
     * @param list Reference to a vector of pointers to \a InxWidget that will be appended with the new \a InxWidgets
     */
    virtual void get_widgets(std::vector<InxWidget *> &list);


    /** Recommended margin of boxes containing multiple widgets (in px) */
    const static int GUI_BOX_MARGIN = 10;
    /** Recommended spacing between multiple widgets packed into a box (in px) */
    const static int GUI_BOX_SPACING = 4;
    /** Recommended indentation width of widgets(in px) */
    const static int GUI_INDENTATION = 12;
    /** Recommended maximum line length for wrapping textual wdgets (in chars) */
    const static int GUI_MAX_LINE_LENGTH = 60;

protected:
    enum Translatable {
        UNSET, YES, NO
    };

    /** Which extension is this Widget attached to. */
    Inkscape::Extension::Extension *_extension = nullptr;

    /** Child widgets of this widget (might be empty if there are none) */
    std::vector<InxWidget *> _children;

    /** Whether the widget is visible. */
    bool _hidden = false;

    /** Indentation level of the widget. */
    int _indent = 0;

    /** Appearance of the widget (not used by all widgets). */
    char *_appearance = nullptr;

    /** Is widget translatable? */
    Translatable _translatable = UNSET;

    /** context for translation of translatable strings. */
    char *_context = nullptr;


    /* **** member functions **** */

    /** gets the gettext translation for msgid
      *
      * Handles translation domain of the extension and message context of the widget internally
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
