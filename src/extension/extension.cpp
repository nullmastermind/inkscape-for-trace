// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Inkscape::Extension::Extension:
 * the ability to have features that are more modular so that they
 * can be added and removed easily.  This is the basis for defining
 * those actions.
 */

/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extension.h"
#include "implementation/implementation.h"

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include <glibmm/i18n.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

#include "db.h"
#include "dependency.h"
#include "inkscape.h"
#include "timer.h"

#include "io/resource.h"
#include "io/sys.h"

#include "prefdialog/parameter.h"
#include "prefdialog/widget.h"

#include "xml/repr.h"


namespace Inkscape {
namespace Extension {

/* Inkscape::Extension::Extension */

FILE *Extension::error_file = nullptr;

/**
    \return  none
    \brief   Constructs an Extension from a Inkscape::XML::Node
    \param   in_repr        The repr that should be used to build it
    \param   base_directory Base directory of extensions that were loaded from a file (.inx file's location)

    This function is the basis of building an extension for Inkscape.  It
    currently extracts the fields from the Repr that are used in the
    extension.  The Repr will likely include other children that are
    not related to the module directly.  If the Repr does not include
    a name and an ID the module will be left in an errored state.
*/
Extension::Extension(Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory)
    : _gui(true)
    , execution_env(nullptr)
{
    g_return_if_fail(in_repr); // should be ensured in system.cpp
    repr = in_repr;
    Inkscape::GC::anchor(repr);

    if (in_imp == nullptr) {
        imp = new Implementation::Implementation();
    } else {
        imp = in_imp;
    }

    if (base_directory) {
        _base_directory = *base_directory;
    }

    // get name of the translation catalog ("gettext textdomain") that the extension wants to use for translations
    // and lookup the locale directory for it
    const char *translationdomain = repr->attribute("translationdomain");
    if (translationdomain) {
        _translationdomain = translationdomain;
    } else {
        _translationdomain = "inkscape"; // default to the Inkscape catalog
    }
    if (!strcmp(_translationdomain, "none")) {
        // special keyword "none" means the extension author does not want translation of extension strings
        _translation_enabled = false;
        _translationdomain = nullptr;
    } else if (!strcmp(_translationdomain, "inkscape")) {
        // this is our default domain; we know the location already (also respects INKSCAPE_LOCALEDIR)
        _gettext_catalog_dir = bindtextdomain("inkscape", nullptr);
    } else {
        lookup_translation_catalog();
    }

    // Read XML tree and parse extension
    Inkscape::XML::Node *child_repr = repr->firstChild();
    while (child_repr) {
        const char *chname = child_repr->name();
        if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
            chname += strlen(INKSCAPE_EXTENSION_NS);
        }
        if (chname[0] == '_') { // allow leading underscore in tag names for backwards-compatibility
            chname++;
        }

        if (!strcmp(chname, "id")) {
            const char *id = child_repr->firstChild() ? child_repr->firstChild()->content() : nullptr;
            if (id) {
                _id = g_strdup(id);
            } else {
                throw extension_no_id();
            }
        } else if (!strcmp(chname, "name")) {
            const char *name = child_repr->firstChild() ? child_repr->firstChild()->content() : nullptr;
            if (name) {
                _name = g_strdup(name);
            } else {
                throw extension_no_name();
            }
        } else if (InxWidget::is_valid_widget_name(chname)) {
            InxWidget *widget = InxWidget::make(child_repr, this);
            if (widget) {
                _widgets.push_back(widget);
            }
        } else if (!strcmp(chname, "dependency")) {
            _deps.push_back(new Dependency(child_repr, this));
        } else if (!strcmp(chname, "script")) { // TODO: should these be parsed in their respective Implementation?
            for (Inkscape::XML::Node *child = child_repr->firstChild(); child != nullptr; child = child->next()) {
                if (child->type() == Inkscape::XML::ELEMENT_NODE) { // skip non-element nodes (see LP #1372200)
                    const char *interpreted = child->attribute("interpreter");
                    Dependency::type_t type = interpreted ? Dependency::TYPE_FILE : Dependency::TYPE_EXECUTABLE;
                    _deps.push_back(new Dependency(child, this, type));
                    break;
                }
            }
        } else if (!strcmp(chname, "xslt")) { // TODO: should these be parsed in their respective Implementation?
            for (Inkscape::XML::Node *child = child_repr->firstChild(); child != nullptr; child = child->next()) {
                if (child->type() == Inkscape::XML::ELEMENT_NODE) { // skip non-element nodes (see LP #1372200)
                    _deps.push_back(new Dependency(child, this, Dependency::TYPE_FILE));
                    break;
                }
            }
        } else {
            // We could do some sanity checking here.
            // However, we don't really know which additional elements Extension subclasses might need...
        }

        child_repr = child_repr->next();
    }

    // all extensions need an ID and a name
    if (!_id) {
        throw extension_no_id();
    }
    if (!_name) {
        throw extension_no_name();
    }

    // filter out extensions that are not compatible with the current platform
#ifndef _WIN32
    if (strstr(_id, "win32")) {
        throw extension_not_compatible();
    }
#endif

    // finally register the extension if all checks passed
    db.register_ext (this);
}

/**
    \return   none
    \brief    Destroys the Extension

    This function frees all of the strings that could be attached
    to the extension and also unreferences the repr.  This is better
    than freeing it because it may (I wouldn't know why) be referenced
    in another place.
*/
Extension::~Extension ()
{
    set_state(STATE_UNLOADED);

    db.unregister_ext(this);

    Inkscape::GC::release(repr);

    g_free(_id);
    g_free(_name);

    delete timer;
    timer = nullptr;

    for (auto widget : _widgets) {
        delete widget;
    }

    for (auto & _dep : _deps) {
        delete _dep;
    }
    _deps.clear();
}

/**
    \return   none
    \brief    A function to set whether the extension should be loaded
              or unloaded
    \param    in_state  Which state should the extension be in?

    It checks to see if this is a state change or not.  If we're changing
    states it will call the appropriate function in the implementation,
    load or unload.  Currently, there is no error checking in this
    function.  There should be.
*/
void
Extension::set_state (state_t in_state)
{
    if (_state == STATE_DEACTIVATED) return;
    if (in_state != _state) {
        /** \todo Need some more error checking here! */
        switch (in_state) {
            case STATE_LOADED:
                if (imp->load(this))
                    _state = STATE_LOADED;

                if (timer != nullptr) {
                    delete timer;
                }
                timer = new ExpirationTimer(this);

                break;
            case STATE_UNLOADED:
                // std::cout << "Unloading: " << name << std::endl;
                imp->unload(this);
                _state = STATE_UNLOADED;

                if (timer != nullptr) {
                    delete timer;
                    timer = nullptr;
                }
                break;
            case STATE_DEACTIVATED:
                _state = STATE_DEACTIVATED;

                if (timer != nullptr) {
                    delete timer;
                    timer = nullptr;
                }
                break;
            default:
                break;
        }
    }

    return;
}

/**
    \return   The state the extension is in
    \brief    A getter for the state variable.
*/
Extension::state_t
Extension::get_state ()
{
    return _state;
}

/**
    \return  Whether the extension is loaded or not
    \brief   A quick function to test the state of the extension
*/
bool
Extension::loaded ()
{
    return get_state() == STATE_LOADED;
}

/**
    \return  A boolean saying whether the extension passed the checks
    \brief   A function to check the validity of the extension

    This function chekcs to make sure that there is an id, a name, a
    repr and an implementation for this extension.  Then it checks all
    of the dependencies to see if they pass.  Finally, it asks the
    implementation to do a check of itself.

    On each check, if there is a failure, it will print a message to the
    error log for that failure.  It is important to note that the function
    keeps executing if it finds an error, to try and get as many of them
    into the error log as possible.  This should help people debug
    installations, and figure out what they need to get for the full
    functionality of Inkscape to be available.
*/
bool
Extension::check ()
{
    const char * inx_failure = _("  This is caused by an improper .inx file for this extension."
                                 "  An improper .inx file could have been caused by a faulty installation of Inkscape.");

    if (repr == nullptr) {
        printFailure(Glib::ustring(_("the XML description of it got lost.")) + inx_failure);
        return false;
    }
    if (imp == nullptr) {
        printFailure(Glib::ustring(_("no implementation was defined for the extension.")) + inx_failure);
        return false;
    }

    bool retval = true;
    for (auto _dep : _deps) {
        if (_dep->check() == false) {
            printFailure(Glib::ustring(_("a dependency was not met.")));
            error_file_write(_dep->info_string());
            retval = false;
        }
    }

    if (retval) {
        return imp->check(this);
    }

    error_file_write("");
    return retval;
}

/** \brief A quick function to print out a standard start of extension
           errors in the log.
    \param reason  A string explaining why this failed

    Real simple, just put everything into \c error_file.
*/
void
Extension::printFailure (Glib::ustring reason)
{
    _error_reason = Glib::ustring::compose(_("Extension \"%1\" failed to load because %2"), _name, reason);
    error_file_write(_error_reason);
}

/**
    \return  The XML tree that is used to define the extension
    \brief   A getter for the internal Repr, does not add a reference.
*/
Inkscape::XML::Node *
Extension::get_repr ()
{
    return repr;
}

/**
    \return  The textual id of this extension
    \brief   Get the ID of this extension - not a copy don't delete!
*/
gchar *
Extension::get_id () const
{
    return _id;
}

/**
    \return  The textual name of this extension
    \brief   Get the name of this extension - not a copy don't delete!
*/
gchar *
Extension::get_name () const
{
    return _name;
}

/**
    \return  None
    \brief   This function diactivates the extension (which makes it
             unusable, but not deleted)

    This function is used to removed an extension from functioning, but
    not delete it completely.  It sets the state to \c STATE_DEACTIVATED to
    mark to the world that it has been deactivated.  It also removes
    the current implementation and replaces it with a standard one.  This
    makes it so that we don't have to continually check if there is an
    implementation, but we are guaranteed to have a benign one.

    \warning It is important to note that there is no 'activate' function.
    Running this function is irreversable.
*/
void
Extension::deactivate ()
{
    set_state(STATE_DEACTIVATED);

    /* Removing the old implementation, and making this use the default. */
    /* This should save some memory */
    delete imp;
    imp = new Implementation::Implementation();

    return;
}

/**
    \return  Whether the extension has been deactivated
    \brief   Find out the status of the extension
*/
bool
Extension::deactivated ()
{
    return get_state() == STATE_DEACTIVATED;
}

/** Gets the location of the dependency file as an absolute path
  *
  * Iterates over all dependencies of this extension and finds the one with matching name,
  * then returns the absolute path to this dependency file as determined previously.
  *
  * TODO: This function should not be necessary, but we parse script dependencies twice:
  *       - Once here in the Extension::Extension() constructor
  *       - A second time in Script::load() in "script.cpp" when determining the script location
  *       Theoretically we could return the wrong path if an extension depends on two files with the same name
  *       in different relative locations. In practice this risk should be close to zero, though.
  *
  * @return Absolute path of the dependency file
  */
std::string Extension::get_dependency_location(const char *name)
{
    for (auto dep : _deps) {
        if (!strcmp(name, dep->get_name())) {
            return dep->get_path();
        }
    }

    return "";
}

/** recursively searches directory for a file named filename; returns true if found */
static bool _find_filename_recursive(std::string directory, std::string filename) {
    Glib::Dir dir(directory);

    std::string name = dir.read_name();
    while (!name.empty()) {
        std::string fullpath = Glib::build_filename(directory, name);
        // g_message("%s", fullpath.c_str());

        if (Glib::file_test(fullpath, Glib::FILE_TEST_IS_DIR)) {
            if (_find_filename_recursive(fullpath, filename)) {
                return true;
            }
        } else if (name == filename) {
            return true;
        }
        name = dir.read_name();
    }

    return false;
}

/** Searches for a gettext catalog matching the extension's translationdomain
  *
  * This function will attempt to find the correct gettext catalog for the translationdomain
  * requested by the extension.
  *
  * For this the following three locations are recursively searched for "${translationdomain}.mo":
  *  - the 'locale' directory in the .inx file's folder
  *  - the 'locale' directory in the "extensions" folder containing the .inx
  *  - the system location for gettext catalogs, i.e. where Inkscape's own catalog is located
  *
  * If one matching file is found, the directory is assumed to be the correct location and registered with gettext
  */
void Extension::lookup_translation_catalog() {
    g_assert(!_base_directory.empty());

    // get locale folder locations
    std::string locale_dir_current_extension;
    std::string locale_dir_extensions;
    std::string locale_dir_system;

    locale_dir_current_extension = Glib::build_filename(_base_directory, "locale");

    size_t index = _base_directory.find_last_of("extensions");
    if (index != std::string::npos) {
        locale_dir_extensions = Glib::build_filename(_base_directory.substr(0, index+1), "locale");
    }

    locale_dir_system = bindtextdomain("inkscape", nullptr);

    // collect unique locations into vector
    std::vector<std::string> locale_dirs;
    if (locale_dir_current_extension != locale_dir_extensions) {
        locale_dirs.push_back(std::move(locale_dir_current_extension));
    }
    locale_dirs.push_back(std::move(locale_dir_extensions));
    locale_dirs.push_back(std::move(locale_dir_system));

    // iterate over locations and look for the one that has the correct catalog
    std::string search_name;
    search_name += _translationdomain;
    search_name += ".mo";
    for (auto locale_dir : locale_dirs) {
        if (!Glib::file_test(locale_dir, Glib::FILE_TEST_IS_DIR)) {
            continue;
        }

        if (_find_filename_recursive(locale_dir, search_name)) {
            _gettext_catalog_dir = locale_dir;
            break;
        }
    }

    // register catalog with gettext if found, disable translation for this extension otherwise
    if (!_gettext_catalog_dir.empty()) {
        const char *current_dir = bindtextdomain(_translationdomain, nullptr);
        if (_gettext_catalog_dir != current_dir) {
            g_info("Binding textdomain '%s' to '%s'.", _translationdomain, _gettext_catalog_dir.c_str());
            bindtextdomain(_translationdomain, _gettext_catalog_dir.c_str());
            bind_textdomain_codeset(_translationdomain, "UTF-8");
        }
    } else {
        g_warning("Failed to locate message catalog for textdomain '%s'.", _translationdomain);
        _translation_enabled = false;
        _translationdomain = nullptr;
    }
}

/** Gets a translation within the context of the current extension
  *
  * Query gettext for the translated version of the input string,
  * handling the preferred translation domain of the extension internally.
  *
  * @param   msgid   String to translate
  * @param   msgctxt Context for the translation
  *
  * @return  Translated string (or original string if extension is not supposed to be translated)
  */
const char *Extension::get_translation(const char *msgid, const char *msgctxt) {
    if (!_translation_enabled) {
        return msgid;
    }

    if (!strcmp(msgid, "")) {
        g_warning("Attempting to translate an empty string in extension '%s', which is not supported.", _id);
        return msgid;
    }

    if (msgctxt) {
        return g_dpgettext2(_translationdomain, msgctxt, msgid);
    } else {
        return g_dgettext(_translationdomain, msgid);
    }
}

/** Sets environment suitable for executing this Extension
  *
  * Currently sets the environment variables INKEX_GETTEXT_DOMAIN and INKEX_GETTEXT_DIRECTORY
  * to make the "translationdomain" accessible to child processes spawned by this extension's Implementation.
  */
void Extension::set_environment() {
    Glib::unsetenv("INKEX_GETTEXT_DOMAIN");
    Glib::unsetenv("INKEX_GETTEXT_DIRECTORY");

    if (_translationdomain) {
        Glib::setenv("INKEX_GETTEXT_DOMAIN", std::string(_translationdomain));
    }
    if (!_gettext_catalog_dir.empty()) {
        Glib::setenv("INKEX_GETTEXT_DIRECTORY", _gettext_catalog_dir);
    }
}

/**
    \brief  A function to get the parameters in a string form
    \return An array with all the parameters in it.

*/
void
Extension::paramListString (std::list <std::string> &retlist)
{
    // first collect all widgets in the current extension
    std::vector<InxWidget *> widget_list;
    for (auto widget : _widgets) {
        widget->get_widgets(widget_list);
    }

    // then build a list of parameter strings from parameter names and values, as '--name=value'
    for (auto widget : widget_list) {
        InxParameter *parameter = dynamic_cast<InxParameter *>(widget); // filter InxParameters from InxWidgets
        if (parameter) {
            const char *name = parameter->name();
            std::string value = parameter->value_to_string();

            if (name && !value.empty()) { // TODO: Shouldn't empty string values be allowed?
                std::string parameter_string;
                parameter_string += "--";
                parameter_string += name;
                parameter_string += "=";
                parameter_string += value;
                retlist.push_back(parameter_string);
            }
        }
    }

    return;
}

InxParameter *Extension::get_param(const gchar *name)
{
    if (!name || _widgets.empty()) {
        throw Extension::param_not_exist();
    }

    // first collect all widgets in the current extension
    std::vector<InxWidget *> widget_list;
    for (auto widget : _widgets) {
        widget->get_widgets(widget_list);
    }

    // then search for a parameter with a matching name
    for (auto widget : widget_list) {
        InxParameter *parameter = dynamic_cast<InxParameter *>(widget); // filter InxParameters from InxWidgets
        if (parameter && !strcmp(parameter->name(), name)) {
            return parameter;
        }
    }

    // if execution reaches here, no parameter matching 'name' was found
    throw Extension::param_not_exist();
}

InxParameter const *Extension::get_param(const gchar *name) const
{
    return const_cast<Extension *>(this)->get_param(name);
}


/**
    \return   The value of the parameter identified by the name
    \brief    Gets a parameter identified by name with the bool placed in value.
    \param    name   The name of the parameter to get

    Look up in the parameters list, const then execute the function on that found parameter.
*/
bool
Extension::get_param_bool(const gchar *name) const
{
    const InxParameter *param;
    param = get_param(name);
    return param->get_bool();
}

/**
    \return   The integer value for the parameter specified
    \brief    Gets a parameter identified by name with the integer placed in value.
    \param    name   The name of the parameter to get

    Look up in the parameters list, const then execute the function on that found parameter.
*/
int
Extension::get_param_int(const gchar *name) const
{
    const InxParameter *param;
    param = get_param(name);
    return param->get_int();
}

/**
    \return   The float value for the parameter specified
    \brief    Gets a parameter identified by name with the float in value.
    \param    name   The name of the parameter to get

    Look up in the parameters list, const then execute the function on that found parameter.
*/
float
Extension::get_param_float(const gchar *name) const
{
    const InxParameter *param;
    param = get_param(name);
    return param->get_float();
}

/**
    \return   The string value for the parameter specified
    \brief    Gets a parameter identified by name with the string placed in value.
    \param    name   The name of the parameter to get

    Look up in the parameters list, const then execute the function on that found parameter.
*/
const char *
Extension::get_param_string(const gchar *name) const
{
    const InxParameter *param;
    param = get_param(name);
    return param->get_string();
}

/**
    \return   The string value for the parameter specified
    \brief    Gets a parameter identified by name with the string placed in value.
    \param    name   The name of the parameter to get

    Look up in the parameters list, const then execute the function on that found parameter.
*/
const char *
Extension::get_param_optiongroup(const gchar *name) const
{
    const InxParameter *param;
    param = get_param(name);
    return param->get_optiongroup();
}

/**
 * This is useful to find out, if a given string \c value is selectable in a optiongroup named \cname.
 *
 * @param  name The name of the optiongroup parameter to get.
 * @return true if value exists, false if not
 */
bool
Extension::get_param_optiongroup_contains(const gchar *name, const char *value) const
{
    const InxParameter *param;
    param = get_param(name);
    return param->get_optiongroup_contains(value);
}

/**
    \return   The unsigned integer RGBA value for the parameter specified
    \brief    Gets a parameter identified by name with the unsigned int placed in value.
    \param    name   The name of the parameter to get

    Look up in the parameters list, const then execute the function on that found parameter.
*/
guint32
Extension::get_param_color(const gchar *name) const
{
    const InxParameter *param;
    param = get_param(name);
    return param->get_color();
}

/**
    \return   The passed in value
    \brief    Sets a parameter identified by name with the boolean in the parameter value.
    \param    name   The name of the parameter to set
    \param    value  The value to set the parameter to

    Look up in the parameters list, const then execute the function on that found parameter.
*/
bool
Extension::set_param_bool(const gchar *name, const bool value)
{
    InxParameter *param;
    param = get_param(name);
    return param->set_bool(value);
}

/**
    \return   The passed in value
    \brief    Sets a parameter identified by name with the integer in the parameter value.
    \param    name   The name of the parameter to set
    \param    value  The value to set the parameter to

    Look up in the parameters list, const then execute the function on that found parameter.
*/
int
Extension::set_param_int(const gchar *name, const int value)
{
    InxParameter *param;
    param = get_param(name);
    return param->set_int(value);
}

/**
    \return   The passed in value
    \brief    Sets a parameter identified by name with the float in the parameter value.
    \param    name   The name of the parameter to set
    \param    value  The value to set the parameter to

    Look up in the parameters list, const then execute the function on that found parameter.
*/
float
Extension::set_param_float(const gchar *name, const float value)
{
    InxParameter *param;
    param = get_param(name);
    return param->set_float(value);
}

/**
    \return   The passed in value
    \brief    Sets a parameter identified by name with the string in the parameter value.
    \param    name   The name of the parameter to set
    \param    value  The value to set the parameter to

    Look up in the parameters list, const then execute the function on that found parameter.
*/
const char *
Extension::set_param_string(const gchar *name, const char *value)
{
    InxParameter *param;
    param = get_param(name);
    return param->set_string(value);
}

/**
    \return   The passed in value
    \brief    Sets a parameter identified by name with the string in the parameter value.
    \param    name   The name of the parameter to set
    \param    value  The value to set the parameter to

    Look up in the parameters list, const then execute the function on that found parameter.
*/
const char *
Extension::set_param_optiongroup(const gchar *name, const char *value)
{
    InxParameter *param;
    param = get_param(name);
    return param->set_optiongroup(value);
}

/**
    \return   The passed in value
    \brief    Sets a parameter identified by name with the unsigned integer RGBA value in the parameter value.
    \param    name   The name of the parameter to set
    \param    value  The value to set the parameter to

Look up in the parameters list, const then execute the function on that found parameter.
*/
guint32
Extension::set_param_color(const gchar *name, const guint32 color)
{
    InxParameter *param;
    param = get_param(name);
    return param->set_color(color);
}


/** \brief A function to open the error log file. */
void
Extension::error_file_open ()
{
    gchar *ext_error_file = Inkscape::IO::Resource::log_path(EXTENSION_ERROR_LOG_FILENAME);
    error_file = Inkscape::IO::fopen_utf8name(ext_error_file, "w+");
    if (!error_file) {
        g_warning(_("Could not create extension error log file '%s'"), ext_error_file);
    }
    g_free(ext_error_file);
};

/** \brief A function to close the error log file. */
void
Extension::error_file_close ()
{
    if (error_file) {
        fclose(error_file);
    }
};

/** \brief A function to write to the error log file. */
void
Extension::error_file_write (Glib::ustring text)
{
    if (error_file) {
        g_fprintf(error_file, "%s\n", text.c_str());
    }
};

/** \brief  A widget to represent the inside of an AutoGUI widget */
class AutoGUI : public Gtk::VBox {
public:
    /** \brief  Create an AutoGUI object */
    AutoGUI () : Gtk::VBox() {};

    /**
     * Adds a widget with a tool tip into the autogui.
     *
     * If there is no widget, nothing happens.  Otherwise it is just
     * added into the VBox.  If there is a tooltip (non-NULL) then it
     * is placed on the widget.
     *
     * @param widg Widget to add.
     * @param tooltip Tooltip for the widget.
     */
    void addWidget(Gtk::Widget *widg, gchar const *tooltip, int indent) {
        if (widg) {
            widg->set_margin_start(indent * InxParameter::GUI_INDENTATION);
            this->pack_start(*widg, false, true, 0); // fill=true does not have an effect here, but allows the
                                                     // child to choose to expand by setting hexpand/vexpand
            if (tooltip) {
                widg->set_tooltip_text(tooltip);
            } else {
                widg->set_tooltip_text("");
                widg->set_has_tooltip(false);
            }
        }
    };
};

/** \brief  A function to automatically generate a GUI from the extensions' widgets
    \return Generated widget

    This function just goes through each widget, and calls it's 'get_widget'.
    Then, each of those is placed into a Gtk::VBox, which is then returned to the calling function.

    If there are no visible parameters, this function just returns NULL.
*/
Gtk::Widget *
Extension::autogui (SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
{
    if (!_gui || widget_visible_count() == 0) {
        return nullptr;
    }

    AutoGUI * agui = Gtk::manage(new AutoGUI());
    agui->set_border_width(InxParameter::GUI_BOX_MARGIN);
    agui->set_spacing(InxParameter::GUI_BOX_SPACING);

    // go through the list of widgets and add the all non-hidden ones
    for (auto widget : _widgets) {
        if (widget->get_hidden()) {
            continue;
        }

        Gtk::Widget *widg = widget->get_widget(changeSignal);
        gchar const *tip = widget->get_tooltip();
        int indent = widget->get_indent();

        agui->addWidget(widg, tip, indent);
    }

    agui->show();
    return agui;
};

/* Extension editor dialog stuff */

Gtk::VBox *
Extension::get_info_widget()
{
    Gtk::VBox * retval = Gtk::manage(new Gtk::VBox());
    retval->set_border_width(4);

    Gtk::Frame * info = Gtk::manage(new Gtk::Frame("General Extension Information"));
    retval->pack_start(*info, true, true, 4);

    auto table = Gtk::manage(new Gtk::Grid());
    table->set_border_width(4);
    table->set_column_spacing(4);

    info->add(*table);

    int row = 0;
    add_val(_("Name:"), get_translation(_name), table, &row);
    add_val(_("ID:"), _id, table, &row);
    add_val(_("State:"), _state == STATE_LOADED ? _("Loaded") : _state == STATE_UNLOADED ? _("Unloaded") : _("Deactivated"), table, &row);

    retval->show_all();
    return retval;
}

void Extension::add_val(Glib::ustring labelstr, Glib::ustring valuestr, Gtk::Grid * table, int * row)
{
    Gtk::Label * label;
    Gtk::Label * value;

    (*row)++;
    label = Gtk::manage(new Gtk::Label(labelstr, Gtk::ALIGN_START));
    value = Gtk::manage(new Gtk::Label(valuestr, Gtk::ALIGN_START));

    table->attach(*label, 0, (*row) - 1, 1, 1);
    table->attach(*value, 1, (*row) - 1, 1, 1);

    label->show();
    value->show();

    return;
}

Gtk::VBox *
Extension::get_params_widget()
{
    Gtk::VBox * retval = Gtk::manage(new Gtk::VBox());
    Gtk::Widget * content = Gtk::manage(new Gtk::Label("Params"));
    retval->pack_start(*content, true, true, 4);
    content->show();
    retval->show();
    return retval;
}

unsigned int Extension::widget_visible_count ( )
{
    unsigned int _visible_count = 0;
    for (auto widget : _widgets) {
        if (!widget->get_hidden()) {
            _visible_count++;
        }
    }
    return _visible_count;
}

}  /* namespace Extension */
}  /* namespace Inkscape */


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
