#include "newfromtemplate.h"
#include "gtkmm/alignment.h"


NewFromTemplate::NewFromTemplate() :
    createButton("Create from template")
{
    set_title("New From Template");
    resize(400, 250);
    
    get_vbox()->pack_start(mainWidget);
    mainWidget.append_page(tab1, "Static Templates");
    mainWidget.append_page(tab2, "Procedural Templates");
    
    Gtk::Alignment *align;
    align = manage(new Gtk::Alignment(Gtk::ALIGN_END, Gtk::ALIGN_CENTER, 0.0, 0.0));
    get_vbox()->pack_end(*align, Gtk::PACK_SHRINK, 5);
    align->add(createButton);
    
    createButton.signal_pressed().connect(
    sigc::mem_fun(*this, &NewFromTemplate::createFromTemplate));
   
    show_all();
}


void NewFromTemplate::createFromTemplate()
{
    if (mainWidget.get_current_page() == 0)
        tab1.createTemplate();
    else
        tab2.createTemplate();
    
    response(0);
}