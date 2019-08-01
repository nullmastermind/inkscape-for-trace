// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *extension parameter for options with multiple predefined value choices
 *
 * Currently implemented as either Gtk::RadioButton or Gtk::ComboBoxText
 */

/*
 * Author:
 *   Johan Engelen <johan@shouraizou.nl>
 *
 * Copyright (C) 2006-2007 Johan Engelen
 * Copyright (C) 2008 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-optiongroup.h"

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/radiobutton.h>

#include "xml/node.h"
#include "extension/extension.h"
#include "preferences.h"

/**
 * The root directory in the preferences database for extension
 * related parameters.
 */
#define PREF_DIR "extensions"

namespace Inkscape {
namespace Extension {

ParamOptionGroup::ParamOptionGroup(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : Parameter(xml, ext)
{
    // Read valid optiongroup choices from XML tree, i,e.
    //   - <option> elements
    //   - <item> elements (for backwards-compatibility with params of type enum)
    //   - underscored variants of both (for backwards-compatibility)
    if (xml) {
        Inkscape::XML::Node *child_repr = xml->firstChild();
        while (child_repr) {
            const char *chname = child_repr->name();
            if (chname && (!strcmp(chname, INKSCAPE_EXTENSION_NS "option") ||
                           !strcmp(chname, INKSCAPE_EXTENSION_NS "_option") ||
                           !strcmp(chname, INKSCAPE_EXTENSION_NS "item") ||
                           !strcmp(chname, INKSCAPE_EXTENSION_NS "_item")) ) {
                Glib::ustring newtext;
                Glib::ustring newvalue;

                // get content (=label) of option and translate it
                const char *text = nullptr;
                if (child_repr->firstChild()) {
                    text = child_repr->firstChild()->content();
                }
                if (text) {
                    if (_translatable != NO) { // translate unless explicitly marked untranslatable
                        newtext = get_translation(text);
                    } else {
                        newtext = text;
                    }
                } else {
                    g_warning("Missing content in option of parameter '%s' in extension '%s'.",
                              _name, _extension->get_id());
                }

                // get string value of option
                const char *value = child_repr->attribute("value");
                if (value) {
                    newvalue = value;
                } else {
                    g_warning("Missing value for option '%s' of parameter '%s' in extension '%s'.",
                              newtext.c_str(), _name, _extension->get_id());
                }

                choices.push_back(new optionentry(newvalue, newtext));
            } else if (child_repr->type() == XML::ELEMENT_NODE) {
                g_warning("Invalid child element ('%s') for parameter '%s' in extension '%s'. Expected 'option'.",
                          chname, _name, _extension->get_id());
            } else if (child_repr->type() != XML::COMMENT_NODE){
                g_warning("Invalid child element found in parameter '%s' in extension '%s'. Expected 'option'.",
                          _name, _extension->get_id());
            }
            child_repr = child_repr->next();
        }
    }
    if (choices.empty()) {
        g_warning("No (valid) choices for parameter '%s' in extension '%s'", _name, _extension->get_id());
    }

    // get value (initialize with value of first choice if pref is empty)
    gchar *pref_name = this->pref_name();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getString(extension_pref_root + pref_name);
    g_free(pref_name);

    if (_value.empty()) {
        if (!choices.empty()) {
            _value = choices[0]->value;
        }
    }

    // parse appearance
    // (we support "combo" and "radio"; "minimal" is for backwards-compatibility)
    if (_appearance) {
        if (!strcmp(_appearance, "combo") || !strcmp(_appearance, "minimal")) {
            _mode = COMBOBOX;
        } else if (!strcmp(_appearance, "radio")) {
            _mode = RADIOBUTTON;
        } else {
            g_warning("Invalid value ('%s') for appearance of parameter '%s' in extension '%s'",
                      _appearance, _name, _extension->get_id());
        }
    }
}

ParamOptionGroup::~ParamOptionGroup ()
{
    // destroy choice strings
    for (auto choice : choices) {
        delete choice;
    }
}


/**
 * A function to set the \c _value.
 *
 * This function sets ONLY the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place, \c PREF_DIR
 * and \c pref_name() are used.
 *
 * @param  in   The value to set.
 * @param  doc  A document that should be used to set the value.
 * @param  node The node where the value may be placed.
 */
const Glib::ustring& ParamOptionGroup::set(Glib::ustring in, SPDocument *doc, Inkscape::XML::Node *node)
{
    if (contains(in, doc, node)) {
        _value = in;
        gchar *pref_name = this->pref_name();
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(extension_pref_root + pref_name, _value.c_str());
        g_free(pref_name);
    } else {
        g_warning("Could not set value ('%s') for parameter '%s' in extension '%s'. Not a valid choice.",
                  in.c_str(), _name, _extension->get_id());
    }

    return _value;
}

bool ParamOptionGroup::contains(const Glib::ustring text, SPDocument const * /*doc*/, Inkscape::XML::Node const * /*node*/) const
{
    for (auto choice : choices) {
        if (choice->value == text) {
            return true;
        }
    }

    return false;
}

void ParamOptionGroup::string(std::string &string) const
{
    string += _value;
}

/**
 * Returns the value for the options label parameter
 */
Glib::ustring ParamOptionGroup::value_from_label(const Glib::ustring label)
{
    Glib::ustring value;

    for (auto choice : choices) {
        if (choice->text == label) {
            value = choice->value;
            break;
        }
    }

    return value;
}



/** A special RadioButton class to use in ParamOptionGroup. */
class RadioWidget : public Gtk::RadioButton {
private:
    ParamOptionGroup *_pref;
    SPDocument *_doc;
    Inkscape::XML::Node *_node;
    sigc::signal<void> *_changeSignal;
public:
    RadioWidget(Gtk::RadioButtonGroup& group, const Glib::ustring& label,
                ParamOptionGroup *pref, SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
        : Gtk::RadioButton(group, label)
        , _pref(pref)
        , _doc(doc)
        , _node(node)
        , _changeSignal(changeSignal)
    {
        add_changesignal();
    };

    void add_changesignal() {
        this->signal_toggled().connect(sigc::mem_fun(this, &RadioWidget::changed));
    };

    void changed();
};

/**
 * Respond to the selected radiobutton changing.
 *
 * This function responds to the radiobutton selection changing by grabbing the value
 * from the text box and putting it in the parameter.
 */
void RadioWidget::changed()
{
    if (this->get_active()) {
        Glib::ustring value = _pref->value_from_label(this->get_label());
        _pref->set(value.c_str(), _doc, _node);
    }

    if (_changeSignal) {
        _changeSignal->emit();
    }
}


/** A special ComboBoxText class to use in ParamOptionGroup. */
class ComboWidget : public Gtk::ComboBoxText {
private:
    ParamOptionGroup *_pref;
    SPDocument *_doc;
    Inkscape::XML::Node *_node;
    sigc::signal<void> *_changeSignal;

public:
    ComboWidget(ParamOptionGroup *pref, SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
        : _pref(pref)
        , _doc(doc)
        , _node(node)
        , _changeSignal(changeSignal)
    {
        this->signal_changed().connect(sigc::mem_fun(this, &ComboWidget::changed));
    }

    ~ComboWidget() override = default;

    void changed();
};

void ComboWidget::changed()
{
    if (_pref) {
        Glib::ustring value = _pref->value_from_label(get_active_text());
        _pref->set(value.c_str(), _doc, _node);
    }

    if (_changeSignal) {
        _changeSignal->emit();
    }
}



/**
 * Creates the widget for the optiongroup parameter.
 */
Gtk::Widget *ParamOptionGroup::get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    auto hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, Parameter::GUI_PARAM_WIDGETS_SPACING));

    Gtk::Label *label = Gtk::manage(new Gtk::Label(_text, Gtk::ALIGN_START));
    hbox->pack_start(*label, false, false);

    if (_mode == COMBOBOX) {
        ComboWidget *combo = Gtk::manage(new ComboWidget(this, doc, node, changeSignal));

        for (auto choice : choices) {
            combo->append(choice->text);
            if (choice->value == _value) {
                combo->set_active_text(choice->text);
            }
        }

        if (combo->get_active_row_number() == -1) {
            combo->set_active(0);
        }

        hbox->pack_end(*combo, false, false);
    } else if (_mode == RADIOBUTTON) {
        label->set_valign(Gtk::ALIGN_START); // align label and first radio

        Gtk::Box *radios = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        Gtk::RadioButtonGroup group;

        for (auto choice : choices) {
            RadioWidget *radio = Gtk::manage(new RadioWidget(group, choice->text, this, doc, node, changeSignal));
            radios->pack_start(*radio, true, true);
            if (choice->value == _value) {
                radio->set_active();
            }
        }

        hbox->pack_end(*radios, false, false);
    }

    hbox->show_all();
    return static_cast<Gtk::Widget *>(hbox);
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
