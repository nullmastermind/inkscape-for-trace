// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Filter Effects dialog.
 */
/* Authors:
 *   Marc Jeanmougin
 *
 * Copyright (C) 2017 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/convert.h>
#include <glibmm/error.h>
#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <glibmm/stringutils.h>
#include "io/sys.h"
#include "io/resource.h"

namespace Inkscape {
namespace UI {

LPESelector::LPESelector() : Gtk::Box()
{
     
    const std::string req_widgets[] = {"LPESelector", "FilterList", "FilterFERX", "FilterFERY", "FilterFERH", "FilterFERW", "FilterPreview", "FilterPrimitiveDescImage", "FilterPrimitiveList", "FilterPrimitiveDescText", "FilterPrimitiveAdd"};
    Glib::ustring gladefile = get_filename(UIS, "lpe-selector.glade");
    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch(const Glib::Error& ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }

    Gtk::Object* test;
    for(std::string w:req_widgets) {
        builder->get_widget(w,test);
        if(!test){
            g_warning("Required widget %s does not exist", w.c_str());
            return;
            }
    }

    builder->get_widget("LPESelector", LPESelector);
    _getContents()->add(*LPESelector);

}
LPESelector::~LPESelector()= default;






} // Never put these namespaces together unless you are using gcc 6+
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
