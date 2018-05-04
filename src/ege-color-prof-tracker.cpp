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
 * The Original Code is EGE Color Profile Tracker.
 *
 * The Initial Developer of the Original Code is
 * Jon A. Cruz.
 * Portions created by the Initial Developer are Copyright (C) 2007
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

#include <string.h>
#include <vector>
#include <algorithm>

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>

#include <gdk/gdkx.h>
#endif /* GDK_WINDOWING_X11 */

#include "ege-color-prof-tracker.h"
#include "helper/sp-marshal.h"

enum {
    CHANGED = 0,
    ADDED,
    REMOVED,
    MODIFIED,
    LAST_SIGNAL
};

static void ege_color_prof_tracker_get_property( GObject* obj, guint propId, GValue* value, GParamSpec * pspec );
static void ege_color_prof_tracker_set_property( GObject* obj, guint propId, const GValue *value, GParamSpec* pspec );

class ScreenTrack {
    public:
#ifdef GDK_WINDOWING_X11
    gboolean zeroSeen;
    gboolean otherSeen;
#endif /* GDK_WINDOWING_X11 */
    std::vector<EgeColorProfTracker *> *trackers;
    GPtrArray* profiles;
    ~ScreenTrack(){ delete trackers; }
};


#ifdef GDK_WINDOWING_X11
GdkFilterReturn x11_win_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data);
void handle_property_change(GdkScreen* screen, const gchar* name);
void add_x11_tracking_for_screen(GdkScreen* screen);
static void fire(gint monitor);
static void clear_profile( guint monitor );
static void set_profile( guint monitor, const guint8* data, guint len );
#endif /* GDK_WINDOWING_X11 */

static guint signals[LAST_SIGNAL] = {0};

// There is only one GdkScreen in Gtk+ 3
static ScreenTrack *tracked_screen = nullptr;

static std::vector<EgeColorProfTracker *> abstract_trackers;

struct _EgeColorProfTrackerPrivate
{
    GtkWidget* _target;
    gint _monitor;
};

#define EGE_GET_PRIVATE( o ) ( G_TYPE_INSTANCE_GET_PRIVATE( (o), EGE_COLOR_PROF_TRACKER_TYPE, EgeColorProfTrackerPrivate ) )


static void target_finalized( gpointer data, GObject* where_the_object_was );
static void window_finalized( gpointer data, GObject* where_the_object_was );
static void event_after_cb( GtkWidget* widget, GdkEvent* event, gpointer user_data );
static void target_hierarchy_changed_cb(GtkWidget* widget, GtkWidget* prev_top, gpointer user_data);
static void target_screen_changed_cb(GtkWidget* widget, GdkScreen* prev_screen, gpointer user_data);
static void screen_size_changed_cb(GdkScreen* screen, gpointer user_data);
static void track_screen( GdkScreen* screen, EgeColorProfTracker* tracker );

G_DEFINE_TYPE(EgeColorProfTracker, ege_color_prof_tracker, G_TYPE_OBJECT);

void ege_color_prof_tracker_class_init( EgeColorProfTrackerClass* klass )
{
    if ( klass ) {
        GObjectClass* objClass = G_OBJECT_CLASS( klass );

        objClass->get_property = ege_color_prof_tracker_get_property;
        objClass->set_property = ege_color_prof_tracker_set_property;
        klass->changed = 0;

        signals[CHANGED] = g_signal_new( "changed",
                                         G_TYPE_FROM_CLASS(klass),
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET(EgeColorProfTrackerClass, changed),
                                         NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0 );

        signals[ADDED] = g_signal_new( "added",
                                       G_TYPE_FROM_CLASS(klass),
                                       G_SIGNAL_RUN_FIRST,
                                       0,
                                       NULL, NULL,
                                       sp_marshal_VOID__INT_INT,
                                       G_TYPE_NONE, 2,
                                       G_TYPE_INT,
                                       G_TYPE_INT);

        signals[REMOVED] = g_signal_new( "removed",
                                         G_TYPE_FROM_CLASS(klass),
                                         G_SIGNAL_RUN_FIRST,
                                         0,
                                         NULL, NULL,
                                         sp_marshal_VOID__INT_INT,
                                         G_TYPE_NONE, 2,
                                         G_TYPE_INT,
                                         G_TYPE_INT);

        signals[MODIFIED] = g_signal_new( "modified",
                                          G_TYPE_FROM_CLASS(klass),
                                          G_SIGNAL_RUN_FIRST,
                                          0,
                                          NULL, NULL,
                                          g_cclosure_marshal_VOID__INT,
                                          G_TYPE_NONE, 1,
                                          G_TYPE_INT);

        g_type_class_add_private( klass, sizeof(EgeColorProfTrackerClass) );
    }
}


void ege_color_prof_tracker_init( EgeColorProfTracker* tracker )
{
    tracker->private_data = EGE_GET_PRIVATE( tracker );
    tracker->private_data->_target = 0;
    tracker->private_data->_monitor = 0;
}

EgeColorProfTracker* ege_color_prof_tracker_new( GtkWidget* target )
{
    GObject* obj = (GObject*)g_object_new( EGE_COLOR_PROF_TRACKER_TYPE,
                                           NULL );

    EgeColorProfTracker* tracker = EGE_COLOR_PROF_TRACKER( obj );
    tracker->private_data->_target = target;

    if ( target ) {
        g_object_weak_ref( G_OBJECT(target), target_finalized, obj );
        g_signal_connect( G_OBJECT(target), "hierarchy-changed", G_CALLBACK( target_hierarchy_changed_cb ), obj );
        g_signal_connect( G_OBJECT(target), "screen-changed",    G_CALLBACK( target_screen_changed_cb ),    obj );

        /* invoke the callbacks now to connect if the widget is already visible */
        target_hierarchy_changed_cb( target, 0, obj );
        target_screen_changed_cb( target, 0, obj );
    } else {
        abstract_trackers.push_back(tracker);

        if(tracked_screen) {
            for ( gint monitor = 0; monitor < (gint)tracked_screen->profiles->len; monitor++ ) {
                g_signal_emit( G_OBJECT(tracker), signals[MODIFIED], 0, monitor );
            }
        }

    }

    return tracker;
}

void ege_color_prof_tracker_get_profile( EgeColorProfTracker const * tracker, gpointer* ptr, guint* len )
{
    gpointer dataPos = 0;
    guint dataLen = 0;
    if (tracker) {
        if (tracker->private_data->_target ) {
            GdkScreen* screen = gtk_widget_get_screen(tracker->private_data->_target);
            if ( tracked_screen ) {
                if ( tracker->private_data->_monitor >= 0 && tracker->private_data->_monitor < (static_cast<gint>(tracked_screen->profiles->len))) {
                    GByteArray* gba = static_cast<GByteArray*>(g_ptr_array_index(tracked_screen->profiles, tracker->private_data->_monitor));
                    if ( gba ) {
                        dataPos = gba->data;
                        dataLen = gba->len;
                    }
                } else {
                    g_warning("No profile data tracked for the specified item.");
                }
            }
        }
    }
    if ( ptr ) {
        *ptr = dataPos;
    }
    if ( len ) {
        *len = dataLen;
    }
}

void ege_color_prof_tracker_get_profile_for( guint monitor, gpointer* ptr, guint* len )
{
    gpointer dataPos = 0;
    guint dataLen = 0;
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen  *screen  = gdk_display_get_default_screen(display);

    if ( screen ) {
        if ( tracked_screen ) {
            if ( monitor < tracked_screen->profiles->len ) {
                GByteArray* gba = (GByteArray*)g_ptr_array_index( tracked_screen->profiles, monitor );
                if ( gba ) {
                    dataPos = gba->data;
                    dataLen = gba->len;
                }
            } else {
                g_warning("No profile data tracked for the specified item.");
            }
        }
    }

    if ( ptr ) {
        *ptr = dataPos;
    }
    if ( len ) {
        *len = dataLen;
    }
}

void ege_color_prof_tracker_get_property( GObject* obj, guint propId, GValue* value, GParamSpec * pspec )
{
    EgeColorProfTracker* tracker = EGE_COLOR_PROF_TRACKER( obj );
    (void)tracker;
    (void)value;

    switch ( propId ) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, propId, pspec );
    }
}

void ege_color_prof_tracker_set_property( GObject* obj, guint propId, const GValue *value, GParamSpec* pspec )
{
    EgeColorProfTracker* tracker = EGE_COLOR_PROF_TRACKER( obj );
    (void)tracker;
    (void)value;
    switch ( propId ) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID( obj, propId, pspec );
    }
}


void track_screen( GdkScreen* screen, EgeColorProfTracker* tracker )
{
    if ( tracked_screen ) {
        /* We found the screen already being tracked */
        if ( std::find(tracked_screen->trackers->begin(),tracked_screen->trackers->end(),tracker)==tracked_screen->trackers->end() ) {
            tracked_screen->trackers->push_back(tracker);
        }
    } else {
        tracked_screen = g_new(ScreenTrack, 1);

        GdkDisplay* display = gdk_display_get_default();

#if GTK_CHECK_VERSION(3,22,0)
        int numMonitors = gdk_display_get_n_monitors(display);
#else
        gint numMonitors = gdk_screen_get_n_monitors(screen);
#endif

#ifdef GDK_WINDOWING_X11
        tracked_screen->zeroSeen = FALSE;
        tracked_screen->otherSeen = FALSE;
#endif /* GDK_WINDOWING_X11 */
        tracked_screen->trackers= new std::vector<EgeColorProfTracker *>;
        tracked_screen->trackers->push_back(tracker );
        tracked_screen->profiles = g_ptr_array_new();
        for ( int i = 0; i < numMonitors; i++ ) {
            g_ptr_array_add( tracked_screen->profiles, 0 );
        }

        g_signal_connect( G_OBJECT(screen), "size-changed", G_CALLBACK( screen_size_changed_cb ), tracker );

#ifdef GDK_WINDOWING_X11
        if (GDK_IS_X11_DISPLAY (display) ) {
            // printf( "track_screen: Display is using X11\n" );
            add_x11_tracking_for_screen(screen);
        } else {
            // printf( "track_screen: Display is not using X11\n" );
        }
#endif // GDK_WINDOWING_X11
    }
}


void target_finalized( gpointer data, GObject* where_the_object_was )
{
    (void)data;
    if ( tracked_screen ) {
        for (auto i = tracked_screen->trackers->begin(); i != tracked_screen->trackers->end(); ++i) {
            if ( (void*)((*i)->private_data->_target) == (void*)where_the_object_was ) {
                /* The tracked widget is now gone, remove it */
                (*i)->private_data->_target = 0;
                tracked_screen->trackers->erase(i);
                break;
            }
        }
    }
}

void window_finalized( gpointer data, GObject* where_the_object_was )
{
    (void)data;
    (void)where_the_object_was;
/*     g_message("Window at %p is now going away", where_the_object_was); */
}

void event_after_cb( GtkWidget* widget, GdkEvent* event, gpointer user_data )
{
    if ( event->type == GDK_CONFIGURE ) {
        GdkWindow* window = gtk_widget_get_window (widget);
        EgeColorProfTracker* tracker = (EgeColorProfTracker*)user_data;

        // In old Gtk+ versions, we can directly find the ID number for a monitor.
        // In Gtk+ >= 3.22, however, we need to figure out the ID
#if GTK_CHECK_VERSION(3,22,0)
        auto display = gdk_display_get_default();
        auto monitor = gdk_display_get_monitor_at_window(display, window);

        int n_monitors = gdk_display_get_n_monitors(display);

        int monitorNum = -1;

        // Now loop through the set of monitors and figure out whether this monitor matches
        for (int i_monitor = 0; i_monitor < n_monitors; ++i_monitor) {
            auto monitor_at_index = gdk_display_get_monitor(display, i_monitor);
            if(monitor_at_index == monitor) monitorNum = i_monitor;
        }
#else
        GdkScreen* screen = gtk_widget_get_screen(widget);
        gint monitorNum = gdk_screen_get_monitor_at_window(screen, window);
#endif

        if ( monitorNum != tracker->private_data->_monitor && monitorNum != -1 ) {
            tracker->private_data->_monitor = monitorNum;
            g_signal_emit( G_OBJECT(tracker), signals[CHANGED], 0 );
        }
    }
}

void target_hierarchy_changed_cb(GtkWidget* widget, GtkWidget* prev_top, gpointer user_data)
{
    if ( !prev_top && gtk_widget_get_toplevel(widget) ) {
        GtkWidget* top = gtk_widget_get_toplevel(widget);
        if ( gtk_widget_is_toplevel(top) ) {
            GtkWindow* win = GTK_WINDOW(top);
            g_signal_connect( G_OBJECT(win), "event-after", G_CALLBACK( event_after_cb ), user_data );
            g_object_weak_ref( G_OBJECT(win), window_finalized, user_data );
        }
    }
}

void target_screen_changed_cb(GtkWidget* widget, GdkScreen* prev_screen, gpointer user_data)
{
    GdkScreen* screen = gtk_widget_get_screen(widget);

    if ( screen && (screen != prev_screen) ) {
        track_screen( screen, EGE_COLOR_PROF_TRACKER(user_data) );
    }
}

void screen_size_changed_cb(GdkScreen* screen, gpointer user_data)
{
    (void)user_data;
/*     g_message("screen size changed to (%d, %d) with %d monitors for obj:%p", */
/*               gdk_screen_get_width(screen), gdk_screen_get_height(screen), */
/*               gdk_screen_get_n_monitors(screen), */
/*               user_data); */
    if ( tracked_screen ) {
        GdkDisplay* display = gdk_display_get_default();

#if GTK_CHECK_VERSION(3,22,0)
        int numMonitors  = gdk_display_get_n_monitors(display);
#else
        gint numMonitors = gdk_screen_get_n_monitors(screen);
#endif

        if ( numMonitors > (gint)tracked_screen->profiles->len ) {
            for ( guint i = tracked_screen->profiles->len; i < (guint)numMonitors; i++ ) {
                g_ptr_array_add( tracked_screen->profiles, 0 );
#ifdef GDK_WINDOWING_X11
                if (GDK_IS_X11_DISPLAY (display) ) {
                    gchar* name = g_strdup_printf( "_ICC_PROFILE_%d", i );
                    handle_property_change( screen, name );
                    g_free(name);
                }
#endif /* GDK_WINDOWING_X11 */
            }
        } else if ( numMonitors < (gint)tracked_screen->profiles->len ) {
/*             g_message("The count of monitors decreased, remove some"); */
        }
    }
}

#ifdef GDK_WINDOWING_X11
GdkFilterReturn x11_win_filter(GdkXEvent *xevent,
                               GdkEvent *event,
                               gpointer data)
{
    XEvent* x11 = (XEvent*)xevent;
    (void)event;
    (void)data;

    if ( x11->type == PropertyNotify ) {
        XPropertyEvent* note = (XPropertyEvent*)x11;
        /*GdkAtom gatom = gdk_x11_xatom_to_atom(note->atom);*/
        const gchar* name = gdk_x11_get_xatom_name(note->atom);
        if ( strncmp("_ICC_PROFILE", name, 12 ) == 0 ) {
            XEvent* native = (XEvent*)xevent;
            XWindowAttributes tmp;
            Status stat = XGetWindowAttributes( native->xproperty.display, native->xproperty.window, &tmp );
            if ( stat ) {
                GdkDisplay* display = gdk_x11_lookup_xdisplay(native->xproperty.display);
                if ( display ) {
                    GdkScreen* targetScreen = 0;
                    GdkScreen* sc = gdk_display_get_default_screen(display);
                    if ( tmp.screen == GDK_SCREEN_XSCREEN(sc) ) {
                        targetScreen = sc;
                    }

                    handle_property_change( targetScreen, name );
                }
            } else {
/*                 g_message("%d           failed XGetWindowAttributes with %d", GPOINTER_TO_INT(data), stat); */
            }
        }
    }

    return GDK_FILTER_CONTINUE;
}

void handle_property_change(GdkScreen* screen, const gchar* name)
{
    Display* xdisplay = GDK_SCREEN_XDISPLAY(screen);
    gint monitor = 0;
    Atom atom = XInternAtom(xdisplay, name, True);
    if ( strncmp("_ICC_PROFILE_", name, 13 ) == 0 ) {
        gint64 tmp = g_ascii_strtoll(name + 13, NULL, 10);
        if ( tmp != 0 && tmp != G_MAXINT64 && tmp != G_MININT64 ) {
            monitor = (gint)tmp;
        }
    }
    if ( atom != None ) {
        Atom actualType = None;
        int actualFormat = 0;
        unsigned long size = 128 * 1042;
        unsigned long nitems = 0;
        unsigned long bytesAfter = 0;
        unsigned char* prop = 0;

        clear_profile( monitor );

        if ( XGetWindowProperty( xdisplay, GDK_WINDOW_XID(gdk_screen_get_root_window(screen)),
                                 atom, 0, size, False, AnyPropertyType,
                                 &actualType, &actualFormat, &nitems, &bytesAfter, &prop ) == Success ) {
            if ( (actualType != None) && (bytesAfter + nitems) ) {
                size = nitems + bytesAfter;
                bytesAfter = 0;
                nitems = 0;
                if ( prop ) {
                    XFree(prop);
                    prop = 0;
                }
                if ( XGetWindowProperty( xdisplay, GDK_WINDOW_XID(gdk_screen_get_root_window(screen)),
                                         atom, 0, size, False, AnyPropertyType,
                                         &actualType, &actualFormat, &nitems, &bytesAfter, &prop ) == Success ) {
                    gpointer profile = g_memdup( prop, nitems );
                    set_profile( monitor, (const guint8*)profile, nitems );
                    XFree(prop);
                } else {
                    g_warning("Problem reading profile from root window");
                }
            } else {
                /* clear it */
                set_profile( monitor, 0, 0 );
            }
        } else {
            g_warning("error loading profile property");
        }
    }
    fire(monitor);
}

void add_x11_tracking_for_screen(GdkScreen* screen)
{
    Display* xdisplay = GDK_SCREEN_XDISPLAY(screen);
    GdkWindow* root = gdk_screen_get_root_window(screen);
    if ( root ) {
        Window rootWin = GDK_WINDOW_XID(root);
        Atom baseAtom = XInternAtom(xdisplay, "_ICC_PROFILE", True);
        int numWinProps = 0;
        Atom* propArray = XListProperties(xdisplay, rootWin, &numWinProps);
        gint i;

        gdk_window_set_events(root, (GdkEventMask)(gdk_window_get_events(root) | GDK_PROPERTY_CHANGE_MASK));
        gdk_window_add_filter(root, x11_win_filter, GINT_TO_POINTER(1));

        /* Look for any profiles attached to this root window */
        if ( propArray ) {
            int j = 0;

#if GTK_CHECK_VERSION(3,22,0)
            auto display = gdk_display_get_default();
            int numMonitors = gdk_display_get_n_monitors(display);
#else
            gint numMonitors = gdk_screen_get_n_monitors(screen);
#endif

            if ( baseAtom != None ) {
                for ( i = 0; i < numWinProps; i++ ) {
                    if ( baseAtom == propArray[i] ) {
                        tracked_screen->zeroSeen = TRUE;
                        handle_property_change( screen, "_ICC_PROFILE" );
                    }
                }
            } else {
/*                 g_message("Base atom not found"); */
            }

            for ( i = 1; i < numMonitors; i++ ) {
                gchar* name = g_strdup_printf("_ICC_PROFILE_%d", i);
                Atom atom = XInternAtom(xdisplay, name, True);
                if ( atom != None ) {
                    for ( j = 0; j < numWinProps; j++ ) {
                        if ( atom == propArray[j] ) {
                            tracked_screen->otherSeen = TRUE;
                            handle_property_change( screen, name );
                        }
                    }
                }
                g_free(name);
            }
            XFree(propArray);
            propArray = 0;
        }
    }
}

void fire(gint monitor)
{
    if ( tracked_screen ) {
        for (auto tracker:(*(tracked_screen->trackers))) {
            if ( (monitor == -1) || (tracker->private_data->_monitor == monitor) ) {
                g_signal_emit( G_OBJECT(tracker), signals[CHANGED], 0 );
            }
        }
    }
}

static void clear_profile( guint monitor )
{
    if ( tracked_screen ) {
        GByteArray* previous = 0;
        for ( guint i = tracked_screen->profiles->len; i <= monitor; i++ ) {
            g_ptr_array_add( tracked_screen->profiles, 0 );
        }
        previous = (GByteArray*)g_ptr_array_index( tracked_screen->profiles, monitor );
        if ( previous ) {
            g_byte_array_free( previous, TRUE );
        }

        tracked_screen->profiles->pdata[monitor] = 0;
    }
}

static void set_profile( guint monitor, const guint8* data, guint len )
{
    if ( tracked_screen ) {
        for ( guint i = tracked_screen->profiles->len; i <= monitor; i++ ) {
            g_ptr_array_add( tracked_screen->profiles, 0 );
        }
        GByteArray *previous = (GByteArray*)g_ptr_array_index( tracked_screen->profiles, monitor );
        if ( previous ) {
            g_byte_array_free( previous, TRUE );
        }

        if ( data && len ) {
            GByteArray* newBytes = g_byte_array_sized_new( len );
            newBytes = g_byte_array_append( newBytes, data, len );
            tracked_screen->profiles->pdata[monitor] = newBytes;
        } else {
            tracked_screen->profiles->pdata[monitor] = 0;
        }

        for (auto i:abstract_trackers) {
            g_signal_emit( G_OBJECT(i), signals[MODIFIED], 0, monitor );
        }
    }
}
#endif /* GDK_WINDOWING_X11 */
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
