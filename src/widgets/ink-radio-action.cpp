// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "ink-radio-action.h"
#include "ui/icon-loader.h"

static void ink_radio_action_finalize( GObject* obj );
static void ink_radio_action_get_property( GObject* obj, guint propId, GValue* value, GParamSpec * pspec );
static void ink_radio_action_set_property( GObject* obj, guint propId, const GValue *value, GParamSpec* pspec );

static GtkWidget* ink_radio_action_create_menu_item( GtkAction* action );
static GtkWidget* ink_radio_action_create_tool_item( GtkAction* action );

typedef struct
{
    gchar* iconId;
    GtkIconSize iconSize;
} InkRadioActionPrivate;

#define INK_RADIO_ACTION_GET_PRIVATE(o) \
    reinterpret_cast<InkRadioActionPrivate *>(ink_radio_action_get_instance_private (o));

G_DEFINE_TYPE_WITH_PRIVATE(InkRadioAction, ink_radio_action, GTK_TYPE_RADIO_ACTION);

enum {
    PROP_INK_ID = 1,
    PROP_INK_SIZE
};

static void ink_radio_action_class_init( InkRadioActionClass* klass )
{
    if ( klass ) {
        GObjectClass * objClass = G_OBJECT_CLASS( klass );

        objClass->finalize = ink_radio_action_finalize;
        objClass->get_property = ink_radio_action_get_property;
        objClass->set_property = ink_radio_action_set_property;

        klass->parent_class.parent_class.parent_class.create_menu_item = ink_radio_action_create_menu_item;
        klass->parent_class.parent_class.parent_class.create_tool_item = ink_radio_action_create_tool_item;
        /*klass->parent_class.connect_proxy = connect_proxy;*/
        /*klass->parent_class.disconnect_proxy = disconnect_proxy;*/

        g_object_class_install_property( objClass,
                                         PROP_INK_ID,
                                         g_param_spec_string( "iconId",
                                                              "Icon ID",
                                                              "The id for the icon",
                                                              "",
                                                              (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );

        g_object_class_install_property( objClass,
                                         PROP_INK_SIZE,
                                         g_param_spec_int( "iconSize",
                                                           "Icon Size",
                                                           "The size the icon",
                                                           (int)GTK_ICON_SIZE_MENU,
                                                           (int)GTK_ICON_SIZE_DIALOG,
                                                           (int)GTK_ICON_SIZE_SMALL_TOOLBAR,
                                                           (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT) ) );
    }
}

static void ink_radio_action_init( InkRadioAction* action )
{
    auto priv = INK_RADIO_ACTION_GET_PRIVATE( action );
    priv->iconId = nullptr;
    priv->iconSize = GTK_ICON_SIZE_SMALL_TOOLBAR;
}

static void ink_radio_action_finalize( GObject* obj )
{
    InkRadioAction* action = INK_RADIO_ACTION( obj );
    auto priv = INK_RADIO_ACTION_GET_PRIVATE( action );

    g_free( priv->iconId );
    g_free( priv );

}

InkRadioAction* ink_radio_action_new( const gchar *name,
                           const gchar *label,
                           const gchar *tooltip,
                           const gchar *inkId,
                           GtkIconSize size )
{
    GObject* obj = (GObject*)g_object_new( INK_RADIO_ACTION_TYPE,
                                           "name", name,
                                           "label", label,
                                           "tooltip", tooltip,
                                           "iconId", inkId,
                                           "iconSize", size,
                                           NULL );

    InkRadioAction* action = INK_RADIO_ACTION( obj );

    return action;
}

static void ink_radio_action_get_property( GObject* obj, guint propId, GValue* value, GParamSpec * pspec )
{
    InkRadioAction* action = INK_RADIO_ACTION( obj );
    auto priv = INK_RADIO_ACTION_GET_PRIVATE (action);

    switch ( propId ) {
        case PROP_INK_ID:
        {
            g_value_set_string( value, priv->iconId );
        }
        break;

        case PROP_INK_SIZE:
        {
            g_value_set_int( value, priv->iconSize );
        }
        break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, propId, pspec );
    }
}

void ink_radio_action_set_property( GObject* obj, guint propId, const GValue *value, GParamSpec* pspec )
{
    InkRadioAction* action = INK_RADIO_ACTION( obj );
    auto priv = INK_RADIO_ACTION_GET_PRIVATE (action);

    switch ( propId ) {
        case PROP_INK_ID:
        {
            gchar* tmp = priv->iconId;
            priv->iconId = g_value_dup_string( value );
            g_free( tmp );
        }
        break;

        case PROP_INK_SIZE:
        {
            priv->iconSize = (GtkIconSize)g_value_get_int( value );
        }
        break;

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, propId, pspec );
        }
    }
}

static GtkWidget* ink_radio_action_create_menu_item( GtkAction* action )
{
    GtkWidget* item = GTK_RADIO_ACTION_CLASS(ink_radio_action_parent_class)->parent_class.parent_class.create_menu_item(action);

    return item;
}

static GtkWidget* ink_radio_action_create_tool_item( GtkAction* action )
{
    InkRadioAction* act = INK_RADIO_ACTION( action );
    auto priv = INK_RADIO_ACTION_GET_PRIVATE (act);
    GtkWidget* item = GTK_RADIO_ACTION_CLASS(ink_radio_action_parent_class)->parent_class.parent_class.create_tool_item(action);

    if ( priv->iconId ) {
        if ( GTK_IS_TOOL_BUTTON(item) ) {
            GtkToolButton* button = GTK_TOOL_BUTTON(item);

            GtkWidget *child = sp_get_icon_image(priv->iconId, priv->iconSize);
            gtk_widget_set_hexpand(child, FALSE);
            gtk_widget_set_vexpand(child, FALSE);
            gtk_tool_button_set_icon_widget(button, child);
        } else {
            // For now trigger a warning but don't do anything else
            GtkToolButton* button = GTK_TOOL_BUTTON(item);
            (void)button;
        }
    }

    // TODO investigate if needed
    gtk_widget_show_all( item );

    return item;
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
