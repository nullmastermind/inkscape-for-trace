/** @file
 * @brief New From Template static templates tab - implementation
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński   
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "static-template-load-tab.h"

#include <gtkmm/alignment.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <iostream>

#include "file.h"


namespace Inkscape {
namespace UI {
    

StaticTemplateLoadTab::StaticTemplateLoadTab()
    : TemplateLoadTab()
    , _more_info_button("More info")
    , _short_description_label("Short description - I like trains. ad asda asd asdweqe gdfg")
    , _template_author_label("by template_author")
    , _template_name_label("Template_name")
    , _preview_image("preview.png")
{
    _loading_path = "";
    _loadTemplates();
    _initLists();
    
    _info_box.pack_start(_template_name_label, Gtk::PACK_SHRINK, 4);
    _info_box.pack_start(_template_author_label, Gtk::PACK_SHRINK, 0);
    _info_box.pack_start(_preview_image, Gtk::PACK_SHRINK, 15);
    _info_box.pack_start(_short_description_label, Gtk::PACK_SHRINK, 4);
    
    _short_description_label.set_line_wrap(true);
    _short_description_label.set_size_request(200);

    Gtk::Alignment *align;
    align = manage(new Gtk::Alignment(Gtk::ALIGN_END, Gtk::ALIGN_CENTER, 0.0, 0.0));
    _info_box.pack_start(*align, Gtk::PACK_SHRINK, 5);
    align->add(_more_info_button);
    
    _more_info_button.signal_pressed().connect(
    sigc::mem_fun(*this, &StaticTemplateLoadTab::_displayTemplateDetails));
}


void StaticTemplateLoadTab::createTemplate()
{
    Glib::ustring path;
    if (_tdata.find(_current_template) != _tdata.end()){
        path = _tdata[_current_template].path;
    }
    else
        path = "";
    
    sp_file_new(path);
}


void StaticTemplateLoadTab::_displayTemplateInfo()
{
    TemplateLoadTab::_displayTemplateInfo();
    _template_name_label.set_text(_current_template);
    _template_author_label.set_text(_tdata[_current_template].author);
    _short_description_label.set_text(_tdata[_current_template].short_description);
    
    Glib::ustring imagePath = Glib::build_filename(Glib::path_get_dirname(_tdata[_current_template].path),  _tdata[_current_template].preview_name);
    _preview_image.set(imagePath);
}

}
}
