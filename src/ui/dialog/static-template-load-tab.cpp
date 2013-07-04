#include "static-template-load-tab.h"

#include <gtkmm/alignment.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <iostream>

#include "src/file.h"


namespace Inkscape {
namespace UI {
    

StaticTemplateLoadTab::StaticTemplateLoadTab()
    : TemplateLoadTab()
    , _more_info_button("More info")
    , _preview_image("preview.png")
    , _short_description_label("Short description - I like trains. ad asda asd asdweqe gdfg")
    , _template_name_label("Template_name")
    , _template_author_label("by template_author")
{
    _loading_path = "/static";
    _loadTemplates();
    _initLists();
    
    _template_info_column.pack_start(_template_name_label, Gtk::PACK_SHRINK, 4);
    _template_info_column.pack_start(_template_author_label, Gtk::PACK_SHRINK, 0);
    _template_info_column.pack_start(_preview_image, Gtk::PACK_SHRINK, 15);
    _template_info_column.pack_start(_short_description_label, Gtk::PACK_SHRINK, 4);
    
    _short_description_label.set_line_wrap(true);
    _short_description_label.set_size_request(200);

    Gtk::Alignment *align;
    align = manage(new Gtk::Alignment(Gtk::ALIGN_END, Gtk::ALIGN_CENTER, 0.0, 0.0));
    _template_info_column.pack_start(*align, Gtk::PACK_SHRINK, 5);
    align->add(_more_info_button);
}


void StaticTemplateLoadTab::createTemplate()
{
    Glib::ustring path;
    if (_templates.find(_current_template) != _templates.end()){
        path = _templates[_current_template].path;
    }
    else
        path = "";
    
    sp_file_new(path);
}


void StaticTemplateLoadTab::_displayTemplateInfo()
{
    TemplateLoadTab::_displayTemplateInfo();
    _template_name_label.set_text(_current_template);
    _template_author_label.set_text(_templates[_current_template].author);
    _short_description_label.set_text(_templates[_current_template].short_description);
}

}
}
