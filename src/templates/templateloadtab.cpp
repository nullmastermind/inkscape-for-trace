#include <gtkmm/alignment.h>
#include <iostream>

#include "templateloadtab.h"


TemplateLoadTab::TemplateLoadTab()
{
    set_border_width(10);

    Gtk::Label *title;
    title = manage(new Gtk::Label("Search Tags:"));
    templatesColumn.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    templatesColumn.pack_start(keywordsCombo, Gtk::PACK_SHRINK, 0);
    
    title = manage(new Gtk::Label("Templates"));
    templatesColumn.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    title = manage(new Gtk::Label("Selected template"));
    templateInfoColumn.pack_start(*title, Gtk::PACK_SHRINK, 10);
    
    initLists();
    
    add(mainBox);
    mainBox.pack_start(templatesColumn, Gtk::PACK_SHRINK, 20);
    mainBox.pack_start(templateInfoColumn, Gtk::PACK_EXPAND_WIDGET, 10);
    
    templatesColumn.pack_start(templatesView, Gtk::PACK_SHRINK, 5);

    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef =
    templatesView.get_selection();
    
    templateSelectionRef->signal_changed().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::displayTemplateInfo));
    
    keywordsCombo.signal_changed().connect(
    sigc::mem_fun(*this, &TemplateLoadTab::keywordSelected));
    this->show_all();
}


TemplateLoadTab::~TemplateLoadTab()
{
}


void TemplateLoadTab::createTemplate()
{
    std::cout << "Default Template Tab" << std::endl;
}


void TemplateLoadTab::displayTemplateInfo()
{
    Glib::RefPtr<Gtk::TreeSelection> templateSelectionRef = templatesView.get_selection();
    if (templateSelectionRef->get_selected())
        currentTemplate = (*templateSelectionRef->get_selected())[templatesColumns.textValue];
}


void TemplateLoadTab::initKeywordsList()
{
    keywordsCombo.append_text("All");
    keywordsCombo.set_active_text("All");
    
    for (int i=0; i<10; ++i){

        keywordsCombo.append_text( "Keyword" + Glib::ustring::format(i));
    }
}


void TemplateLoadTab::initLists()
{
    templatesRef = Gtk::ListStore::create(templatesColumns);
    templatesView.set_model(templatesRef);
    templatesView.append_column("", templatesColumns.textValue);
    templatesView.set_headers_visible(false);
    
    initKeywordsList();
    refreshTemplatesList();
}


void TemplateLoadTab::keywordSelected()
{
    currentKeyword = keywordsCombo.get_active_text();
    refreshTemplatesList();
}


void TemplateLoadTab::refreshTemplatesList()
{
     templatesRef->clear();
     for (int i=0; i<7; ++i){
        Gtk::TreeModel::iterator iter = templatesRef->append();
        Gtk::TreeModel::Row row = *iter;
        row[templatesColumns.textValue] = "Template" + Glib::ustring::format(i);
    }
}