// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @brief Arranges Objects into a Grid
 */
/* Authors:
 *   Bob Jamison ( based off trace dialog)
 *   John Cliff
 *   Other dudes from The Inkscape Organization
 *   Abhishek Sharma
 *   Declara Denis
 *
 * Copyright (C) 2004 Bob Jamison
 * Copyright (C) 2004 John Cliff
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_GRID_ARRANGE_TAB_H
#define INKSCAPE_UI_DIALOG_GRID_ARRANGE_TAB_H

#include "ui/widget/scalar-unit.h"
#include "ui/dialog/arrange-tab.h"

#include "ui/widget/anchor-selector.h"
#include "ui/widget/spinbutton.h"

#include <gtkmm/checkbutton.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/radiobuttongroup.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

class ArrangeDialog;

/**
 * Dialog for tiling an object
 */
class GridArrangeTab : public ArrangeTab {
public:
    GridArrangeTab(ArrangeDialog *parent);
    ~GridArrangeTab() override = default;;

    /**
     * Do the actual work
     */
    void arrange() override;

    /**
     * Respond to selection change
     */
    void updateSelection();

    // Callbacks from spinbuttons
    void on_row_spinbutton_changed();
    void on_col_spinbutton_changed();
    void on_xpad_spinbutton_changed();
    void on_ypad_spinbutton_changed();
    void on_RowSize_checkbutton_changed();
    void on_ColSize_checkbutton_changed();
    void on_rowSize_spinbutton_changed();
    void on_colSize_spinbutton_changed();
    void Spacing_button_changed();
    void Align_changed();


private:
    GridArrangeTab(GridArrangeTab const &d) = delete; // no copy
    void operator=(GridArrangeTab const &d) = delete; // no assign

    ArrangeDialog         *Parent;

    bool userHidden;
    bool updating;

    Gtk::Box               TileBox;
    Gtk::Button           *TileOkButton;
    Gtk::Button           *TileCancelButton;

    // Number selected label
    Gtk::Label            SelectionContentsLabel;


    Gtk::Box              AlignHBox;
    Gtk::Box              SpinsHBox;

    // Number per Row
    Gtk::Box              NoOfColsBox;
    Gtk::Label            NoOfColsLabel;
    Inkscape::UI::Widget::SpinButton NoOfColsSpinner;
    bool AutoRowSize;
    Gtk::CheckButton      RowHeightButton;

    Gtk::Box              XByYLabelVBox;
    Gtk::Label            padXByYLabel;
    Gtk::Label            XByYLabel;

    // Number per Column
    Gtk::Box              NoOfRowsBox;
    Gtk::Label            NoOfRowsLabel;
    Inkscape::UI::Widget::SpinButton NoOfRowsSpinner;
    bool AutoColSize;
    Gtk::CheckButton      ColumnWidthButton;

    // Alignment
    Gtk::Label            AlignLabel;
    Inkscape::UI::Widget::AnchorSelector        AlignmentSelector;
    double VertAlign;
    double HorizAlign;

    Inkscape::UI::Widget::UnitMenu      PaddingUnitMenu;
    Inkscape::UI::Widget::ScalarUnit    XPadding;
    Inkscape::UI::Widget::ScalarUnit    YPadding;
    Gtk::Grid                          *PaddingTable;

    // BBox or manual spacing
    Gtk::VBox             SpacingVBox;
    Gtk::RadioButtonGroup SpacingGroup;
    Gtk::RadioButton      SpaceByBBoxRadioButton;
    Gtk::RadioButton      SpaceManualRadioButton;
    bool ManualSpacing;

    // Row height
    Gtk::Box              RowHeightBox;
    Inkscape::UI::Widget::SpinButton RowHeightSpinner;

    // Column width
    Gtk::Box              ColumnWidthBox;
    Inkscape::UI::Widget::SpinButton ColumnWidthSpinner;
};

} //namespace Dialog
} //namespace UI
} //namespace Inkscape

#endif /* INKSCAPE_UI_DIALOG_GRID_ARRANGE_TAB_H */

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
