// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Text-edit
 */
/* Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   John Smith
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Tavmjong Bah
 *
 * Copyright (C) 1999-2013 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_TEXT_EDIT_H
#define INKSCAPE_UI_DIALOG_TEXT_EDIT_H

#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/button.h>
#include <gtkmm/frame.h>
#include <gtkmm/scrolledwindow.h>
#include "ui/widget/panel.h"
#include "ui/widget/frame.h"
#include "ui/dialog/desktop-tracker.h"

#include "ui/widget/font-selector.h"
#include "ui/widget/font-variants.h"

class SPItem;
struct SPFontSelector;
class font_instance;
class SPCSSAttr;

namespace Inkscape {
namespace UI {
namespace Dialog {

#define VB_MARGIN 4
/**
 * The TextEdit class defines the Text and font dialog.
 * 
 * The Text and font dialog allows you to set the font family, style and size
 * and shows a preview of the result. The dialogs layout settings include
 * horizontal and vertical alignment and inter line distance.
 */
class TextEdit : public UI::Widget::Panel {
public:
    TextEdit();
    ~TextEdit() override;

    /**
     * Helper function which returns a new instance of the dialog.
     * getInstance is needed by the dialog manager (Inkscape::UI::Dialog::DialogManager).
     */
    static TextEdit &getInstance() { return *new TextEdit(); }

protected:

    /**
     * Callback for pressing the default button.
     */
    void onSetDefault ();

    /**
     * Callback for pressing the apply button.
     */
    void onApply ();
    void onSelectionChange ();
    void onSelectionModified (guint flags);
    
    /**
     * Called whenever something 'changes' on canvas.
     * 
     * onReadSelection gets the currently selected item from the canvas and sets all the controls in this dialog to the correct state.
     * 
     * @param dostyle Indicates whether the modification of the user includes a style change.
     * @param content Indicates whether the modification of the user includes a style change. Actually refers to the question if we do want to show the content? (Parameter currently not used)
     */
    void onReadSelection (gboolean style, gboolean content);
    
    /**
     * Callback invoked when the user modifies the text of the selected text object.
     *
     * onTextChange is responsible for initiating the commands after the user
     * modified the text in the selected object. The UI of the dialog is
     * updated. The subfunction setPreviewText updates the preview label.
     *
     * @param self pointer to the current instance of the dialog.
     */
    void onChange ();
    void onFontFeatures (Gtk::Widget * widgt, int pos);
    static void onTextChange (GtkTextBuffer *text_buffer, TextEdit *self);
    
    /**
     * Callback invoked when the user modifies the font through the dialog or the tools control bar.
     *
     * onFontChange updates the dialog UI. The subfunction setPreviewText updates the preview label.
     *
     * @param fontspec for the text to be previewed.
     */
    void onFontChange (Glib::ustring fontspec);

    /**
     * Get the selected text off the main canvas.
     *
     * @return SPItem pointer to the selected text object
     */
    SPItem *getSelectedTextItem ();

    /**
     * Count the number of text objects in the selection on the canvas.
     */
    unsigned getSelectedTextCount ();

    /**
     * Helper function to create markup from a fontspec and display in the preview label.
     * 
     * @param fontspec for the text to be previewed.
     * @param font_features for text to be previewed (in CSS format).
     * @param phrase text to be shown.
     */
    void setPreviewText (Glib::ustring font_spec, Glib::ustring font_features, Glib::ustring phrase);

    void updateObjectText ( SPItem *text );
    SPCSSAttr *fillTextStyle ();

    /**
     * Can be invoked for setting the desktop. Currently not used.
     */
    void setDesktop(SPDesktop *desktop) override;

    /**
     * Is invoked by the desktop tracker when the desktop changes.
     *
     * @see DesktopTracker
     */
    void setTargetDesktop(SPDesktop *desktop);



private:

    /*
     * All the dialogs widgets
     */
    Gtk::Notebook notebook;

    // Tab 1: Font ---------------------- //
    Gtk::VBox font_vbox;
    Gtk::Label font_label;

    Inkscape::UI::Widget::FontSelector font_selector;
    Inkscape::UI::Widget::FontVariations font_variations;
    Gtk::Label preview_label;  // Share with variants tab?

    // Tab 2: Text ---------------------- //
    Gtk::VBox text_vbox;
    Gtk::Label text_label;

    Gtk::ScrolledWindow scroller;
    GtkWidget *text_view; // TODO - Convert this to a Gtk::TextView, but GtkSpell doesn't seem to work with it
    GtkTextBuffer *text_buffer;

    // Tab 3: Features  ----------------- //
    Gtk::VBox feat_vbox;
    Inkscape::UI::Widget::FontVariants font_features;
    Gtk::Label feat_label;
    Gtk::Label preview_label2; // Could reparent preview_label but having a second label is probably easier.

    // Shared ------- ------------------ //
    Gtk::HBox button_row;
    Gtk::Button setasdefault_button;
    Gtk::Button close_button;
    Gtk::Button apply_button;

    // Signals
    SPDesktop *desktop;
    DesktopTracker deskTrack;
    sigc::connection desktopChangeConn;
    sigc::connection selectChangedConn;
    sigc::connection subselChangedConn;
    sigc::connection selectModifiedConn;
    sigc::connection fontChangedConn;
    sigc::connection fontFeaturesChangedConn;

    // Other
    double selected_fontsize;
    bool blocked;
    const Glib::ustring samplephrase;


    TextEdit(TextEdit const &d) = delete;
    TextEdit operator=(TextEdit const &d) = delete;
};


} //namespace Dialog
} //namespace UI
} //namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_TEXT_EDIT_H

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
