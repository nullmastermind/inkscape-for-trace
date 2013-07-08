/** @file
 * @brief New From Template static templates tab
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński  
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef INKSCAPE_SEEN_UI_DIALOG_STATIC_TEMPLATE_LOAD_TAB_H
#define INKSCAPE_SEEN_UI_DIALOG_STATIC_TEMPLATE_LOAD_TAB_H

#include "template-load-tab.h"

#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>


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
