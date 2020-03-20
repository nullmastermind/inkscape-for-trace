// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-string.h"

#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <glibmm/regex.h>

#include "xml/node.h"
#include "extension/extension.h"
#include "preferences.h"

namespace Inkscape {
namespace Extension {

ParamString::ParamString(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // get value
    const char *value = nullptr;
    if (xml->firstChild()) {
        value = xml->firstChild()->content();
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getString(pref_name());

    if (_value.empty() && value) {
        _value = value;
    }

    // translate value
    if (!_value.empty()) {
        if (_translatable == YES) { // translate only if explicitly marked translatable
            _value = get_translation(_value.c_str());
        }
    }

    // max-length
    const char *max_length = xml->attribute("max-length");
    if (!max_length) {
        max_length = xml->attribute("max_length"); // backwards-compatibility with old name (underscore)
    }
    if (max_length) {
        _max_length = strtoul(max_length, nullptr, 0);
    }

    // parse appearance
    if (_appearance) {
        if (!strcmp(_appearance, "multiline")) {
            _mode = MULTILINE;
        } else {
            g_warning("Invalid value ('%s') for appearance of parameter '%s' in extension '%s'",
                      _appearance, _name, _extension->get_id());
        }
    }
}

/**
 * A function to set the \c _value.
 *
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place \c pref_name() is used.
 *
 * To copy the data into _value the old memory must be free'd first.
 * It is important to note that \c g_free handles \c NULL just fine.  Then
 * the passed in value is duplicated using \c g_strdup().
 *
 * @param  in   The value to set to.
 */
const Glib::ustring& ParamString::set(const Glib::ustring in)
{
    _value = in;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString(pref_name(), _value);

    return _value;
}

std::string ParamString::value_to_string() const
{
    return _value.raw();
}



/** A special type of Gtk::Entry to handle string parameters. */
class ParamStringEntry : public Gtk::Entry {
private:
    ParamString *_pref;
    sigc::signal<void> *_changeSignal;
public:
    /**
     * Build a string preference for the given parameter.
     * @param  pref  Where to get the string from, and where to put it
     *                when it changes.
     */
    ParamStringEntry(ParamString *pref, sigc::signal<void> *changeSignal)
        : Gtk::Entry()
        , _pref(pref)
        , _changeSignal(changeSignal)
    {
        this->set_text(_pref->get());
        this->set_max_length(_pref->getMaxLength()); //Set the max length - default zero means no maximum
        this->signal_changed().connect(sigc::mem_fun(this, &ParamStringEntry::changed_text));
    };
    void changed_text();
};


/**
 * Respond to the text box changing.
 *
 * This function responds to the box changing by grabbing the value
 * from the text box and putting it in the parameter.
 */
void ParamStringEntry::changed_text()
{
    Glib::ustring data = this->get_text();
    _pref->set(data.c_str());
    if (_changeSignal != nullptr) {
        _changeSignal->emit();
    }
}



/** A special type of Gtk::TextView to handle multiline string parameters. */
class ParamMultilineStringEntry : public Gtk::TextView {
private:
    ParamString *_pref;
    sigc::signal<void> *_changeSignal;
public:
    /**
     * Build a string preference for the given parameter.
     * @param  pref  Where to get the string from, and where to put it
     *                when it changes.
     */
    ParamMultilineStringEntry(ParamString *pref, sigc::signal<void> *changeSignal)
        : Gtk::TextView()
        , _pref(pref)
        , _changeSignal(changeSignal)
    {
        // replace literal '\n' with actual newlines for multiline strings
        Glib::ustring value = Glib::Regex::create("\\\\n")->replace_literal(_pref->get(), 0, "\n", (Glib::RegexMatchFlags)0);

        this->get_buffer()->set_text(value);
        this->get_buffer()->signal_changed().connect(sigc::mem_fun(this, &ParamMultilineStringEntry::changed_text));
    };
    void changed_text();
};

/**
 * Respond to the text box changing.
 *
 * This function responds to the box changing by grabbing the value
 * from the text box and putting it in the parameter.
 */
void ParamMultilineStringEntry::changed_text()
{
    Glib::ustring data = this->get_buffer()->get_text();

    // always store newlines as literal '\n'
    data = Glib::Regex::create("\n")->replace_literal(data, 0, "\\n", (Glib::RegexMatchFlags)0);

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
Gtk::Widget *ParamString::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::Box *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, GUI_PARAM_WIDGETS_SPACING));

    Gtk::Label *label = Gtk::manage(new Gtk::Label(_text, Gtk::ALIGN_START));
    label->show();
    box->pack_start(*label, false, false);

    if (_mode == MULTILINE) {
        box->set_orientation(Gtk::ORIENTATION_VERTICAL);

        Gtk::ScrolledWindow *textarea = Gtk::manage(new Gtk::ScrolledWindow());
        textarea->set_vexpand();
        textarea->set_shadow_type(Gtk::SHADOW_IN);

        ParamMultilineStringEntry *entry = Gtk::manage(new ParamMultilineStringEntry(this, changeSignal));
        entry->show();

        textarea->add(*entry);
        textarea->show();

        box->pack_start(*textarea, true, true);
    } else {
        Gtk::Widget *entry = Gtk::manage(new ParamStringEntry(this, changeSignal));
        entry->show();

        box->pack_start(*entry, true, true);
    }

    box->show();

    return dynamic_cast<Gtk::Widget *>(box);
}

}  /* namespace Extension */
}  /* namespace Inkscape */
