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
#include "selection-chemistry.h"

#include <gtkmm/colorbutton.h>
#include <gdkmm/general.h>
#include <gtkmm/checkbutton.h>

using namespace Inkscape::Filters;

namespace Inkscape::UI::Dialog {

FilterEditorDialog::FilterEditorDialog() {
     
    builder = Gtk::Builder::create_from_file("/home/mc/Desktop/test.glade");
    builder->get_widget("FilterEditor", FilterEditor);
    builder->get_widget("FilterList",   FilterList);
    builder->get_widget("FilterFERX",   FilterFERX);
    builder->get_widget("FilterFERY",   FilterFERY);
    builder->get_widget("FilterFERH",   FilterFERH);
    builder->get_widget("FilterFERW",   FilterFERW);
    builder->get_widget("FilterPreview",FilterPreview);
    builder->get_widget("FilterStore",  FilterStore);
    builder->get_widget("FilterPrimitiveDescImage", FilterPrimitiveDescImage);
    //builder->get_widget("FilterPrimitiveParameters",FilterPrimitiveParameters);
    builder->get_widget("FilterPrimitiveDescText",  FilterPrimitiveDescText);
    builder->get_widget("FilterPrimitiveList",      FilterPrimitiveList);
    builder->get_widget("FilterPrimitiveAdd",       FilterPrimitiveAdd);
    if (!(FilterList && FilterFERX && FilterFERY && FilterFERH && FilterFERW && FilterPreview
        && FilterStore && FilterPrimitiveDescImage && FilterPrimitiveDescText && FilterPrimitiveList
        && FilterPrimitiveAdd )) {
        g_warning("Some widget does not exist!");
    }
    _getContents()->add(*FilterEditor);

}
FilterEditorDialog::~FilterEditorDialog(){}






} // namespace Inkscape::UI::Dialog

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
