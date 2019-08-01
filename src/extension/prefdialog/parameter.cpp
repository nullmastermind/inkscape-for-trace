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
#include "parameter-description.h"
#include "parameter-float.h"
#include "parameter-int.h"
#include "parameter-notebook.h"
#include "parameter-optiongroup.h"
#include "parameter-string.h"

#include "extension/extension.h"

#include "object/sp-defs.h"

#include "ui/widget/color-notebook.h"

#include "xml/node.h"


namespace Inkscape {
namespace Extension {

Parameter *Parameter::make(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *in_ext)
{
    Parameter *param = nullptr;

    const char *type = in_repr->attribute("type");
    if (!type) {
        // we can't create a parameter without type
        g_warning("Parameter without type in extension '%s'.", in_ext->get_id());
    } else if(!strcmp(type, "boolean")) {
        param = new ParamBool(in_repr, in_ext);
    } else if (!strcmp(type, "int")) {
        param = new ParamInt(in_repr, in_ext);
    } else if (!strcmp(type, "float")) {
        param = new ParamFloat(in_repr, in_ext);
    } else if (!strcmp(type, "string")) {
        param = new ParamString(in_repr, in_ext);
    } else if (!strcmp(type, "description")) {
        param = new ParamDescription(in_repr, in_ext);
    } else if (!strcmp(type, "notebook")) {
        param = new ParamNotebook(in_repr, in_ext);
    } else if (!strcmp(type, "optiongroup")) {
        param = new ParamOptionGroup(in_repr, in_ext);
    } else if (!strcmp(type, "enum")) { // support deprecated "enum" for backwards-compatibilty
        in_repr->setAttribute("appearance", "combo");
        param = new ParamOptionGroup(in_repr, in_ext);
    } else if (!strcmp(type, "color")) {
        param = new ParamColor(in_repr, in_ext);
    } else {
        g_warning("Unknown parameter type ('%s') in extension '%s'", type, in_ext->get_id());
    }

    // Note: param could equal nullptr
    return param;
}

bool Parameter::get_bool(SPDocument const *doc, Inkscape::XML::Node const *node) const
{
    ParamBool const *boolpntr = dynamic_cast<ParamBool const *>(this);
    if (!boolpntr) {
        throw param_not_bool_param();
    }
    return boolpntr->get(doc, node);
}

int Parameter::get_int(SPDocument const *doc, Inkscape::XML::Node const *node) const
{
    ParamInt const *intpntr = dynamic_cast<ParamInt const *>(this);
    if (!intpntr) {
        throw param_not_int_param();
    }
    return intpntr->get(doc, node);
}

float Parameter::get_float(SPDocument const *doc, Inkscape::XML::Node const *node) const
{
    ParamFloat const *floatpntr = dynamic_cast<ParamFloat const *>(this);
    if (!floatpntr) {
        throw param_not_float_param();
    }
    return floatpntr->get(doc, node);
}

const char *Parameter::get_string(SPDocument const *doc, Inkscape::XML::Node const *node) const
{
    ParamString const *stringpntr = dynamic_cast<ParamString const *>(this);
    if (!stringpntr) {
        throw param_not_string_param();
    }
    return stringpntr->get(doc, node).c_str();
}

const char *Parameter::get_optiongroup(SPDocument const *doc, Inkscape::XML::Node const *node) const
{
    ParamOptionGroup const *param = dynamic_cast<ParamOptionGroup const *>(this);
    if (!param) {
        throw param_not_optiongroup_param();
    }
    return param->get(doc, node).c_str();
}

bool Parameter::get_optiongroup_contains(const char *value, SPDocument const *doc, Inkscape::XML::Node const *node) const
{
    ParamOptionGroup const *param = dynamic_cast<ParamOptionGroup const *>(this);
    if (!param) {
        throw param_not_optiongroup_param();
    }
    return param->contains(value, doc, node);
}

guint32 Parameter::get_color(const SPDocument* doc, Inkscape::XML::Node const *node) const
{
    ParamColor const *param = dynamic_cast<ParamColor const *>(this);
    if (!param) {
        throw param_not_color_param();
    }
    return param->get(doc, node);
}

bool Parameter::set_bool(bool in, SPDocument *doc, Inkscape::XML::Node *node)
{
    ParamBool * boolpntr = dynamic_cast<ParamBool *>(this);
    if (boolpntr == nullptr)
        throw param_not_bool_param();
    return boolpntr->set(in, doc, node);
}

int Parameter::set_int(int in, SPDocument *doc, Inkscape::XML::Node *node)
{
    ParamInt *intpntr = dynamic_cast<ParamInt *>(this);
    if (intpntr == nullptr)
        throw param_not_int_param();
    return intpntr->set(in, doc, node);
}

float Parameter::set_float(float in, SPDocument *doc, Inkscape::XML::Node *node)
{
    ParamFloat * floatpntr;
    floatpntr = dynamic_cast<ParamFloat *>(this);
    if (floatpntr == nullptr)
        throw param_not_float_param();
    return floatpntr->set(in, doc, node);
}

const char *Parameter::set_string(const char *in, SPDocument *doc, Inkscape::XML::Node *node)
{
    ParamString * stringpntr = dynamic_cast<ParamString *>(this);
    if (stringpntr == nullptr)
        throw param_not_string_param();
    return stringpntr->set(in, doc, node).c_str();
}

const char *Parameter::set_optiongroup(const char *in, SPDocument *doc, Inkscape::XML::Node *node)
{
    ParamOptionGroup *param = dynamic_cast<ParamOptionGroup *>(this);
    if (!param) {
        throw param_not_optiongroup_param();
    }
    return param->set(in, doc, node).c_str();
}

guint32 Parameter::set_color(guint32 in, SPDocument *doc, Inkscape::XML::Node *node)
{
    ParamColor*param = dynamic_cast<ParamColor *>(this);
    if (param == nullptr)
        throw param_not_color_param();
    return param->set(in, doc, node);
}


Parameter::Parameter (Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *ext)
    : _extension(ext)
{
    // name (mandatory for all paramters)
    const char *name = in_repr->attribute("name");
    if (!name) {
        throw param_no_name();
    }
    _name = g_strdup(name);


    // translatable (optional, required to translate gui-text and gui-description)
    const char *translatable = in_repr->attribute("translatable");
    if (translatable) {
        if (!strcmp(translatable, "yes")) {
            _translatable = YES;
        } else if (!strcmp(translatable, "no"))  {
            _translatable = NO;
        } else {
            g_warning("Invalid value ('%s') for translatable attribute of parameter '%s' in extension '%s'",
                      translatable, _name, _extension->get_id());
        }
    }

    // context (optional, required to translate gui-text and gui-description)
    const char *context = in_repr->attribute("context");
    if (!context) {
        context = in_repr->attribute("msgctxt"); // backwards-compatibility with previous name
    }
    if (context) {
        _context = g_strdup(context);
    }

    // gui-text (TODO: should likely be mandatory for all parameters; maybe not for hidden ones?)
    const char *gui_text = in_repr->attribute("gui-text");
    if (!gui_text) {
        gui_text = in_repr->attribute("_gui-text"); // backwards-compatibility with underscored variants
    }
    if (gui_text) {
        if (_translatable != NO) { // translate unless explicitly marked untranslatable
            if (_context) {
                gui_text = g_dpgettext2(nullptr, context, gui_text);
            } else {
                gui_text = _(gui_text);
            }
        }
        _text = g_strdup(gui_text);
    }

    // gui-description (optional)
    const char *gui_description = in_repr->attribute("gui-description");
    if (!gui_description) {
        gui_description = in_repr->attribute("_gui-description"); // backwards-compatibility with underscored variants
    }
    if (gui_description) {
        if (_translatable != NO) { // translate unless explicitly marked untranslatable
            if (_context) {
                gui_description = g_dpgettext2(nullptr, context, gui_description);
            } else {
                gui_description = _(gui_description);
            }
        }
        _description = g_strdup(gui_description);
    }

    // gui-hidden (optional)
    const char *gui_hidden = in_repr->attribute("gui-hidden");
    if (gui_hidden != nullptr) {
        if (strcmp(gui_hidden, "true") == 0) {
            _hidden = true;
        }
    }

    // indent (optional)
    const char *indent = in_repr->attribute("indent");
    if (indent != nullptr) {
        _indent = strtol(indent, nullptr, 0);
    }

    // appearance (optional, does not apply to all parameters)
    const char *appearance = in_repr->attribute("appearance");
    if (appearance) {
        _appearance = g_strdup(appearance);
    }
}

Parameter::~Parameter()
{
    g_free(_name);
    _name = nullptr;

    g_free(_text);
    _text = nullptr;

    g_free(_description);
    _description = nullptr;

    g_free(_appearance);
    _description = nullptr;

    g_free(_context);
    _context = nullptr;
}

gchar *Parameter::pref_name() const
{
    return g_strdup_printf("%s.%s", _extension->get_id(), _name);
}

/** Basically, if there is no widget pass a NULL. */
Gtk::Widget *
Parameter::get_widget (SPDocument * /*doc*/, Inkscape::XML::Node * /*node*/, sigc::signal<void> * /*changeSignal*/)
{
    return nullptr;
}

/** If I'm not sure which it is, just don't return a value. */
void Parameter::string(std::string &/*string*/) const
{
    // TODO investigate clearing the target string.
}

void Parameter::string(std::list <std::string> &list) const
{
    std::string value;
    string(value);
    if (!value.empty()) {
        std::string final;
        final += "--";
        final += name();
        final += "=";
        final += value;

        list.insert(list.end(), final);
    }
}

Parameter *Parameter::get_param(const gchar */*name*/)
{
    return nullptr;
}

Glib::ustring const extension_pref_root = "/extensions/";

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
