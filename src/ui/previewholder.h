
#ifndef SEEN_PREVIEW_HOLDER_H
#define SEEN_PREVIEW_HOLDER_H
/*
 * A simple interface for previewing representations.
 * Used by Swatches
 *
 * Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2005 Jon A. Cruz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtkmm/bin.h>

namespace Gtk {
class Grid;
class ScrolledWindow;
}

#include "../widgets/eek-preview.h"
#include "enums.h"

namespace Inkscape {
namespace UI {

class Previewable;

class PreviewHolder : public Gtk::Bin
{
public:
    PreviewHolder();
    ~PreviewHolder() override;

    virtual void clear();
    virtual void addPreview( Previewable* preview );
    virtual void freezeUpdates();
    virtual void thawUpdates();
    virtual void setStyle( ::PreviewSize size, ViewType view, guint ratio, ::BorderStyle border );
    virtual void setOrientation(SPAnchorType how);
    virtual int getColumnPref() const { return _prefCols; }
    virtual void setColumnPref( int cols );
    virtual ::PreviewSize getPreviewSize() const { return _baseSize; }
    virtual ViewType getPreviewType() const { return _view; }
    virtual guint getPreviewRatio() const { return _ratio; }
    virtual ::BorderStyle getPreviewBorder() const { return _border; }
    virtual void setWrap( bool wrap );
    virtual bool getWrap() const { return _wrap; }

protected:
    bool on_scroll_event(GdkEventScroll*) override;

private:
    void rebuildUI();
    void calcGridSize( const Gtk::Widget* item, int itemCount, int& ncols, int& nrows );

    std::vector<Previewable*> items;
    Gtk::ScrolledWindow *_scroller;
    Gtk::Grid *_insides;

    int _prefCols;
    bool _updatesFrozen;
    SPAnchorType _anchor;
    ::PreviewSize _baseSize;
    guint _ratio;
    ViewType _view;
    bool _wrap;
    ::BorderStyle _border;
};

} //namespace UI
} //namespace Inkscape

#endif // SEEN_PREVIEW_HOLDER_H

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
