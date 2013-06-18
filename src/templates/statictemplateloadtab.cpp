#include <gtkmm/alignment.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <iostream>

#include "statictemplateloadtab.h"


StaticTemplateLoadTab::StaticTemplateLoadTab() :
    moreInfoButton("More info"),
    previewImage("preview.png"),
    shortDescriptionLabel("Short description - I like trains. ad asda asd asdweqe gdfg"),
    templateNameLabel("Template_name"),
    templateAuthorLabel("by template_author")
{
    templateInfoColumn.pack_start(templateNameLabel, Gtk::PACK_SHRINK, 4);
    templateInfoColumn.pack_start(templateAuthorLabel, Gtk::PACK_SHRINK, 0);
    templateInfoColumn.pack_start(previewImage, Gtk::PACK_SHRINK, 15);
    templateInfoColumn.pack_start(shortDescriptionLabel, Gtk::PACK_SHRINK, 4);
    
    shortDescriptionLabel.set_line_wrap(true);
    shortDescriptionLabel.set_size_request(200);

    Gtk::Alignment *align;
    align = manage(new Gtk::Alignment(Gtk::ALIGN_END, Gtk::ALIGN_CENTER, 0.0, 0.0));
    templateInfoColumn.pack_start(*align, Gtk::PACK_SHRINK, 5);
    align->add(moreInfoButton);
}


void StaticTemplateLoadTab::createTemplate()
{
    std::cout << "Static template\n";
}


void StaticTemplateLoadTab::displayTemplateInfo()
{
    TemplateLoadTab::displayTemplateInfo();
    templateNameLabel.set_text(currentTemplate);
}

