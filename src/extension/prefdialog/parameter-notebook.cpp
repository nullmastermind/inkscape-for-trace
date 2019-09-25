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

#include <unordered_set>

#include <gtkmm/box.h>
#include <gtkmm/notebook.h>

#include "preferences.h"

#include "extension/extension.h"

#include "xml/node.h"

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

            if (InxWidget::is_valid_widget_name(chname)) {
                InxWidget *widget = InxWidget::make(child_repr, _extension);
                if (widget) {
                    _children.push_back(widget);
                }
            } else if (child_repr->type() == XML::ELEMENT_NODE) {
                g_warning("Invalid child element ('%s') in notebook page in extension '%s'.",
                          chname, _extension->get_id());
            } else if (child_repr->type() != XML::COMMENT_NODE){
                g_warning("Invalid child element found in notebook page in extension '%s'.", _extension->get_id());
            }

            child_repr = child_repr->next();
        }
    }
}


/**
 * Creates a notebookpage widget for a notebook.
 *
 * Builds a notebook page (a vbox) and puts parameters on it.
 */
Gtk::Widget *ParamNotebook::ParamNotebookPage::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::VBox * vbox = Gtk::manage(new Gtk::VBox);
    vbox->set_border_width(GUI_BOX_MARGIN);
    vbox->set_spacing(GUI_BOX_SPACING);

    // add parameters onto page (if any)
    for (auto child : _children) {
        Gtk::Widget *child_widget = child->get_widget(changeSignal);
        if (child_widget) {
            int indent = child->get_indent();
            child_widget->set_margin_start(indent *GUI_INDENTATION);
            vbox->pack_start(*child_widget, false, true, 0); // fill=true does not have an effect here, but allows the
                                                             // child to choose to expand by setting hexpand/vexpand

            const char *tooltip = child->get_tooltip();
            if (tooltip) {
                child_widget->set_tooltip_text(tooltip);
            }
        }
    }

    vbox->show();

    return dynamic_cast<Gtk::Widget *>(vbox);
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
                    _children.push_back(page);
                }
            } else if (child_repr->type() == XML::ELEMENT_NODE) {
                g_warning("Invalid child element ('%s') for parameter '%s' in extension '%s'. Expected 'page'.",
                          chname, _name, _extension->get_id());
            } else if (child_repr->type() != XML::COMMENT_NODE){
                g_warning("Invalid child element found in parameter '%s' in extension '%s'. Expected 'page'.",
                          _name, _extension->get_id());
            }
            child_repr = child_repr->next();
        }
    }
    if (_children.empty()) {
        g_warning("No (valid) pages for parameter '%s' in extension '%s'", _name, _extension->get_id());
    }

    // check for duplicate page names
    std::unordered_set<std::string> names;
    for (auto child : _children) {
        ParamNotebookPage *page = static_cast<ParamNotebookPage *>(child);
        auto ret = names.emplace(page->_name);
        if (!ret.second) {
            g_warning("Duplicate page name ('%s') for parameter '%s' in extension '%s'.",
                      page->_name, _name, _extension->get_id());
        }
    }

    // get value (initialize with value of first page if pref is empty)
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _value = prefs->getString(pref_name());

    if (_value.empty()) {
        if (!_children.empty()) {
            ParamNotebookPage *first_page = dynamic_cast<ParamNotebookPage *>(_children[0]);
            _value = first_page->_name;
        }
    }
}


/**
 * A function to set the \c _value.
 *
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place \c pref_name() is used.
 *
 * @param  in   The number of the page to set as new value.
 */
const Glib::ustring& ParamNotebook::set(const int in)
{
    int i = in < _children.size() ? in : _children.size()-1;
    ParamNotebookPage *page = dynamic_cast<ParamNotebookPage *>(_children[i]);

    if (page) {
        _value = page->_name;

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(pref_name(), _value);
    }

    return _value;
}

std::string ParamNotebook::value_to_string() const
{
    return _value;
}


/** A special category of Gtk::Notebook to handle notebook parameters. */
class NotebookWidget : public Gtk::Notebook {
private:
    ParamNotebook *_pref;
public:
    /**
     * Build a notebookpage preference for the given parameter.
     * @param  pref  Where to get the string (pagename) from, and where to put it when it changes.
     */
    NotebookWidget(ParamNotebook *pref)
        : Gtk::Notebook()
        , _pref(pref)
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
        _pref->set((int)pagenum);
    }
}

/**
 * Creates a Notebook widget for a notebook parameter.
 *
 * Builds a notebook and puts pages in it.
 */
Gtk::Widget *ParamNotebook::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    NotebookWidget *notebook = Gtk::manage(new NotebookWidget(this));

    // add pages (if any) and switch to previously selected page
    int current_page = -1;
    int selected_page = -1;
    for (auto child : _children) {
        ParamNotebookPage *page = dynamic_cast<ParamNotebookPage *>(child);
        g_assert(child); // A ParamNotebook has only children of type ParamNotebookPage.
                         // If we receive a non-page child here something is very wrong!
        current_page++;

        Gtk::Widget *page_widget = page->get_widget(changeSignal);

        Glib::ustring page_text = page->_text;
        if (page->_translatable != NO) { // translate unless explicitly marked untranslatable
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
