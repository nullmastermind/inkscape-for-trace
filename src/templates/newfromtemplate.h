#ifndef GTKMM_NEW_FROM_TEMPLATE_H
#define GTKMM_TNEW_FROM_TEMPLATE_H

#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/notebook.h>

#include "templateloadtab.h"
#include "statictemplateloadtab.h"


class NewFromTemplate : public Gtk::Dialog
{
public:
    NewFromTemplate();
    
private:
    Gtk::Notebook mainWidget;
    Gtk::Button createButton;
    StaticTemplateLoadTab tab1;
    TemplateLoadTab tab2;
    
    void createFromTemplate();
};
#endif