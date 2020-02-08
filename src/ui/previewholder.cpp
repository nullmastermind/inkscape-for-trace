// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple interface for previewing representations.
 *
 * Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2005 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "previewable.h"
#include "previewholder.h"

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/grid.h>

#define COLUMNS_FOR_SMALL 16
#define COLUMNS_FOR_LARGE 8
//#define COLUMNS_FOR_SMALL 48
//#define COLUMNS_FOR_LARGE 32


namespace Inkscape {
namespace UI {


PreviewHolder::PreviewHolder() :
    Bin(),
    _scroller(nullptr),
    _insides(nullptr),
    _prefCols(0),
    _updatesFrozen(false),
    _anchor(SP_ANCHOR_CENTER),
    _baseSize(UI::Widget::PREVIEW_SIZE_SMALL),
    _ratio(100),
    _view(UI::Widget::VIEW_TYPE_LIST),
    _wrap(false),
    _border(UI::Widget::BORDER_NONE)
{
    set_name( "PreviewHolder" );
    _scroller = Gtk::manage(new Gtk::ScrolledWindow());
    _scroller->set_name( "PreviewHolderScroller" );
    _scroller->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _insides = Gtk::manage(new Gtk::Grid());
    _insides->set_name( "PreviewHolderGrid" );
    _insides->set_column_spacing(8);

    _scroller->set_hexpand();
    _scroller->set_vexpand();
    _scroller->add( *_insides );

    // Disable overlay scrolling as the scrollbar covers up swatches.
    // For some reason this also makes the height 55px.
    _scroller->set_overlay_scrolling(false);

    add(*_scroller);
}

PreviewHolder::~PreviewHolder()
= default;

/**
 * Translates vertical scrolling into horizontal
 */
bool PreviewHolder::on_scroll_event(GdkEventScroll *event)
{
    if (_wrap) {
        return FALSE;
    }

    // Scroll horizontally by page on mouse wheel
    auto adj = _scroller->get_hadjustment();

    if (!adj) {
        return FALSE;
    }

    double move;
    switch (event->direction) {
        case GDK_SCROLL_UP:
        case GDK_SCROLL_LEFT:
            move = -adj->get_page_size();
            break;
        case GDK_SCROLL_DOWN:
        case GDK_SCROLL_RIGHT:
            move =  adj->get_page_size();
            break;
        case GDK_SCROLL_SMOOTH:
            if (fabs(event->delta_y) <= fabs(event->delta_x)) {
                return FALSE;
            }
#ifdef GDK_WINDOWING_QUARTZ
            move = event->delta_y;
#else
            move = event->delta_y * adj->get_page_size();
#endif
            break;
        default:
            return FALSE;
    }

    double value = adj->get_value() + move;

    adj->set_value(value);

    return TRUE;
}

void PreviewHolder::clear()
{
    items.clear();
    _prefCols = 0;
    // Kludge to restore scrollbars
    if ( !_wrap && (_view != UI::Widget::VIEW_TYPE_LIST) && (_anchor == SP_ANCHOR_NORTH || _anchor == SP_ANCHOR_SOUTH) ) {
        _scroller->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER );
    }
    rebuildUI();
}

/**
 * Add a Previewable item to the PreviewHolder
 *
 * \param[in] preview The Previewable item to add
 */
void PreviewHolder::addPreview( Previewable* preview )
{
    items.push_back(preview);
    if ( !_updatesFrozen )
    {
        int i = items.size() - 1;

        switch(_view) {
            case UI::Widget::VIEW_TYPE_LIST:
                {
                    Gtk::Widget* label = Gtk::manage(preview->getPreview(UI::Widget::PREVIEW_STYLE_BLURB,
                                                                         UI::Widget::VIEW_TYPE_LIST,
                                                                         _baseSize, _ratio, _border));
                    Gtk::Widget* item = Gtk::manage(preview->getPreview(UI::Widget::PREVIEW_STYLE_PREVIEW,
                                                                        UI::Widget::VIEW_TYPE_LIST,
                                                                        _baseSize, _ratio, _border));

                    item->set_hexpand();
                    item->set_vexpand();
                    _insides->attach(*item, 0, i, 1, 1);

                    label->set_hexpand();
                    label->set_valign(Gtk::ALIGN_CENTER);
                    _insides->attach(*label, 1, i, 1, 1);
                }

                break;
            case UI::Widget::VIEW_TYPE_GRID:
                {
                    Gtk::Widget* item = Gtk::manage(items[i]->getPreview(UI::Widget::PREVIEW_STYLE_PREVIEW,
                                                                         UI::Widget::VIEW_TYPE_GRID,
                                                                         _baseSize, _ratio, _border));

                    int ncols = 1;
                    int nrows = 1;
                    int col = 0;
                    int row = 0;

                    // To get size
		    auto kids = _insides->get_children();
		    int childCount = (int)kids.size();
                    if (childCount > 0 ) {

                        // Need already shown widget
                        calcGridSize( kids[0], items.size()+1, ncols, nrows );

                        // Column and row for the new widget
                        col = i % ncols;
                        row = i / ncols;

                    }

		    // Loop through the existing widgets and move them to new location
		    for ( int j = 1; j < childCount; j++ ) {
			    auto target = kids[childCount - (j + 1)];
			    int col2 = j % ncols;
			    int row2 = j / ncols;
			    _insides->remove( *target );

			    target->set_hexpand();
			    target->set_vexpand();
			    _insides->attach( *target, col2, row2, 1, 1);
		    }
		    item->set_hexpand();
		    item->set_vexpand();
		    _insides->attach(*item, col, row, 1, 1);
                }
        }

        _scroller->show_all_children();
    }
}

void PreviewHolder::freezeUpdates()
{
    _updatesFrozen = true;
}

void PreviewHolder::thawUpdates()
{
    _updatesFrozen = false;
    rebuildUI();
}

void
PreviewHolder::setStyle(UI::Widget::PreviewSize size,
                        UI::Widget::ViewType    view,
                        guint                   ratio,
                        UI::Widget::BorderStyle border )
{
    if ( size != _baseSize || view != _view || ratio != _ratio || border != _border ) {
        _baseSize = size;
        _view = view;
        _ratio = ratio;
        _border = border;
        // Kludge to restore scrollbars
        if ( !_wrap && (_view != UI::Widget::VIEW_TYPE_LIST) && (_anchor == SP_ANCHOR_NORTH || _anchor == SP_ANCHOR_SOUTH) ) {
            _scroller->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER );
        }
        rebuildUI();
    }
}

void PreviewHolder::setOrientation(SPAnchorType anchor)
{
    if ( _anchor != anchor )
    {
        _anchor = anchor;
        switch ( _anchor )
        {
            case SP_ANCHOR_NORTH:
            case SP_ANCHOR_SOUTH:
            {
                _scroller->set_policy( Gtk::POLICY_AUTOMATIC, _wrap ? Gtk::POLICY_AUTOMATIC : Gtk::POLICY_NEVER );
            }
            break;

            case SP_ANCHOR_EAST:
            case SP_ANCHOR_WEST:
            {
                _scroller->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC );
            }
            break;

            default:
            {
                _scroller->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
            }
        }
        rebuildUI();
    }
}

void PreviewHolder::setWrap( bool wrap )
{
    if (_wrap != wrap) {
        _wrap = wrap;
        switch ( _anchor )
        {
            case SP_ANCHOR_NORTH:
            case SP_ANCHOR_SOUTH:
            {
                _scroller->set_policy( Gtk::POLICY_AUTOMATIC, _wrap ? Gtk::POLICY_AUTOMATIC : Gtk::POLICY_NEVER );
            }
            break;
            default:
            {
                (void)0;
                // do nothing;
            }
        }
        rebuildUI();
    }
}

void PreviewHolder::setColumnPref( int cols )
{
    _prefCols = cols;
}


/**
 * Calculate the grid side of a preview holder
 *
 * \param[in]  item       A sample preview widget.
 * \param[in]  itemCount  The number of items to pack into the grid.
 * \param[out] ncols      The number of columns in grid.
 * \param[out] nrows      The number of rows in grid.
 */
void PreviewHolder::calcGridSize( const Gtk::Widget* item, int itemCount, int& ncols, int& nrows )
{
    // Initially set all items in a horizontal row
    ncols = itemCount;
    nrows = 1;

    if ( _anchor == SP_ANCHOR_SOUTH || _anchor == SP_ANCHOR_NORTH ) {
        Gtk::Requisition req;
        Gtk::Requisition req_natural;
        _scroller->get_preferred_size(req, req_natural);
        int currW = _scroller->get_width();
        if ( currW > req.width ) {
            req.width = currW;
        }

        if (_wrap && item != nullptr) {

            // Get width of bar.
            int width_scroller = _scroller->get_width();

            // Get width of one item (must be visible).
            int minimum_width_item = 0;
            int natural_width_item = 0;
            item->get_preferred_width(minimum_width_item, natural_width_item);

            // Calculate columns and rows.
            if (natural_width_item < 1) {
                natural_width_item = 1;
            }
            ncols = width_scroller / natural_width_item - 1;

            // On first run, scroller width is not set correct... so we need to fudge it:
            if (ncols < 2) {
                ncols = itemCount/2;
                nrows = 2;
            } else {
                nrows = itemCount / ncols;
            }
        }
    } else {
        ncols = (_baseSize == UI::Widget::PREVIEW_SIZE_SMALL || _baseSize == UI::Widget::PREVIEW_SIZE_TINY) ?
            COLUMNS_FOR_SMALL : COLUMNS_FOR_LARGE;
        if ( _prefCols > 0 ) {
            ncols = _prefCols;
        }
        nrows = (itemCount + (ncols - 1)) / ncols;
        if ( nrows < 1 ) {
            nrows = 1;
        }
    }
}

void PreviewHolder::rebuildUI()
{
    auto children = _insides->get_children();
    for (auto child : children) {
        _insides->remove(*child);
        delete child;
    }

    _insides->set_column_spacing(0);
    _insides->set_row_spacing(0);
    if (_border == UI::Widget::BORDER_WIDE) {
        _insides->set_column_spacing(1);
        _insides->set_row_spacing(1);
    }

    switch (_view) {
        case UI::Widget::VIEW_TYPE_LIST:
        {
            _insides->set_column_spacing(8);

            for ( unsigned int i = 0; i < items.size(); i++ ) {
                Gtk::Widget* label = Gtk::manage(items[i]->getPreview(UI::Widget::PREVIEW_STYLE_BLURB, _view, _baseSize, _ratio, _border));
                //label->set_alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER);

                Gtk::Widget* item = Gtk::manage(items[i]->getPreview(UI::Widget::PREVIEW_STYLE_PREVIEW, _view, _baseSize, _ratio, _border));

                item->set_hexpand();
                item->set_vexpand();
                _insides->attach(*item, 0, i, 1, 1);

                label->set_hexpand();
                label->set_valign(Gtk::ALIGN_CENTER);
                _insides->attach(*label, 1, i, 1, 1);
            }
        }
        break;

        case UI::Widget::VIEW_TYPE_GRID:
        {
            int col = 0;
            int row = 0;
            int ncols = 2;
            int nrows = 1;

            for ( unsigned int i = 0; i < items.size(); i++ ) {

                // If this is the last row, flag so the previews can draw a bottom
                UI::Widget::BorderStyle border = ((row == nrows -1) && (_border == UI::Widget::BORDER_SOLID)) ?
                    UI::Widget::BORDER_SOLID_LAST_ROW : _border;

                Gtk::Widget* item = Gtk::manage(items[i]->getPreview(UI::Widget::PREVIEW_STYLE_PREVIEW, _view, _baseSize, _ratio, border));
                item->set_hexpand();
                item->set_vexpand();

                if (i == 0) {
                    // We need one item shown before we can call calcGridSize()...
                    _insides->attach( *item, 0, 0, 1, 1);
                    _scroller->show_all_children();
                    calcGridSize( item, items.size(), ncols, nrows );
                } else {
                    // We've already calculated grid size.
                    _insides->attach( *item, col, row, 1, 1);
                }

                if ( ++col >= ncols ) {
                    col = 0;
                    row++;
                }
            }
        }
    }

    _scroller->show_all_children();
}





} //namespace UI
} //namespace Inkscape


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
