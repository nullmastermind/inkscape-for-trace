/** @file
 * @brief New From Template abstract tab class
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński   
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef INKSCAPE_SEEN_UI_DIALOG_TEMPLATE_LOAD_TAB_H
#define INKSCAPE_SEEN_UI_DIALOG_TEMPLATE_LOAD_TAB_H

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/frame.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <map>
#include <set>


namespace Inkscape {
namespace UI {
    
    
class TemplateLoadTab : public Gtk::Frame
{

public:
    TemplateLoadTab();
    virtual ~TemplateLoadTab();
    virtual void createTemplate();

protected:
    struct TemplateData
    {
        Glib::ustring path;
        Glib::ustring display_name;
        Glib::ustring author;
        Glib::ustring short_description;
        std::set<Glib::ustring> keywords;
    };
    
    class StringModelColumns : public Gtk::TreeModelColumnRecord
    {
        public:
        StringModelColumns()
        {
            add(textValue);
        }
        
        Gtk::TreeModelColumn<Glib::ustring> textValue;
    };
    
    Glib::ustring _current_keyword;
    Glib::ustring _current_template;
    Glib::ustring _loading_path;
    std::map<Glib::ustring, TemplateData> _tdata;
    
    
    virtual void _displayTemplateInfo();
    virtual void _initKeywordsList();
    virtual void _refreshTemplatesList();
    void _loadTemplates();
    void _initLists();
    
    Gtk::HBox _main_box;
    Gtk::VBox _tlist_box;
    Gtk::VBox _info_box;
    
    Gtk::ComboBoxText _keywords_combo;
    
    Gtk::TreeView _tlist_view;
    Glib::RefPtr<Gtk::ListStore> _tlist_store;
    StringModelColumns _columns;
    
private:
    void _getTemplatesFromDir(const Glib::ustring &);
    void _keywordSelected();    
    TemplateData _processTemplateFile(const Glib::ustring &);

};

}
}

#endif
