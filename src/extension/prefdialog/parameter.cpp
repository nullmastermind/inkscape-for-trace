// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Parameters for extensions.
 */
/* Author:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2005-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <list>

#include <glibmm/i18n.h>
#include <sigc++/sigc++.h>

#include "parameter.h"
#include "parameter-bool.h"
#include "parameter-color.h"
#include "parameter-float.h"
#include "parameter-int.h"
#include "parameter-notebook.h"
#include "parameter-optiongroup.h"
#include "parameter-path.h"
#include "parameter-string.h"
#include "widget.h"
#include "widget-label.h"

#include "extension/extension.h"

#include "object/sp-defs.h"

#include "ui/widget/color-notebook.h"

#include "xml/node.h"


namespace Inkscape {
namespace Extension {


// Re-implement ParamDescription for backwards-compatibility, deriving from both, WidgetLabel and InxParameter.
// TODO: Should go away eventually...
class ParamDescription : public virtual WidgetLabel, public virtual InxParameter {
public:
    ParamDescription(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
        : WidgetLabel(xml, ext)
        , InxParameter(xml, ext)
    {}

    Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override
    {
        return this->WidgetLabel::get_widget(changeSignal);
    }

    // Well, no, I don't have a value! That's why I should not be an InxParameter!
    std::string value_to_string() const override { return ""; }
};



InxParameter *InxParameter::make(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *in_ext)
{
    InxParameter *param = nullptr;

    try {
        const char *type = in_repr->attribute("type");
        if (!type) {
            // we can't create a parameter without type
            g_warning("Parameter without type in extension '%s'.", in_ext->get_id());
        } else if(!strcmp(type, "bool") || !strcmp(type, "boolean")) { // support "boolean" for backwards-compatibility
            param = new ParamBool(in_repr, in_ext);
        } else if (!strcmp(type, "int")) {
            param = new ParamInt(in_repr, in_ext);
        } else if (!strcmp(type, "float")) {
            param = new ParamFloat(in_repr, in_ext);
        } else if (!strcmp(type, "string")) {
            param = new ParamString(in_repr, in_ext);
        } else if (!strcmp(type, "path")) {
            param = new ParamPath(in_repr, in_ext);
        } else if (!strcmp(type, "description")) {
            // support deprecated "description" for backwards-compatibility
            in_repr->setAttribute("gui-text", "description"); // TODO: hack to allow descriptions to be parameters
            param = new ParamDescription(in_repr, in_ext);
        } else if (!strcmp(type, "notebook")) {
            in_repr->setAttribute("gui-text", "notebook"); // notebooks have no 'gui-text' (but Parameters need one)
            param = new ParamNotebook(in_repr, in_ext);
        } else if (!strcmp(type, "optiongroup")) {
            param = new ParamOptionGroup(in_repr, in_ext);
        } else if (!strcmp(type, "enum")) { // support deprecated "enum" for backwards-compatibility
            in_repr->setAttribute("appearance", "combo");
            param = new ParamOptionGroup(in_repr, in_ext);
        } else if (!strcmp(type, "color")) {
            param = new ParamColor(in_repr, in_ext);
        } else {
            g_warning("Unknown parameter type ('%s') in extension '%s'", type, in_ext->get_id());
        }
    } catch (const param_no_name&) {
    } catch (const param_no_text&) {
    }

    // Note: param could equal nullptr
    return param;
}

bool InxParameter::get_bool() const
{
    ParamBool const *boolpntr = dynamic_cast<ParamBool const *>(this);
    if (!boolpntr) {
        throw param_not_bool_param();
    }
    return boolpntr->get();
}

int InxParameter::get_int() const
{
    ParamInt const *intpntr = dynamic_cast<ParamInt const *>(this);
    if (!intpntr) {
        throw param_not_int_param();
    }
    return intpntr->get();
}

float InxParameter::get_float() const
{
    ParamFloat const *floatpntr = dynamic_cast<ParamFloat const *>(this);
    if (!floatpntr) {
        throw param_not_float_param();
    }
    return floatpntr->get();
}

const char *InxParameter::get_string() const
{
    ParamString const *stringpntr = dynamic_cast<ParamString const *>(this);
    if (!stringpntr) {
        throw param_not_string_param();
    }
    return stringpntr->get().c_str();
}

const char *InxParameter::get_optiongroup() const
{
    ParamOptionGroup const *param = dynamic_cast<ParamOptionGroup const *>(this);
    if (!param) {
        throw param_not_optiongroup_param();
    }
    return param->get().c_str();
}

bool InxParameter::get_optiongroup_contains(const char *value) const
{
    ParamOptionGroup const *param = dynamic_cast<ParamOptionGroup const *>(this);
    if (!param) {
        throw param_not_optiongroup_param();
    }
    return param->contains(value);
}

unsigned int InxParameter::get_color() const
{
    ParamColor const *param = dynamic_cast<ParamColor const *>(this);
    if (!param) {
        throw param_not_color_param();
    }
    return param->get();
}

bool InxParameter::set_bool(bool in)
{
    ParamBool * boolpntr = dynamic_cast<ParamBool *>(this);
    if (boolpntr == nullptr)
        throw param_not_bool_param();
    return boolpntr->set(in);
}

int InxParameter::set_int(int in)
{
    ParamInt *intpntr = dynamic_cast<ParamInt *>(this);
    if (intpntr == nullptr)
        throw param_not_int_param();
    return intpntr->set(in);
}

float InxParameter::set_float(float in)
{
    ParamFloat * floatpntr;
    floatpntr = dynamic_cast<ParamFloat *>(this);
    if (floatpntr == nullptr)
        throw param_not_float_param();
    return floatpntr->set(in);
}

const char *InxParameter::set_string(const char *in)
{
    ParamString * stringpntr = dynamic_cast<ParamString *>(this);
    if (stringpntr == nullptr)
        throw param_not_string_param();
    return stringpntr->set(in).c_str();
}

const char *InxParameter::set_optiongroup(const char *in)
{
    ParamOptionGroup *param = dynamic_cast<ParamOptionGroup *>(this);
    if (!param) {
        throw param_not_optiongroup_param();
    }
    return param->set(in).c_str();
}

unsigned int InxParameter::set_color(unsigned int in)
{
    ParamColor*param = dynamic_cast<ParamColor *>(this);
    if (param == nullptr)
        throw param_not_color_param();
    return param->set(in);
}


InxParameter::InxParameter(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *ext)
    : InxWidget(in_repr, ext)
{
    // name (mandatory for all parameters)
    const char *name = in_repr->attribute("name");
    if (name) {
        _name = g_strstrip(g_strdup(name));
    }
    if (!_name || !strcmp(_name, "")) {
        g_warning("Parameter without name in extension '%s'.", _extension->get_id());
        throw param_no_name();
    }

    // gui-text
    const char *gui_text = in_repr->attribute("gui-text");
    if (!gui_text) {
        gui_text = in_repr->attribute("_gui-text"); // backwards-compatibility with underscored variants
    }
    if (gui_text) {
        if (_translatable != NO) { // translate unless explicitly marked untranslatable
            gui_text = get_translation(gui_text);
        }
        _text = g_strdup(gui_text);
    }
    if (!_text && !_hidden) {
        g_warning("Parameter '%s' in extension '%s' is visible but does not have a 'gui-text'.",
                  _name, _extension->get_id());
        throw param_no_text();
    }

    // gui-description (optional)
    const char *gui_description = in_repr->attribute("gui-description");
    if (!gui_description) {
        gui_description = in_repr->attribute("_gui-description"); // backwards-compatibility with underscored variants
    }
    if (gui_description) {
        if (_translatable != NO) { // translate unless explicitly marked untranslatable
            gui_description = get_translation(gui_description);
        }
        _description = g_strdup(gui_description);
    }
}

InxParameter::~InxParameter()
{
    g_free(_name);
    _name = nullptr;

    g_free(_text);
    _text = nullptr;

    g_free(_description);
    _description = nullptr;
}

Glib::ustring InxParameter::pref_name() const
{
    return Glib::ustring::compose("/extensions/%1.%2", _extension->get_id(), _name);
}

std::string InxParameter::value_to_string() const
{
    // if we end up here we're missing a definition of ::string() in one of the subclasses
    g_critical("InxParameter::value_to_string called from parameter '%s' in extension '%s'", _name, _extension->get_id());
    g_assert_not_reached();
    return "";
}

}  // namespace Extension
}  // namespace Inkscape

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
