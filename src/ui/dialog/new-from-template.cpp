// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief New From Template main dialog - implementation
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "new-from-template.h"
#include "file.h"

#include "include/gtkmm_version.h"

namespace Inkscape {
namespace UI {


NewFromTemplate::NewFromTemplate()
    : _create_template_button(_("Create from template"))
{
    set_title(_("New From Template"));
    resize(400, 400);

    _main_widget = new TemplateLoadTab(this);

    get_content_area()->pack_start(*_main_widget);
   
    _create_template_button.set_halign(Gtk::ALIGN_END);
    _create_template_button.set_valign(Gtk::ALIGN_END);
    _create_template_button.set_margin_end(15);

    get_content_area()->pack_end(_create_template_button, Gtk::PACK_SHRINK);
    
    _create_template_button.signal_clicked().connect(
    sigc::mem_fun(*this, &NewFromTemplate::_createFromTemplate));
    _create_template_button.set_sensitive(false);
   
    show_all();
}

NewFromTemplate::~NewFromTemplate()
{
    delete _main_widget;
}

void NewFromTemplate::setCreateButtonSensitive(bool value)
{
    _create_template_button.set_sensitive(value);
}

void NewFromTemplate::_createFromTemplate()
{
    _main_widget->createTemplate();
    _onClose();
}

void NewFromTemplate::_onClose()
{
    response(0);
}

void NewFromTemplate::load_new_from_template()
{
    NewFromTemplate dl;
    dl.run();
}

}
}
