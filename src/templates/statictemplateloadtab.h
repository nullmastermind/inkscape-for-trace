#ifndef GTKMM_STATIC_TEMPLATE_LOAD_TAB_H
#define GTKMM_STATIC_TEMPLATE_LOAD_TAB_H

#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

#include "templateloadtab.h"


class StaticTemplateLoadTab : public TemplateLoadTab
{
public:
    StaticTemplateLoadTab();
    virtual void createTemplate();
    
protected:
    virtual void displayTemplateInfo();
    
    Gtk::Button moreInfoButton;
    Gtk::Label shortDescriptionLabel;
    Gtk::Label templateAuthorLabel;
    Gtk::Label templateNameLabel;
    Gtk::Image previewImage;
};

#endif