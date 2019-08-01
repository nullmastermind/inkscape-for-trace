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

#include "xml/node.h"
#include "extension/extension.h"
#include "preferences.h"

namespace Inkscape {
namespace Extension {

ParamString::ParamString(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : Parameter(xml, ext)
{
    // get value
    const char *value = nullptr;
    if (xml->firstChild()) {
        value = xml->firstChild()->content();
    }

    gchar *pref_name = this->pref_name();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getString(extension_pref_root + pref_name);
    g_free(pref_name);

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
}

/**
 * A function to set the \c _value.
 *
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place, \c PREF_DIR
 * and \c pref_name() are used.
 *
 * To copy the data into _value the old memory must be free'd first.
 * It is important to note that \c g_free handles \c NULL just fine.  Then
 * the passed in value is duplicated using \c g_strdup().
 *
 * @param  in   The value to set to.
 * @param  doc  A document that should be used to set the value.
 * @param  node The node where the value may be placed.
 */
const Glib::ustring& ParamString::set(const Glib::ustring in, SPDocument * /*doc*/, Inkscape::XML::Node * /*node*/)
{
    _value = in;

    gchar *pref_name = this->pref_name();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString(extension_pref_root + pref_name, _value);
    g_free(pref_name);

    return _value;
}

void ParamString::string(std::string &string) const
{
    string += _value;
}


/** A special type of Gtk::Entry to handle string parameteres. */
class ParamStringEntry : public Gtk::Entry {
private:
    ParamString *_pref;
    SPDocument *_doc;
    Inkscape::XML::Node *_node;
    sigc::signal<void> *_changeSignal;
public:
    /**
     * Build a string preference for the given parameter.
     * @param  pref  Where to get the string from, and where to put it
     *                when it changes.
     */
    ParamStringEntry(ParamString *pref, SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
        : Gtk::Entry()
        , _pref(pref)
        , _doc(doc)
        , _node(node)
        , _changeSignal(changeSignal)
    {
        this->set_text(_pref->get(nullptr, nullptr));
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
    _pref->set(data.c_str(), _doc, _node);
    if (_changeSignal != nullptr) {
        _changeSignal->emit();
    }
}

/**
 * Creates a text box for the string parameter.
 *
 * Builds a hbox with a label and a text box in it.
 */
Gtk::Widget *ParamString::get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox(false, Parameter::GUI_PARAM_WIDGETS_SPACING));
    Gtk::Label *label = Gtk::manage(new Gtk::Label(_text, Gtk::ALIGN_START));
    label->show();
    hbox->pack_start(*label, false, false);

    ParamStringEntry * textbox = new ParamStringEntry(this, doc, node, changeSignal);
    textbox->show();
    hbox->pack_start(*textbox, true, true);

    hbox->show();

    return dynamic_cast<Gtk::Widget *>(hbox);
}

}  /* namespace Extension */
}  /* namespace Inkscape */
