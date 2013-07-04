#include "template-load-tab.h"

#include <gtkmm/alignment.h>
#include <iostream>

#include "src/interface.h"
#include "src/file.h"
#include "src/path-prefix.h"
#include "src/preferences.h"
#include "src/inkscape.h"


namespace Inkscape {
namespace UI {
    

TemplateLoadTab::TemplateLoadTab()
    : _keywords_combo(true)
    , _current_keyword("")
{
    set_border_width(10);

    Gtk::Label *title;
    title = manage(new Gtk::Label("Search Tags:"));
    _templates_column.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    _templates_column.pack_start(_keywords_combo, Gtk::PACK_SHRINK, 0);
    
    title = manage(new Gtk::Label("Templates"));
    _templates_column.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    title = manage(new Gtk::Label("Selected template"));
    _template_info_column.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    add(_main_box);
    _main_box.pack_start(_templates_column, Gtk::PACK_SHRINK, 20);
    _main_box.pack_start(_template_info_column, Gtk::PACK_EXPAND_WIDGET, 10);
    
    _templates_column.pack_start(_templates_view, Gtk::PACK_SHRINK, 5);

    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef =
    _templates_view.get_selection();
    
    templateSelectionRef->signal_changed().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::_displayTemplateInfo));
    
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
    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef = _templates_view.get_selection();
    if (templateSelectionRef->get_selected()) {
        _current_template = (*templateSelectionRef->get_selected())[_templates_columns.textValue];
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
    _templates_ref = Gtk::ListStore::create(_templates_columns);
    _templates_view.set_model(_templates_ref);
    _templates_view.append_column("", _templates_columns.textValue);
    _templates_view.set_headers_visible(false);
    
    _initKeywordsList();
    _refreshTemplatesList();
}


void TemplateLoadTab::_keywordSelected()
{
    _current_keyword = _keywords_combo.get_active_text();
    _refreshTemplatesList();
}


void TemplateLoadTab::_refreshTemplatesList()
{
     _templates_ref->clear();
    
    for (std::map<Glib::ustring, TemplateData>::iterator it = _templates.begin() ; it != _templates.end() ; ++it) {
        Gtk::TreeModel::iterator iter = _templates_ref->append();
        Gtk::TreeModel::Row row = *iter;
        row[_templates_columns.textValue]  = it->first;
    }
} 


void TemplateLoadTab::_loadTemplates()
{
    // user's local dir
    _getTemplatesFromDir(profile_path("templates") + _loading_path);

    // system templates dir
    _getTemplatesFromDir(INKSCAPE_TEMPLATESDIR + _loading_path);
}


TemplateLoadTab::TemplateData TemplateLoadTab::_processTemplateFile(Glib::ustring path)
{
    TemplateData result;
    result.path = path;
    result.display_name = Glib::path_get_basename(path);
    result.short_description = "LaLaLaLa";
    result.author = "JAASDASD";
    
    return result;
}


void TemplateLoadTab::_getTemplatesFromDir(Glib::ustring path)
{
    if ( !Glib::file_test(path, Glib::FILE_TEST_EXISTS) ||
         !Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
        return;
    
    Glib::Dir dir(path);
    path += "/";
    Glib::ustring file = path + dir.read_name();
    while (file != path){
        if (Glib::str_has_suffix(file, ".svg")){
            TemplateData tmp = _processTemplateFile(file);
            _templates[Glib::path_get_basename(file)] = tmp;
        }
        file = path + dir.read_name();
    }
}

}
}
