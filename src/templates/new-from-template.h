#ifndef INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H
#define INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H

#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/notebook.h>

#include "template-load-tab.h"
#include "static-template-load-tab.h"

namespace Inkscape {
namespace UI {
    

class NewFromTemplate : public Gtk::Dialog
{
public:
    NewFromTemplate();
    
private:
    Gtk::Notebook _mainWidget;
    Gtk::Button _createButton;
    StaticTemplateLoadTab _tab1;
    TemplateLoadTab _tab2;
    
    void _createFromTemplate();
};

}
}
#endif
