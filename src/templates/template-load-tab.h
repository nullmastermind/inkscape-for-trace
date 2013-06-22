#ifndef INKSCAPE_SEEN_UI_DIALOG_TEMPLATE_LOAD_TAB_H
#define INKSCAPE_SEEN_UI_DIALOG_TEMPLATE_LOAD_TAB_H

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/frame.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>

namespace Inkscape {
namespace UI {
    
    
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
    
    Glib::ustring _currentKeyword;
    Glib::ustring _currentTemplate;
    
    virtual void _displayTemplateInfo();
    virtual void _initKeywordsList();
    virtual void _refreshTemplatesList();
    
    void _initLists();
    void _keywordSelected();    

    Gtk::HBox _mainBox;
    Gtk::VBox _templatesColumn;
    Gtk::VBox _templateInfoColumn;
    
    Gtk::ComboBoxText _keywordsCombo;
    
    Gtk::TreeView _templatesView;
    Glib::RefPtr<Gtk::ListStore> _templatesRef;
    StringModelColumns _templatesColumns;

};

}
}

#endif
