/** @file
 * @brief New From Template abstract tab implementation
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński   
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "template-load-tab.h"

#include <gtkmm/scrolledwindow.h>
#include <iostream>

#include "interface.h"
#include "file.h"
#include "path-prefix.h"
#include "preferences.h"
#include "inkscape.h"


namespace Inkscape {
namespace UI {
    

TemplateLoadTab::TemplateLoadTab()
    : _current_keyword("")
    , _keywords_combo(true)
{
    set_border_width(10);

    Gtk::Label *title;
    title = manage(new Gtk::Label("Search:"));
    _tlist_box.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    _tlist_box.pack_start(_keywords_combo, Gtk::PACK_SHRINK, 0);
    
    title = manage(new Gtk::Label("Templates"));
    _tlist_box.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    title = manage(new Gtk::Label("Selected template"));
    _info_box.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    add(_main_box);
    _main_box.pack_start(_tlist_box, Gtk::PACK_SHRINK, 20);
    _main_box.pack_start(_info_box, Gtk::PACK_EXPAND_WIDGET, 10);
    
    Gtk::ScrolledWindow *scrolled;
    scrolled = manage(new Gtk::ScrolledWindow());
    scrolled->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scrolled->add(_tlist_view);
    _tlist_box.pack_start(*scrolled, Gtk::PACK_EXPAND_WIDGET, 5);
    
    _keywords_combo.signal_changed().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::_keywordSelected));
    this->show_all();
}


TemplateLoadTab::~TemplateLoadTab()
{
}


void TemplateLoadTab::createTemplate()
{
    std::cout << "Default Template Tab" << std::endl;
}


void TemplateLoadTab::_displayTemplateInfo()
{
    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef = _tlist_view.get_selection();
    if (templateSelectionRef->get_selected()) {
        _current_template = (*templateSelectionRef->get_selected())[_columns.textValue];
    }
}


void TemplateLoadTab::_initKeywordsList()
{
    _keywords_combo.append("All");
    
    for (int i = 0 ; i < 10 ; ++i) {
        _keywords_combo.append( "Keyword" + Glib::ustring::format(i));
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
}


void TemplateLoadTab::_keywordSelected()
{
    _current_keyword = _keywords_combo.get_active_text();
    _refreshTemplatesList();
}


void TemplateLoadTab::_refreshTemplatesList()
{
     _tlist_store->clear();
    
    for (std::map<Glib::ustring, TemplateData>::iterator it = _tdata.begin() ; it != _tdata.end() ; ++it) {
        Gtk::TreeModel::iterator iter = _tlist_store->append();
        Gtk::TreeModel::Row row = *iter;
        row[_columns.textValue]  = it->first;
    }
} 


void TemplateLoadTab::_loadTemplates()
{
    // user's local dir
    _getTemplatesFromDir(profile_path("templates") + _loading_path);

    // system templates dir
    _getTemplatesFromDir(INKSCAPE_TEMPLATESDIR + _loading_path);
}


TemplateLoadTab::TemplateData TemplateLoadTab::_processTemplateFile(const Glib::ustring &path)
{
    TemplateData result;
    result.path = path;
    result.display_name = Glib::path_get_basename(path);
    result.short_description = "LaLaLaLa";
    result.author = "JAASDASD";
    
    return result;
}


void TemplateLoadTab::_getTemplatesFromDir(const Glib::ustring &path)
{
    if ( !Glib::file_test(path, Glib::FILE_TEST_EXISTS) ||
         !Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
        return;
    
    Glib::Dir dir(path);

    Glib::ustring file = Glib::build_filename(path, dir.read_name());
    while (file != path){
        if (Glib::str_has_suffix(file, ".svg") && !Glib::str_has_prefix(Glib::path_get_basename(file), "default")){
            TemplateData tmp = _processTemplateFile(file);
            _tdata[Glib::path_get_basename(file)] = tmp;
        }
        file = Glib::build_filename(path, dir.read_name());
    }
}

}
}
