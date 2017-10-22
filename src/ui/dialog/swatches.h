/** @file
 * @brief Color swatches dialog
 */
/* Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2005 Jon A. Cruz
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SEEN_DIALOGS_SWATCHES_H
#define SEEN_DIALOGS_SWATCHES_H

#include "ui/widget/panel.h"

namespace Gtk {
    class Menu;
    class MenuItem;
    class CheckMenuItem;
}

namespace Inkscape {
namespace UI {

class PreviewFillable;
class PreviewHolder;

namespace Dialogs {

class ColorItem;
class SwatchPage;
class DocTrack;

/**
 * A panel that displays paint swatches.
 *
 * It comes in two flavors, depending on the prefsPath argument passed to
 * the construtor: the default "/dialog/swatches" is just a regular panel;
 * the "/embedded/swatches/" is the horizontal color swatches at the bottom
 * of window.
 */
class SwatchesPanel : public Inkscape::UI::Widget::Panel
{
public:
    SwatchesPanel(gchar const* prefsPath = "/dialogs/swatches");
    virtual ~SwatchesPanel();

    static SwatchesPanel& getInstance();

    virtual void setDesktop( SPDesktop* desktop );
    virtual SPDesktop* getDesktop() {return _currentDesktop;}

    virtual int getSelectedIndex() {return _currentIndex;} // temporary

protected:
    static void handleGradientsChange(SPDocument *document);

    virtual void _updateFromSelection();
    virtual void _setDocument( SPDocument *document );
    virtual void _rebuild();

    virtual std::vector<SwatchPage*> _getSwatchSets() const;

private:
    SwatchesPanel(SwatchesPanel const &); // no copy
    SwatchesPanel &operator=(SwatchesPanel const &); // no assign

    void _build_menu();

    static void _trackDocument( SwatchesPanel *panel, SPDocument *document );
    static void handleDefsModified(SPDocument *document);

    PreviewHolder* _holder;
    ColorItem* _clear;
    ColorItem* _remove;
    int _currentIndex;
    SPDesktop*  _currentDesktop;
    SPDocument* _currentDocument;


    void _setTargetFillable(PreviewFillable *target);
    void _regItem(Gtk::MenuItem* item, int id);

    void _updateSettings(int settings, int value);

    void _wrapToggled(Gtk::CheckMenuItem *toggler);

    Gtk::Menu       *_menu;
    PreviewFillable *_fillable;

    sigc::connection _documentConnection;
    sigc::connection _selChanged;

    friend class DocTrack;
};

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape



#endif // SEEN_SWATCHES_H

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
