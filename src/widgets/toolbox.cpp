// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file
 * Inkscape toolbar definitions and general utility functions.
 * Each tool should have its own xxx-toolbar implementation file
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2015 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/box.h>
#include <gtkmm/action.h>
#include <gtkmm/actiongroup.h>
#include <gtkmm/toolitem.h>
#include <glibmm/i18n.h>

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "inkscape.h"
#include "verbs.h"

#include "ink-action.h"
#include "ink-toggle-action.h"

#include "helper/action.h"
#include "helper/verb-action.h"

#include "include/gtkmm_version.h"

#include "io/resource.h"

#include "object/sp-namedview.h"

#include "ui/icon-names.h"
#include "ui/interface.h"
#include "ui/tools-switch.h"
#include "ui/uxmanager.h"
#include "ui/widget/button.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/style-swatch.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/spinbutton-events.h"
#include "widgets/spw-utilities.h"
#include "widgets/widget-sizes.h"

#include "xml/attribute-record.h"
#include "xml/node-event-vector.h"

#include "ui/toolbar/arc-toolbar.h"
#include "ui/toolbar/box3d-toolbar.h"
#include "ui/toolbar/calligraphy-toolbar.h"
#include "ui/toolbar/connector-toolbar.h"
#include "ui/toolbar/dropper-toolbar.h"
#include "ui/toolbar/eraser-toolbar.h"
#include "ui/toolbar/gradient-toolbar.h"
#include "ui/toolbar/lpe-toolbar.h"
#include "ui/toolbar/mesh-toolbar.h"
#include "ui/toolbar/measure-toolbar.h"
#include "ui/toolbar/node-toolbar.h"
#include "ui/toolbar/rect-toolbar.h"

#if HAVE_POTRACE
# include "ui/toolbar/paintbucket-toolbar.h"
#endif

#include "ui/toolbar/pencil-toolbar.h"
#include "ui/toolbar/select-toolbar.h"
#include "ui/toolbar/spray-toolbar.h"
#include "ui/toolbar/spiral-toolbar.h"
#include "ui/toolbar/star-toolbar.h"
#include "ui/toolbar/tweak-toolbar.h"
#include "ui/toolbar/text-toolbar.h"
#include "ui/toolbar/zoom-toolbar.h"

#include "toolbox.h"

#include "ui/tools/tool-base.h"

//#define DEBUG_TEXT

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::Tools::ToolBase;

using Inkscape::IO::Resource::get_filename;
using Inkscape::IO::Resource::UIS;

typedef void (*SetupFunction)(GtkWidget *toolbox, SPDesktop *desktop);
typedef void (*UpdateFunction)(SPDesktop *desktop, ToolBase *eventcontext, GtkWidget *toolbox);

enum BarId {
    BAR_TOOL = 0,
    BAR_AUX,
    BAR_COMMANDS,
    BAR_SNAP,
};

#define BAR_ID_KEY "BarIdValue"
#define HANDLE_POS_MARK "x-inkscape-pos"

GtkIconSize ToolboxFactory::prefToSize( Glib::ustring const &path, int base ) {
    static GtkIconSize sizeChoices[] = {
        GTK_ICON_SIZE_LARGE_TOOLBAR,
        GTK_ICON_SIZE_SMALL_TOOLBAR,
        GTK_ICON_SIZE_MENU,
        GTK_ICON_SIZE_DIALOG
    };
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int index = prefs->getIntLimited( path, base, 0, G_N_ELEMENTS(sizeChoices) );
    return sizeChoices[index];
}

Gtk::IconSize ToolboxFactory::prefToSize_mm(Glib::ustring const &path, int base)
{
    static Gtk::IconSize sizeChoices[] = { Gtk::ICON_SIZE_LARGE_TOOLBAR, Gtk::ICON_SIZE_SMALL_TOOLBAR,
                                           Gtk::ICON_SIZE_MENU, Gtk::ICON_SIZE_DIALOG };
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int index = prefs->getIntLimited(path, base, 0, G_N_ELEMENTS(sizeChoices));
    return sizeChoices[index];
}

static struct {
    gchar const *type_name;
    gchar const *data_name;
    sp_verb_t verb;
    sp_verb_t doubleclick_verb;
} const tools[] = {
	{ "/tools/select",   "select_tool",    SP_VERB_CONTEXT_SELECT,  SP_VERB_CONTEXT_SELECT_PREFS},
	{ "/tools/nodes",     "node_tool",      SP_VERB_CONTEXT_NODE, SP_VERB_CONTEXT_NODE_PREFS },
	{ "/tools/tweak",    "tweak_tool",     SP_VERB_CONTEXT_TWEAK, SP_VERB_CONTEXT_TWEAK_PREFS },
	{ "/tools/spray",    "spray_tool",     SP_VERB_CONTEXT_SPRAY, SP_VERB_CONTEXT_SPRAY_PREFS },
	{ "/tools/zoom",     "zoom_tool",      SP_VERB_CONTEXT_ZOOM, SP_VERB_CONTEXT_ZOOM_PREFS },
	{ "/tools/measure",  "measure_tool",   SP_VERB_CONTEXT_MEASURE, SP_VERB_CONTEXT_MEASURE_PREFS },
	{ "/tools/shapes/rect",     "rect_tool",      SP_VERB_CONTEXT_RECT, SP_VERB_CONTEXT_RECT_PREFS },
	{ "/tools/shapes/3dbox",      "3dbox_tool",     SP_VERB_CONTEXT_3DBOX, SP_VERB_CONTEXT_3DBOX_PREFS },
	{ "/tools/shapes/arc",      "arc_tool",       SP_VERB_CONTEXT_ARC, SP_VERB_CONTEXT_ARC_PREFS },
	{ "/tools/shapes/star",     "star_tool",      SP_VERB_CONTEXT_STAR, SP_VERB_CONTEXT_STAR_PREFS },
	{ "/tools/shapes/spiral",   "spiral_tool",    SP_VERB_CONTEXT_SPIRAL, SP_VERB_CONTEXT_SPIRAL_PREFS },
	{ "/tools/freehand/pencil",   "pencil_tool",    SP_VERB_CONTEXT_PENCIL, SP_VERB_CONTEXT_PENCIL_PREFS },
	{ "/tools/freehand/pen",      "pen_tool",       SP_VERB_CONTEXT_PEN, SP_VERB_CONTEXT_PEN_PREFS },
	{ "/tools/calligraphic", "dyna_draw_tool", SP_VERB_CONTEXT_CALLIGRAPHIC, SP_VERB_CONTEXT_CALLIGRAPHIC_PREFS },
	{ "/tools/lpetool",  "lpetool_tool",   SP_VERB_CONTEXT_LPETOOL, SP_VERB_CONTEXT_LPETOOL_PREFS },
	{ "/tools/eraser",   "eraser_tool",    SP_VERB_CONTEXT_ERASER, SP_VERB_CONTEXT_ERASER_PREFS },
#if HAVE_POTRACE
	{ "/tools/paintbucket",    "paintbucket_tool",     SP_VERB_CONTEXT_PAINTBUCKET, SP_VERB_CONTEXT_PAINTBUCKET_PREFS },
#else
        // Replacement blank action for ToolPaintBucket to prevent loading errors in ui file
	{ "/tools/paintbucket",    "ToolPaintBucket",     SP_VERB_NONE, SP_VERB_NONE },
#endif
	{ "/tools/text",     "text_tool",      SP_VERB_CONTEXT_TEXT, SP_VERB_CONTEXT_TEXT_PREFS },
	{ "/tools/connector","connector_tool", SP_VERB_CONTEXT_CONNECTOR, SP_VERB_CONTEXT_CONNECTOR_PREFS },
	{ "/tools/gradient", "gradient_tool",  SP_VERB_CONTEXT_GRADIENT, SP_VERB_CONTEXT_GRADIENT_PREFS },
	{ "/tools/mesh",     "mesh_tool",      SP_VERB_CONTEXT_MESH, SP_VERB_CONTEXT_MESH_PREFS },
	{ "/tools/dropper",  "dropper_tool",   SP_VERB_CONTEXT_DROPPER, SP_VERB_CONTEXT_DROPPER_PREFS },
	{ nullptr, nullptr, 0, 0 }
};

static struct {
    gchar const *type_name;
    gchar const *data_name;
    GtkWidget *(*create_func)(SPDesktop *desktop);
    GtkWidget *(*prep_func)(SPDesktop *desktop, GtkActionGroup* mainActions);
    gchar const *ui_name;
    gint swatch_verb_id;
    gchar const *swatch_tool;
    gchar const *swatch_tip;
} const aux_toolboxes[] = {
    { "/tools/select", "select_toolbox", Inkscape::UI::Toolbar::SelectToolbar::create, nullptr,           "SelectToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/nodes",   "node_toolbox",  Inkscape::UI::Toolbar::NodeToolbar::create, nullptr,             "NodeToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/tweak",   "tweak_toolbox",   Inkscape::UI::Toolbar::TweakToolbar::create, nullptr,          "TweakToolbar",
      SP_VERB_CONTEXT_TWEAK_PREFS, "/tools/tweak", N_("Color/opacity used for color tweaking")},
    { "/tools/spray",   "spray_toolbox",  Inkscape::UI::Toolbar::SprayToolbar::create, nullptr,  "SprayToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/zoom",   "zoom_toolbox",   Inkscape::UI::Toolbar::ZoomToolbar::create, nullptr, "ZoomToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/measure",   "measure_toolbox", Inkscape::UI::Toolbar::MeasureToolbar::create, nullptr,         "MeasureToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/shapes/star",   "star_toolbox",   Inkscape::UI::Toolbar::StarToolbar::create,   nullptr,  "StarToolbar",
      SP_VERB_CONTEXT_STAR_PREFS,   "/tools/shapes/star",     N_("Style of new stars")},
    { "/tools/shapes/rect",   "rect_toolbox",   Inkscape::UI::Toolbar::RectToolbar::create, nullptr,             "RectToolbar",
      SP_VERB_CONTEXT_RECT_PREFS,   "/tools/shapes/rect",     N_("Style of new rectangles")},
    { "/tools/shapes/3dbox",  "3dbox_toolbox",  Inkscape::UI::Toolbar::Box3DToolbar::create, nullptr,            "3DBoxToolbar",
      SP_VERB_CONTEXT_3DBOX_PREFS,  "/tools/shapes/3dbox",    N_("Style of new 3D boxes")},
    { "/tools/shapes/arc",    "arc_toolbox",    Inkscape::UI::Toolbar::ArcToolbar::create, nullptr,               "ArcToolbar",
      SP_VERB_CONTEXT_ARC_PREFS,    "/tools/shapes/arc",      N_("Style of new ellipses")},
    { "/tools/shapes/spiral", "spiral_toolbox", Inkscape::UI::Toolbar::SpiralToolbar::create, nullptr,           "SpiralToolbar",
      SP_VERB_CONTEXT_SPIRAL_PREFS, "/tools/shapes/spiral",   N_("Style of new spirals")},
    { "/tools/freehand/pencil", "pencil_toolbox", Inkscape::UI::Toolbar::PencilToolbar::create_pencil, nullptr,  "PencilToolbar",
      SP_VERB_CONTEXT_PENCIL_PREFS, "/tools/freehand/pencil", N_("Style of new paths created by Pencil")},
    { "/tools/freehand/pen", "pen_toolbox", Inkscape::UI::Toolbar::PencilToolbar::create_pen, nullptr,     "PenToolbar",
      SP_VERB_CONTEXT_PEN_PREFS,    "/tools/freehand/pen",    N_("Style of new paths created by Pen")},
    { "/tools/calligraphic", "calligraphy_toolbox", Inkscape::UI::Toolbar::CalligraphyToolbar::create, nullptr, "CalligraphyToolbar",
      SP_VERB_CONTEXT_CALLIGRAPHIC_PREFS, "/tools/calligraphic", N_("Style of new calligraphic strokes")},
    { "/tools/eraser", "eraser_toolbox", Inkscape::UI::Toolbar::EraserToolbar::create, nullptr, "EraserToolbar",
      SP_VERB_CONTEXT_ERASER_PREFS, "/tools/eraser", _("TBD")},
    { "/tools/lpetool", "lpetool_toolbox", Inkscape::UI::Toolbar::LPEToolbar::create, nullptr, "LPEToolToolbar",
      SP_VERB_CONTEXT_LPETOOL_PREFS, "/tools/lpetool", _("TBD")},
    // If you change TextToolbar here, change it also in desktop-widget.cpp
    { "/tools/text",   "text_toolbox",   nullptr, Inkscape::UI::Toolbar::TextToolbar::prep, "TextToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/dropper", "dropper_toolbox", Inkscape::UI::Toolbar::DropperToolbar::create, nullptr,         "DropperToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/connector", "connector_toolbox", Inkscape::UI::Toolbar::ConnectorToolbar::create, nullptr,  "ConnectorToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/gradient", "gradient_toolbox", Inkscape::UI::Toolbar::GradientToolbar::create, nullptr, "GradientToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
    { "/tools/mesh", "mesh_toolbox", nullptr, Inkscape::UI::Toolbar::MeshToolbar::prep, "MeshToolbar",
      SP_VERB_INVALID, nullptr, nullptr},
#if HAVE_POTRACE
    { "/tools/paintbucket",  "paintbucket_toolbox", Inkscape::UI::Toolbar::PaintbucketToolbar::create, nullptr, "PaintbucketToolbar",
      SP_VERB_CONTEXT_PAINTBUCKET_PREFS, "/tools/paintbucket", N_("Style of Paint Bucket fill objects")},
#else
    { "/tools/paintbucket",  "paintbucket_toolbox",  nullptr, nullptr, "PaintbucketToolbar", SP_VERB_NONE, "/tools/paintbucket", N_("Disabled")},
#endif
    { nullptr, nullptr, nullptr, nullptr, nullptr, SP_VERB_INVALID, nullptr, nullptr }
};


static Glib::RefPtr<Gtk::ActionGroup> create_or_fetch_actions( SPDesktop* desktop );

static void setup_snap_toolbox(GtkWidget *toolbox, SPDesktop *desktop);

static void setup_tool_toolbox(GtkWidget *toolbox, SPDesktop *desktop);
static void update_tool_toolbox(SPDesktop *desktop, ToolBase *eventcontext, GtkWidget *toolbox);

static void setup_aux_toolbox(GtkWidget *toolbox, SPDesktop *desktop);
static void update_aux_toolbox(SPDesktop *desktop, ToolBase *eventcontext, GtkWidget *toolbox);

static void setup_commands_toolbox(GtkWidget *toolbox, SPDesktop *desktop);
static void update_commands_toolbox(SPDesktop *desktop, ToolBase *eventcontext, GtkWidget *toolbox);

static void trigger_sp_action( GtkAction* /*act*/, gpointer user_data )
{
    SPAction* targetAction = SP_ACTION(user_data);
    if ( targetAction ) {
        sp_action_perform( targetAction, nullptr );
    }
}

static GtkAction* create_action_for_verb( Inkscape::Verb* verb, Inkscape::UI::View::View* view, GtkIconSize size )
{
    GtkAction* act = nullptr;

    SPAction* targetAction = verb->get_action(Inkscape::ActionContext(view));
    InkAction* inky = ink_action_new( verb->get_id(), _(verb->get_name()), verb->get_tip(), verb->get_image(), size  );
    act = GTK_ACTION(inky);
    gtk_action_set_sensitive( act, targetAction->sensitive );

    g_signal_connect( G_OBJECT(inky), "activate", G_CALLBACK(trigger_sp_action), targetAction );

    // FIXME: memory leak: this is not unrefed anywhere
    g_object_ref(G_OBJECT(targetAction));
    g_object_set_data_full(G_OBJECT(inky), "SPAction", (void*) targetAction, (GDestroyNotify) &g_object_unref);
    targetAction->signal_set_sensitive.connect(
        sigc::bind<0>(
            sigc::ptr_fun(&gtk_action_set_sensitive),
            GTK_ACTION(inky)));

    return act;
}

static std::map<SPDesktop*, Glib::RefPtr<Gtk::ActionGroup> > groups;

static void desktopDestructHandler(SPDesktop *desktop)
{
    std::map<SPDesktop*, Glib::RefPtr<Gtk::ActionGroup> >::iterator it = groups.find(desktop);
    if (it != groups.end())
    {
        groups.erase(it);
    }
}

static Glib::RefPtr<Gtk::ActionGroup> create_or_fetch_actions( SPDesktop* desktop )
{
    Inkscape::UI::View::View *view = desktop;
    gint verbsToUse[] = {
        // disabled until we have icons for them:
        //find
        //SP_VERB_EDIT_TILE,
        //SP_VERB_EDIT_UNTILE,
        SP_VERB_DIALOG_ALIGN_DISTRIBUTE,
        SP_VERB_DIALOG_DISPLAY,
        SP_VERB_DIALOG_FILL_STROKE,
        SP_VERB_DIALOG_NAMEDVIEW,
        SP_VERB_DIALOG_TEXT,
        SP_VERB_DIALOG_XML_EDITOR,
        SP_VERB_DIALOG_LAYERS,
        SP_VERB_EDIT_CLONE,
        SP_VERB_EDIT_COPY,
        SP_VERB_EDIT_CUT,
        SP_VERB_EDIT_DUPLICATE,
        SP_VERB_EDIT_PASTE,
        SP_VERB_EDIT_REDO,
        SP_VERB_EDIT_UNDO,
        SP_VERB_EDIT_UNLINK_CLONE,
        //SP_VERB_FILE_EXPORT,
        SP_VERB_DIALOG_EXPORT,
        SP_VERB_FILE_IMPORT,
        SP_VERB_FILE_NEW,
        SP_VERB_FILE_OPEN,
        SP_VERB_FILE_PRINT,
        SP_VERB_FILE_SAVE,
        SP_VERB_OBJECT_TO_CURVE,
        SP_VERB_SELECTION_GROUP,
        SP_VERB_SELECTION_OUTLINE,
        SP_VERB_SELECTION_UNGROUP,
        SP_VERB_ZOOM_1_1,
        SP_VERB_ZOOM_1_2,
        SP_VERB_ZOOM_2_1,
        SP_VERB_ZOOM_DRAWING,
        SP_VERB_ZOOM_IN,
        SP_VERB_ZOOM_NEXT,
        SP_VERB_ZOOM_OUT,
        SP_VERB_ZOOM_PAGE,
        SP_VERB_ZOOM_PAGE_WIDTH,
        SP_VERB_ZOOM_PREV,
        SP_VERB_ZOOM_SELECTION
    };

    GtkIconSize toolboxSize = ToolboxFactory::prefToSize("/toolbox/small");
    Glib::RefPtr<Gtk::ActionGroup> mainActions;
    if (desktop == nullptr)
    {
        return mainActions;
    }

    if ( groups.find(desktop) != groups.end() ) {
        mainActions = groups[desktop];
    }

    if ( !mainActions ) {
        mainActions = Gtk::ActionGroup::create("main");
        groups[desktop] = mainActions;
        desktop->connectDestroy(&desktopDestructHandler);
    }

    for (int i : verbsToUse) {
        Inkscape::Verb* verb = Inkscape::Verb::get(i);
        if ( verb ) {
            if (!mainActions->get_action(verb->get_id())) {
                GtkAction* act = create_action_for_verb( verb, view, toolboxSize );
                mainActions->add(Glib::wrap(act));
            }
        }
    }

    if ( !mainActions->get_action("ToolZoom") ) {
        for ( guint i = 0; i < G_N_ELEMENTS(tools) && tools[i].type_name; i++ ) {
            Glib::RefPtr<VerbAction> va = VerbAction::create(Inkscape::Verb::get(tools[i].verb), Inkscape::Verb::get(tools[i].doubleclick_verb), view);
            if ( va ) {
                mainActions->add(va);
                if ( i == 0 ) {
                    va->set_active(true);
                }
            } else {
                // This creates a blank action using the data_name, this can replace
                // tools that have been disabled by compile time options.
                Glib::RefPtr<Gtk::Action> act = Gtk::Action::create(Glib::ustring(tools[i].data_name));
                act->set_sensitive(false);
                mainActions->add(act);
            }
        }
    }

    return mainActions;
}


static GtkWidget* toolboxNewCommon( GtkWidget* tb, BarId id, GtkPositionType /*handlePos*/ )
{
    g_object_set_data(G_OBJECT(tb), "desktop", nullptr);

    gtk_widget_set_sensitive(tb, FALSE);

    GtkWidget *hb = gtk_event_box_new(); // A simple, neutral container.
    gtk_widget_set_name(hb, "ToolboxCommon");

    gtk_container_add(GTK_CONTAINER(hb), tb);
    gtk_widget_show(GTK_WIDGET(tb));

    sigc::connection* conn = new sigc::connection;
    g_object_set_data(G_OBJECT(hb), "event_context_connection", conn);

    gpointer val = GINT_TO_POINTER(id);
    g_object_set_data(G_OBJECT(hb), BAR_ID_KEY, val);

    return hb;
}

GtkWidget *ToolboxFactory::createToolToolbox()
{
    auto tb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(tb, "ToolToolbox");
    gtk_box_set_homogeneous(GTK_BOX(tb), FALSE);

    return toolboxNewCommon( tb, BAR_TOOL, GTK_POS_TOP );
}

GtkWidget *ToolboxFactory::createAuxToolbox()
{
    auto tb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(tb, "AuxToolbox");
    gtk_box_set_homogeneous(GTK_BOX(tb), FALSE);

    return toolboxNewCommon( tb, BAR_AUX, GTK_POS_LEFT );
}

//####################################
//# Commands Bar
//####################################

GtkWidget *ToolboxFactory::createCommandsToolbox()
{
    auto tb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(tb, "CommandsToolbox");
    gtk_box_set_homogeneous(GTK_BOX(tb), FALSE);

    return toolboxNewCommon( tb, BAR_COMMANDS, GTK_POS_LEFT );
}

GtkWidget *ToolboxFactory::createSnapToolbox()
{
    auto tb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(tb, "SnapToolbox");
    gtk_box_set_homogeneous(GTK_BOX(tb), FALSE);

    return toolboxNewCommon( tb, BAR_SNAP, GTK_POS_LEFT );
}

static GtkWidget* createCustomSlider( GtkAdjustment *adjustment, gdouble climbRate, guint digits, Inkscape::UI::Widget::UnitTracker *unit_tracker)
{
    auto adj = Glib::wrap(adjustment, true);
    auto inkSpinner = new Inkscape::UI::Widget::SpinButton(adj, climbRate, digits);
    inkSpinner->addUnitTracker(unit_tracker);
    inkSpinner = Gtk::manage( inkSpinner );
    GtkWidget *widget = GTK_WIDGET( inkSpinner->gobj() );
    return widget;
}

EgeAdjustmentAction * create_adjustment_action( gchar const *name,
                                                       gchar const *label, gchar const *shortLabel, gchar const *tooltip,
                                                       Glib::ustring const &path, gdouble def,
                                                       gboolean altx, gchar const *altx_mark,
                                                       gdouble lower, gdouble upper, gdouble step, gdouble page,
                                                       gchar const** descrLabels, gdouble const* descrValues, guint descrCount,
                                                       Inkscape::UI::Widget::UnitTracker *unit_tracker,
                                                       gdouble climb/* = 0.1*/, guint digits/* = 3*/, double factor/* = 1.0*/ )
{
    static bool init = false;
    if ( !init ) {
        init = true;
        ege_adjustment_action_set_compact_tool_factory( createCustomSlider );
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    GtkAdjustment* adj = GTK_ADJUSTMENT( gtk_adjustment_new( prefs->getDouble(path, def) * factor,
                                                             lower, upper, step, page, 0 ) );

    EgeAdjustmentAction* act = ege_adjustment_action_new( adj, name, label, tooltip, nullptr, climb, digits, unit_tracker );
    if ( shortLabel ) {
        g_object_set( act, "short_label", shortLabel, NULL );
    }

    if ( (descrCount > 0) && descrLabels && descrValues ) {
        ege_adjustment_action_set_descriptions( act, descrLabels, descrValues, descrCount );
    }

    // The EgeAdjustmentAction class uses this to create a data member
    // with the specified name, that simply points to the object itself.
    // It appears to only be used by the DesktopWidget to find the named
    // object
    //
    // TODO: Get rid of this and look up widgets by name instead.
    if ( altx && altx_mark ) {
        g_object_set( G_OBJECT(act), "self-id", altx_mark, NULL );
    }

    if (unit_tracker) {
        unit_tracker->addAdjustment(adj);
    }

    // Using a cast just to make sure we pass in the right kind of function pointer
    g_object_set( G_OBJECT(act), "tool-post", static_cast<EgeWidgetFixup>(sp_set_font_size_smaller), NULL );

    return act;
}


void ToolboxFactory::setToolboxDesktop(GtkWidget *toolbox, SPDesktop *desktop)
{
    sigc::connection *conn = static_cast<sigc::connection*>(g_object_get_data(G_OBJECT(toolbox),
                                                                              "event_context_connection"));

    BarId id = static_cast<BarId>( GPOINTER_TO_INT(g_object_get_data(G_OBJECT(toolbox), BAR_ID_KEY)) );

    SetupFunction setup_func = nullptr;
    UpdateFunction update_func = nullptr;

    switch (id) {
        case BAR_TOOL:
            setup_func = setup_tool_toolbox;
            update_func = update_tool_toolbox;
            break;

        case BAR_AUX:
            toolbox = gtk_bin_get_child(GTK_BIN(toolbox));
            setup_func = setup_aux_toolbox;
            update_func = update_aux_toolbox;
            break;

        case BAR_COMMANDS:
            setup_func = setup_commands_toolbox;
            update_func = update_commands_toolbox;
            break;

        case BAR_SNAP:
            setup_func = setup_snap_toolbox;
            update_func = updateSnapToolbox;
            break;
        default:
            g_warning("Unexpected toolbox id encountered.");
    }

    gpointer ptr = g_object_get_data(G_OBJECT(toolbox), "desktop");
    SPDesktop *old_desktop = static_cast<SPDesktop*>(ptr);

    if (old_desktop) {
        std::vector<Gtk::Widget*> children = Glib::wrap(GTK_CONTAINER(toolbox))->get_children();
        for ( auto i:children ) {
            gtk_container_remove( GTK_CONTAINER(toolbox), i->gobj() );
        }
    }

    g_object_set_data(G_OBJECT(toolbox), "desktop", (gpointer)desktop);

    if (desktop && setup_func && update_func) {
        gtk_widget_set_sensitive(toolbox, TRUE);
        setup_func(toolbox, desktop);
        update_func(desktop, desktop->event_context, toolbox);
        *conn = desktop->connectEventContextChanged(sigc::bind (sigc::ptr_fun(update_func), toolbox));
    } else {
        gtk_widget_set_sensitive(toolbox, FALSE);
    }

} // end of sp_toolbox_set_desktop()


static void setupToolboxCommon( GtkWidget *toolbox,
                                SPDesktop *desktop,
                                gchar const *ui_file,
                                gchar const* toolbarName,
                                gchar const* sizePref )
{
    Glib::RefPtr<Gtk::ActionGroup> mainActions = create_or_fetch_actions( desktop );
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    GtkUIManager* mgr = gtk_ui_manager_new();
    GError* err = nullptr;

    GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL;

    gtk_ui_manager_insert_action_group( mgr, mainActions->gobj(), 0 );

    Glib::ustring filename = get_filename(UIS, ui_file);
    gtk_ui_manager_add_ui_from_file( mgr, filename.c_str(), &err );
    if(err) {
        g_warning("Failed to load %s: %s", filename.c_str(), err->message);
        g_error_free(err);
        return;
    }

    GtkWidget* toolBar = gtk_ui_manager_get_widget( mgr, toolbarName );
    if ( prefs->getBool("/toolbox/icononly", true) ) {
        gtk_toolbar_set_style( GTK_TOOLBAR(toolBar), GTK_TOOLBAR_ICONS );
    }

    GtkIconSize toolboxSize = ToolboxFactory::prefToSize(sizePref);
    gtk_toolbar_set_icon_size( GTK_TOOLBAR(toolBar), static_cast<GtkIconSize>(toolboxSize) );

    GtkPositionType pos = static_cast<GtkPositionType>(GPOINTER_TO_INT(g_object_get_data( G_OBJECT(toolbox), HANDLE_POS_MARK )));
    orientation = ((pos == GTK_POS_LEFT) || (pos == GTK_POS_RIGHT)) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    gtk_orientable_set_orientation (GTK_ORIENTABLE(toolBar), orientation);
    gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolBar), TRUE);

    g_object_set_data(G_OBJECT(toolBar), "desktop", nullptr);

    GtkWidget* child = gtk_bin_get_child(GTK_BIN(toolbox));
    if ( child ) {
        gtk_container_remove( GTK_CONTAINER(toolbox), child );
    }

    gtk_container_add( GTK_CONTAINER(toolbox), toolBar );
}

#define noDUMP_DETAILS 1

void ToolboxFactory::setOrientation(GtkWidget* toolbox, GtkOrientation orientation)
{
#if DUMP_DETAILS
    g_message("Set orientation for %p to be %d", toolbox, orientation);
    GType type = G_OBJECT_TYPE(toolbox);
    g_message("        [%s]", g_type_name(type));
    g_message("             %p", g_object_get_data(G_OBJECT(toolbox), BAR_ID_KEY));
#endif

    GtkPositionType pos = (orientation == GTK_ORIENTATION_HORIZONTAL) ? GTK_POS_LEFT : GTK_POS_TOP;

    if (GTK_IS_BIN(toolbox)) {
#if DUMP_DETAILS
        g_message("            is a BIN");
#endif // DUMP_DETAILS
        GtkWidget* child = gtk_bin_get_child(GTK_BIN(toolbox));
        if (child) {
#if DUMP_DETAILS
            GType type2 = G_OBJECT_TYPE(child);
            g_message("            child    [%s]", g_type_name(type2));
#endif // DUMP_DETAILS

            if (GTK_IS_BOX(child)) {
#if DUMP_DETAILS
                g_message("                is a BOX");
#endif // DUMP_DETAILS

                std::vector<Gtk::Widget*> children = Glib::wrap(GTK_CONTAINER(child))->get_children();
                if (!children.empty()) {
                    for (auto curr:children) {
                        GtkWidget* child2 = curr->gobj();
#if DUMP_DETAILS
                        GType type3 = G_OBJECT_TYPE(child2);
                        g_message("                child2   [%s]", g_type_name(type3));
#endif // DUMP_DETAILS

                        if (GTK_IS_CONTAINER(child2)) {
                            std::vector<Gtk::Widget*> children2 = Glib::wrap(GTK_CONTAINER(child2))->get_children();
                            if (!children2.empty()) {
                                for (auto curr2:children2) {
                                    GtkWidget* child3 = curr2->gobj();
#if DUMP_DETAILS
                                    GType type4 = G_OBJECT_TYPE(child3);
                                    g_message("                    child3   [%s]", g_type_name(type4));
#endif // DUMP_DETAILS
                                    if (GTK_IS_TOOLBAR(child3)) {
                                        GtkToolbar* childBar = GTK_TOOLBAR(child3);
                                        gtk_orientable_set_orientation(GTK_ORIENTABLE(childBar), orientation);
                                    }
                                }
                            }
                        }


                        if (GTK_IS_TOOLBAR(child2)) {
                            GtkToolbar* childBar = GTK_TOOLBAR(child2);
                            gtk_orientable_set_orientation(GTK_ORIENTABLE(childBar), orientation);
                        } else {
                            g_message("need to add dynamic switch");
                        }
                    }
                } else {
                    // The call is being made before the toolbox proper has been setup.
                    g_object_set_data(G_OBJECT(toolbox), HANDLE_POS_MARK, GINT_TO_POINTER(pos));
                }
            } else if (GTK_IS_TOOLBAR(child)) {
                GtkToolbar* toolbar = GTK_TOOLBAR(child);
                gtk_orientable_set_orientation( GTK_ORIENTABLE(toolbar), orientation );
            }
        }
    }
}

void setup_tool_toolbox(GtkWidget *toolbox, SPDesktop *desktop)
{
    setupToolboxCommon( toolbox, desktop,
            "tool-toolbar.ui",
            "/ui/ToolToolbar",
            "/toolbox/tools/small");
}

void update_tool_toolbox( SPDesktop *desktop, ToolBase *eventcontext, GtkWidget * /*toolbox*/ )
{
    gchar const *const tname = ( eventcontext
                                 ? eventcontext->getPrefsPath().c_str() //g_type_name(G_OBJECT_TYPE(eventcontext))
                                 : nullptr );
    Glib::RefPtr<Gtk::ActionGroup> mainActions = create_or_fetch_actions( desktop );

    for (int i = 0 ; tools[i].type_name ; i++ ) {
        Glib::RefPtr<Gtk::Action> act = mainActions->get_action( Inkscape::Verb::get(tools[i].verb)->get_id() );
        if ( act ) {
            bool setActive = tname && !strcmp(tname, tools[i].type_name);
            Glib::RefPtr<VerbAction> verbAct = Glib::RefPtr<VerbAction>::cast_dynamic(act);
            if ( verbAct ) {
                verbAct->set_active(setActive);
            }
        }
    }
}

/**
 * \brief Generate the auxiliary toolbox
 *
 * \details This is the one that appears below the main menu, and contains
 *          tool-specific toolbars.  Each toolbar is created here, using
 *          either:
 *          * Its "create" method - this directly prepares a GtkToolbar
 *            widget, containing all the tools, or
 *          * Its "prep" method - this defines a set of GtkActions, which
 *            are later used to populate a toolbar.
 *          The actual method used for each toolbar is specified in the
 *          "aux_toolboxes" array, defined above.
 *
 * \todo Needs to be rewritten so that GtkActions and GtkUIManager
 *       are not used.  This means that the "prep" approach is deprecated
 *       and we should adapt all toolbars to have a "create" method instead.
 */
void setup_aux_toolbox(GtkWidget *toolbox, SPDesktop *desktop)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    GtkSizeGroup* grouper = gtk_size_group_new( GTK_SIZE_GROUP_BOTH );
    Glib::RefPtr<Gtk::ActionGroup> mainActions = create_or_fetch_actions( desktop );

    // The UI Manager creates widgets based on the definitions in the
    // "select-toolbar.ui" file.  This is only used with the "prep"
    // method of toolbar-creation
    GtkUIManager* mgr = gtk_ui_manager_new();
    GError *err = nullptr;
    gtk_ui_manager_insert_action_group( mgr, mainActions->gobj(), 0 );
    Glib::ustring filename = get_filename(UIS, "select-toolbar.ui");
    guint ret = gtk_ui_manager_add_ui_from_file(mgr, filename.c_str(), &err);
    if(err) {
      g_warning("Failed to load aux toolbar %s: %s", filename.c_str(), err->message);
      g_error_free(err);
      return;
    }

    // For the "prep" method, we create a "fake" set of toolbars
    // that just contain a set of GtkActions.  These are stored
    // in the "dataHolders" map.
    std::map<std::string, GtkWidget*> dataHolders;

    // Loop through all the toolboxes and create them using either
    // their "prep" or "create" methods.
    for (int i = 0 ; aux_toolboxes[i].type_name ; i++ ) {
        if ( aux_toolboxes[i].prep_func ) {

            // For the "prep" method, create a "fake" toolbar
            // that only contains a set of GtkActions.  In other
            // words, this doesn't actually show anything... it
            // just defines behaviour.
            GtkWidget* kludge = aux_toolboxes[i].prep_func(desktop, mainActions->gobj());
            gtk_widget_set_name( kludge, "Kludge" );
            dataHolders[aux_toolboxes[i].type_name] = kludge;
        } else if (aux_toolboxes[i].create_func) {

            // For the "create" method, directly create a "real" toolbar,
            // which contains visible, fully functional widgets.  Note that
            // this should also contain any swatches that are needed.
            GtkWidget *sub_toolbox = aux_toolboxes[i].create_func(desktop);
            gtk_widget_set_name( sub_toolbox, "SubToolBox" );

            auto holder = gtk_grid_new();
            gtk_grid_attach(GTK_GRID(holder), sub_toolbox, 0, 0, 1, 1);

            // This part is just for styling
            if ( prefs->getBool( "/toolbox/icononly", true) ) {
                gtk_toolbar_set_style( GTK_TOOLBAR(sub_toolbox), GTK_TOOLBAR_ICONS );
            }

            GtkIconSize toolboxSize = ToolboxFactory::prefToSize("/toolbox/small");
            gtk_toolbar_set_icon_size( GTK_TOOLBAR(sub_toolbox), static_cast<GtkIconSize>(toolboxSize) );
            gtk_widget_set_hexpand(sub_toolbox, TRUE);

            // Add a swatch widget if one was specified
            if ( aux_toolboxes[i].swatch_verb_id != SP_VERB_INVALID ) {
                auto swatch = new Inkscape::UI::Widget::StyleSwatch( nullptr, _(aux_toolboxes[i].swatch_tip) );
                swatch->setDesktop( desktop );
                swatch->setClickVerb( aux_toolboxes[i].swatch_verb_id );
                swatch->setWatchedTool( aux_toolboxes[i].swatch_tool, true );

#if GTKMM_CHECK_VERSION(3,12,0)
                swatch->set_margin_start(AUX_BETWEEN_BUTTON_GROUPS);
                swatch->set_margin_end(AUX_BETWEEN_BUTTON_GROUPS);
#else
                swatch->set_margin_left(AUX_BETWEEN_BUTTON_GROUPS);
                swatch->set_margin_right(AUX_BETWEEN_BUTTON_GROUPS);
#endif

                swatch->set_margin_top(AUX_SPACING);
                swatch->set_margin_bottom(AUX_SPACING);

                auto swatch_ = GTK_WIDGET( swatch->gobj() );
                gtk_grid_attach( GTK_GRID(holder), swatch_, 1, 0, 1, 1);
            }

            // Add the new toolbar into the toolbox (i.e., make it the visible toolbar)
            // and also store a pointer to it inside the toolbox.  This allows the
            // active toolbar to be changed.
            gtk_container_add(GTK_CONTAINER(toolbox), holder);
            gtk_size_group_add_widget(grouper, holder);
            sp_set_font_size_smaller( holder );

            // TODO: We could make the toolbox a custom subclass of GtkEventBox
            //       so that we can store a list of toolbars, rather than using
            //       GObject data
            g_object_set_data(G_OBJECT(toolbox), aux_toolboxes[i].data_name, holder);
        } else {
            g_warning("Could not create toolbox %s", aux_toolboxes[i].ui_name);
        }
    }

    // Second pass to create toolbars *after* all GtkActions are created
    // This is only used for toolbars that are being created using the "prep"
    // method
    for (int i = 0 ; aux_toolboxes[i].type_name ; i++ ) {
        if ( aux_toolboxes[i].prep_func ) {

            // Get the previously created "fake" toolbar that just contains
            // invisible GtkAction definitions
            auto kludge = dataHolders[aux_toolboxes[i].type_name];

            // The thing that we put into the toolbox is actually a GtkGrid.
            // It contains three elements... from left-to-right:
            // * A "real" toolbar containing all the visible, fully functional
            //   widgets
            // * (optionally) A swatch widget for use with that toolbar
            // * The "fake" toolbar containing the action definitions, which we
            //   created previously
            auto holder = gtk_grid_new();
            gtk_widget_set_name( holder, aux_toolboxes[i].ui_name );

            // First pack the "fake" toolbar with the action definitions
            gtk_grid_attach( GTK_GRID(holder), kludge, 2, 0, 1, 1);

            // Now, use the UI Manager to create a "real" toolbar.  This works
            // because the actions needed by the UI file have all been defined
            // in the "fake" toolbar
            gchar* tmp = g_strdup_printf( "/ui/%s", aux_toolboxes[i].ui_name );
            GtkWidget* toolBar = gtk_ui_manager_get_widget( mgr, tmp );
            g_free( tmp );
            tmp = nullptr;

            // This part is just for styling
            if ( prefs->getBool( "/toolbox/icononly", true) ) {
                gtk_toolbar_set_style( GTK_TOOLBAR(toolBar), GTK_TOOLBAR_ICONS );
            }

            GtkIconSize toolboxSize = ToolboxFactory::prefToSize("/toolbox/small");
            gtk_toolbar_set_icon_size( GTK_TOOLBAR(toolBar), static_cast<GtkIconSize>(toolboxSize) );
            gtk_widget_set_hexpand(toolBar, TRUE);
            gtk_grid_attach( GTK_GRID(holder), toolBar, 0, 0, 1, 1);

            // Add a swatch widget if one was specified
            if ( aux_toolboxes[i].swatch_verb_id != SP_VERB_INVALID ) {
                Inkscape::UI::Widget::StyleSwatch *swatch = new Inkscape::UI::Widget::StyleSwatch( nullptr, _(aux_toolboxes[i].swatch_tip) );
                swatch->setDesktop( desktop );
                swatch->setClickVerb( aux_toolboxes[i].swatch_verb_id );
                swatch->setWatchedTool( aux_toolboxes[i].swatch_tool, true );

#if GTKMM_CHECK_VERSION(3,12,0)
                swatch->set_margin_start(AUX_BETWEEN_BUTTON_GROUPS);
                swatch->set_margin_end(AUX_BETWEEN_BUTTON_GROUPS);
#else
                swatch->set_margin_left(AUX_BETWEEN_BUTTON_GROUPS);
                swatch->set_margin_right(AUX_BETWEEN_BUTTON_GROUPS);
#endif

                swatch->set_margin_top(AUX_SPACING);
                swatch->set_margin_bottom(AUX_SPACING);

                auto swatch_ = GTK_WIDGET( swatch->gobj() );
                gtk_grid_attach( GTK_GRID(holder), swatch_, 1, 0, 1, 1);
            }
            if(i==0){
                gtk_widget_show_all( holder );
            } else {
                gtk_widget_show_now( holder );
            }
            sp_set_font_size_smaller( holder );

            gtk_size_group_add_widget( grouper, holder );

            // Finally add the grid to the toolbox.
            // As described above, a pointer is also stored so that toolbars can be
            // switched later.
            gtk_container_add( GTK_CONTAINER(toolbox), holder );
            g_object_set_data( G_OBJECT(toolbox), aux_toolboxes[i].data_name, holder );
        }
    }

    g_object_unref( G_OBJECT(grouper) );
}

void update_aux_toolbox(SPDesktop * /*desktop*/, ToolBase *eventcontext, GtkWidget *toolbox)
{
    gchar const *tname = ( eventcontext
                           ? eventcontext->getPrefsPath().c_str() //g_type_name(G_OBJECT_TYPE(eventcontext))
                           : nullptr );
    for (int i = 0 ; aux_toolboxes[i].type_name ; i++ ) {
        GtkWidget *sub_toolbox = GTK_WIDGET(g_object_get_data(G_OBJECT(toolbox), aux_toolboxes[i].data_name));
        if (tname && !strcmp(tname, aux_toolboxes[i].type_name)) {
            gtk_widget_show_now(sub_toolbox);
            g_object_set_data(G_OBJECT(toolbox), "shows", sub_toolbox);
        } else {
            gtk_widget_hide(sub_toolbox);
        }
    }
}

void setup_commands_toolbox(GtkWidget *toolbox, SPDesktop *desktop)
{
    setupToolboxCommon( toolbox, desktop,
            "commands-toolbar.ui",
            "/ui/CommandsToolbar",
            "/toolbox/small" );
}

void update_commands_toolbox(SPDesktop * /*desktop*/, ToolBase * /*eventcontext*/, GtkWidget * /*toolbox*/)
{
}

static void toggle_snap_callback(GtkToggleAction *act, gpointer data) //data points to the toolbox
{
    if (g_object_get_data(G_OBJECT(data), "freeze" )) {
        return;
    }

    gpointer ptr = g_object_get_data(G_OBJECT(data), "desktop");
    g_assert(ptr != nullptr);

    SPDesktop *dt = reinterpret_cast<SPDesktop*>(ptr);
    SPNamedView *nv = dt->getNamedView();
    if (nv == nullptr) {
        g_warning("No namedview specified (in toggle_snap_callback)!");
        return;
    }

    SPDocument *doc = nv->document;
    Inkscape::XML::Node *repr = nv->getRepr();

    if (repr == nullptr) {
        g_warning("This namedview doesn't have a xml representation attached!");
        return;
    }

    DocumentUndo::ScopedInsensitive _no_undo(doc);

    bool v = false;
    SPAttributeEnum attr = (SPAttributeEnum) GPOINTER_TO_INT(g_object_get_data(G_OBJECT(act), "SP_ATTR_INKSCAPE"));

    switch (attr) {
        case SP_ATTR_INKSCAPE_SNAP_GLOBAL:
            dt->toggleSnapGlobal();
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE);
            sp_repr_set_boolean(repr, "inkscape:bbox-paths", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_CORNER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_CORNER);
            sp_repr_set_boolean(repr, "inkscape:bbox-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_NODE:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH);
            sp_repr_set_boolean(repr, "inkscape:object-paths", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH_CLIP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_CLIP);
            sp_repr_set_boolean(repr, "inkscape:snap-path-clip", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH_MASK:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_MASK);
            sp_repr_set_boolean(repr, "inkscape:snap-path-mask", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_NODE_CUSP:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP);
            sp_repr_set_boolean(repr, "inkscape:object-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_NODE_SMOOTH:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH);
            sp_repr_set_boolean(repr, "inkscape:snap-smooth-nodes", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PATH_INTERSECTION:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION);
            sp_repr_set_boolean(repr, "inkscape:snap-intersection-paths", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_OTHERS:
            v = nv->snap_manager.snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY);
            sp_repr_set_boolean(repr, "inkscape:snap-others", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_ROTATION_CENTER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER);
            sp_repr_set_boolean(repr, "inkscape:snap-center", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_GRID:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID);
            sp_repr_set_boolean(repr, "inkscape:snap-grids", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_GUIDE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE);
            sp_repr_set_boolean(repr, "inkscape:snap-to-guides", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_PAGE_BORDER:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER);
            sp_repr_set_boolean(repr, "inkscape:snap-page", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_LINE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-midpoints", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_OBJECT_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-object-midpoints", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_TEXT_BASELINE:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE);
            sp_repr_set_boolean(repr, "inkscape:snap-text-baseline", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox-edge-midpoints", !v);
            break;
        case SP_ATTR_INKSCAPE_SNAP_BBOX_MIDPOINT:
            v = nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_BBOX_MIDPOINT);
            sp_repr_set_boolean(repr, "inkscape:snap-bbox-midpoints", !v);
            break;
        default:
            g_warning("toggle_snap_callback has been called with an ID for which no action has been defined");
            break;
    }

    // The snapping preferences are stored in the document, and therefore toggling makes the document dirty
    doc->setModifiedSinceSave();
}

void setup_snap_toolbox(GtkWidget *toolbox, SPDesktop *desktop)
{
    Glib::RefPtr<Gtk::ActionGroup> mainActions = create_or_fetch_actions(desktop);

    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    {
        // TODO: This is a cludge. On the one hand we have verbs+actions,
        //       on the other we have all these explicit callbacks specified here.
        //       We should really unify these (should save some lines of code as well).
        //       For example, this action could be based on the verb(+action) + PrefsPusher.
        Inkscape::Verb* verb = Inkscape::Verb::get(SP_VERB_TOGGLE_SNAPPING);
        InkToggleAction* act = ink_toggle_action_new(verb->get_id(),
                                                     verb->get_name(), verb->get_tip(), INKSCAPE_ICON("snap"), secondarySize,
                                                     SP_ATTR_INKSCAPE_SNAP_GLOBAL);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapFromBBoxCorner",
                                                     _("Bounding box"), _("Snap bounding boxes"), INKSCAPE_ICON("snap"),
                                                     secondarySize, SP_ATTR_INKSCAPE_SNAP_BBOX);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToBBoxPath",
                                                     _("Bounding box edges"), _("Snap to edges of a bounding box"),
                                                     INKSCAPE_ICON("snap-bounding-box-edges"), secondarySize, SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToBBoxNode",
                                                     _("Bounding box corners"), _("Snap bounding box corners"),
                                                     INKSCAPE_ICON("snap-bounding-box-corners"), secondarySize, SP_ATTR_INKSCAPE_SNAP_BBOX_CORNER);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToFromBBoxEdgeMidpoints",
                                                     _("BBox Edge Midpoints"), _("Snap midpoints of bounding box edges"),
                                                     INKSCAPE_ICON("snap-bounding-box-midpoints"), secondarySize,
                                                     SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToFromBBoxCenters",
                                                     _("BBox Centers"), _("Snapping centers of bounding boxes"),
                                                     INKSCAPE_ICON("snap-bounding-box-center"), secondarySize, SP_ATTR_INKSCAPE_SNAP_BBOX_MIDPOINT);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapFromNode",
                                                     _("Nodes"), _("Snap nodes, paths, and handles"), INKSCAPE_ICON("snap"), secondarySize, SP_ATTR_INKSCAPE_SNAP_NODE);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToItemPath",
                                                     _("Paths"), _("Snap to paths"), INKSCAPE_ICON("snap-nodes-path"), secondarySize,
                                                     SP_ATTR_INKSCAPE_SNAP_PATH);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToPathIntersections",
                                                     _("Path intersections"), _("Snap to path intersections"),
                                                     INKSCAPE_ICON("snap-nodes-intersection"), secondarySize, SP_ATTR_INKSCAPE_SNAP_PATH_INTERSECTION);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToItemNode",
                                                     _("To nodes"), _("Snap cusp nodes, incl. rectangle corners"), INKSCAPE_ICON("snap-nodes-cusp"), secondarySize,
                                                     SP_ATTR_INKSCAPE_SNAP_NODE_CUSP);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToSmoothNodes",
                                                     _("Smooth nodes"), _("Snap smooth nodes, incl. quadrant points of ellipses"), INKSCAPE_ICON("snap-nodes-smooth"),
                                                     secondarySize, SP_ATTR_INKSCAPE_SNAP_NODE_SMOOTH);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToFromLineMidpoints",
                                                     _("Line Midpoints"), _("Snap midpoints of line segments"),
                                                     INKSCAPE_ICON("snap-nodes-midpoint"), secondarySize, SP_ATTR_INKSCAPE_SNAP_LINE_MIDPOINT);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapFromOthers",
                                                     _("Others"), _("Snap other points (centers, guide origins, gradient handles, etc.)"), INKSCAPE_ICON("snap"), secondarySize, SP_ATTR_INKSCAPE_SNAP_OTHERS);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToFromObjectCenters",
                                                     _("Object Centers"), _("Snap centers of objects"),
                                                     INKSCAPE_ICON("snap-nodes-center"), secondarySize, SP_ATTR_INKSCAPE_SNAP_OBJECT_MIDPOINT);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToFromRotationCenter",
                                                     _("Rotation Centers"), _("Snap an item's rotation center"),
                                                     INKSCAPE_ICON("snap-nodes-rotation-center"), secondarySize, SP_ATTR_INKSCAPE_SNAP_ROTATION_CENTER);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToFromTextBaseline",
                                                     _("Text baseline"), _("Snap text anchors and baselines"),
                                                     INKSCAPE_ICON("snap-text-baseline"), secondarySize, SP_ATTR_INKSCAPE_SNAP_TEXT_BASELINE);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }


    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToPageBorder",
                                                     _("Page border"), _("Snap to the page border"), INKSCAPE_ICON("snap-page"),
                                                     secondarySize, SP_ATTR_INKSCAPE_SNAP_PAGE_BORDER);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToGrids",
                                                     _("Grids"), _("Snap to grids"), INKSCAPE_ICON("grid-rectangular"), secondarySize,
                                                     SP_ATTR_INKSCAPE_SNAP_GRID);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    {
        InkToggleAction* act = ink_toggle_action_new("ToggleSnapToGuides",
                                                     _("Guides"), _("Snap guides"), INKSCAPE_ICON("guides"), secondarySize,
                                                     SP_ATTR_INKSCAPE_SNAP_GUIDE);

        gtk_action_group_add_action( mainActions->gobj(), GTK_ACTION( act ) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_snap_callback), toolbox );
    }

    setupToolboxCommon( toolbox, desktop,
            "snap-toolbar.ui",
            "/ui/SnapToolbar",
            "/toolbox/secondary" );
}

Glib::ustring ToolboxFactory::getToolboxName(GtkWidget* toolbox)
{
    Glib::ustring name;
    BarId id = static_cast<BarId>( GPOINTER_TO_INT(g_object_get_data(G_OBJECT(toolbox), BAR_ID_KEY)) );
    switch(id) {
        case BAR_TOOL:
            name = "ToolToolbar";
            break;
        case BAR_AUX:
            name = "AuxToolbar";
            break;
        case BAR_COMMANDS:
            name = "CommandsToolbar";
            break;
        case BAR_SNAP:
            name = "SnapToolbar";
            break;
    }

    return name;
}

void ToolboxFactory::updateSnapToolbox(SPDesktop *desktop, ToolBase * /*eventcontext*/, GtkWidget *toolbox)
{
    g_assert(desktop != nullptr);
    g_assert(toolbox != nullptr);

    SPNamedView *nv = desktop->getNamedView();
    if (nv == nullptr) {
        g_warning("Namedview cannot be retrieved (in updateSnapToolbox)!");
        return;
    }

    Glib::RefPtr<Gtk::ActionGroup> mainActions = create_or_fetch_actions(desktop);

    Glib::RefPtr<Gtk::Action> act1 = mainActions->get_action("ToggleSnapGlobal");
    Glib::RefPtr<Gtk::Action> act2 = mainActions->get_action("ToggleSnapFromBBoxCorner");
    Glib::RefPtr<Gtk::Action> act3 = mainActions->get_action("ToggleSnapToBBoxPath");
    Glib::RefPtr<Gtk::Action> act4 = mainActions->get_action("ToggleSnapToBBoxNode");
    Glib::RefPtr<Gtk::Action> act4b = mainActions->get_action("ToggleSnapToFromBBoxEdgeMidpoints");
    Glib::RefPtr<Gtk::Action> act4c = mainActions->get_action("ToggleSnapToFromBBoxCenters");
    Glib::RefPtr<Gtk::Action> act5 = mainActions->get_action("ToggleSnapFromNode");
    Glib::RefPtr<Gtk::Action> act6 = mainActions->get_action("ToggleSnapToItemPath");
    Glib::RefPtr<Gtk::Action> act6b = mainActions->get_action("ToggleSnapToPathIntersections");
    Glib::RefPtr<Gtk::Action> act7 = mainActions->get_action("ToggleSnapToItemNode");
    Glib::RefPtr<Gtk::Action> act8 = mainActions->get_action("ToggleSnapToSmoothNodes");
    Glib::RefPtr<Gtk::Action> act9 = mainActions->get_action("ToggleSnapToFromLineMidpoints");
    Glib::RefPtr<Gtk::Action> act10 = mainActions->get_action("ToggleSnapFromOthers");
    Glib::RefPtr<Gtk::Action> act10b = mainActions->get_action("ToggleSnapToFromObjectCenters");
    Glib::RefPtr<Gtk::Action> act11 = mainActions->get_action("ToggleSnapToFromRotationCenter");
    Glib::RefPtr<Gtk::Action> act11b = mainActions->get_action("ToggleSnapToFromTextBaseline");
    Glib::RefPtr<Gtk::Action> act12 = mainActions->get_action("ToggleSnapToPageBorder");
    Glib::RefPtr<Gtk::Action> act14 = mainActions->get_action("ToggleSnapToGrids");
    Glib::RefPtr<Gtk::Action> act15 = mainActions->get_action("ToggleSnapToGuides");


    if (!act1) {
        return; // The snap actions haven't been defined yet (might be the case during startup)
    }

    // The ..._set_active calls below will toggle the buttons, but this shouldn't lead to
    // changes in our document because we're only updating the UI;
    // Setting the "freeze" parameter to true will block the code in toggle_snap_callback()
    g_object_set_data(G_OBJECT(toolbox), "freeze", GINT_TO_POINTER(TRUE));

    bool const c1 = nv->snap_manager.snapprefs.getSnapEnabledGlobally();
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act1->gobj()), c1);

    bool const c2 = nv->snap_manager.snapprefs.isTargetSnappable(SNAPTARGET_BBOX_CATEGORY);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act2->gobj()), c2);
    gtk_action_set_sensitive(GTK_ACTION(act2->gobj()), c1);

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act3->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_EDGE));
    gtk_action_set_sensitive(GTK_ACTION(act3->gobj()), c1 && c2);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act4->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_CORNER));
    gtk_action_set_sensitive(GTK_ACTION(act4->gobj()), c1 && c2);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act4b->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_EDGE_MIDPOINT));
    gtk_action_set_sensitive(GTK_ACTION(act4b->gobj()), c1 && c2);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act4c->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(SNAPTARGET_BBOX_MIDPOINT));
    gtk_action_set_sensitive(GTK_ACTION(act4c->gobj()), c1 && c2);

    bool const c3 = nv->snap_manager.snapprefs.isTargetSnappable(SNAPTARGET_NODE_CATEGORY);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act5->gobj()), c3);
    gtk_action_set_sensitive(GTK_ACTION(act5->gobj()), c1);

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act6->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH));
    gtk_action_set_sensitive(GTK_ACTION(act6->gobj()), c1 && c3);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act6b->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PATH_INTERSECTION));
    gtk_action_set_sensitive(GTK_ACTION(act6b->gobj()), c1 && c3);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act7->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_CUSP));
    gtk_action_set_sensitive(GTK_ACTION(act7->gobj()), c1 && c3);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act8->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_NODE_SMOOTH));
    gtk_action_set_sensitive(GTK_ACTION(act8->gobj()), c1 && c3);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act9->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_LINE_MIDPOINT));
    gtk_action_set_sensitive(GTK_ACTION(act9->gobj()), c1 && c3);

    bool const c5 = nv->snap_manager.snapprefs.isTargetSnappable(SNAPTARGET_OTHERS_CATEGORY);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act10->gobj()), c5);
    gtk_action_set_sensitive(GTK_ACTION(act10->gobj()), c1);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act10b->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_OBJECT_MIDPOINT));
    gtk_action_set_sensitive(GTK_ACTION(act10b->gobj()), c1 && c5);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act11->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_ROTATION_CENTER));
    gtk_action_set_sensitive(GTK_ACTION(act11->gobj()), c1 && c5);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act11b->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_TEXT_BASELINE));
    gtk_action_set_sensitive(GTK_ACTION(act11b->gobj()), c1 && c5);

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act12->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_PAGE_BORDER));
    gtk_action_set_sensitive(GTK_ACTION(act12->gobj()), c1);

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act14->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GRID));
    gtk_action_set_sensitive(GTK_ACTION(act14->gobj()), c1);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act15->gobj()), nv->snap_manager.snapprefs.isSnapButtonEnabled(Inkscape::SNAPTARGET_GUIDE));
    gtk_action_set_sensitive(GTK_ACTION(act15->gobj()), c1);


    g_object_set_data(G_OBJECT(toolbox), "freeze", GINT_TO_POINTER(FALSE)); // unfreeze (see above)
}

void ToolboxFactory::showAuxToolbox(GtkWidget *toolbox_toplevel)
{
    gtk_widget_show(toolbox_toplevel);
    GtkWidget *toolbox = gtk_bin_get_child(GTK_BIN(toolbox_toplevel));

    GtkWidget *shown_toolbox = GTK_WIDGET(g_object_get_data(G_OBJECT(toolbox), "shows"));
    if (!shown_toolbox) {
        return;
    }
    gtk_widget_show(toolbox);

    gtk_widget_show_all(shown_toolbox);
}

#define MODE_LABEL_WIDTH 70


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
