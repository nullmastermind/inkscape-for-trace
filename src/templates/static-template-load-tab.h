#ifndef INKSCAPE_SEEN_UI_DIALOG_STATIC_TEMPLATE_LOAD_TAB_H
#define INKSCAPE_SEEN_UI_DIALOG_STATIC_TEMPLATE_LOAD_TAB_H

#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

#include "template-load-tab.h"

namespace Inkscape {
namespace UI {
    

class StaticTemplateLoadTab : public TemplateLoadTab
{
public:
    StaticTemplateLoadTab();
    virtual void createTemplate();
    
protected:
    virtual void _displayTemplateInfo();
    
    Gtk::Button _more_info_button;
    Gtk::Label _short_description_label;
    Gtk::Label _template_author_label;
    Gtk::Label _template_name_label;
    Gtk::Image _preview_image;
};

}
}

#endif
