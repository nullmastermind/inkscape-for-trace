/** @file
 * @brief Dialog for creating grid type arrangements of selected objects
 */
/* Authors:
 *   Bob Jamison ( based off trace dialog)
 *   John Cliff
 *   Other dudes from The Inkscape Organization
 *
 * Copyright (C) 2004 Bob Jamison
 * Copyright (C) 2004 John Cliff
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_UI_DIALOG_TILE_H
#define SEEN_UI_DIALOG_TILE_H

#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/radiobutton.h>

#include "ui/widget/anchor-selector.h"
#include "ui/widget/panel.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/scalar-unit.h"

namespace Gtk {
class Button;
}

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * This interface should be implemented by each arrange mode.
 * The class is a Gtk::VBox and will be displayed as a tab in
 * the dialog
 */
class ArrangeTab : public Gtk::VBox
{
public:
	ArrangeTab() {};
	virtual ~ArrangeTab() {};

	/**
	 * Do the actual work!
	 */
	virtual void arrange() = 0;
};

class GridArrangeTab;

class ArrangeDialog : public UI::Widget::Panel {
private:
	Gtk::VBox      _arrangeBox;
	Gtk::Notebook  _notebook;

	GridArrangeTab *_gridArrangeTab;

	Gtk::Button    *_arrangeButton;

public:
	ArrangeDialog();
	virtual ~ArrangeDialog() {};

    /**
     * Callback from Apply
     */
    virtual void _apply();

	static ArrangeDialog& getInstance() { return *new ArrangeDialog(); }
};

/**
 * Dialog for tiling an object
 */
class GridArrangeTab : public ArrangeTab {
public:
	GridArrangeTab(ArrangeDialog *parent);
    virtual ~GridArrangeTab() {};

    /**
     * Do the actual work
     */
    virtual void arrange();

    /**
     * Respond to selection change
     */
    void updateSelection();

    /**
     * Callback from Apply
     */
    virtual void _apply();

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
    GridArrangeTab(GridArrangeTab const &d); // no copy
    void operator=(GridArrangeTab const &d); // no assign

    ArrangeDialog         *Parent;

    bool userHidden;
    bool updating;

    Gtk::VBox             TileBox;
    Gtk::Button           *TileOkButton;
    Gtk::Button           *TileCancelButton;

    // Number selected label
    Gtk::Label            SelectionContentsLabel;


    Gtk::HBox             AlignHBox;
    Gtk::HBox             SpinsHBox;

    // Number per Row
    Gtk::VBox             NoOfColsBox;
    Gtk::Label            NoOfColsLabel;
    Inkscape::UI::Widget::SpinButton NoOfColsSpinner;
    bool AutoRowSize;
    Gtk::CheckButton      RowHeightButton;

    Gtk::VBox             XByYLabelVBox;
    Gtk::Label            padXByYLabel;
    Gtk::Label            XByYLabel;

    // Number per Column
    Gtk::VBox             NoOfRowsBox;
    Gtk::Label            NoOfRowsLabel;
    Inkscape::UI::Widget::SpinButton NoOfRowsSpinner;
    bool AutoColSize;
    Gtk::CheckButton      ColumnWidthButton;

    // Alignment
    Gtk::Label            AlignLabel;
    AnchorSelector        AlignmentSelector;
    double VertAlign;
    double HorizAlign;

    Inkscape::UI::Widget::ScalarUnit    XPadding;
    Inkscape::UI::Widget::ScalarUnit    YPadding;

    // BBox or manual spacing
    Gtk::VBox             SpacingVBox;
    Gtk::RadioButtonGroup SpacingGroup;
    Gtk::RadioButton      SpaceByBBoxRadioButton;
    Gtk::RadioButton      SpaceManualRadioButton;
    bool ManualSpacing;

    // Row height
    Gtk::VBox             RowHeightVBox;
    Gtk::HBox             RowHeightBox;
    Gtk::Label            RowHeightLabel;
    Inkscape::UI::Widget::SpinButton RowHeightSpinner;

    // Column width
    Gtk::VBox             ColumnWidthVBox;
    Gtk::HBox             ColumnWidthBox;
    Gtk::Label            ColumnWidthLabel;
    Inkscape::UI::Widget::SpinButton ColumnWidthSpinner;
};


} //namespace Dialog
} //namespace UI
} //namespace Inkscape


#endif /* __TILEDIALOG_H__ */

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
