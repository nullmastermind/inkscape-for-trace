/*
 * A notebook with RGB, CMYK, CMS, HSL, and Wheel pages
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * This code is in public domain
 */

#undef SPCS_PREVIEW
#define noDUMP_CHANGE_INFO

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "widgets/icon.h"
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstddef>
#include <gtk/gtk.h>
#include <glibmm/i18n.h>
#include <gtkmm/label.h>

#include "../dialogs/dialog-events.h"
#include "../preferences.h"
#include "sp-color-notebook.h"
#include "spw-utilities.h"
#include "sp-color-scales.h"
#include "sp-color-icc-selector.h"
#include "svg/svg-icc-color.h"
#include "../inkscape.h"
#include "../document.h"
#include "../profile-manager.h"
#include "color-profile.h"
#include "cms-system.h"
#include "tools-switch.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/color-entry.h"
#include "ui/widget/color-wheel-selector.h"

using Inkscape::CMSSystem;

using namespace Inkscape::UI::Widget;

struct SPColorNotebookTracker {
    const gchar* name;
    const gchar* className;
    GType type;
    guint submode;
    gboolean enabledFull;
    gboolean enabledBrief;
    SPColorNotebook *backPointer;
};


static void sp_color_notebook_class_init (SPColorNotebookClass *klass);
static void sp_color_notebook_init (SPColorNotebook *colorbook);
static void sp_color_notebook_dispose(GObject *object);

static void sp_color_notebook_show_all (GtkWidget *widget);
static void sp_color_notebook_hide(GtkWidget *widget);

static SPColorSelectorClass *parent_class;

#define XPAD 4
#define YPAD 1

GType sp_color_notebook_get_type(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPColorNotebookClass),
            0, // base_init
            0, // base_finalize
            (GClassInitFunc)sp_color_notebook_class_init,
            0, // class_finalize
            0, // class_data
            sizeof(SPColorNotebook),
            0, // n_preallocs
            (GInstanceInitFunc)sp_color_notebook_init,
            0 // value_table
        };
        type = g_type_register_static(SP_TYPE_COLOR_SELECTOR, "SPColorNotebook", &info, static_cast<GTypeFlags>(0));
    }
    return type;
}

static void sp_color_notebook_class_init(SPColorNotebookClass *klass)
{
    GObjectClass *object_class = reinterpret_cast<GObjectClass *>(klass);
    GtkWidgetClass *widget_class = reinterpret_cast<GtkWidgetClass *>(klass);

    parent_class = SP_COLOR_SELECTOR_CLASS(g_type_class_peek_parent(klass));

    object_class->dispose = sp_color_notebook_dispose;

    widget_class->show_all = sp_color_notebook_show_all;
    widget_class->hide = sp_color_notebook_hide;
}

static void
sp_color_notebook_switch_page(GtkNotebook *notebook,
                              GtkWidget   *page,
                              guint page_num,
                              SPColorNotebook *colorbook)
{
    if ( colorbook )
    {
        ColorNotebook* nb = dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(colorbook)->base);
        nb->switchPage( notebook, page, page_num );

        // remember the page we switched to
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt("/colorselector/page", page_num);
    }
}

void ColorNotebook::switchPage(GtkNotebook*,
                              GtkWidget*,
                              guint page_num)
{
    SPColorSelector* csel;
    GtkWidget* widget;

    if ( gtk_notebook_get_current_page (GTK_NOTEBOOK (_book)) >= 0 )
    {
        csel = getCurrentSelector();
        if (csel) {
            csel->base->getColorAlpha(_color, _alpha);
        }
    }
    widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK (_book), page_num);
    if ( widget && SP_IS_COLOR_SELECTOR(widget) )
    {
        csel = SP_COLOR_SELECTOR (widget);
        if (csel) {
            csel->base->setColorAlpha( _color, _alpha );
        }

        // Temporary workaround to undo a spurious GRABBED
        _released();
    }
}

static void
sp_color_notebook_init (SPColorNotebook *colorbook)
{
    SP_COLOR_SELECTOR(colorbook)->base = new ColorNotebook( SP_COLOR_SELECTOR(colorbook) );

    if ( SP_COLOR_SELECTOR(colorbook)->base )
    {
        SP_COLOR_SELECTOR(colorbook)->base->init();
    }
}

void ColorNotebook::init()
{
    guint row = 0;

    _updating = false;

    _book = gtk_notebook_new ();
    gtk_widget_show (_book);

    // Dont show the notebook tabs, use radiobuttons instead
    gtk_notebook_set_show_border (GTK_NOTEBOOK (_book), false);
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (_book), false);

#if GTK_CHECK_VERSION(3,0,0)
    _buttonbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_set_homogeneous(GTK_BOX(_buttonbox), TRUE);
#else
    _buttonbox = gtk_hbox_new (TRUE, 2);
#endif

    gtk_widget_show (_buttonbox);
    _buttons = new GtkWidget *[_available_pages.size()];

    for (int i = 0; static_cast<size_t>(i) < _available_pages.size(); i++ )
    {
        _addPage(_available_pages[i]);
    }

#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget* table = gtk_grid_new();
#else
    GtkWidget* table = gtk_table_new(2, 3, FALSE);
#endif

    gtk_widget_show (table);

    gtk_box_pack_start (GTK_BOX (_csel), table, TRUE, TRUE, 0);

    sp_set_font_size_smaller (_buttonbox);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_margin_left(_buttonbox, XPAD);
    gtk_widget_set_margin_right(_buttonbox, XPAD);
    gtk_widget_set_margin_top(_buttonbox, YPAD);
    gtk_widget_set_margin_bottom(_buttonbox, YPAD);
    gtk_widget_set_hexpand(_buttonbox, TRUE);
    gtk_widget_set_valign(_buttonbox, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(table), _buttonbox, 0, row, 2, 1);
#else
    gtk_table_attach (GTK_TABLE (table), _buttonbox, 0, 2, row, row + 1,
                      static_cast<GtkAttachOptions>(GTK_EXPAND|GTK_FILL),
                      static_cast<GtkAttachOptions>(0),
                      XPAD, YPAD);
#endif

    row++;

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_margin_left(_book, XPAD*2);
    gtk_widget_set_margin_right(_book, XPAD*2);
    gtk_widget_set_margin_top(_book, YPAD);
    gtk_widget_set_margin_bottom(_book, YPAD);
    gtk_widget_set_hexpand(_book, TRUE);
    gtk_widget_set_vexpand(_book, TRUE);
    gtk_grid_attach(GTK_GRID(table), _book, 0, row, 2, 1);
#else
    gtk_table_attach (GTK_TABLE (table), _book, 0, 2, row, row + 1,
                      static_cast<GtkAttachOptions>(GTK_EXPAND|GTK_FILL),
                      static_cast<GtkAttachOptions>(GTK_EXPAND|GTK_FILL),
                      XPAD*2, YPAD);
#endif

    // restore the last active page
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _setCurrentPage(prefs->getInt("/colorselector/page", 0));


    /* Commented out: see comment at the bottom of the header file
    {
        gboolean found = FALSE;

        _popup = gtk_menu_new();
        GtkMenu *menu = GTK_MENU (_popup);

        for (int i = 0; i < _trackerList->len; i++ )
        {
            SPColorNotebookTracker *entry = reinterpret_cast< SPColorNotebookTracker* > (g_ptr_array_index (_trackerList, i));
            if ( entry )
            {
                GtkWidget *item = gtk_check_menu_item_new_with_label (_(entry->name));
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), entry->enabledFull);
                gtk_widget_show (item);
                gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

                g_signal_connect (G_OBJECT (item), "activate",
                                  G_CALLBACK (sp_color_notebook_menuitem_response),
                                  reinterpret_cast< gpointer > (entry) );
                found = TRUE;
            }
        }

        GtkWidget *arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
        gtk_widget_show (arrow);

        _btn = gtk_button_new ();
        gtk_widget_show (_btn);
        gtk_container_add (GTK_CONTAINER (_btn), arrow);

        GtkWidget *align = gtk_alignment_new (1.0, 0.0, 0.0, 0.0);
        gtk_widget_show (align);
        gtk_container_add (GTK_CONTAINER (align), _btn);

        // uncomment to reenable the "show/hide modes" menu,
        // but first fix it so it remembers its settings in prefs and does not take that much space (entire vertical column!)
        gtk_table_attach (GTK_TABLE (table), align, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, XPAD, YPAD);

        g_signal_connect_swapped(G_OBJECT(_btn), "event", G_CALLBACK (sp_color_notebook_menu_handler), G_OBJECT(_csel));
        if ( !found )
        {
            gtk_widget_set_sensitive (_btn, FALSE);
        }
    }
    */

    row++;

#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *rgbabox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    GtkWidget *rgbabox = gtk_hbox_new (FALSE, 0);
#endif

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    /* Create color management icons */
    _box_colormanaged = gtk_event_box_new ();
    GtkWidget *colormanaged = gtk_image_new_from_icon_name ("color-management-icon", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add (GTK_CONTAINER (_box_colormanaged), colormanaged);
    gtk_widget_set_tooltip_text (_box_colormanaged, _("Color Managed"));
    gtk_widget_set_sensitive (_box_colormanaged, false);
    gtk_box_pack_start(GTK_BOX(rgbabox), _box_colormanaged, FALSE, FALSE, 2);

    _box_outofgamut = gtk_event_box_new ();
    GtkWidget *outofgamut = gtk_image_new_from_icon_name ("out-of-gamut-icon", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add (GTK_CONTAINER (_box_outofgamut), outofgamut);
    gtk_widget_set_tooltip_text (_box_outofgamut, _("Out of gamut!"));
    gtk_widget_set_sensitive (_box_outofgamut, false);
    gtk_box_pack_start(GTK_BOX(rgbabox), _box_outofgamut, FALSE, FALSE, 2);

    _box_toomuchink = gtk_event_box_new ();
    GtkWidget *toomuchink = gtk_image_new_from_icon_name ("too-much-ink-icon", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add (GTK_CONTAINER (_box_toomuchink), toomuchink);
    gtk_widget_set_tooltip_text (_box_toomuchink, _("Too much ink!"));
    gtk_widget_set_sensitive (_box_toomuchink, false);
    gtk_box_pack_start(GTK_BOX(rgbabox), _box_toomuchink, FALSE, FALSE, 2);
#endif //defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)


    /* Color picker */
    GtkWidget *picker = gtk_image_new_from_icon_name ("color-picker", GTK_ICON_SIZE_SMALL_TOOLBAR);
    _btn_picker = gtk_button_new ();
    gtk_button_set_relief(GTK_BUTTON(_btn_picker), GTK_RELIEF_NONE);
    gtk_container_add (GTK_CONTAINER (_btn_picker), picker);
    gtk_widget_set_tooltip_text (_btn_picker, _("Pick colors from image"));
    gtk_box_pack_start(GTK_BOX(rgbabox), _btn_picker, FALSE, FALSE, 2);
    g_signal_connect(G_OBJECT(_btn_picker), "clicked", G_CALLBACK(ColorNotebook::_picker_clicked), _csel);

    /* Create RGBA entry and color preview */
    _rgbal = gtk_label_new_with_mnemonic (_("RGBA_:"));
    gtk_misc_set_alignment (GTK_MISC (_rgbal), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(rgbabox), _rgbal, TRUE, TRUE, 2);

    ColorEntry* rgba_entry = Gtk::manage(new ColorEntry(_selected_color));
    sp_dialog_defocus_on_enter (GTK_WIDGET(rgba_entry->gobj()));
    gtk_box_pack_start(GTK_BOX(rgbabox), GTK_WIDGET(rgba_entry->gobj()), FALSE, FALSE, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL(_rgbal), GTK_WIDGET(rgba_entry->gobj()));

    sp_set_font_size_smaller (rgbabox);
    gtk_widget_show_all (rgbabox);

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    //the "too much ink" icon is initially hidden
    gtk_widget_hide(GTK_WIDGET(_box_toomuchink));
#endif //defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_margin_left(rgbabox, XPAD);
    gtk_widget_set_margin_right(rgbabox, XPAD);
    gtk_widget_set_margin_top(rgbabox, YPAD);
    gtk_widget_set_margin_bottom(rgbabox, YPAD);
    gtk_grid_attach(GTK_GRID(table), rgbabox, 0, row, 2, 1);
#else
    gtk_table_attach (GTK_TABLE (table), rgbabox, 0, 2, row, row + 1, GTK_FILL, GTK_SHRINK, XPAD, YPAD);
#endif

#ifdef SPCS_PREVIEW
    _p = sp_color_preview_new (0xffffffff);
    gtk_widget_show (_p);
    gtk_table_attach (GTK_TABLE (table), _p, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, XPAD, YPAD);
#endif

    _switchId = g_signal_connect(G_OBJECT (_book), "switch-page",
                                 G_CALLBACK (sp_color_notebook_switch_page), SP_COLOR_NOTEBOOK(_csel));

    _selected_color.signal_changed.connect(sigc::mem_fun(this, &ColorNotebook::_onSelectedColorChanged));
    _selected_color.signal_dragged.connect(sigc::mem_fun(this, &ColorNotebook::_onSelectedColorDragged));
    _selected_color.signal_grabbed.connect(sigc::mem_fun(this, &ColorNotebook::_onSelectedColorGrabbed));
    _selected_color.signal_released.connect(sigc::mem_fun(this, &ColorNotebook::_onSelectedColorReleased));
}

static void sp_color_notebook_dispose(GObject *object)
{
    if (((GObjectClass *) (parent_class))->dispose)
        (* ((GObjectClass *) (parent_class))->dispose) (object);
}

ColorNotebook::~ColorNotebook()
{
    if ( _switchId )
    {
        if ( _book )
        {
            g_signal_handler_disconnect (_book, _switchId);
            _switchId = 0;
        }
    }

    if ( _buttons )
    {
        delete [] _buttons;
        _buttons = 0;
    }

}

static void
sp_color_notebook_show_all (GtkWidget *widget)
{
    gtk_widget_show (widget);
}

static void sp_color_notebook_hide(GtkWidget *widget)
{
    gtk_widget_hide(widget);
}

GtkWidget *sp_color_notebook_new()
{
    SPColorNotebook *colorbook = SP_COLOR_NOTEBOOK(g_object_new (SP_TYPE_COLOR_NOTEBOOK, NULL));

    return GTK_WIDGET(colorbook);
}

ColorNotebook::ColorNotebook( SPColorSelector* csel )
    : ColorSelector( csel )
{
    Page *page;

    page = new Page(new ColorScalesFactory(SP_COLOR_SCALES_MODE_RGB), true);
    _available_pages.push_back(page);
    page = new Page(new ColorScalesFactory(SP_COLOR_SCALES_MODE_HSV), true);
    _available_pages.push_back(page);
    page = new Page(new ColorScalesFactory(SP_COLOR_SCALES_MODE_CMYK), true);
    _available_pages.push_back(page);
    page = new Page(new ColorWheelSelectorFactory, true);
    _available_pages.push_back(page);
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    page = new Page(new ColorICCSelectorFactory, true);
    _available_pages.push_back(page);
#endif
}

SPColorSelector* ColorNotebook::getCurrentSelector()
{
    SPColorSelector* csel = NULL;
    gint current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (_book));

    if ( current_page >= 0 )
    {
        GtkWidget* widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK (_book), current_page);
        if ( SP_IS_COLOR_SELECTOR (widget) )
        {
            csel = SP_COLOR_SELECTOR (widget);
        }
    }

    return csel;
}

ColorNotebook::Page::Page(Inkscape::UI::ColorSelectorFactory *selector_factory, bool enabled_full)
    : selector_factory(selector_factory)
    , enabled_full(enabled_full)
{
}

void ColorNotebook::_colorChanged()
{
    _selected_color.setColorAlpha(_color, _alpha, true);

    SPColorSelector* cselPage = getCurrentSelector();
    if ( cselPage )
    {
        cselPage->base->setColorAlpha( _color, _alpha );
    }
}

void ColorNotebook::_picker_clicked(GtkWidget * /*widget*/, SPColorNotebook * /*colorbook*/)
{
    // Set the dropper into a "one click" mode, so it reverts to the previous tool after a click
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/dropper/onetimepick", true);
    Inkscape::UI::Tools::sp_toggle_dropper(SP_ACTIVE_DESKTOP);
}

// TODO pass in param so as to avoid the need for SP_ACTIVE_DOCUMENT
void ColorNotebook::_updateRgbaEntry( const SPColor& color, gfloat alpha )
{
    g_return_if_fail( ( 0.0 <= alpha ) && ( alpha <= 1.0 ) );

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    /* update color management icon*/
    gtk_widget_set_sensitive (_box_colormanaged, color.icc != NULL);

    /* update out-of-gamut icon */
    gtk_widget_set_sensitive (_box_outofgamut, false);
    if (color.icc){
        Inkscape::ColorProfile* target_profile = SP_ACTIVE_DOCUMENT->profileManager->find(color.icc->colorProfile.c_str());
        if ( target_profile )
            gtk_widget_set_sensitive(_box_outofgamut, target_profile->GamutCheck(color));
    }

    /* update too-much-ink icon */
    gtk_widget_set_sensitive (_box_toomuchink, false);
    if (color.icc){
        Inkscape::ColorProfile* prof = SP_ACTIVE_DOCUMENT->profileManager->find(color.icc->colorProfile.c_str());
        if ( prof && CMSSystem::isPrintColorSpace(prof) ) {
            gtk_widget_show(GTK_WIDGET(_box_toomuchink));
            double ink_sum = 0;
            for (unsigned int i=0; i<color.icc->colors.size(); i++){
                ink_sum += color.icc->colors[i];
            }

            /* Some literature states that when the sum of paint values exceed 320%, it is considered to be a satured color,
                which means the paper can get too wet due to an excessive ammount of ink. This may lead to several issues
                such as misalignment and poor quality of printing in general.*/
            if ( ink_sum > 3.2 )
                gtk_widget_set_sensitive (_box_toomuchink, true);
        } else {
            gtk_widget_hide(GTK_WIDGET(_box_toomuchink));
        }
    }
#endif //defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
}

void ColorNotebook::_setCurrentPage(int i)
{
    gtk_notebook_set_current_page(GTK_NOTEBOOK(_book), i);

    if (_buttons && (static_cast<size_t>(i) < _available_pages.size())) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_buttons[i]), TRUE);
    }
}

void ColorNotebook::_buttonClicked(GtkWidget *widget,  SPColorNotebook *colorbook)
{
    ColorNotebook* nb = dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(colorbook)->base);

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget))) {
        return;
    }

    for(gint i = 0; i < gtk_notebook_get_n_pages (GTK_NOTEBOOK (nb->_book)); i++) {
        if (nb->_buttons[i] == widget) {
            gtk_notebook_set_current_page (GTK_NOTEBOOK (nb->_book), i);
        }
    }
}

void ColorNotebook::_entryGrabbed (SPColorSelector *, SPColorNotebook *colorbook)
{
    ColorNotebook* nb = dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(colorbook)->base);
    nb->_grabbed();
}

void ColorNotebook::_entryDragged (SPColorSelector *csel, SPColorNotebook *colorbook)
{
    gboolean oldState;
    ColorNotebook* nb = dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(colorbook)->base);

    oldState = nb->_dragging;

    nb->_dragging = TRUE;
    nb->_entryModified( csel, colorbook );

    nb->_dragging = oldState;
}

void ColorNotebook::_entryReleased (SPColorSelector *, SPColorNotebook *colorbook)
{
    ColorNotebook* nb = dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(colorbook)->base);
    nb->_released();
}

void ColorNotebook::_entryChanged (SPColorSelector *csel, SPColorNotebook *colorbook)
{
    gboolean oldState;
    ColorNotebook* nb = dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(colorbook)->base);

    oldState = nb->_dragging;

    nb->_dragging = FALSE;
    nb->_entryModified( csel, colorbook );

    nb->_dragging = oldState;
}

void ColorNotebook::_entryModified (SPColorSelector *csel, SPColorNotebook *colorbook)
{
    g_return_if_fail (colorbook != NULL);
    g_return_if_fail (SP_IS_COLOR_NOTEBOOK (colorbook));
    g_return_if_fail (csel != NULL);
    g_return_if_fail (SP_IS_COLOR_SELECTOR (csel));

    ColorNotebook* nb = dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(colorbook)->base);
    SPColor color;
    gfloat alpha = 1.0;

    csel->base->getColorAlpha( color, alpha );

    nb->_updating = true;
    nb->_selected_color.setColorAlpha(color, alpha, true);
    nb->_updating = false;
    nb->_updateInternals( color, alpha, nb->_dragging );
}

void ColorNotebook::_onSelectedColorChanged() {
    if (_updating) {
        return;
    }

    SPColor color;
    gfloat alpha = 1.0;

    _updating = true;
    _selected_color.colorAlpha(color, alpha);
    _updateInternals(color, alpha, _dragging);
    _updating = false;
}

void ColorNotebook::_onSelectedColorDragged() {
    if (_updating) {
        return;
    }
    bool oldState = _dragging;

    _dragging = TRUE;
    SPColor color;
    gfloat alpha = 1.0;

    _updating = true;
    _selected_color.colorAlpha(color, alpha);
    _updateInternals(color, alpha, _dragging);
    _updating = false;

    _dragging = oldState;
}

void ColorNotebook::_onSelectedColorGrabbed() {
    _grabbed();
}

void ColorNotebook::_onSelectedColorReleased() {
    _released();
}

GtkWidget* ColorNotebook::_addPage(Page& page) {
    Gtk::Widget *selector_widget;

    selector_widget = page.selector_factory->createWidget(_selected_color);
    if (selector_widget) {
        selector_widget->show();

        Glib::ustring mode_name = page.selector_factory->modeName();
        Gtk::Widget* tab_label = Gtk::manage(new Gtk::Label(mode_name));
        gint page_num = gtk_notebook_append_page( GTK_NOTEBOOK(_book), selector_widget->gobj(), tab_label->gobj());

        _buttons[page_num] = gtk_radio_button_new_with_label(NULL, mode_name.c_str());
        gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(_buttons[page_num]), FALSE);
        if (page_num > 0) {
            GSList *group =  gtk_radio_button_get_group (GTK_RADIO_BUTTON(_buttons[0]));
            gtk_radio_button_set_group (GTK_RADIO_BUTTON(_buttons[page_num]), group);
        }
        gtk_widget_show (_buttons[page_num]);
        gtk_box_pack_start (GTK_BOX (_buttonbox), _buttons[page_num], TRUE, TRUE, 0);

        g_signal_connect (G_OBJECT (_buttons[page_num]), "clicked", G_CALLBACK (_buttonClicked), _csel);

        if (SP_IS_COLOR_SELECTOR(selector_widget->gobj())) {
            //Connect glib signals of non-refactored widgets
            g_signal_connect (selector_widget->gobj(), "grabbed", G_CALLBACK (_entryGrabbed), _csel);
            g_signal_connect (selector_widget->gobj(), "dragged", G_CALLBACK (_entryDragged), _csel);
            g_signal_connect (selector_widget->gobj(), "released", G_CALLBACK (_entryReleased), _csel);
            g_signal_connect (selector_widget->gobj(), "changed", G_CALLBACK (_entryChanged), _csel);
        }
    }

    return selector_widget->gobj();
}


/* Commented out: see comment at the bottom of the header file

GtkWidget* ColorNotebook::getPage(GType page_type, guint submode)
{
    gint count = 0;
    gint i = 0;
    GtkWidget* page = 0;

//     count = gtk_notebook_get_n_pages (_book);
    count = 200;
    for ( i = 0; i < count && !page; i++ )
    {
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (_book), i);
        if ( page )
        {
            SPColorSelector* csel;
            guint pagemode;
            csel = SP_COLOR_SELECTOR (page);
            pagemode = csel->base->getSubmode();
            if ( G_TYPE_FROM_INSTANCE (page) == page_type
                 && pagemode == submode )
            {
                // found it.
                break;
            }
            else
            {
                page = 0;
            }
        }
        else
        {
            break;
        }
    }
    return page;
}

void ColorNotebook::removePage( GType page_type, guint submode )
{
    GtkWidget *page = 0;

    page = getPage(page_type, submode);
    if ( page )
    {
        gint where = gtk_notebook_page_num (GTK_NOTEBOOK (_book), page);
        if ( where >= 0 )
        {
            if ( gtk_notebook_get_current_page (GTK_NOTEBOOK (_book)) == where )
            {
//                 getColorAlpha(_color, &_alpha);
            }
            gtk_notebook_remove_page (GTK_NOTEBOOK (_book), where);
        }
    }
}


static gint sp_color_notebook_menu_handler( GtkWidget *widget, GdkEvent *event )
{
    if (event->type == GDK_BUTTON_PRESS)
    {
        SPColorSelector* csel = SP_COLOR_SELECTOR(widget);
        (dynamic_cast<ColorNotebook*>(csel->base))->menuHandler( event );

        // Tell calling code that we have handled this event; the buck
        // stops here.
        return TRUE;
    }

   //Tell calling code that we have not handled this event; pass it on.
    return FALSE;
}

gint ColorNotebook::menuHandler( GdkEvent* event )
{
    GdkEventButton *bevent = (GdkEventButton *) event;
    gtk_menu_popup (GTK_MENU( _popup ), NULL, NULL, NULL, NULL,
                    bevent->button, bevent->time);
    return TRUE;
}

static void sp_color_notebook_menuitem_response (GtkMenuItem *menuitem, gpointer user_data)
{
    gboolean active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    SPColorNotebookTracker *entry = reinterpret_cast< SPColorNotebookTracker* > (user_data);
    if ( entry )
    {
        if ( active )
        {
            (dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(entry->backPointer)->base))->addPage(entry->type, entry->submode);
        }
        else
        {
            (dynamic_cast<ColorNotebook*>(SP_COLOR_SELECTOR(entry->backPointer)->base))->removePage(entry->type, entry->submode);
        }
    }
}
*/

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
