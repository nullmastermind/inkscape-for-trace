#include <gtkmm/alignment.h>
#include <iostream>

#include "template-load-tab.h"

namespace Inkscape {
namespace UI {
    

TemplateLoadTab::TemplateLoadTab()
{
    set_border_width(10);

    Gtk::Label *title;
    title = manage(new Gtk::Label("Search Tags:"));
    _templatesColumn.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    _templatesColumn.pack_start(_keywordsCombo, Gtk::PACK_SHRINK, 0);
    
    title = manage(new Gtk::Label("Templates"));
    _templatesColumn.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    title = manage(new Gtk::Label("Selected template"));
    _templateInfoColumn.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    _initLists();
    
    add(_mainBox);
    _mainBox.pack_start(_templatesColumn, Gtk::PACK_SHRINK, 20);
    _mainBox.pack_start(_templateInfoColumn, Gtk::PACK_EXPAND_WIDGET, 10);
    
    _templatesColumn.pack_start(_templatesView, Gtk::PACK_SHRINK, 5);

    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef =
    _templatesView.get_selection();
    
    templateSelectionRef->signal_changed().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::_displayTemplateInfo));
    
    _keywordsCombo.signal_changed().connect(
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
    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef = _templatesView.get_selection();
    if (templateSelectionRef->get_selected()) {
        _currentTemplate = (*templateSelectionRef->get_selected())[_templatesColumns.textValue];
    }
}


void TemplateLoadTab::_initKeywordsList()
{
    _keywordsCombo.append_text("All");
    _keywordsCombo.set_active_text("All");
    
    for (int i = 0 ; i < 10 ; ++i) {
        _keywordsCombo.append_text( "Keyword" + Glib::ustring::format(i));
    }
}


void TemplateLoadTab::_initLists()
{
    _templatesRef = Gtk::ListStore::create(_templatesColumns);
    _templatesView.set_model(_templatesRef);
    _templatesView.append_column("", _templatesColumns.textValue);
    _templatesView.set_headers_visible(false);
    
    _initKeywordsList();
    _refreshTemplatesList();
}


void TemplateLoadTab::_keywordSelected()
{
    _currentKeyword = _keywordsCombo.get_active_text();
    _refreshTemplatesList();
}


void TemplateLoadTab::_refreshTemplatesList()
{
     _templatesRef->clear();
     for (int i = 0 ; i < 7 ; ++i) {
        Gtk::TreeModel::iterator iter = _templatesRef->append();
        Gtk::TreeModel::Row row = *iter;
        row[_templatesColumns.textValue] = "Template" + Glib::ustring::format(i);
    }
} 

}
}
