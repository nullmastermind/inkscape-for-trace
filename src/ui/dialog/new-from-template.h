/** @file
 * @brief New From Template main dialog
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński    
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

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
