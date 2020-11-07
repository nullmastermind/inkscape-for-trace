// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path parameter for extensions
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-path.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/join.hpp>

#include <glibmm/i18n.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/regex.h>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechoosernative.h>

#include "xml/node.h"
#include "extension/extension.h"
#include "preferences.h"

namespace Inkscape {
namespace Extension {

ParamPath::ParamPath(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // get value
    const char *value = nullptr;
    if (xml->firstChild()) {
        value = xml->firstChild()->content();
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getString(pref_name()).raw();

    if (_value.empty() && value) {
        _value = value;
    }

    // parse selection mode
    const char *mode = xml->attribute("mode");
    if (mode) {
        if (!strcmp(mode, "file")) {
            // this is the default
        } else if (!strcmp(mode, "files")) {
            _select_multiple = true;
        } else if (!strcmp(mode, "folder")) {
            _mode = FOLDER;
        } else if (!strcmp(mode, "folders")) {
            _mode = FOLDER;
            _select_multiple = true;
        } else if (!strcmp(mode, "file_new")) {
            _mode = FILE_NEW;
        } else if (!strcmp(mode, "folder_new")) {
            _mode = FOLDER_NEW;
        } else {
            g_warning("Invalid value ('%s') for mode of parameter '%s' in extension '%s'",
                      mode, _name, _extension->get_id());
        }
    }

    // parse filetypes
    const char *filetypes = xml->attribute("filetypes");
    if (filetypes) {
        _filetypes = Glib::Regex::split_simple("," , filetypes);
    }
}

/**
 * A function to set the \c _value.
 *
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place \c pref_name() is used.
 *
 * @param  in   The value to set to.
 */
const std::string& ParamPath::set(const std::string &in)
{
    _value = in;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString(pref_name(), _value);

    return _value;
}

std::string ParamPath::value_to_string() const
{
    if (!Glib::path_is_absolute(_value)) {
        return Glib::build_filename(_extension->get_base_directory(), _value);
    } else {
        return _value;
    }
}


/** A special type of Gtk::Entry to handle path parameters. */
class ParamPathEntry : public Gtk::Entry {
private:
    ParamPath *_pref;
    sigc::signal<void> *_changeSignal;
public:
    /**
     * Build a string preference for the given parameter.
     * @param  pref  Where to get the string from, and where to put it
     *                when it changes.
     */
    ParamPathEntry(ParamPath *pref, sigc::signal<void> *changeSignal)
        : Gtk::Entry()
        , _pref(pref)
        , _changeSignal(changeSignal)
    {
        this->set_text(_pref->get());
        this->signal_changed().connect(sigc::mem_fun(this, &ParamPathEntry::changed_text));
    };
    void changed_text();
};


/**
 * Respond to the text box changing.
 *
 * This function responds to the box changing by grabbing the value
 * from the text box and putting it in the parameter.
 */
void ParamPathEntry::changed_text()
{
    auto data = this->get_text();
    _pref->set(data.c_str());
    if (_changeSignal != nullptr) {
        _changeSignal->emit();
    }
}

/**
 * Creates a text box for the string parameter.
 *
 * Builds a hbox with a label and a text box in it.
 */
Gtk::Widget *ParamPath::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, GUI_PARAM_WIDGETS_SPACING));
    Gtk::Label *label = Gtk::manage(new Gtk::Label(_text, Gtk::ALIGN_START));
    label->show();
    hbox->pack_start(*label, false, false);

    ParamPathEntry *textbox = Gtk::manage(new ParamPathEntry(this, changeSignal));
    textbox->show();
    hbox->pack_start(*textbox, true, true);
    _entry = textbox;

    Gtk::Button *button = Gtk::manage(new Gtk::Button("…"));
	button->show();
    hbox->pack_end(*button, false, false);
    button->signal_clicked().connect(sigc::mem_fun(*this, &ParamPath::on_button_clicked));

    hbox->show();

    return dynamic_cast<Gtk::Widget *>(hbox);
}

/**
 * Create and show the file chooser dialog when the "…" button is clicked
 * Then set the value of the ParamPathEntry holding the current value accordingly
 */
void ParamPath::on_button_clicked()
{
    // set-up action and dialog title according to 'mode'
    Gtk::FileChooserAction action;
    std::string dialog_title;
    if (_mode == FILE) {
        // pick the "save" variants here - otherwise the dialog will only accept existing files
        action = Gtk::FILE_CHOOSER_ACTION_OPEN;
        if (_select_multiple) {
            dialog_title = _("Select existing files");
        } else {
            dialog_title = _("Select existing file");
        }
    } else if (_mode == FOLDER) {
        // pick the "create" variant here - otherwise the dialog will only accept existing folders
        action = Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER;
        if (_select_multiple) {
            dialog_title = _("Select existing folders");
        } else {
            dialog_title = _("Select existing folder");
        }
    } else if (_mode == FILE_NEW) {
        action = Gtk::FILE_CHOOSER_ACTION_SAVE;
        dialog_title = _("Choose file name");
    } else if (_mode == FOLDER_NEW) {
        action = Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER;
        dialog_title = _("Choose folder name");
    } else {
        g_assert_not_reached();
    }

    // create file chooser dialog
    auto file_chooser = Gtk::FileChooserNative::create(dialog_title + "…", action, _("Select"));
    file_chooser->set_select_multiple(_select_multiple);
    file_chooser->set_do_overwrite_confirmation(true);
    file_chooser->set_create_folders(true);

    // set FileFilter according to 'filetype' attribute
    if (!_filetypes.empty() && _mode != FOLDER && _mode != FOLDER_NEW) {
        Glib::RefPtr<Gtk::FileFilter> file_filter = Gtk::FileFilter::create();

        for (auto filetype : _filetypes) {
            file_filter->add_pattern(Glib::ustring::compose("*.%1", filetype));
        }

        std::string filter_name = boost::algorithm::join(_filetypes, "+");
        boost::algorithm::to_upper(filter_name);
        file_filter->set_name(filter_name);

        file_chooser->add_filter(file_filter);
    }

    // set current file/folder suitable for current value
    // (use basepath of first filename; relative paths are considered relative to .inx file's location)
    if (!_value.empty()) {
        std::string first_filename = _value.substr(0, _value.find("|"));

        if (!Glib::path_is_absolute(first_filename)) {
            first_filename = Glib::build_filename(_extension->get_base_directory(), first_filename);
        }

        std::string dirname = Glib::path_get_dirname(first_filename);
        if (Glib::file_test(dirname, Glib::FILE_TEST_IS_DIR)) {
            file_chooser->set_current_folder(dirname);
        }

        if(_mode == FILE_NEW || _mode == FOLDER_NEW) {
            file_chooser->set_current_name(Glib::path_get_basename(first_filename));
        } else {
            if (Glib::file_test(first_filename, Glib::FILE_TEST_EXISTS)) {
                // TODO: This does not seem to work (at least on Windows)
                // file_chooser->set_filename(first_filename);
            }
        }
    }

    // show dialog and parse result
    int res = file_chooser->run();
    if (res == Gtk::ResponseType::RESPONSE_ACCEPT) {
        std::vector<std::string> filenames = file_chooser->get_filenames();
        std::string filenames_joined = boost::algorithm::join(filenames, "|");
        _entry->set_text(filenames_joined); // let the ParamPathEntry handle the rest (including setting the preference)
    }
}

}  /* namespace Extension */
}  /* namespace Inkscape */
