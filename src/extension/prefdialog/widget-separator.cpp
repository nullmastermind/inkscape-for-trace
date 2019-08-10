// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Separator widget for extensions
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "widget-separator.h"

#include <gtkmm/separator.h>

#include "xml/node.h"
#include "extension/extension.h"

namespace Inkscape {
namespace Extension {


WidgetSeparator::WidgetSeparator(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxWidget(xml, ext)
{
}

/** \brief  Create a label for the description */
Gtk::Widget *WidgetSeparator::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::Separator *separator = Gtk::manage(new Gtk::Separator());
    separator->show();

    return dynamic_cast<Gtk::Widget *>(separator);
}

}  /* namespace Extension */
}  /* namespace Inkscape */
