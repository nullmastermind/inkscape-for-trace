#ifndef SEEN_SP_COLOR_NOTEBOOK_H
#define SEEN_SP_COLOR_NOTEBOOK_H

/*
 * A notebook with RGB, CMYK, CMS, HSL, and Wheel pages
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * This code is in public domain
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if GLIBMM_DISABLE_DEPRECATED && HAVE_GLIBMM_THREADS_H
#include <glibmm/threads.h>
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <gtk/gtk.h>
#include <glib.h>

#include "../color.h"
#include "sp-color-selector.h"
#include "ui/selected-color.h"


struct SPColorNotebook;

class ColorNotebook: public ColorSelector
{
public:
    ColorNotebook( SPColorSelector* csel );
    virtual ~ColorNotebook();

    virtual void init();

    SPColorSelector* getCurrentSelector();
    void switchPage( GtkNotebook *notebook, GtkWidget *page, guint page_num );

protected:
    struct Page {
        Page(Inkscape::UI::ColorSelectorFactory *selector_factory, bool enabled_full);

        Inkscape::UI::ColorSelectorFactory *selector_factory;
        bool enabled_full;
    };

protected:
    static void _rgbaEntryChangedHook( GtkEntry* entry, SPColorNotebook *colorbook );
    static void _entryGrabbed( SPColorSelector *csel, SPColorNotebook *colorbook );
    static void _entryDragged( SPColorSelector *csel, SPColorNotebook *colorbook );
    static void _entryReleased( SPColorSelector *csel, SPColorNotebook *colorbook );
    static void _entryChanged( SPColorSelector *csel, SPColorNotebook *colorbook );
    static void _entryModified( SPColorSelector *csel, SPColorNotebook *colorbook );
    static void _buttonClicked(GtkWidget *widget,  SPColorNotebook *colorbook);
    static void _picker_clicked(GtkWidget *widget,  SPColorNotebook *colorbook);

    virtual void _colorChanged();

    virtual void _onSelectedColorChanged();

    void _updateRgbaEntry( const SPColor& color, gfloat alpha );
    void _setCurrentPage(int i);

    GtkWidget* _addPage(Page& page);

    Inkscape::UI::SelectedColor _selected_color;
    gboolean _updating : 1;
    gboolean _updatingrgba : 1;
    gboolean _dragging : 1;
    gulong _switchId;
    gulong _entryId;
    GtkWidget *_book;
    GtkWidget *_buttonbox;
    GtkWidget **_buttons;
    GtkWidget *_rgbal; /* RGBA entry */
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    GtkWidget *_box_outofgamut, *_box_colormanaged, *_box_toomuchink;
#endif //defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    GtkWidget *_btn_picker;
    GtkWidget *_p; /* Color preview */
    boost::ptr_vector<Page> _available_pages;

private:
    // By default, disallow copy constructor and assignment operator
    ColorNotebook( const ColorNotebook& obj );
    ColorNotebook& operator=( const ColorNotebook& obj );

    /* Following methods support the pop-up menu to choose
     * active color selectors (notebook tabs). This function
     * is not used in Inkscape. If you want to re-enable it you have to
     *  * port the code to c++
     *  * fix it so it remembers its settings in prefs
     *  * fix it so it does not take that much space (entire vertical column!)
     * Current class design supports dynamic addtion and removal of color selectors
     *
    GtkWidget* addPage( GType page_type, guint submode );
    void removePage( GType page_type, guint submode );
    GtkWidget* getPage( GType page_type, guint submode );
    gint menuHandler( GdkEvent* event );

    */
};





#define SP_TYPE_COLOR_NOTEBOOK (sp_color_notebook_get_type ())
#define SP_COLOR_NOTEBOOK(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_COLOR_NOTEBOOK, SPColorNotebook))
#define SP_COLOR_NOTEBOOK_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), SP_TYPE_COLOR_NOTEBOOK, SPColorNotebookClass))
#define SP_IS_COLOR_NOTEBOOK(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_COLOR_NOTEBOOK))
#define SP_IS_COLOR_NOTEBOOK_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), SP_TYPE_COLOR_NOTEBOOK))

struct SPColorNotebook {
    SPColorSelector parent;    /* Parent */
};

struct SPColorNotebookClass {
    SPColorSelectorClass parent_class;

    void (* grabbed) (SPColorNotebook *rgbsel);
    void (* dragged) (SPColorNotebook *rgbsel);
    void (* released) (SPColorNotebook *rgbsel);
    void (* changed) (SPColorNotebook *rgbsel);
};

GType sp_color_notebook_get_type(void);

GtkWidget *sp_color_notebook_new (void);

/* void sp_color_notebook_set_mode (SPColorNotebook *csel, SPColorNotebookMode mode); */
/* SPColorNotebookMode sp_color_notebook_get_mode (SPColorNotebook *csel); */



#endif // SEEN_SP_COLOR_NOTEBOOK_H

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

