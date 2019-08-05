// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INK_EXTENSION_PARAMOPTIONGROUP_H_SEEN
#define INK_EXTENSION_PARAMOPTIONGROUP_H_SEEN

/** \file
 * extension parameter for options with multiple predefined value choices
 *
 * Currently implemented as either Gtk::RadioButton or Gtk::ComboBoxText
 */

/*
 * Authors:
 *   Johan Engelen <johan@shouraizou.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2006-2007 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"

#include <vector>

#include <glibmm/ustring.h>

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace Extension {

class Extension;



// \brief  A class to represent an optiongroup (option with multiple predefined value choices) parameter of an extension
class ParamOptionGroup : public InxParameter {
public:
    enum AppearanceMode {
        RADIOBUTTON, COMBOBOX
    };

    ParamOptionGroup(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);
    ~ParamOptionGroup() override;

    Gtk::Widget *get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal) override;

    // Explicitly call superclass version to avoid method being hidden.
    void string(std::list <std::string> &list) const override { return InxParameter::string(list); }

    void string(std::string &string) const override;

    Glib::ustring value_from_label(const Glib::ustring label);

    const Glib::ustring& get(const SPDocument * /*doc*/, const Inkscape::XML::Node * /*node*/) const { return _value; }

    const Glib::ustring& set(const Glib::ustring in, SPDocument *doc, Inkscape::XML::Node *node);

    /**
     * @returns    true if text is a valid choice for this option group
     * @param text string value to check (this is an actual option value, not the user-visible option name!)
     */
    bool contains(const Glib::ustring text, SPDocument const * /*doc*/, Inkscape::XML::Node const * /*node*/) const;

private:
    /** \brief  Internal value. */
    Glib::ustring _value;

    /** appearance mode **/
    AppearanceMode _mode = RADIOBUTTON;

    /* For internal use only. */
    class ParamOptionGroupOption : public InxParameter {
        friend class ParamOptionGroup;
    public:
        ParamOptionGroupOption(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext,
                               const Inkscape::Extension::ParamOptionGroup *parent);
    private:
        Glib::ustring _value;
        Glib::ustring _text;
    };

    std::vector<ParamOptionGroupOption *> choices; /**< List of available options for the option group */
}; /* class ParamOptionGroup */





}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /*INK_EXTENSION_PARAMOPTIONGROUP_H_SEEN */

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
