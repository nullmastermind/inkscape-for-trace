// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 1999-2007 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_EXPORT_H
#define SP_EXPORT_H

#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/expander.h>
#include <gtkmm/grid.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>

#include "extension/output.h"
#include "ui/dialog/dialog-base.h"
#include "ui/widget/scrollprotected.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

class ExportProgressDialog;

/** What type of button is being pressed */
enum selection_type {
    SELECTION_PAGE = 0,  /**< Export the whole page */
    SELECTION_DRAWING,   /**< Export everything drawn on the page */
    SELECTION_SELECTION, /**< Export everything that is selected */
    SELECTION_CUSTOM,    /**< Allows the user to set the region exported */
    SELECTION_NUMBER_OF  /**< A counter for the number of these guys */
};

/**
 * A dialog widget to export to various image formats such as bitmap and png.
 *
 * Creates a dialog window for exporting an image to a bitmap if one doesn't already exist and
 * shows it to the user. If the dialog has already been created, it simply shows the window.
 *
 */
class Export : public DialogBase
{
public:
    Export ();
    ~Export () override;

    static Export &getInstance() {
        return *new Export();
    }

private:

    /**
     * A function to set the xdpi.
     *
     * This function grabs all of the x values and then figures out the
     * new bitmap size based on the changing dpi value.  The dpi value is
     * gotten from the xdpi setting as these can not currently be independent.
     *
     */
    void setImageX();

    /**
     * A function to set the ydpi.
     *
     * This function grabs all of the y values and then figures out the
     * new bitmap size based on the changing dpi value.  The dpi value is
     * gotten from the xdpi setting as these can not currently be independent.
     */
    void setImageY();
    bool bbox_equal(Geom::Rect const &one, Geom::Rect const &two);
    void updateCheckbuttons ();
    inline void findDefaultSelection();
    void detectSize();
    void setArea ( double x0, double y0, double x1, double y1);
    /*
     * Getter/setter style functions for the spinbuttons
     */
    void setValue(Glib::RefPtr<Gtk::Adjustment>& adj, double val);
    void setValuePx(Glib::RefPtr<Gtk::Adjustment>& adj, double val);
    float getValue(Glib::RefPtr<Gtk::Adjustment>& adj);
    float getValuePx(Glib::RefPtr<Gtk::Adjustment>& adj);

    /**
     * Helper function to create, style and pack spinbuttons for the export dialog.
     *
     * Creates a new spin button for the export dialog.
     * @param  key  The name of the spin button
     * @param  val  A default value for the spin button
     * @param  min  Minimum value for the spin button
     * @param  max  Maximum value for the spin button
     * @param  step The step size for the spin button
     * @param  page Size of the page increment
     * @param  t    Table to put the spin button in
     * @param  x    X location in the table \c t to start with
     * @param  y    Y location in the table \c t to start with
     * @param  ll   Text to put on the left side of the spin button (optional)
     * @param  lr   Text to put on the right side of the spin button (optional)
     * @param  digits  Number of digits to display after the decimal
     * @param  sensitive  Whether the spin button is sensitive or not
     * @param  cb   Callback for when this spin button is changed (optional)
     *
     * No unit_selector is stored in the created spinbutton, relies on external unit management
     */
    Glib::RefPtr<Gtk::Adjustment> createSpinbutton( gchar const *key,
            double val, double min, double max, double step, double page,
                                                    Gtk::Grid *t, int x, int y,
                                                    const Glib::ustring& ll, const Glib::ustring& lr,
                                                    int digits, unsigned int sensitive,
                                                    void (Export::*cb)() );

    /**
     * One of the area select radio buttons was pressed
     */
    void onAreaTypeToggled();
    void refreshArea();

    /**
     * Export button callback
     */
    void onExport ();
    void _export_raster(Inkscape::Extension::Output *extension);

    /**
     * File Browse button callback
     */
    void onBrowse ();

    /**
     * Area X value changed callback
     */
    void onAreaX0Change() {
        areaXChange(x0_adj);
    } ;
    void onAreaX1Change() {
        areaXChange(x1_adj);
    } ;
    void areaXChange(Glib::RefPtr<Gtk::Adjustment>& adj);

    /**
     * Area Y value changed callback
     */
    void onAreaY0Change() {
        areaYChange(y0_adj);
    } ;
    void onAreaY1Change() {
        areaYChange(y1_adj);
    } ;
    void areaYChange(Glib::RefPtr<Gtk::Adjustment>& adj);

    /**
     * Unit changed callback
     */
    void onUnitChanged();

    /**
     * Hide except selected callback
     */
    void onHideExceptSelected ();

    /**
     * Area width value changed callback
     */
    void onAreaWidthChange   ();

    /**
     * Area height value changed callback
     */
    void onAreaHeightChange  ();

    /**
     * Bitmap width value changed callback
     */
    void onBitmapWidthChange ();

    /**
     * Bitmap height value changed callback
     */
    void onBitmapHeightChange ();

    /**
     * Export xdpi value changed callback
     */
    void onExportXdpiChange ();

    /**
     * Batch export callback
     */
    void onBatchClicked ();

    /**
     * Inkscape selection change callback
     */
    void onSelectionChanged ();
    void onSelectionModified (guint flags);

    /**
     * Filename modified callback
     */
    void onFilenameModified ();

    /**
     * Can be invoked for setting the desktop. Currently not used.
     */
    void setDesktop(SPDesktop *desktop);

    /**
     * Update active window.
     */
    void update() override;

    /**
     * Creates progress dialog for batch exporting.
     *
     * @param progress_text Text to be shown in the progress bar
     */
    ExportProgressDialog * create_progress_dialog(Glib::ustring progress_text);

    /**
     * Callback to be used in for loop to update the progress bar.
     *
     * @param value number between 0 and 1 indicating the fraction of progress (0.17 = 17 % progress)
     * @param dlg void pointer to the Gtk::Dialog progress dialog
     */
    static unsigned int onProgressCallback(float value, void *dlg);

    /**
     * Callback for pressing the cancel button.
     */
    void onProgressCancel ();

    /**
     * Callback invoked on closing the progress dialog.
     */
    bool onProgressDelete (GdkEventAny *event);

    /**
     * Handles state changes as exporting starts or stops.
     */
    void setExporting(bool exporting, Glib::ustring const &text = "");

    /*
     * Utility filename and path functions
     */
    void set_default_filename ();

    /*
     * Currently selected export area type
     * can be changed by code
     */
    selection_type current_key;
    /*
     * Manually selected export area type(only changed by buttons)
     */
    selection_type manual_key;
    /*
     * Original name for the export object
     */
    Glib::ustring original_name;
    Glib::ustring doc_export_name;
    /*
     * Was the Original name modified
     */
    bool filename_modified;

    /*
     * Flag to stop simultaneous updates
     */
    bool update_flag;

    /* Area selection radio buttons */
    Gtk::Box togglebox;
    Gtk::RadioButton *selectiontype_buttons[SELECTION_NUMBER_OF];

    Gtk::Box area_box;
    Gtk::Box singleexport_box;

    /* Custom size widgets */
    Glib::RefPtr<Gtk::Adjustment> x0_adj;
    Glib::RefPtr<Gtk::Adjustment> x1_adj;
    Glib::RefPtr<Gtk::Adjustment> y0_adj;
    Glib::RefPtr<Gtk::Adjustment> y1_adj;
    Glib::RefPtr<Gtk::Adjustment> width_adj;
    Glib::RefPtr<Gtk::Adjustment> height_adj;

    /* Bitmap size widgets */
    Glib::RefPtr<Gtk::Adjustment> bmwidth_adj;
    Glib::RefPtr<Gtk::Adjustment> bmheight_adj;
    Glib::RefPtr<Gtk::Adjustment> xdpi_adj;
    Glib::RefPtr<Gtk::Adjustment> ydpi_adj;

    Gtk::Box size_box;
    Gtk::Label* bm_label;

    Gtk::Box file_box;
    Gtk::Label *flabel;
    Gtk::Entry filename_entry;

    /* Unit selector widgets */
    Gtk::Box unitbox;
    Inkscape::UI::Widget::UnitMenu unit_selector;
    Gtk::Label units_label;

    /* Filename widgets  */
    Gtk::Box filename_box;
    Gtk::Button browse_button;
    Gtk::Label browse_label;
    Gtk::Image browse_image;

    Gtk::Box batch_box;
    Gtk::CheckButton    batch_export;

    Gtk::Box hide_box;
    Gtk::CheckButton    hide_export;

    Gtk::CheckButton closeWhenDone;

    /* Advanced */
    Gtk::Expander expander;
    Gtk::CheckButton interlacing;
    Gtk::Label                        bitdepth_label;
    Inkscape::UI::Widget::ScrollProtected<Gtk::ComboBoxText> bitdepth_cb;
    Gtk::Label                        zlib_label;
    Inkscape::UI::Widget::ScrollProtected<Gtk::ComboBoxText> zlib_compression;
    Gtk::Label                        pHYs_label;
    Glib::RefPtr<Gtk::Adjustment>     pHYs_adj;
    Inkscape::UI::Widget::ScrollProtected<Gtk::SpinButton> pHYs_sb;
    Gtk::Label                        antialiasing_label;
    Inkscape::UI::Widget::ScrollProtected<Gtk::ComboBoxText> antialiasing_cb;

    /* Export Button widgets */
    Gtk::Box button_box;
    Gtk::Button export_button;

    Gtk::ProgressBar _prog;

    ExportProgressDialog *prog_dlg;
    bool interrupted; // indicates whether export needs to be interrupted (read: user pressed cancel in the progress dialog)

    Inkscape::Preferences *prefs;
    sigc::connection selectChangedConn;
    sigc::connection subselChangedConn;
    sigc::connection selectModifiedConn;
    sigc::connection unitChangedConn;

};

}
}
}
#endif

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
