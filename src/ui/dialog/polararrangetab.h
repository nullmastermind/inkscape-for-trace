/**
 * @brief Arranges Objects into a Circle/Ellipse
 */
/* Authors:
 *   Declara Denis
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef INKSCAPE_UI_DIALOG_POLAR_ARRANGE_TAB_H
#define INKSCAPE_UI_DIALOG_POLAR_ARRANGE_TAB_H

#include <gtkmm.h>

#include "ui/dialog/arrangetab.h"

#include "ui/widget/anchor-selector.h"
#include "ui/widget/scalar-unit.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

class ArrangeDialog;

class PolarArrangeTab : public ArrangeTab {
public:
	PolarArrangeTab(ArrangeDialog *parent_);
    virtual ~PolarArrangeTab() {};

    /**
     * Do the actual work
     */
    virtual void arrange();

    /**
     * Respond to selection change
     */
    void updateSelection();

    void on_anchor_radio_changed();
    void on_arrange_radio_changed();

private:
    PolarArrangeTab(PolarArrangeTab const &d); // no copy
    void operator=(PolarArrangeTab const &d); // no assign

    ArrangeDialog         *parent;

    Gtk::Label             anchorPointLabel;

    Gtk::RadioButtonGroup  anchorRadioGroup;
    Gtk::RadioButton       anchorBoundingBoxRadio;
    Gtk::RadioButton       anchorObjectPivotRadio;
    AnchorSelector         anchorSelector;

    Gtk::Label             arrangeOnLabel;

    Gtk::RadioButtonGroup  arrangeRadioGroup;
    Gtk::RadioButton       arrangeOnFirstCircleRadio;
    Gtk::RadioButton       arrangeOnLastCircleRadio;
    Gtk::RadioButton       arrangeOnParametersRadio;

    Gtk::Table             parametersTable;

    Gtk::Label             centerLabel;
    Inkscape::UI::Widget::ScalarUnit centerY;
    Inkscape::UI::Widget::ScalarUnit centerX;

    Gtk::Label             radiusLabel;
    Inkscape::UI::Widget::ScalarUnit radiusY;
    Inkscape::UI::Widget::ScalarUnit radiusX;

    Gtk::Label             angleLabel;
    Inkscape::UI::Widget::ScalarUnit angleY;
    Inkscape::UI::Widget::ScalarUnit angleX;

    Gtk::CheckButton       rotateObjectsCheckBox;


};

} //namespace Dialog
} //namespace UI
} //namespace Inkscape

#endif /* INKSCAPE_UI_DIALOG_POLAR_ARRANGE_TAB_H */
