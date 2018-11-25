// SPDX-License-Identifier: GPL-2.0-or-later OR MPL-1.1 OR LGPL-2.1-or-later
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is EGE Adjustment Action.
 *
 * The Initial Developer of the Original Code is
 * Jon A. Cruz.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* Note: this file should be kept compilable as both .cpp and .c */

#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

#include <gtkmm/container.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/adjustment.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/display.h>

#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/widget/ink-spinscale.h"
#include "widgets/ege-adjustment-action.h"

static void ege_adjustment_action_finalize( GObject* object );
static void ege_adjustment_action_get_property( GObject* obj, guint propId, GValue* value, GParamSpec * pspec );
static void ege_adjustment_action_set_property( GObject* obj, guint propId, const GValue *value, GParamSpec* pspec );

static GtkWidget* create_menu_item( GtkAction* action );
static GtkWidget* create_tool_item( GtkAction* action );
static void connect_proxy( GtkAction *action, GtkWidget *proxy );
static void disconnect_proxy( GtkAction *action, GtkWidget *proxy );

static gboolean focus_in_cb( GtkWidget *widget, GdkEventKey *event, gpointer data );
static gboolean focus_out_cb( GtkWidget *widget, GdkEventKey *event, gpointer data );
static gboolean keypress_cb( GtkWidget *widget, GdkEventKey *event, gpointer data );

static void ege_adjustment_action_defocus( EgeAdjustmentAction* action );

static void egeAct_free_description( gpointer data, gpointer user_data );
static void egeAct_free_all_descriptions( EgeAdjustmentAction* action );

static EgeCreateAdjWidgetCB gFactoryCb = nullptr;
static GQuark gDataName = 0;

enum {
    APPEARANCE_UNKNOWN = -1,
    APPEARANCE_NONE = 0,
    APPEARANCE_FULL,    /* label, then all choices represented by separate buttons */
    APPEARANCE_COMPACT, /* label, then choices in a drop-down menu */
    APPEARANCE_MINIMAL, /* no label, just choices in a drop-down menu */
};

/* TODO need to have appropriate icons setup for these: */
static const gchar *floogles[] = {
    INKSCAPE_ICON("list-remove"),
    INKSCAPE_ICON("list-add"),
    INKSCAPE_ICON("go-down"),
    INKSCAPE_ICON("help-about"),
    INKSCAPE_ICON("go-up"),
    nullptr};

typedef struct _EgeAdjustmentDescr EgeAdjustmentDescr;

struct _EgeAdjustmentDescr
{
    gchar* descr;
    gdouble value;
};

typedef struct {
    GtkAdjustment* adj;
    GtkWidget* focusWidget;
    gdouble climbRate;
    guint digits;
    gdouble epsilon;
    gchar* format;
    gchar* selfId;
    EgeWidgetFixup toolPost;
    gdouble lastVal;
    gdouble step;
    gdouble page;
    gint appearanceMode;
    gboolean transferFocus;
    std::vector<EgeAdjustmentDescr*> descriptions;
    gchar* appearance;
    gchar* iconId;
    GtkIconSize iconSize;
    Inkscape::UI::Widget::UnitTracker *unitTracker;
} EgeAdjustmentActionPrivate;

#define EGE_ADJUSTMENT_ACTION_GET_PRIVATE(o) \
    reinterpret_cast<EgeAdjustmentActionPrivate *>( ege_adjustment_action_get_instance_private (o))

enum {
    PROP_ADJUSTMENT = 1,
    PROP_FOCUS_WIDGET,
    PROP_CLIMB_RATE,
    PROP_DIGITS,
    PROP_SELFID,
    PROP_TOOL_POST,
    PROP_APPEARANCE,
    PROP_ICON_ID,
    PROP_ICON_SIZE,
    PROP_UNIT_TRACKER
};

enum {
    BUMP_TOP = 0,
    BUMP_PAGE_UP,
    BUMP_UP,
    BUMP_NONE,
    BUMP_DOWN,
    BUMP_PAGE_DOWN,
    BUMP_BOTTOM,
    BUMP_CUSTOM = 100
};

G_DEFINE_TYPE_WITH_PRIVATE(EgeAdjustmentAction, ege_adjustment_action, GTK_TYPE_ACTION);

static void ege_adjustment_action_class_init( EgeAdjustmentActionClass* klass )
{
    if ( klass ) {
        GObjectClass * objClass = G_OBJECT_CLASS( klass );

        gDataName = g_quark_from_string("ege-adj-action");

  
        objClass->finalize = ege_adjustment_action_finalize;

        objClass->get_property = ege_adjustment_action_get_property;
        objClass->set_property = ege_adjustment_action_set_property;

        klass->parent_class.create_menu_item = create_menu_item;
        klass->parent_class.create_tool_item = create_tool_item;
        klass->parent_class.connect_proxy = connect_proxy;
        klass->parent_class.disconnect_proxy = disconnect_proxy;

        g_object_class_install_property( objClass,
                                         PROP_ADJUSTMENT,
                                         g_param_spec_object( "adjustment",
                                                              "Adjustment",
                                                              "The adjustment to change",
                                                              GTK_TYPE_ADJUSTMENT,
                                                              (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_FOCUS_WIDGET,
                                         g_param_spec_pointer( "focus-widget",
                                                               "Focus Widget",
                                                               "The widget to return focus to",
                                                               (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_CLIMB_RATE,
                                         g_param_spec_double( "climb-rate",
                                                              "Climb Rate",
                                                              "The acelleraton rate",
                                                              0.0, G_MAXDOUBLE, 0.0,
                                                              (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_DIGITS,
                                         g_param_spec_uint( "digits",
                                                            "Digits",
                                                            "The number of digits to show",
                                                            0, 20, 0,
                                                            (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_SELFID,
                                         g_param_spec_string( "self-id",
                                                              "Self ID",
                                                              "Marker for self pointer",
                                                              nullptr,
                                                              (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_TOOL_POST,
                                         g_param_spec_pointer( "tool-post",
                                                               "Tool Widget post process",
                                                               "Function for final adjustments",
                                                               (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_APPEARANCE,
                                         g_param_spec_string( "appearance",
                                                              "Appearance hint",
                                                              "A hint for how to display",
                                                              "",
                                                              (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_ICON_ID,
                                         g_param_spec_string( "iconId",
                                                              "Icon ID",
                                                              "The id for the icon",
                                                              "",
                                                              (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_ICON_SIZE,
                                         g_param_spec_int( "iconSize",
                                                           "Icon Size",
                                                           "The size the icon",
                                                           (int)GTK_ICON_SIZE_MENU,
                                                           (int)GTK_ICON_SIZE_DIALOG,
                                                           (int)GTK_ICON_SIZE_SMALL_TOOLBAR,
                                                           (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_UNIT_TRACKER,
                                         g_param_spec_pointer( "unit_tracker",
                                                               "Unit Tracker",
                                                               "The widget that keeps track of the unit",
                                                               (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );
    }
}

void ege_adjustment_action_set_compact_tool_factory( EgeCreateAdjWidgetCB factoryCb )
{
    gFactoryCb = factoryCb;
}

static void ege_adjustment_action_init( EgeAdjustmentAction* action )
{
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE( action );
    priv->adj = nullptr;
    priv->focusWidget = nullptr;
    priv->climbRate = 0.0;
    priv->digits = 2;
    priv->epsilon = 0.009;
    priv->format = g_strdup_printf("%%0.%df%%s%%s", priv->digits);
    priv->selfId = nullptr;
    priv->toolPost = nullptr;
    priv->lastVal = 0.0;
    priv->step = 0.0;
    priv->page = 0.0;
    priv->appearanceMode = APPEARANCE_NONE;
    priv->transferFocus = FALSE;
    //priv->descriptions = 0;
    priv->appearance = nullptr;
    priv->iconId = nullptr;
    priv->iconSize = GTK_ICON_SIZE_SMALL_TOOLBAR;
    priv->unitTracker = nullptr;
}

static void ege_adjustment_action_finalize( GObject* object )
{
    EgeAdjustmentAction* action = nullptr;
    g_return_if_fail( object != nullptr );
    g_return_if_fail( IS_EGE_ADJUSTMENT_ACTION(object) );

    action = EGE_ADJUSTMENT_ACTION( object );
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);

    // g_free(NULL) does nothing
    g_free( priv->format );
    g_free( priv->selfId );
    g_free( priv->appearance );
    g_free( priv->iconId );

    egeAct_free_all_descriptions( action );

    if ( G_OBJECT_CLASS(ege_adjustment_action_parent_class)->finalize ) {
        (*G_OBJECT_CLASS(ege_adjustment_action_parent_class)->finalize)(object);
    }
}

EgeAdjustmentAction* ege_adjustment_action_new( GtkAdjustment* adjustment,
                                                const gchar *name,
                                                const gchar *label,
                                                const gchar *tooltip,
                                                const gchar *stock_id,
                                                gdouble climb_rate,
                                                guint digits,
                                                Inkscape::UI::Widget::UnitTracker *unit_tracker )
{
    GObject* obj = (GObject*)g_object_new( EGE_ADJUSTMENT_ACTION_TYPE,
                                           "name", name,
                                           "label", label,
                                           "tooltip", tooltip,
                                           "stock_id", stock_id,
                                           "adjustment", adjustment,
                                           "climb-rate", climb_rate,
                                           "digits", digits,
                                           "unit_tracker", unit_tracker,
                                           NULL );

    EgeAdjustmentAction* action = EGE_ADJUSTMENT_ACTION( obj );

    return action;
}

static void ege_adjustment_action_get_property( GObject* obj, guint propId, GValue* value, GParamSpec * pspec )
{
    EgeAdjustmentAction* action = EGE_ADJUSTMENT_ACTION( obj );
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);
    switch ( propId ) {
        case PROP_ADJUSTMENT:
            g_value_set_object( value, priv->adj );
            break;

        case PROP_FOCUS_WIDGET:
            g_value_set_pointer( value, priv->focusWidget );
            break;

        case PROP_CLIMB_RATE:
            g_value_set_double( value, priv->climbRate );
            break;

        case PROP_DIGITS:
            g_value_set_uint( value, priv->digits );
            break;

        case PROP_SELFID:
            g_value_set_string( value, priv->selfId );
            break;

        case PROP_TOOL_POST:
            g_value_set_pointer( value, (void*)priv->toolPost );
            break;

        case PROP_APPEARANCE:
            g_value_set_string( value, priv->appearance );
            break;

        case PROP_ICON_ID:
            g_value_set_string( value, priv->iconId );
            break;

        case PROP_ICON_SIZE:
            g_value_set_int( value, priv->iconSize );
            break;

        case PROP_UNIT_TRACKER:
            g_value_set_pointer( value, priv->unitTracker );
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, propId, pspec );
    }
}

void ege_adjustment_action_set_property( GObject* obj, guint propId, const GValue *value, GParamSpec* pspec )
{
    EgeAdjustmentAction* action = EGE_ADJUSTMENT_ACTION( obj );
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);
    switch ( propId ) {
        case PROP_ADJUSTMENT:
        {
            priv->adj = GTK_ADJUSTMENT( g_value_get_object( value ) );
            g_object_get( G_OBJECT(priv->adj),
                          "step-increment", &priv->step,
                          "page-increment", &priv->page,
                          NULL );
        }
        break;

        case PROP_FOCUS_WIDGET:
        {
            /* TODO unhook prior */
            priv->focusWidget = (GtkWidget*)g_value_get_pointer( value );
        }
        break;

        case PROP_CLIMB_RATE:
        {
            /* TODO pass on */
            priv->climbRate = g_value_get_double( value );
        }
        break;

        case PROP_DIGITS:
        {
            /* TODO pass on */
            priv->digits = g_value_get_uint( value );
            switch ( priv->digits ) {
                case 0: priv->epsilon = 0.9; break;
                case 1: priv->epsilon = 0.09; break;
                case 2: priv->epsilon = 0.009; break;
                case 3: priv->epsilon = 0.0009; break;
                case 4: priv->epsilon = 0.00009; break;
            }
            if ( priv->format ) {
                g_free( priv->format );
            }
            priv->format = g_strdup_printf("%%0.%df%%s%%s", priv->digits);
        }
        break;

        case PROP_SELFID:
        {
            /* TODO pass on */
            gchar* prior = priv->selfId;
            priv->selfId = g_value_dup_string( value );
            g_free( prior );
        }
        break;

        case PROP_TOOL_POST:
        {
            priv->toolPost = (EgeWidgetFixup)g_value_get_pointer( value );
        }
        break;

        case PROP_APPEARANCE:
        {
            gchar* tmp = priv->appearance;
            gchar* newVal = g_value_dup_string( value );
            priv->appearance = newVal;
            g_free( tmp );

            if ( !priv->appearance || (strcmp("", newVal) == 0) ) {
                priv->appearanceMode = APPEARANCE_NONE;
            } else if ( strcmp("full", newVal) == 0 ) {
                priv->appearanceMode = APPEARANCE_FULL;
            } else if ( strcmp("compact", newVal) == 0 ) {
                priv->appearanceMode = APPEARANCE_COMPACT;
            } else if ( strcmp("minimal", newVal) == 0 ) {
                priv->appearanceMode = APPEARANCE_MINIMAL;
            } else {
                priv->appearanceMode = APPEARANCE_UNKNOWN;
            }
        }
        break;

        case PROP_ICON_ID:
        {
            gchar* tmp = priv->iconId;
            priv->iconId = g_value_dup_string( value );
            g_free( tmp );
        }
        break;

        case PROP_ICON_SIZE:
        {
            priv->iconSize = (GtkIconSize)g_value_get_int( value );
        }
        break;

        case PROP_UNIT_TRACKER:
        {
            priv->unitTracker = (Inkscape::UI::Widget::UnitTracker*)g_value_get_pointer( value );
        }
        break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, propId, pspec );
    }
}

GtkAdjustment* ege_adjustment_action_get_adjustment( EgeAdjustmentAction* action )
{
    g_return_val_if_fail( IS_EGE_ADJUSTMENT_ACTION(action), NULL );

    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);

    return priv->adj;
}

void ege_adjustment_action_set_focuswidget( EgeAdjustmentAction* action, GtkWidget* widget )
{
    g_return_if_fail( IS_EGE_ADJUSTMENT_ACTION(action) );
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);

    /* TODO unhook prior */

    priv->focusWidget = widget;
}

GtkWidget* ege_adjustment_action_get_focuswidget( EgeAdjustmentAction* action )
{
    g_return_val_if_fail( IS_EGE_ADJUSTMENT_ACTION(action), NULL );
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);

    return priv->focusWidget;
}

static void egeAct_free_description( gpointer data, gpointer user_data ) {
    (void)user_data;
    if ( data ) {
        EgeAdjustmentDescr* descr = (EgeAdjustmentDescr*)data;
        if ( descr->descr ) {
            g_free( descr->descr );
            descr->descr = nullptr;
        }
        g_free( descr );
    }
}

static void egeAct_free_all_descriptions( EgeAdjustmentAction* action )
{
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);
    for(auto i:priv->descriptions) {
        egeAct_free_description(i,nullptr);
    }
    for(auto i:priv->descriptions) {
        g_free(i);
    }
    priv->descriptions.clear();
}

static gint egeAct_compare_descriptions( gconstpointer a, gconstpointer b )
{
    gint val = 0;

    EgeAdjustmentDescr const * aa = (EgeAdjustmentDescr const *)a;
    EgeAdjustmentDescr const * bb = (EgeAdjustmentDescr const *)b;

    if ( aa && bb ) {
        if ( aa->value < bb->value ) {
            val = -1;
        } else if ( aa->value > bb->value ) {
            val = 1;
        }
    }

    return val;
}

void ege_adjustment_action_set_descriptions( EgeAdjustmentAction* action, gchar const** descriptions, gdouble const* values, guint count )
{
    g_return_if_fail( IS_EGE_ADJUSTMENT_ACTION(action) );
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);

    egeAct_free_all_descriptions( action );

    if ( count && descriptions && values ) {
        guint i = 0;
        for ( i = 0; i < count; i++ ) {
            EgeAdjustmentDescr* descr = g_new0( EgeAdjustmentDescr, 1 );
            descr->descr = descriptions[i] ? g_strdup( descriptions[i] ) : nullptr;
            descr->value = values[i];
            priv->descriptions.push_back(descr);
            std::sort(priv->descriptions.begin(),priv->descriptions.end());
        }
    }
}

void ege_adjustment_action_set_appearance( EgeAdjustmentAction* action, gchar const* val )
{
    g_object_set( G_OBJECT(action), "appearance", val, NULL );
}

static void process_menu_action( GtkWidget* obj, gpointer data )
{
    GtkCheckMenuItem* item = GTK_CHECK_MENU_ITEM(obj);
    if ( gtk_check_menu_item_get_active (item)) {
        EgeAdjustmentAction* act = (EgeAdjustmentAction*)g_object_get_qdata( G_OBJECT(obj), gDataName );
        auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (act);
        gint what = GPOINTER_TO_INT(data);


        gdouble base = gtk_adjustment_get_value( priv->adj );
        gdouble lower = 0.0;
        gdouble upper = 0.0;
        gdouble step = 0.0;
        gdouble page = 0.0;
        g_object_get( G_OBJECT(priv->adj),
                      "lower", &lower,
                      "upper", &upper,
                      "step-increment", &step,
                      "page-increment", &page,
                      NULL );

        switch ( what ) {
            case BUMP_TOP:
                gtk_adjustment_set_value( priv->adj, upper );
                break;

            case BUMP_PAGE_UP:
                gtk_adjustment_set_value( priv->adj, base + page );
                break;

            case BUMP_UP:
                gtk_adjustment_set_value( priv->adj, base + step );
                break;

            case BUMP_DOWN:
                gtk_adjustment_set_value( priv->adj, base - step );
                break;

            case BUMP_PAGE_DOWN:
                gtk_adjustment_set_value( priv->adj, base - page );
                break;

            case BUMP_BOTTOM:
                gtk_adjustment_set_value( priv->adj, lower );
                break;

            default:
                if ( what >= BUMP_CUSTOM ) {
                    guint index = what - BUMP_CUSTOM;
                    if ( index < priv->descriptions.size() ) {
                        EgeAdjustmentDescr* descr = priv->descriptions[index];
                        if ( descr ) {
                            gtk_adjustment_set_value( priv->adj, descr->value );
                        }
                    }
                }
        }
    }
}

static void create_single_menu_item( GCallback toggleCb, int val, GtkWidget* menu, EgeAdjustmentAction* act, GtkWidget** dst, Gtk::RadioMenuItem::Group *group, gdouble num, gboolean active )
{
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (act);
    char* str = nullptr;
    EgeAdjustmentDescr* marker = nullptr;
    std::vector<EgeAdjustmentDescr*> cur = priv->descriptions;

    for (auto descr:cur) {
        gdouble delta = num - descr->value;
        if ( delta < 0.0 ) {
            delta = -delta;
        }
        if ( delta < priv->epsilon ) {
            marker = descr;
            break;
        }
    }

    str = g_strdup_printf( priv->format, num,
                           ((marker && marker->descr) ? ": " : ""),
                           ((marker && marker->descr) ? marker->descr : ""));

    auto rmi = new Gtk::RadioMenuItem(*group,Glib::ustring(str));
    *dst = GTK_WIDGET(rmi->gobj());
    if ( active ) {
        gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(*dst), TRUE );
    }
    gtk_menu_shell_append( GTK_MENU_SHELL(menu), *dst );
    g_object_set_qdata( G_OBJECT(*dst), gDataName, act );

    g_signal_connect( G_OBJECT(*dst), "toggled", toggleCb, GINT_TO_POINTER(val) );

    g_free(str);
}

static int flush_explicit_items( std::vector<EgeAdjustmentDescr*> descriptions,
                                    int pos,
                                    GCallback toggleCb,
                                    int val,
                                    GtkWidget* menu,
                                    EgeAdjustmentAction* act,
                                    GtkWidget** dst,
                                    Gtk::RadioMenuItem::Group *group,
                                    gdouble num )
{
    if(pos >= descriptions.size() || pos < 0) return pos;
    EgeAdjustmentDescr* descr = descriptions[pos];
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (act);

    gdouble valUpper = num + priv->epsilon;
    gdouble valLower = num - priv->epsilon;

    while ( pos>=0 && pos<descriptions.size() && descr && (descr->value >= valLower) ) {
        if ( descr->value > valUpper ) {
            create_single_menu_item( toggleCb, val + ( std::find(priv->descriptions.begin(),priv->descriptions.end(),descr) - priv->descriptions.begin() ) , menu, act, dst, group, descr->value, FALSE );
        }
        pos--;
        descr = (pos<0) ? descriptions[pos] : nullptr;
    }

    return pos;
}

static GtkWidget* create_popup_number_menu( EgeAdjustmentAction* act )
{
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (act);
    GtkWidget* menu = gtk_menu_new();

    Gtk::RadioMenuItem::Group group;
    GtkWidget* single = nullptr;
    std::vector<EgeAdjustmentDescr*> list = priv->descriptions;
    int addOns = list.size() - 1;

    gdouble base = gtk_adjustment_get_value( priv->adj );
    gdouble lower = 0.0;
    gdouble upper = 0.0;
    gdouble step = 0.0;
    gdouble page = 0.0;
    g_object_get( G_OBJECT(priv->adj),
                  "lower", &lower,
                  "upper", &upper,
                  "step-increment", &step,
                  "page-increment", &page,
                  NULL );


    if ( base < upper ) {
        addOns = flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, upper );
        create_single_menu_item( G_CALLBACK(process_menu_action), BUMP_TOP, menu, act, &single, &group, upper, FALSE );
        if ( (base + page) < upper ) {
            addOns = flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, base + page );
            create_single_menu_item( G_CALLBACK(process_menu_action), BUMP_PAGE_UP, menu, act, &single, &group, base + page, FALSE );
        }
        if ( (base + step) < upper ) {
            addOns = flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, base + step );
            create_single_menu_item( G_CALLBACK(process_menu_action), BUMP_UP, menu, act, &single, &group, base + step, FALSE );
        }
    }

    addOns = flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, base );
    create_single_menu_item( G_CALLBACK(process_menu_action), BUMP_NONE, menu, act, &single, &group, base, TRUE );

    if ( base > lower ) {
        if ( (base - step) > lower ) {
            addOns = flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, base - step );
            create_single_menu_item( G_CALLBACK(process_menu_action), BUMP_DOWN, menu, act, &single, &group, base - step, FALSE );
        }
        if ( (base - page) > lower ) {
            addOns = flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, base - page );
            create_single_menu_item( G_CALLBACK(process_menu_action), BUMP_PAGE_DOWN, menu, act, &single, &group, base - page, FALSE );
        }
        addOns = flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, lower );
        create_single_menu_item( G_CALLBACK(process_menu_action), BUMP_BOTTOM, menu, act, &single, &group, lower, FALSE );
    }

    if ( !priv->descriptions.empty() ) {
        gdouble value = ((EgeAdjustmentDescr*)priv->descriptions[0])->value;
        flush_explicit_items( list, addOns, G_CALLBACK(process_menu_action), BUMP_CUSTOM, menu, act, &single, &group, value );
    }

    return menu;
}

static GtkWidget* create_menu_item( GtkAction* action )
{
    GtkWidget* item = nullptr;

    if ( IS_EGE_ADJUSTMENT_ACTION(action) ) {
        EgeAdjustmentAction* act = EGE_ADJUSTMENT_ACTION( action );
        GValue value;
        GtkWidget*  subby = nullptr;

        memset( &value, 0, sizeof(value) );
        g_value_init( &value, G_TYPE_STRING );
        g_object_get_property( G_OBJECT(action), "label", &value );

        item = gtk_menu_item_new_with_label( g_value_get_string( &value ) );

        subby = create_popup_number_menu( act );
        gtk_menu_item_set_submenu( GTK_MENU_ITEM(item), subby );
        gtk_widget_show_all( subby );
        g_value_unset( &value );
    } else {
        item = GTK_ACTION_CLASS(ege_adjustment_action_parent_class)->create_menu_item( action );
    }

    return item;
}

static void value_changed_cb( GtkSpinButton* spin, EgeAdjustmentAction* act )
{
    if ( gtk_widget_has_focus( GTK_WIDGET(spin) ) ) {
        gint start = 0, end = 0;
        if (GTK_IS_EDITABLE(spin) && gtk_editable_get_selection_bounds (GTK_EDITABLE(spin), &start, &end)
                && start != end) {
            // #167846, #363000 If the spin button has a selection, its probably
            // because we got here from a Tab key from another spin, if so don't defocus
            return;
        }
        ege_adjustment_action_defocus( act );
    }
}

static gboolean event_cb( EgeAdjustmentAction* act, GdkEvent* evt )
{
    gboolean handled = FALSE;
    if ( evt->type == GDK_BUTTON_PRESS ) {
        if ( evt->button.button == 3 ) {
            if ( IS_EGE_ADJUSTMENT_ACTION(act) ) {
                GtkWidget* menu = create_popup_number_menu(act);
                gtk_widget_show_all( menu );
#if GTK_CHECK_VERSION(3,22,0)
                gtk_menu_popup_at_pointer( GTK_MENU(menu), evt );
#else
                GdkEventButton* btnevt = (GdkEventButton*)evt;
                gtk_menu_popup( GTK_MENU(menu), NULL, NULL, NULL, NULL, btnevt->button, btnevt->time );
#endif
            }
            handled = TRUE;
        }
    }

    return handled;
}

static GtkWidget* create_tool_item( GtkAction* action )
{
    GtkWidget* item = nullptr;

    if ( IS_EGE_ADJUSTMENT_ACTION(action) ) {
        EgeAdjustmentAction* act = EGE_ADJUSTMENT_ACTION( action );
        auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (act);
        GtkWidget* spinbutton = nullptr;
	auto hb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_set_homogeneous(GTK_BOX(hb), FALSE);
        GValue value;
        memset( &value, 0, sizeof(value) );
        g_value_init( &value, G_TYPE_STRING );
        g_object_get_property( G_OBJECT(action), "short_label", &value );

        if ( priv->appearanceMode == APPEARANCE_FULL ) {
            /* Slider */
            InkSpinScale* inkspinscale =
              new InkSpinScale(Glib::wrap(priv->adj));
            inkspinscale->set_label( g_value_get_string( &value ));
            inkspinscale->set_digits(0);
            spinbutton = (GtkWidget*)inkspinscale->gobj();
            gtk_widget_set_size_request(spinbutton, 100, -1);

        } else if ( priv->appearanceMode == APPEARANCE_MINIMAL ) {
            spinbutton = gtk_scale_button_new( GTK_ICON_SIZE_MENU, 0, 100, 2, nullptr );
            gtk_scale_button_set_adjustment( GTK_SCALE_BUTTON(spinbutton), priv->adj );
            gtk_scale_button_set_icons( GTK_SCALE_BUTTON(spinbutton), floogles );
        } else {
            if ( gFactoryCb ) {
                spinbutton = gFactoryCb( priv->adj, priv->climbRate, priv->digits, priv->unitTracker );
            } else {
                spinbutton = gtk_spin_button_new( priv->adj, priv->climbRate, priv->digits );
            }
        }

        item = GTK_WIDGET( gtk_tool_item_new() );

        {
            GValue tooltip;
            memset( &tooltip, 0, sizeof(tooltip) );
            g_value_init( &tooltip, G_TYPE_STRING );
            g_object_get_property( G_OBJECT(action), "tooltip", &tooltip );
            const gchar* tipstr = g_value_get_string( &tooltip );
            if ( tipstr && *tipstr ) {
                gtk_widget_set_tooltip_text( spinbutton, tipstr );
            }
            g_value_unset( &tooltip );
        }

        if ( priv->appearanceMode != APPEARANCE_FULL ) {
            GtkWidget* filler1 = gtk_label_new(" ");
            gtk_box_pack_start( GTK_BOX(hb), filler1, FALSE, FALSE, 0 );

            /* Use an icon if available or use short-label */
            if ( priv->iconId && strcmp( priv->iconId, "" ) != 0 ) {
                GtkWidget *icon = sp_get_icon_image(priv->iconId, priv->iconSize);
                gtk_box_pack_start( GTK_BOX(hb), icon, FALSE, FALSE, 0 );
            } else {
                GtkWidget* lbl = gtk_label_new( g_value_get_string( &value ) ? g_value_get_string( &value ) : "wwww" );
                gtk_widget_set_halign(lbl, GTK_ALIGN_END);
                gtk_box_pack_start( GTK_BOX(hb), lbl, FALSE, FALSE, 0 );
            }
        }

        if ( priv->appearanceMode == APPEARANCE_FULL ) {
            gtk_box_pack_start( GTK_BOX(hb), spinbutton, TRUE, TRUE, 0 );
        }  else {
            gtk_box_pack_start( GTK_BOX(hb), spinbutton, FALSE, FALSE, 0 );
        }

        gtk_container_add( GTK_CONTAINER(item), hb );

        if ( priv->selfId ) {
            g_object_set_data( G_OBJECT(spinbutton), priv->selfId, spinbutton );
        }

        g_signal_connect( G_OBJECT(spinbutton), "focus-in-event", G_CALLBACK(focus_in_cb), action );
        g_signal_connect( G_OBJECT(spinbutton), "focus-out-event", G_CALLBACK(focus_out_cb), action );
        g_signal_connect( G_OBJECT(spinbutton), "key-press-event", G_CALLBACK(keypress_cb), action );

        g_signal_connect( G_OBJECT(spinbutton), "value-changed", G_CALLBACK(value_changed_cb), action );

        g_signal_connect_swapped( G_OBJECT(spinbutton), "event", G_CALLBACK(event_cb), action );
        if ( priv->appearanceMode == APPEARANCE_FULL ) {
            /* */
        } else if ( priv->appearanceMode == APPEARANCE_MINIMAL ) {
            /* */
        } else {
            gtk_entry_set_width_chars( GTK_ENTRY(spinbutton), priv->digits + 3 );
        }

        gtk_widget_show_all( item );

        /* Shrink or whatnot after shown */
        if ( priv->toolPost ) {
            priv->toolPost( item );
        }

        g_value_unset( &value );
    } else {
        item = GTK_ACTION_CLASS(ege_adjustment_action_parent_class)->create_tool_item( action );
    }

    return item;
}

static void connect_proxy( GtkAction *action, GtkWidget *proxy )
{
    GTK_ACTION_CLASS(ege_adjustment_action_parent_class)->connect_proxy( action, proxy );
}

static void disconnect_proxy( GtkAction *action, GtkWidget *proxy )
{
    GTK_ACTION_CLASS(ege_adjustment_action_parent_class)->disconnect_proxy( action, proxy );
}

void ege_adjustment_action_defocus( EgeAdjustmentAction* action )
{
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);
    if ( priv->transferFocus ) {
        if ( priv->focusWidget ) {
            gtk_widget_grab_focus( priv->focusWidget );
        }
    }
}

gboolean focus_in_cb( GtkWidget *widget, GdkEventKey *event, gpointer data )
{
    (void)event;
    if ( IS_EGE_ADJUSTMENT_ACTION(data) ) {
        EgeAdjustmentAction* action = EGE_ADJUSTMENT_ACTION( data );
        auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);
        if ( GTK_IS_SPIN_BUTTON(widget) ) {
            priv->lastVal = gtk_spin_button_get_value( GTK_SPIN_BUTTON(widget) );
        } else if ( GTK_IS_SCALE_BUTTON(widget) ) {
            priv->lastVal = gtk_scale_button_get_value( GTK_SCALE_BUTTON(widget) );
        } else if (GTK_IS_RANGE(widget) ) {
            priv->lastVal = gtk_range_get_value( GTK_RANGE(widget) );
        }
        priv->transferFocus = TRUE;
    }

    return FALSE; /* report event not consumed */
}

static gboolean focus_out_cb( GtkWidget *widget, GdkEventKey *event, gpointer data )
{
    (void)widget;
    (void)event;
    if ( IS_EGE_ADJUSTMENT_ACTION(data) ) {
        EgeAdjustmentAction* action = EGE_ADJUSTMENT_ACTION( data );
        auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);
        priv->transferFocus = FALSE;
    }

    return FALSE; /* report event not consumed */
}


static gboolean process_tab( GtkWidget* widget, int direction )
{
    gboolean handled = FALSE;
    GtkWidget* parent = gtk_widget_get_parent(widget);
    GtkWidget* gp = parent ? gtk_widget_get_parent(parent) : nullptr;
    GtkWidget* ggp = gp ? gtk_widget_get_parent(gp) : nullptr;

    if ( ggp && GTK_IS_TOOLBAR(ggp) ) {
        std::vector<Gtk::Widget*> kids = Glib::wrap(GTK_CONTAINER(ggp))->get_children();
        if ( !kids.empty() ) {
            GtkWidget* curr = widget;
            while ( curr && (gtk_widget_get_parent(curr) != ggp) ) {
                curr = gtk_widget_get_parent( curr );
            }
            if ( curr ) {
                std::vector<Gtk::Widget*>::iterator mid=kids.end();
                for(auto i = kids.begin(); i!= kids.end(); ++i){
                    if(curr == (*i)->gobj())
                        mid = i;
                }
                while ( mid != kids.end() && !(mid==kids.begin() && direction<0 )  ) {
                    mid = ( direction < 0 ) ? std::prev(mid) : ++mid;
                    if ( mid!=kids.end() && GTK_IS_TOOL_ITEM((*mid)->gobj()) ) {
                        /* potential target */
                        GtkWidget* child = gtk_bin_get_child( GTK_BIN((*mid)->gobj()) );
                        if ( child && GTK_IS_BOX(child) ) { /* could be ours */
                            std::vector<Gtk::Widget*> subChildren = Glib::wrap(GTK_CONTAINER(child))->get_children();
                            if ( ! subChildren.empty() ) {
                                Gtk::Widget *last = subChildren[subChildren.size()-1];
                                if ( GTK_IS_SPIN_BUTTON(last->gobj()) && gtk_widget_is_sensitive( GTK_WIDGET(last->gobj()) ) ) {
                                    gtk_widget_grab_focus( GTK_WIDGET(last->gobj()) );
                                    handled = TRUE;
                                    mid = kids.end(); /* to stop loop */
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return handled;
}

gboolean keypress_cb( GtkWidget *widget, GdkEventKey *event, gpointer data )
{
    gboolean wasConsumed = FALSE; /* default to report event not consumed */
    EgeAdjustmentAction* action = EGE_ADJUSTMENT_ACTION(data);
    auto priv = EGE_ADJUSTMENT_ACTION_GET_PRIVATE (action);
    guint key = 0;
    gdk_keymap_translate_keyboard_state( Gdk::Display::get_default()->get_keymap(),
                                         event->hardware_keycode, (GdkModifierType)event->state,
                                         0, &key, nullptr, nullptr, nullptr );

    switch ( key ) {
        case GDK_KEY_Escape:
        {
            priv->transferFocus = TRUE;
            gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), priv->lastVal );
            ege_adjustment_action_defocus( action );
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        {
            priv->transferFocus = TRUE;
            ege_adjustment_action_defocus( action );
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_Tab:
        {
            priv->transferFocus = FALSE;
            wasConsumed = process_tab( widget, 1 );
        }
        break;

        case GDK_KEY_ISO_Left_Tab:
        {
            priv->transferFocus = FALSE;
            wasConsumed = process_tab( widget, -1 );
        }
        break;

        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
        {
            priv->transferFocus = FALSE;
            gdouble val = gtk_spin_button_get_value( GTK_SPIN_BUTTON(widget) );
            gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), val + priv->step );
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
        {
            priv->transferFocus = FALSE;
            gdouble val = gtk_spin_button_get_value( GTK_SPIN_BUTTON(widget) );
            gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), val - priv->step );
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_Page_Up:
        case GDK_KEY_KP_Page_Up:
        {
            priv->transferFocus = FALSE;
            gdouble val = gtk_spin_button_get_value( GTK_SPIN_BUTTON(widget) );
            gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), val + priv->page );
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_Page_Down:
        case GDK_KEY_KP_Page_Down:
        {
            priv->transferFocus = FALSE;
            gdouble val = gtk_spin_button_get_value( GTK_SPIN_BUTTON(widget) );
            gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), val - priv->page );
            wasConsumed = TRUE;
        }
        break;

        case GDK_KEY_z:
        case GDK_KEY_Z:
        {
            priv->transferFocus = FALSE;
            gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), priv->lastVal );
            wasConsumed = TRUE;
        }
        break;

    }

    return wasConsumed;
}
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
