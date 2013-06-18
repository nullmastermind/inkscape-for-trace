#ifndef GTKMM_TEMPLATE_LOAD_TAB_H
#define GTKMM_TEMPLATE_LOAD_TAB_H

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/frame.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>


class TemplateLoadTab : public Gtk::Frame
{

public:
    TemplateLoadTab();
    virtual ~TemplateLoadTab();
    virtual void createTemplate();

protected:
    class StringModelColumns : public Gtk::TreeModelColumnRecord
    {
        public:
        StringModelColumns()
        {
            add(textValue);
        }
        
        Gtk::TreeModelColumn<Glib::ustring> textValue;
    };
    
    Glib::ustring currentKeyword;
    Glib::ustring currentTemplate;
    
    virtual void displayTemplateInfo();
    virtual void initKeywordsList();
    virtual void refreshTemplatesList();
    
    void initLists();
    void keywordSelected();    

    Gtk::HBox mainBox;
    Gtk::VBox templatesColumn;
    Gtk::VBox templateInfoColumn;
    
    Gtk::ComboBoxText keywordsCombo;
    
    Gtk::TreeView templatesView;
    Glib::RefPtr<Gtk::ListStore> templatesRef;
    StringModelColumns templatesColumns;

};

#endif // GTKMM_TEMPLATE_LOAD_TAB_H