// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Notebook and NotebookPage parameters for extensions.
 */

/*
 * Authors:
 *   Johan Engelen <johan@shouraizou.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2006 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter-notebook.h"

#include <gtkmm/box.h>
#include <gtkmm/notebook.h>

#include "preferences.h"

#include "extension/extension.h"

#include "xml/node.h"


/**
 * The root directory in the preferences database for extension
 * related parameters.
 */
#define PREF_DIR "extensions"

namespace Inkscape {
namespace Extension {


ParamNotebook::ParamNotebookPage::ParamNotebookPage(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // Read XML tree of page and parse parameters
    if (xml) {
        Inkscape::XML::Node *child_repr = xml->firstChild();
        while (child_repr) {
            const char *chname = child_repr->name();
            if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
                chname += strlen(INKSCAPE_EXTENSION_NS);
            }
            if (chname[0] == '_') { // allow leading underscore in tag names for backwards-compatibility
                chname++;
            }

            if (!strcmp(chname, "param") || !strcmp(chname, "_param")) {
                InxParameter *param = InxParameter::make(child_repr, ext);
                if (param) {
                    parameters.push_back(param);
                }
            }

            child_repr = child_repr->next();
        }
    }
}

ParamNotebook::ParamNotebookPage::~ParamNotebookPage ()
{
    //destroy parameters
    for (auto parameter : parameters) {
        delete parameter;
    }
}

/** Return the value as a string. */
void ParamNotebook::ParamNotebookPage::paramString(std::list <std::string> &list)
{
    for (auto parameter : parameters) {
        parameter->string(list);
    }
}

/**
 * Creates a notebookpage widget for a notebook.
 *
 * Builds a notebook page (a vbox) and puts parameters on it.
 */
Gtk::Widget *ParamNotebook::ParamNotebookPage::get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::VBox * vbox = Gtk::manage(new Gtk::VBox);
    vbox->set_border_width(GUI_BOX_MARGIN);
    vbox->set_spacing(GUI_BOX_SPACING);

    // add parameters onto page (if any)
    for (auto parameter : parameters) {
        Gtk::Widget *parameter_widget = parameter->get_widget(doc, node, changeSignal);
        if (parameter_widget) {
            int indent = parameter->get_indent();
            parameter_widget->set_margin_start(indent *GUI_INDENTATION);
            vbox->pack_start(*parameter_widget, false, false, 0);

            const char *tooltip = parameter->get_tooltip();
            if (tooltip) {
                parameter_widget->set_tooltip_text(tooltip);
            }
        }
    }

    vbox->show();

    return dynamic_cast<Gtk::Widget *>(vbox);
}

/** Search the parameter's name in the page content. */
InxParameter *ParamNotebook::ParamNotebookPage::get_param(const char *name)
{
    if (name == nullptr) {
        throw Extension::param_not_exist();
    }
    if (this->parameters.empty()) {
        // the list of parameters is empty
        throw Extension::param_not_exist();
    }

    for (auto param : parameters) {
        if (!strcmp(param->name(), name)) {
            return param;
        }
    }

    return nullptr;
}

/** End ParamNotebookPage **/
/** ParamNotebook **/

ParamNotebook::ParamNotebook(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
{
    // Read XML tree to add pages (allow _page for backwards compatibility)
    if (xml) {
        Inkscape::XML::Node *child_repr = xml->firstChild();
        while (child_repr) {
            const char *chname = child_repr->name();
            if (chname && (!strcmp(chname, INKSCAPE_EXTENSION_NS "page") ||
                           !strcmp(chname, INKSCAPE_EXTENSION_NS "_page") )) {
                ParamNotebookPage *page;
                page = new ParamNotebookPage(child_repr, ext);

                if (page) {
                    pages.push_back(page);
                }
            }
            child_repr = child_repr->next();
        }
    }
    if (pages.empty()) {
        g_warning("No (valid) pages for parameter '%s' in extension '%s'", _name, _extension->get_id());
    }

    // get value (initialize with value of first page if pref is empty)
    char *pref_name = this->pref_name();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getString(extension_pref_root + pref_name);
    g_free(pref_name);

    if (_value.empty()) {
        if (!pages.empty()) {
            _value = pages[0]->_name;
        }
    }
}

ParamNotebook::~ParamNotebook ()
{
    //destroy pages
    for (auto page : pages) {
        delete page;
    }
}


/**
 * A function to set the \c _value.
 *
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place, \c PREF_DIR
 * and \c pref_name() are used.
 *
 * @param  in   The number of the page to set as new value.
 * @param  doc  A document that should be used to set the value.
 * @param  node The node where the value may be placed.
 */
const Glib::ustring& ParamNotebook::set(const int in, SPDocument * /*doc*/, Inkscape::XML::Node * /*node*/)
{
    int i = in < pages.size() ? in : pages.size()-1;
    ParamNotebookPage *page = pages[i];

    if (page) {
        _value = page->_name;

        char *pref_name = this->pref_name();
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(extension_pref_root + pref_name, _value);
        g_free(pref_name);
    }

    return _value;
}

void ParamNotebook::string(std::list <std::string> &list) const
{
    std::string param_string;
    param_string += "--";
    param_string += _name;
    param_string += "=";

    param_string += "\"";
    param_string += _value;  // the name of the current page
    param_string += "\"";
    list.insert(list.end(), param_string);

    for (auto page : pages) {
        page->paramString(list);
    }
}

/** A special category of Gtk::Notebook to handle notebook parameters. */
class NotebookWidget : public Gtk::Notebook {
private:
    ParamNotebook *_pref;
    SPDocument *_doc;
    Inkscape::XML::Node *_node;
public:
    /**
     * Build a notebookpage preference for the given parameter.
     * @param  pref  Where to get the string (pagename) from, and where to put it when it changes.
     */
    NotebookWidget (ParamNotebook *pref, SPDocument *doc, Inkscape::XML::Node *node)
        : Gtk::Notebook()
        , _pref(pref)
        , _doc(doc)
        , _node(node)
        , activated(false)
    {
        // don't have to set the correct page: this is done in ParamNotebook::get_widget hook function
        this->signal_switch_page().connect(sigc::mem_fun(this, &NotebookWidget::changed_page));
    }

    void changed_page(Gtk::Widget *page, guint pagenum);

    bool activated;
};

/**
 * Respond to the selected page of notebook changing.
 * This function responds to the changing by reporting it to
 * ParamNotebook. The change is only reported when the notebook
 * is actually visible. This to exclude 'fake' changes when the
 * notebookpages are added or removed.
 */
void NotebookWidget::changed_page(Gtk::Widget * /*page*/, guint pagenum)
{
    if (get_visible()) {
        _pref->set((int)pagenum, _doc, _node);
    }
}

/** Search the parameter's name in the notebook content. */
InxParameter *ParamNotebook::get_param(const char *name)
{
    if (name == nullptr) {
        throw Extension::param_not_exist();
    }
    for (auto page : pages) {
        InxParameter *subparam = page->get_param(name);
        if (subparam) {
            return subparam;
        }
    }

    return nullptr;
}


/**
 * Creates a Notebook widget for a notebook parameter.
 *
 * Builds a notebook and puts pages in it.
 */
Gtk::Widget *ParamNotebook::get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    NotebookWidget *notebook = Gtk::manage(new NotebookWidget(this, doc, node));

    // add pages (if any) and switch to previously selected page
    int current_page = -1;
    int selected_page = -1;
    for (auto page : pages) {
        current_page++;

        Gtk::Widget *page_widget = page->get_widget(doc, node, changeSignal);

        Glib::ustring page_text = page->_text;
        if (_translatable != NO) { // translate unless explicitly marked untranslatable
            page_text = page->get_translation(page_text.c_str());
        }

        notebook->append_page(*page_widget, page_text);

        if (_value == page->_name) {
            selected_page = current_page;
        }
    }
    if (selected_page >= 0) {
        notebook->set_current_page(selected_page);
    }

    notebook->show();

    return static_cast<Gtk::Widget *>(notebook);
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
