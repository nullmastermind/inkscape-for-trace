// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief New From Template abstract tab implementation
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "template-widget.h"
#include "new-from-template.h"

#include <glibmm/miscutils.h>
#include <glibmm/stringutils.h>
#include <glibmm/fileutils.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/scrolledwindow.h>
#include <iostream>

#include "extension/extension.h"
#include "extension/db.h"
#include "inkscape.h"
#include "file.h"
#include "path-prefix.h"

using namespace Inkscape::IO::Resource;

namespace Inkscape {
namespace UI {

TemplateLoadTab::TemplateLoadTab(NewFromTemplate* parent)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    , _current_keyword("")
    , _keywords_combo(true)
    , _current_search_type(ALL)
    , _parent_widget(parent)
    , _tlist_box(Gtk::ORIENTATION_VERTICAL)
    , _search_box(Gtk::ORIENTATION_HORIZONTAL)
{
    set_border_width(10);

    _info_widget = Gtk::manage(new TemplateWidget());

    Gtk::Label *title;
    title = Gtk::manage(new Gtk::Label(_("Search:")));
    _search_box.pack_start(*title, Gtk::PACK_SHRINK);
    _search_box.pack_start(_keywords_combo, Gtk::PACK_SHRINK, 5);

    _tlist_box.pack_start(_search_box, Gtk::PACK_SHRINK, 10);

    pack_start(_tlist_box, Gtk::PACK_SHRINK);
    pack_start(*_info_widget, Gtk::PACK_EXPAND_WIDGET, 5);

    Gtk::ScrolledWindow *scrolled;
    scrolled = Gtk::manage(new Gtk::ScrolledWindow());
    scrolled->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scrolled->add(_tlist_view);
    _tlist_box.pack_start(*scrolled, Gtk::PACK_EXPAND_WIDGET, 5);

    _keywords_combo.signal_changed().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::_keywordSelected));
    this->show_all();

    _loadTemplates();
    _initLists();
}


TemplateLoadTab::~TemplateLoadTab()
= default;


void TemplateLoadTab::createTemplate()
{
    _info_widget->create();
}


void TemplateLoadTab::_onRowActivated(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn*)
{
    createTemplate();
    NewFromTemplate* parent = static_cast<NewFromTemplate*> (this->get_toplevel());
    parent->_onClose();
}

void TemplateLoadTab::_displayTemplateInfo()
{
    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef = _tlist_view.get_selection();
    if (templateSelectionRef->get_selected()) {
        _current_template = (*templateSelectionRef->get_selected())[_columns.textValue];

        _info_widget->display(_tdata[_current_template]);
        _parent_widget->setCreateButtonSensitive(true);
    }

}


void TemplateLoadTab::_initKeywordsList()
{
    _keywords_combo.append(_("All"));

    for (const auto & _keyword : _keywords){
        _keywords_combo.append(_keyword);
    }
}


void TemplateLoadTab::_initLists()
{
    _tlist_store = Gtk::ListStore::create(_columns);
    _tlist_view.set_model(_tlist_store);
    _tlist_view.append_column("", _columns.textValue);
    _tlist_view.set_headers_visible(false);

    _initKeywordsList();
    _refreshTemplatesList();

    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef =
    _tlist_view.get_selection();
    templateSelectionRef->signal_changed().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::_displayTemplateInfo));

    _tlist_view.signal_row_activated().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::_onRowActivated));
}

void TemplateLoadTab::_keywordSelected()
{
    _current_keyword = _keywords_combo.get_active_text();
    if (_current_keyword == ""){
        _current_keyword = _keywords_combo.get_entry_text();
        _current_search_type = USER_SPECIFIED;
    }
    else
        _current_search_type = LIST_KEYWORD;

    if (_current_keyword == "" || _current_keyword == _("All"))
        _current_search_type = ALL;

    _refreshTemplatesList();
}


void TemplateLoadTab::_refreshTemplatesList()
{
    _tlist_store->clear();

    switch (_current_search_type){
    case ALL :{
        for (auto & it : _tdata) {
            Gtk::TreeModel::iterator iter = _tlist_store->append();
            Gtk::TreeModel::Row row = *iter;
            row[_columns.textValue]  = it.first;
        }
        break;
    }

    case LIST_KEYWORD: {
        for (auto & it : _tdata) {
            if (it.second.keywords.count(_current_keyword.lowercase()) != 0){
                Gtk::TreeModel::iterator iter = _tlist_store->append();
                Gtk::TreeModel::Row row = *iter;
                row[_columns.textValue]  = it.first;
            }
        }
        break;
    }

    case USER_SPECIFIED : {
        for (auto & it : _tdata) {
            if (it.second.keywords.count(_current_keyword.lowercase()) != 0 ||
                it.second.display_name.lowercase().find(_current_keyword.lowercase()) != Glib::ustring::npos ||
                it.second.author.lowercase().find(_current_keyword.lowercase()) != Glib::ustring::npos ||
                it.second.short_description.lowercase().find(_current_keyword.lowercase()) != Glib::ustring::npos)
            {
                Gtk::TreeModel::iterator iter = _tlist_store->append();
                Gtk::TreeModel::Row row = *iter;
                row[_columns.textValue]  = it.first;
            }
        }
        break;
    }
    }

    // reselect item
    Gtk::TreeIter* item_to_select = nullptr;
    for (Gtk::TreeModel::Children::iterator it = _tlist_store->children().begin(); it != _tlist_store->children().end(); ++it) {
        Gtk::TreeModel::Row row = *it;
        if (_current_template == row[_columns.textValue]) {
            item_to_select = new Gtk::TreeIter(it);
            break;
        }
    }
    if (_tlist_store->children().size() == 1) {
        delete item_to_select;
        item_to_select = new Gtk::TreeIter(_tlist_store->children().begin());
    }
    if (item_to_select) {
        _tlist_view.get_selection()->select(*item_to_select);
        delete item_to_select;
    } else {
        _current_template = "";
        _info_widget->clear();
        _parent_widget->setCreateButtonSensitive(false);
    }
}


void TemplateLoadTab::_loadTemplates()
{
    for(auto &filename: get_filenames(TEMPLATES, {".svg"}, {"default."})) {
        TemplateData tmp = _processTemplateFile(filename.raw());
        if (tmp.display_name != "")
            _tdata[tmp.display_name] = tmp;

    }
    // procedural templates
    _getProceduralTemplates();
}


TemplateLoadTab::TemplateData TemplateLoadTab::_processTemplateFile(const std::string &path)
{
    TemplateData result;
    result.path = path;
    result.is_procedural = false;
    result.preview_name = "";

    // convert path into valid template name
    result.display_name = Glib::path_get_basename(path);
    gsize n = 0;
    while ((n = result.display_name.find_first_of("_", 0)) < Glib::ustring::npos){
        result.display_name.replace(n, 1, 1, ' ');
    }
    n =  result.display_name.rfind(".svg");
    result.display_name.replace(n, 4, 1, ' ');

    Inkscape::XML::Document *rdoc = sp_repr_read_file(path.data(), SP_SVG_NS_URI);
    if (rdoc){
        Inkscape::XML::Node *root = rdoc->root();
        if (strcmp(root->name(), "svg:svg") != 0){     // Wrong file format
            return result;
        }

        Inkscape::XML::Node *templateinfo = sp_repr_lookup_name(root, "inkscape:templateinfo");
        if (!templateinfo) {
            templateinfo = sp_repr_lookup_name(root, "inkscape:_templateinfo"); // backwards-compatibility
        }

        if (templateinfo == nullptr)    // No template info
            return result;
        _getDataFromNode(templateinfo, result);
    }

    return result;
}

void TemplateLoadTab::_getProceduralTemplates()
{
    std::list<Inkscape::Extension::Effect *> effects;
    Inkscape::Extension::db.get_effect_list(effects);

    std::list<Inkscape::Extension::Effect *>::iterator it = effects.begin();
    while (it != effects.end()){
        Inkscape::XML::Node *repr = (*it)->get_repr();
        Inkscape::XML::Node *templateinfo = sp_repr_lookup_name(repr, "inkscape:templateinfo");
        if (!templateinfo) {
            templateinfo = sp_repr_lookup_name(repr, "inkscape:_templateinfo"); // backwards-compatibility
        }

        if (templateinfo){
            TemplateData result;
            result.display_name = (*it)->get_name();
            result.is_procedural = true;
            result.path = "";
            result.tpl_effect = *it;

            _getDataFromNode(templateinfo, result, *it);
            _tdata[result.display_name] = result;
        }
        ++it;
    }
}

// if the template data comes from a procedural template (aka Effect extension),
// attempt to translate within the extension's context (which might use a different gettext textdomain)
const char *_translate(const char* msgid, Extension::Extension *extension)
{
    if (extension) {
        return extension->get_translation(msgid);
    } else {
        return _(msgid);
    }
}

void TemplateLoadTab::_getDataFromNode(Inkscape::XML::Node *dataNode, TemplateData &data, Extension::Extension *extension)
{
    Inkscape::XML::Node *currentData;
    if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:name")) != nullptr)
        data.display_name = _translate(currentData->firstChild()->content(), extension);
    else if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:_name")) != nullptr) // backwards-compatibility
        data.display_name = _translate(currentData->firstChild()->content(), extension);

    if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:author")) != nullptr)
        data.author = currentData->firstChild()->content();

    if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:shortdesc")) != nullptr)
        data.short_description = _translate(currentData->firstChild()->content(), extension);
    else if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:_shortdesc")) != nullptr) // backwards-compatibility
        data.short_description = _translate(currentData->firstChild()->content(), extension);

    if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:preview")) != nullptr)
        data.preview_name = currentData->firstChild()->content();

    if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:date")) != nullptr)
        data.creation_date = currentData->firstChild()->content();

    if ((currentData = sp_repr_lookup_name(dataNode, "inkscape:_keywords")) != nullptr){
        Glib::ustring tplKeywords = _translate(currentData->firstChild()->content(), extension);
        while (!tplKeywords.empty()){
            std::size_t pos = tplKeywords.find_first_of(" ");
            if (pos == Glib::ustring::npos)
                pos = tplKeywords.size();

            Glib::ustring keyword = tplKeywords.substr(0, pos).data();
            data.keywords.insert(keyword.lowercase());
            _keywords.insert(keyword.lowercase());

            if (pos == tplKeywords.size())
                break;
            tplKeywords.erase(0, pos+1);
        }
    }
}

}
}
