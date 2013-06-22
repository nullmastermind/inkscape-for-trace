#include <gtkmm/alignment.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <iostream>

#include "static-template-load-tab.h"

namespace Inkscape {
namespace UI {
    

StaticTemplateLoadTab::StaticTemplateLoadTab() :
    _moreInfoButton("More info"),
    _previewImage("preview.png"),
    _shortDescriptionLabel("Short description - I like trains. ad asda asd asdweqe gdfg"),
    _templateNameLabel("Template_name"),
    _templateAuthorLabel("by template_author")
{
    _templateInfoColumn.pack_start(_templateNameLabel, Gtk::PACK_SHRINK, 4);
    _templateInfoColumn.pack_start(_templateAuthorLabel, Gtk::PACK_SHRINK, 0);
    _templateInfoColumn.pack_start(_previewImage, Gtk::PACK_SHRINK, 15);
    _templateInfoColumn.pack_start(_shortDescriptionLabel, Gtk::PACK_SHRINK, 4);
    
    _shortDescriptionLabel.set_line_wrap(true);
    _shortDescriptionLabel.set_size_request(200);

    Gtk::Alignment *align;
    align = manage(new Gtk::Alignment(Gtk::ALIGN_END, Gtk::ALIGN_CENTER, 0.0, 0.0));
    _templateInfoColumn.pack_start(*align, Gtk::PACK_SHRINK, 5);
    align->add(_moreInfoButton);
}


void StaticTemplateLoadTab::createTemplate()
{
    std::cout << "Static template\n";
}


void StaticTemplateLoadTab::_displayTemplateInfo()
{
    TemplateLoadTab::_displayTemplateInfo();
    _templateNameLabel.set_text(_currentTemplate);
}

}
}
