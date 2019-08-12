// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Parameters for extensions.
 */
/* Authors:
 *   Ted Gould <ted@gould.cx>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2005-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INK_EXTENSION_PARAM_H__
#define SEEN_INK_EXTENSION_PARAM_H__

#include "widget.h"


namespace Glib {
class ustring;
}


namespace Inkscape {
namespace Extension {

/**
 * A class to represent the parameter of an extension.
 *
 * This is really a super class that allows them to abstract all
 * the different types of parameters into some that can be passed
 * around.  There is also a few functions that are used by all the
 * different parameters.
 */
class InxParameter : public InxWidget {
public:
    InxParameter(Inkscape::XML::Node *in_repr,
                 Inkscape::Extension::Extension *ext);

    ~InxParameter() override;

    /** Wrapper to cast to the object and use its function. */
    bool get_bool() const;

    /** Wrapper to cast to the object and use it's function. */
    int get_int() const;

    /** Wrapper to cast to the object and use it's function. */
    float get_float() const;

    /** Wrapper to cast to the object and use it's function. */
    const char *get_string() const;

    /** Wrapper to cast to the object and use it's function. */
    const char *get_optiongroup() const;
    bool get_optiongroup_contains(const char *value) const;

    /** Wrapper to cast to the object and use it's function. */
    unsigned int get_color() const;

    /** Wrapper to cast to the object and use it's function. */
    bool set_bool(bool in);

    /** Wrapper to cast to the object and use it's function. */
    int set_int(int  in);

    /** Wrapper to cast to the object and use it's function. */
    float set_float(float in);

    /** Wrapper to cast to the object and use it's function. */
    const char *set_string(const char *in);

    /** Wrapper to cast to the object and use it's function. */
    const char *set_optiongroup(const char *in);

    /** Wrapper to cast to the object and use it's function. */
    unsigned int set_color(unsigned int in);

    char const *name() const { return _name; }

    /**
     * Creates a new extension parameter for usage in a prefdialog.
     *
     * The type of widget created is parsed from the XML representation passed in,
     * and the suitable subclass constructor is called.
     *
     * Called from base-class method of the same name.
     *
     * @param  in_repr The XML representation describing the widget.
     * @param  in_ext  The extension the widget belongs to.
     * @return a pointer to a new Widget if applicable, null otherwise..
     */
    static InxParameter *make(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *in_ext);

    const char *get_tooltip() const override { return _description; }

    /**
     * Gets the current value of the parameter in a string form.
     *
     * @return String representation of the parameter's value.
     *
     * \internal Must be implemented by all derived classes.
     *           Unfortunately it seems we can't make this a pure virtual function,
     *           as InxParameter is not supposed to be abstract.
     */
    virtual std::string value_to_string() const;

    /** Recommended spacing between the widgets making up a single Parameter (e.g. label and input) (in px) */
    const static int GUI_PARAM_WIDGETS_SPACING = 4;


    /** An error class for when a parameter is called on a type it is not */
    class param_no_name {};
    class param_no_text {};
    class param_not_bool_param {};
    class param_not_color_param {};
    class param_not_float_param {};
    class param_not_int_param {};
    class param_not_optiongroup_param {};
    class param_not_string_param {};


protected:
    /** The name of this parameter. */
    char *_name = nullptr;

    /** Parameter text to show as the GUI label. */
    char *_text = nullptr;

    /** Extended description of the parameter (currently shown as tooltip on hover). */
    char *_description = nullptr;


    /* **** member functions **** */

    /**
     * Build preference name for the current parameter.
     *
     * Returns a preference name that can be used with setters and getters from Inkscape::Preferences.
     * The name is assembled from a fixed root ("/extensions/"), extension ID and parameter name.
     *
     * @return: Preference name
     */
    Glib::ustring pref_name() const;
};

}  // namespace Extension
}  // namespace Inkscape

#endif // SEEN_INK_EXTENSION_PARAM_H__

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
