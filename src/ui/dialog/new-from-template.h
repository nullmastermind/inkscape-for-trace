#ifndef INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H
#define INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H

#include <gtkmm/dialog.h>
#include <gtkmm/button.h>
#include <gtkmm/notebook.h>

#include "template-load-tab.h"
#include "static-template-load-tab.h"


namespace Inkscape {
namespace UI {
    

class NewFromTemplate : public Gtk::Dialog
{
public:
    static void load_new_from_template();
    
private:
    NewFromTemplate();
    Gtk::Notebook _main_widget;
    Gtk::Button _create_template_button;
    StaticTemplateLoadTab _tab1;
    TemplateLoadTab _tab2;
    
    void _createFromTemplate();
};

}
}
#endif
