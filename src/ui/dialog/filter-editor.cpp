/**
 * @file
 * Filter Effects dialog.
 */
/* Authors:
 *   Marc Jeanmougin
 *
 * Copyright (C) 2017 Authors
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dialog-manager.h"

#include <gdkmm/display.h>
#if GTK_CHECK_VERSION(3, 20, 0)
# include <gdkmm/seat.h>
#else
# include <gdkmm/devicemanager.h>
#endif

#include "ui/widget/spinbutton.h"

#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <glibmm/main.h>
#include <glibmm/convert.h>
#include <glibmm/error.h>

#include "desktop.h"

#include "document.h"
#include "document-undo.h"
#include "filter-chemistry.h"
#include "filter-editor.h"
#include "filter-enums.h"
#include "inkscape.h"
#include "filters/blend.h"
#include "filters/colormatrix.h"
#include "filters/componenttransfer.h"
#include "filters/componenttransfer-funcnode.h"
#include "filters/convolvematrix.h"
#include "filters/distantlight.h"
#include "filters/merge.h"
#include "filters/mergenode.h"
#include "filters/pointlight.h"
#include "filters/spotlight.h"

#include "style.h"
#include "svg/svg-color.h"
#include "ui/dialog/filedialog.h"
#include "verbs.h"

#include "io/sys.h"
#include "io/resource.h"
#include "selection-chemistry.h"

#include <string>

#include <gtkmm.h>

using namespace Inkscape::Filters;
using namespace Inkscape::IO::Resource;
namespace Inkscape {
namespace UI {
namespace Dialog {

FilterEditorDialog::FilterEditorDialog() : UI::Widget::Panel("", "/dialogs/filtereffects", SP_VERB_DIALOG_FILTER_EFFECTS)
{
     
    const std::string req_widgets[] = {"FilterEditor", "FilterList", "FilterFERX", "FilterFERY", "FilterFERH", "FilterFERW", "FilterPreview", "FilterPrimitiveDescImage", "FilterPrimitiveList", "FilterPrimitiveDescText", "FilterPrimitiveAdd"};
    Glib::ustring gladefile = get_filename(UIS, "filter-editor.glade");
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

    builder->get_widget("FilterEditor", FilterEditor);
    _getContents()->add(*FilterEditor);

//test
    Gtk::ComboBox *OptionList;
    builder->get_widget("OptionList",OptionList);
    FilterStore = builder->get_object("FilterStore");
    Glib::RefPtr<Gtk::ListStore> fs = Glib::RefPtr<Gtk::ListStore>::cast_static(FilterStore); 
    Gtk::TreeModel::Row row = *(fs->append());





}
FilterEditorDialog::~FilterEditorDialog(){}






} // Never put these namespaces together unless you are using gcc 6+
}
} // P.S. This is for Inkscape::UI::Dialog

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
