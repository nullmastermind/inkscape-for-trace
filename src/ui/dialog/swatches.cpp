// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Jon A. Cruz
 *   John Bintz
 *   Abhishek Sharma
 *
 * Copyright (C) 2005 Jon A. Cruz
 * Copyright (C) 2008 John Bintz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <map>
#include <algorithm>
#include <iomanip>
#include <set>

#include "swatches.h"
#include <gtkmm/radiomenuitem.h>

#include <gtkmm/menu.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/menubutton.h>

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <glibmm/timer.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include "color-item.h"
#include "desktop.h"

#include "desktop-style.h"
#include "document.h"
#include "document-undo.h"
#include "extension/db.h"
#include "inkscape.h"
#include "io/sys.h"
#include "io/resource.h"
#include "message-context.h"
#include "path-prefix.h"

#include "ui/previewholder.h"
#include "widgets/desktop-widget.h"
#include "ui/widget/gradient-vector-selector.h"
#include "display/cairo-utils.h"

#include "object/sp-defs.h"
#include "object/sp-gradient-reference.h"

#include "ui/dialog/dialog-container.h"
#include "verbs.h"
#include "gradient-chemistry.h"
#include "helper/action.h"

namespace Inkscape {
namespace UI {
namespace Dialog {


enum {
    SWATCHES_SETTINGS_SIZE = 0,
    SWATCHES_SETTINGS_MODE = 1,
    SWATCHES_SETTINGS_SHAPE = 2,
    SWATCHES_SETTINGS_WRAP = 3,
    SWATCHES_SETTINGS_BORDER = 4,
    SWATCHES_SETTINGS_PALETTE = 5
};

#define VBLOCK 16
#define PREVIEW_PIXBUF_WIDTH 128

std::list<SwatchPage*> userSwatchPages;
std::list<SwatchPage*> systemSwatchPages;
static std::map<SPDocument*, SwatchPage*> docPalettes;
static std::vector<DocTrack*> docTrackings;
static std::map<SwatchesPanel*, SPDocument*> docPerPanel;


class SwatchesPanelHook : public SwatchesPanel
{
public:
    static void convertGradient( GtkMenuItem *menuitem, gpointer userData );
    static void deleteGradient( GtkMenuItem *menuitem, gpointer userData );
};

static void handleClick( GtkWidget* /*widget*/, gpointer callback_data ) {
    ColorItem* item = reinterpret_cast<ColorItem*>(callback_data);
    if ( item ) {
        item->buttonClicked(false);
    }
}

static void handleSecondaryClick( GtkWidget* /*widget*/, gint /*arg1*/, gpointer callback_data ) {
    ColorItem* item = reinterpret_cast<ColorItem*>(callback_data);
    if ( item ) {
        item->buttonClicked(true);
    }
}

static GtkWidget* popupMenu = nullptr;
static GtkWidget *popupSubHolder = nullptr;
static GtkWidget *popupSub = nullptr;
static std::vector<Glib::ustring> popupItems;
static std::vector<GtkWidget*> popupExtras;
static ColorItem* bounceTarget = nullptr;
static SwatchesPanel* bouncePanel = nullptr;

static void redirClick( GtkMenuItem *menuitem, gpointer /*user_data*/ )
{
    if ( bounceTarget ) {
        handleClick( GTK_WIDGET(menuitem), bounceTarget );
    }
}

static void redirSecondaryClick( GtkMenuItem *menuitem, gpointer /*user_data*/ )
{
    if ( bounceTarget ) {
        handleSecondaryClick( GTK_WIDGET(menuitem), 0, bounceTarget );
    }
}

static void editGradientImpl( SPDesktop* desktop, SPGradient* gr )
{
    g_assert(desktop != nullptr);
    g_assert(desktop->doc() != nullptr);

    if ( gr ) {
        bool shown = false;
        {
            Inkscape::Selection *selection = desktop->getSelection();
            std::vector<SPItem*> const items(selection->items().begin(), selection->items().end());
            if (!items.empty()) {
                SPStyle query( desktop->doc() );
                int result = objects_query_fillstroke((items), &query, true);
                if ( (result == QUERY_STYLE_MULTIPLE_SAME) || (result == QUERY_STYLE_SINGLE) ) {
                    // could be pertinent
                    if (query.fill.isPaintserver()) {
                        SPPaintServer* server = query.getFillPaintServer();
                        if ( SP_IS_GRADIENT(server) ) {
                            SPGradient* grad = SP_GRADIENT(server);
                            if ( grad->isSwatch() && grad->getId() == gr->getId()) {
                                desktop->getContainer()->new_dialog(SP_VERB_DIALOG_FILL_STROKE);
                                shown = true;
                            }
                        }
                    }
                }
            }
        }

        if (!shown) {
            // Invoke the gradient tool
            auto verb = Inkscape::Verb::get(SP_VERB_CONTEXT_GRADIENT);
            if (verb) {
                auto action = verb->get_action(Inkscape::ActionContext(desktop));
                if (action) {
                    sp_action_perform(action, nullptr);
                }
            }
        }
    }
}

static void editGradient( GtkMenuItem */*menuitem*/, gpointer /*user_data*/ )
{
    if ( bounceTarget ) {
        SwatchesPanel* swp = bouncePanel;
        SPDesktop* desktop = swp ? swp->getDesktop() : nullptr;
        SPDocument *doc = desktop ? desktop->doc() : nullptr;
        if (doc) {
            std::string targetName(bounceTarget->def.descr);
            std::vector<SPObject *> gradients = doc->getResourceList("gradient");
            for (auto gradient : gradients) {
                SPGradient* grad = SP_GRADIENT(gradient);
                if ( targetName == grad->getId() ) {
                    editGradientImpl( desktop, grad );
                    break;
                }
            }
        }
    }
}

void SwatchesPanelHook::convertGradient( GtkMenuItem * /*menuitem*/, gpointer userData )
{
    if ( bounceTarget ) {
        SwatchesPanel* swp = bouncePanel;
        SPDesktop* desktop = swp ? swp->getDesktop() : nullptr;
        SPDocument *doc = desktop ? desktop->doc() : nullptr;
        gint index = GPOINTER_TO_INT(userData);
        if ( doc && (index >= 0) && (static_cast<guint>(index) < popupItems.size()) ) {
            Glib::ustring targetName = popupItems[index];
            std::vector<SPObject *> gradients = doc->getResourceList("gradient");
            for (auto gradient : gradients) {
                SPGradient* grad = SP_GRADIENT(gradient);

                if ( targetName == grad->getId() ) {
                    grad->setSwatch();
                    DocumentUndo::done(doc, SP_VERB_CONTEXT_GRADIENT,
                                       _("Add gradient stop"));
                    break;
                }
            }
        }
    }
}

void SwatchesPanelHook::deleteGradient( GtkMenuItem */*menuitem*/, gpointer /*userData*/ )
{
    if ( bounceTarget ) {
        SwatchesPanel* swp = bouncePanel;
        SPDesktop* desktop = swp ? swp->getDesktop() : nullptr;
        sp_gradient_unset_swatch(desktop, bounceTarget->def.descr);
    }
}

static SwatchesPanel* findContainingPanel( GtkWidget *widget )
{
    SwatchesPanel *swp = nullptr;

    std::map<GtkWidget*, SwatchesPanel*> rawObjects;
    for (std::map<SwatchesPanel*, SPDocument*>::iterator it = docPerPanel.begin(); it != docPerPanel.end(); ++it) {
        rawObjects[GTK_WIDGET(it->first->gobj())] = it->first;
    }

    for (GtkWidget* curr = widget; curr && !swp; curr = gtk_widget_get_parent(curr)) {
        if (rawObjects.find(curr) != rawObjects.end()) {
            swp = rawObjects[curr];
        }
    }

    return swp;
}

static void removeit( GtkWidget *widget, gpointer data )
{
    gtk_container_remove( GTK_CONTAINER(data), widget );
}

/* extern'ed from color-item.cpp */
bool colorItemHandleButtonPress(GdkEventButton* event, UI::Widget::Preview *preview, gpointer user_data)
{
    gboolean handled = FALSE;

    if ( event && (event->button == 3) && (event->type == GDK_BUTTON_PRESS) ) {
        SwatchesPanel* swp = findContainingPanel( GTK_WIDGET(preview->gobj()) );

        if ( !popupMenu ) {
            popupMenu = gtk_menu_new();
            GtkWidget* child = nullptr;

            //TRANSLATORS: An item in context menu on a colour in the swatches
            child = gtk_menu_item_new_with_label(_("Set fill"));
            g_signal_connect( G_OBJECT(child),
                              "activate",
                              G_CALLBACK(redirClick),
                              user_data);
            gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), child);

            //TRANSLATORS: An item in context menu on a colour in the swatches
            child = gtk_menu_item_new_with_label(_("Set stroke"));

            g_signal_connect( G_OBJECT(child),
                              "activate",
                              G_CALLBACK(redirSecondaryClick),
                              user_data);
            gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), child);

            child = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), child);
            popupExtras.push_back(child);

            child = gtk_menu_item_new_with_label(_("Delete"));
            g_signal_connect( G_OBJECT(child),
                              "activate",
                              G_CALLBACK(SwatchesPanelHook::deleteGradient),
                              user_data );
            gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), child);
            popupExtras.push_back(child);
            gtk_widget_set_sensitive( child, FALSE );

            child = gtk_menu_item_new_with_label(_("Edit..."));
            g_signal_connect( G_OBJECT(child),
                              "activate",
                              G_CALLBACK(editGradient),
                              user_data );
            gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), child);
            popupExtras.push_back(child);

            child = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), child);
            popupExtras.push_back(child);

            child = gtk_menu_item_new_with_label(_("Convert"));
            gtk_menu_shell_append(GTK_MENU_SHELL(popupMenu), child);
            //popupExtras.push_back(child);
            //gtk_widget_set_sensitive( child, FALSE );
            {
                popupSubHolder = child;
                popupSub = gtk_menu_new();
                gtk_menu_item_set_submenu( GTK_MENU_ITEM(child), popupSub );
            }

            gtk_widget_show_all(popupMenu);
        }

        if ( user_data ) {
            ColorItem* item = reinterpret_cast<ColorItem*>(user_data);
            bool show = swp && (swp->getSelectedIndex() == 0);
            for (auto & popupExtra : popupExtras) {
                gtk_widget_set_sensitive(popupExtra, show);
            }

            bounceTarget = item;
            bouncePanel = swp;
            popupItems.clear();
            if ( popupMenu ) {
                gtk_container_foreach(GTK_CONTAINER(popupSub), removeit, popupSub);
                bool processed = false;
                auto *wdgt = preview->get_ancestor(SPDesktopWidget::get_type());
                if ( wdgt ) {
                    SPDesktopWidget *dtw = SP_DESKTOP_WIDGET(wdgt);
                    if ( dtw && dtw->desktop ) {
                        // Pick up all gradients with vectors
                        std::vector<SPObject *> gradients = (dtw->desktop->doc())->getResourceList("gradient");
                        gint index = 0;
                        for (auto gradient : gradients) {
                            SPGradient* grad = SP_GRADIENT(gradient);
                            if ( grad->hasStops() && !grad->isSwatch() ) {
                                //gl = g_slist_prepend(gl, curr->data);
                                processed = true;
                                GtkWidget *child = gtk_menu_item_new_with_label(grad->getId());
                                gtk_menu_shell_append(GTK_MENU_SHELL(popupSub), child);

                                popupItems.emplace_back(grad->getId());
                                g_signal_connect( G_OBJECT(child),
                                                  "activate",
                                                  G_CALLBACK(SwatchesPanelHook::convertGradient),
                                                  GINT_TO_POINTER(index) );
                                index++;
                            }
                        }

                        gtk_widget_show_all(popupSub);
                    }
                }
                gtk_widget_set_sensitive( popupSubHolder, processed );
                gtk_menu_popup_at_pointer(GTK_MENU(popupMenu), reinterpret_cast<GdkEvent *>(event));
                handled = TRUE;
            }
        }
    }

    return handled;
}


static char* trim( char* str ) {
    char* ret = str;
    while ( *str && (*str == ' ' || *str == '\t') ) {
        str++;
    }
    ret = str;
    while ( *str ) {
        str++;
    }
    str--;
    while ( str >= ret && (( *str == ' ' || *str == '\t' ) || *str == '\r' || *str == '\n') ) {
        *str-- = 0;
    }
    return ret;
}

static void skipWhitespace( char*& str ) {
    while ( *str == ' ' || *str == '\t' ) {
        str++;
    }
}

static bool parseNum( char*& str, int& val ) {
    val = 0;
    while ( '0' <= *str && *str <= '9' ) {
        val = val * 10 + (*str - '0');
        str++;
    }
    bool retval = !(*str == 0 || *str == ' ' || *str == '\t' || *str == '\r' || *str == '\n');
    return retval;
}


static
void _loadPaletteFile(Glib::ustring path, gboolean user/*=FALSE*/)
{
    auto filename = Glib::path_get_basename(path.raw());
    char block[1024];
    FILE *f = Inkscape::IO::fopen_utf8name(path.c_str(), "r");
    if ( f ) {
        char* result = fgets( block, sizeof(block), f );
        if ( result ) {
            if ( strncmp( "GIMP Palette", block, 12 ) == 0 ) {
                bool inHeader = true;
                bool hasErr = false;

                SwatchPage *onceMore = new SwatchPage();
                onceMore->_name = filename.c_str();

                do {
                    result = fgets( block, sizeof(block), f );
                    block[sizeof(block) - 1] = 0;
                    if ( result ) {
                        if ( block[0] == '#' ) {
                            // ignore comment
                        } else {
                            char *ptr = block;
                            // very simple check for header versus entry
                            while ( *ptr == ' ' || *ptr == '\t' ) {
                                ptr++;
                            }
                            if ( (*ptr == 0) || (*ptr == '\r') || (*ptr == '\n') ) {
                                // blank line. skip it.
                            } else if ( '0' <= *ptr && *ptr <= '9' ) {
                                // should be an entry link
                                inHeader = false;
                                ptr = block;
                                Glib::ustring name("");
                                skipWhitespace(ptr);
                                if ( *ptr ) {
                                    int r = 0;
                                    int g = 0;
                                    int b = 0;
                                    hasErr = parseNum(ptr, r);
                                    if ( !hasErr ) {
                                        skipWhitespace(ptr);
                                        hasErr = parseNum(ptr, g);
                                    }
                                    if ( !hasErr ) {
                                        skipWhitespace(ptr);
                                        hasErr = parseNum(ptr, b);
                                    }
                                    if ( !hasErr && *ptr ) {
                                        char* n = trim(ptr);
                                        if (n != nullptr && *n) {
                                            name = g_dpgettext2(nullptr, "Palette", n);
                                        }
                                        if (name == "") {
                                            name = Glib::ustring::compose("#%1%2%3",
                                                       Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), r),
                                                       Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), g),
                                                       Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), b)
                                                   ).uppercase();
                                        }
                                    }
                                    if ( !hasErr ) {
                                        // Add the entry now
                                        Glib::ustring nameStr(name);
                                        ColorItem* item = new ColorItem( r, g, b, nameStr );
                                        onceMore->_colors.push_back(item);
                                    }
                                } else {
                                    hasErr = true;
                                }
                            } else {
                                if ( !inHeader ) {
                                    // Hmmm... probably bad. Not quite the format we want?
                                    hasErr = true;
                                } else {
                                    char* sep = strchr(result, ':');
                                    if ( sep ) {
                                        *sep = 0;
                                        char* val = trim(sep + 1);
                                        char* name = trim(result);
                                        if ( *name ) {
                                            if ( strcmp( "Name", name ) == 0 )
                                            {
                                                onceMore->_name = val;
                                            }
                                            else if ( strcmp( "Columns", name ) == 0 )
                                            {
                                                gchar* endPtr = nullptr;
                                                guint64 numVal = g_ascii_strtoull( val, &endPtr, 10 );
                                                if ( (numVal == G_MAXUINT64) && (ERANGE == errno) ) {
                                                    // overflow
                                                } else if ( (numVal == 0) && (endPtr == val) ) {
                                                    // failed conversion
                                                } else {
                                                    onceMore->_prefWidth = numVal;
                                                }
                                            }
                                        } else {
                                            // error
                                            hasErr = true;
                                        }
                                    } else {
                                        // error
                                        hasErr = true;
                                    }
                                }
                            }
                        }
                    }
                } while ( result && !hasErr );
                if ( !hasErr ) {
                    if (user)
                        userSwatchPages.push_back(onceMore);
                    else
                        systemSwatchPages.push_back(onceMore);
                } else {
                    delete onceMore;
                }
            }
        }

        fclose(f);
    }
}

static bool
compare_swatch_names(SwatchPage const *a, SwatchPage const *b) {

    return g_utf8_collate(a->_name.c_str(), b->_name.c_str()) < 0;
}

static void load_palettes()
{
    static bool init_done = false;

    if (init_done) {
        return;
    }
    init_done = true;

    for (auto &filename: Inkscape::IO::Resource::get_filenames(Inkscape::IO::Resource::PALETTES, {".gpl"})) {
        bool userPalette = Inkscape::IO::file_is_writable(filename.c_str());
        _loadPaletteFile(filename, userPalette);
    }

    // Sort the list of swatches by name, grouped by user/system
    userSwatchPages.sort(compare_swatch_names);
    systemSwatchPages.sort(compare_swatch_names);
}

SwatchesPanel& SwatchesPanel::getInstance()
{
    return *new SwatchesPanel();
}


/**
 * Constructor
 */
SwatchesPanel::SwatchesPanel(gchar const *prefsPath)
    : DialogBase(prefsPath, SP_VERB_DIALOG_SWATCHES)
    , _menu(nullptr)
    , _holder(nullptr)
    , _clear(nullptr)
    , _remove(nullptr)
    , _currentIndex(0)
    , _currentDesktop(nullptr)
    , _currentDocument(nullptr)
{
    _holder = new PreviewHolder();

    _build_menu();

    auto menu_button = Gtk::manage(new Gtk::MenuButton());
    menu_button->set_halign(Gtk::ALIGN_END);
    menu_button->set_relief(Gtk::RELIEF_NONE);
    menu_button->set_image_from_icon_name("pan-start-symbolic", Gtk::ICON_SIZE_SMALL_TOOLBAR);
    menu_button->set_popup(*_menu);

    auto box = Gtk::manage(new Gtk::Box());

    if (_prefs_path == "/dialogs/swatches") {
        box->set_orientation(Gtk::ORIENTATION_VERTICAL);
        box->pack_start(*menu_button, Gtk::PACK_SHRINK);
    } else {
        box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        box->pack_end(*menu_button, Gtk::PACK_SHRINK);
        _updateSettings(SWATCHES_SETTINGS_MODE, 1);
        _holder->setOrientation(SP_ANCHOR_SOUTH);
    }

    box->pack_start(*_holder, Gtk::PACK_EXPAND_WIDGET);
    pack_start(*box);

    load_palettes();

    Gtk::RadioMenuItem* hotItem = nullptr;
    _clear = new ColorItem( ege::PaintDef::CLEAR );
    _remove = new ColorItem( ege::PaintDef::NONE );

    if (docPalettes.empty()) {
        SwatchPage *docPalette = new SwatchPage();

        docPalette->_name = "Auto";
        docPalettes[nullptr] = docPalette;
    }

    if ( !systemSwatchPages.empty() || !userSwatchPages.empty()) {
        SwatchPage* first = nullptr;
        int index = 0;
        Glib::ustring targetName;
        if ( !_prefs_path.empty() ) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            targetName = prefs->getString(_prefs_path + "/palette");
            if (!targetName.empty()) {
                if (targetName == "Auto") {
                    first = docPalettes[nullptr];
                } else {
                    std::vector<SwatchPage*> pages = _getSwatchSets();
                    for (auto & page : pages) {
                        if ( page->_name == targetName ) {
                            first = page;
                            break;
                        }
                        index++;
                    }
                }
            }
        }

        if ( !first ) {
            first = docPalettes[nullptr];
            _currentIndex = 0;
        } else {
            _currentIndex = index;
        }

        _rebuild();

        Gtk::RadioMenuItem::Group groupOne;

        int i = 0;
        std::vector<SwatchPage*> swatchSets = _getSwatchSets();
        for (auto curr : swatchSets) {
            Gtk::RadioMenuItem* single = Gtk::manage(new Gtk::RadioMenuItem(groupOne, curr->_name));
            if ( curr == first ) {
                hotItem = single;
            }
            _regItem(single, i);

            i++;
        }
    }

    if ( hotItem ) {
        hotItem->set_active();
    }

    show_all_children();
}

SwatchesPanel::~SwatchesPanel()
{
    _trackDocument( this, nullptr );

    if ( _clear ) {
        delete _clear;
    }
    if ( _remove ) {
        delete _remove;
    }
    if ( _holder ) {
        delete _holder;
    }

    delete _menu;
}

void SwatchesPanel::_build_menu()
{
    guint panel_size = 0, panel_mode = 0, panel_ratio = 100, panel_border = 0;
    bool panel_wrap = false;
    if (!_prefs_path.empty()) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        panel_wrap = prefs->getBool(_prefs_path + "/panel_wrap");
        panel_size = prefs->getIntLimited(_prefs_path + "/panel_size", 1, 0, UI::Widget::PREVIEW_SIZE_HUGE);
        panel_mode = prefs->getIntLimited(_prefs_path + "/panel_mode", 1, 0, 10);
        panel_ratio = prefs->getIntLimited(_prefs_path + "/panel_ratio", 100, 0, 500 );
        panel_border = prefs->getIntLimited(_prefs_path + "/panel_border", UI::Widget::BORDER_NONE, 0, 2 );
    }

    _menu = new Gtk::Menu();

    if (_prefs_path == "/dialogs/swatches") {
        Gtk::RadioMenuItem::Group group;
        Glib::ustring list_label(_("List"));
        Glib::ustring grid_label(_("Grid"));
        Gtk::RadioMenuItem *list_item = Gtk::manage(new Gtk::RadioMenuItem(group, list_label));
        Gtk::RadioMenuItem *grid_item = Gtk::manage(new Gtk::RadioMenuItem(group, grid_label));

        if (panel_mode == 0) {
            list_item->set_active(true);
        } else if (panel_mode == 1) {
            grid_item->set_active(true);
        }

        _menu->append(*list_item);
        _menu->append(*grid_item);
        _menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        list_item->signal_activate().connect(sigc::bind<int, int>(sigc::mem_fun(*this, &SwatchesPanel::_updateSettings), SWATCHES_SETTINGS_MODE, 0));
        grid_item->signal_activate().connect(sigc::bind<int, int>(sigc::mem_fun(*this, &SwatchesPanel::_updateSettings), SWATCHES_SETTINGS_MODE, 1));
    }

    {
        Glib::ustring heightItemLabel(C_("Swatches", "Size"));

        //TRANSLATORS: Indicates size of colour swatches
        const gchar *heightLabels[] = {
            NC_("Swatches height", "Tiny"),
            NC_("Swatches height", "Small"),
            NC_("Swatches height", "Medium"),
            NC_("Swatches height", "Large"),
            NC_("Swatches height", "Huge")
        };

        Gtk::MenuItem *sizeItem = Gtk::manage(new Gtk::MenuItem(heightItemLabel));
        Gtk::Menu *sizeMenu = Gtk::manage(new Gtk::Menu());
        sizeItem->set_submenu(*sizeMenu);

        Gtk::RadioMenuItem::Group heightGroup;
        for (unsigned int i = 0; i < G_N_ELEMENTS(heightLabels); i++) {
            Glib::ustring _label(g_dpgettext2(nullptr, "Swatches height", heightLabels[i]));
            Gtk::RadioMenuItem* _item = Gtk::manage(new Gtk::RadioMenuItem(heightGroup, _label));
            sizeMenu->append(*_item);
            if (i == panel_size) {
                _item->set_active(true);
            }
            _item->signal_activate().connect(sigc::bind<int, int>(sigc::mem_fun(*this, &SwatchesPanel::_updateSettings), SWATCHES_SETTINGS_SIZE, i));
       }

       _menu->append(*sizeItem);
    }

    {
        Glib::ustring widthItemLabel(C_("Swatches", "Width"));

        //TRANSLATORS: Indicates width of colour swatches
        const gchar *widthLabels[] = {
            NC_("Swatches width", "Narrower"),
            NC_("Swatches width", "Narrow"),
            NC_("Swatches width", "Medium"),
            NC_("Swatches width", "Wide"),
            NC_("Swatches width", "Wider")
        };

        Gtk::MenuItem *item = Gtk::manage( new Gtk::MenuItem(widthItemLabel));
        Gtk::Menu *type_menu = Gtk::manage(new Gtk::Menu());
        item->set_submenu(*type_menu);
        _menu->append(*item);

        Gtk::RadioMenuItem::Group widthGroup;

        guint values[] = {0, 25, 50, 100, 200, 400};
        guint hot_index = 3;
        for ( guint i = 0; i < G_N_ELEMENTS(widthLabels); ++i ) {
            // Assume all values are in increasing order
            if ( values[i] <= panel_ratio ) {
                hot_index = i;
            }
        }
        for ( guint i = 0; i < G_N_ELEMENTS(widthLabels); ++i ) {
            Glib::ustring _label(g_dpgettext2(nullptr, "Swatches width", widthLabels[i]));
            Gtk::RadioMenuItem *_item = Gtk::manage(new Gtk::RadioMenuItem(widthGroup, _label));
            type_menu->append(*_item);
            if ( i <= hot_index ) {
                _item->set_active(true);
            }
            _item->signal_activate().connect(sigc::bind<int, int>(sigc::mem_fun(*this, &SwatchesPanel::_updateSettings), SWATCHES_SETTINGS_SHAPE, values[i]));
        }
    }

    {
        Glib::ustring widthItemLabel(C_("Swatches", "Border"));

        //TRANSLATORS: Indicates border of colour swatches
        const gchar *widthLabels[] = {
            NC_("Swatches border", "None"),
            NC_("Swatches border", "Solid"),
            NC_("Swatches border", "Wide"),
        };

        Gtk::MenuItem *item = Gtk::manage( new Gtk::MenuItem(widthItemLabel));
        Gtk::Menu *type_menu = Gtk::manage(new Gtk::Menu());
        item->set_submenu(*type_menu);
        _menu->append(*item);

        Gtk::RadioMenuItem::Group widthGroup;

        guint values[] = {0, 1, 2};
        guint hot_index = 0;
        for ( guint i = 0; i < G_N_ELEMENTS(widthLabels); ++i ) {
            // Assume all values are in increasing order
            if ( values[i] <= panel_border ) {
                hot_index = i;
            }
        }
        for ( guint i = 0; i < G_N_ELEMENTS(widthLabels); ++i ) {
            Glib::ustring _label(g_dpgettext2(nullptr, "Swatches border", widthLabels[i]));
            Gtk::RadioMenuItem *_item = Gtk::manage(new Gtk::RadioMenuItem(widthGroup, _label));
            type_menu->append(*_item);
            if ( i <= hot_index ) {
                _item->set_active(true);
            }
            _item->signal_activate().connect(sigc::bind<int, int>(sigc::mem_fun(*this, &SwatchesPanel::_updateSettings), SWATCHES_SETTINGS_BORDER, values[i]));
        }
    }

    if (_prefs_path == "/embedded/swatches") {
        //TRANSLATORS: "Wrap" indicates how colour swatches are displayed
        Glib::ustring wrap_label(C_("Swatches","Wrap"));
        Gtk::CheckMenuItem *check = Gtk::manage(new Gtk::CheckMenuItem(wrap_label));
        check->set_active(panel_wrap);
        _menu->append(*check);

        check->signal_toggled().connect(sigc::bind<Gtk::CheckMenuItem*>(sigc::mem_fun(*this, &SwatchesPanel::_wrapToggled), check));
    }

    _menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    _menu->show_all();

    _updateSettings(SWATCHES_SETTINGS_SIZE, panel_size);
    _updateSettings(SWATCHES_SETTINGS_MODE, panel_mode);
    _updateSettings(SWATCHES_SETTINGS_SHAPE, panel_ratio);
    _updateSettings(SWATCHES_SETTINGS_WRAP, panel_wrap);
    _updateSettings(SWATCHES_SETTINGS_BORDER, panel_border);
}

void SwatchesPanel::update()
{
    if (!_app) {
        std::cerr << "SwatchesPanel::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();

    if ( desktop != _currentDesktop ) {
        if ( _currentDesktop ) {
            for (auto &conn : _desktopConnections) {
                conn.disconnect();
            }
        }

        _currentDesktop = desktop;

        if ( desktop ) {
            _desktopConnections.emplace_back(desktop->selection->connectChanged( //
                [this](Inkscape::Selection *) { this->_updateFromSelection(); }));

            _desktopConnections.emplace_back(desktop->selection->connectModified( //
                [this](Inkscape::Selection *, unsigned) { this->_updateFromSelection(); }));

            _desktopConnections.emplace_back(desktop->connectToolSubselectionChanged( //
                [this](gpointer) { this->_updateFromSelection(); }));

            _desktopConnections.emplace_back(desktop->connectDocumentReplaced( //
                [this](SPDesktop *, SPDocument *doc) { this->_setDocument(doc); }));

            _setDocument(desktop->doc());
        } else {
            _setDocument(nullptr);
        }
    }
}


void SwatchesPanel::_updateSettings(int settings, int value)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    switch (settings) {
    case SWATCHES_SETTINGS_SIZE: {
        prefs->setInt(_prefs_path + "/panel_size", value);

        auto curr_type = _holder->getPreviewType();
        guint curr_ratio = _holder->getPreviewRatio();
        auto curr_border = _holder->getPreviewBorder();

        switch (value) {
        case 0:
            _holder->setStyle(UI::Widget::PREVIEW_SIZE_TINY, curr_type, curr_ratio, curr_border);
            break;
        case 1:
            _holder->setStyle(UI::Widget::PREVIEW_SIZE_SMALL, curr_type, curr_ratio, curr_border);
            break;
        case 2:
            _holder->setStyle(UI::Widget::PREVIEW_SIZE_MEDIUM, curr_type, curr_ratio, curr_border);
            break;
        case 3:
            _holder->setStyle(UI::Widget::PREVIEW_SIZE_BIG, curr_type, curr_ratio, curr_border);
            break;
        case 4:
            _holder->setStyle(UI::Widget::PREVIEW_SIZE_HUGE, curr_type, curr_ratio, curr_border);
            break;
        default:
            break;
        }

        break;
    }
    case SWATCHES_SETTINGS_MODE: {
        prefs->setInt(_prefs_path + "/panel_mode", value);

        auto curr_size = _holder->getPreviewSize();
        guint curr_ratio = _holder->getPreviewRatio();
        auto curr_border = _holder->getPreviewBorder();
        switch (value) {
        case 0:
            _holder->setStyle(curr_size, UI::Widget::VIEW_TYPE_LIST, curr_ratio, curr_border);
            break;
        case 1:
            _holder->setStyle(curr_size, UI::Widget::VIEW_TYPE_GRID, curr_ratio, curr_border);
            break;
        default:
            break;
        }
        break;
    }
    case SWATCHES_SETTINGS_SHAPE: {
        prefs->setInt(_prefs_path + "/panel_ratio", value);

        auto curr_type = _holder->getPreviewType();
        auto curr_size = _holder->getPreviewSize();
        auto curr_border = _holder->getPreviewBorder();

        _holder->setStyle(curr_size, curr_type, value, curr_border);
        break;
    }
    case SWATCHES_SETTINGS_BORDER: {
        prefs->setInt(_prefs_path + "/panel_border", value);

        auto curr_size = _holder->getPreviewSize();
        auto curr_type = _holder->getPreviewType();
        guint curr_ratio = _holder->getPreviewRatio();

        switch (value) {
        case 0:
            _holder->setStyle(curr_size, curr_type, curr_ratio, UI::Widget::BORDER_NONE);
            break;
        case 1:
            _holder->setStyle(curr_size, curr_type, curr_ratio, UI::Widget::BORDER_SOLID);
            break;
        case 2:
            _holder->setStyle(curr_size, curr_type, curr_ratio, UI::Widget::BORDER_WIDE);
            break;
        default:
            break;
        }
        break;
    }
    case SWATCHES_SETTINGS_WRAP: {
        prefs->setBool(_prefs_path + "/panel_wrap", value);
        _holder->setWrap(value);
        break;
    }
    case SWATCHES_SETTINGS_PALETTE: {
        std::vector<SwatchPage*> pages = _getSwatchSets();
        if (value >= 0 && value < static_cast<int>(pages.size()) ) {
            _currentIndex = value;

            prefs->setString(_prefs_path + "/palette", pages[_currentIndex]->_name);

            _rebuild();
        }
    }
    default:
        break;
    }
}

void SwatchesPanel::_wrapToggled(Gtk::CheckMenuItem* toggler)
{
    if (toggler) {
        _updateSettings(SWATCHES_SETTINGS_WRAP, toggler->get_active() ? 1 : 0);
    }
}

void SwatchesPanel::_regItem(Gtk::MenuItem* item, int id)
{
    _menu->append(*item);
    item->signal_activate().connect(sigc::bind<int, int>(sigc::mem_fun(*this, &SwatchesPanel::_updateSettings), SWATCHES_SETTINGS_PALETTE, id));
    item->show();
}


class DocTrack
{
public:
    DocTrack(SPDocument *doc, sigc::connection &gradientRsrcChanged, sigc::connection &defsChanged, sigc::connection &defsModified) :
        doc(doc->doRef()),
        updatePending(false),
        lastGradientUpdate(0.0),
        gradientRsrcChanged(gradientRsrcChanged),
        defsChanged(defsChanged),
        defsModified(defsModified)
    {
        if ( !timer ) {
            timer = new Glib::Timer();
            refreshTimer = Glib::signal_timeout().connect( sigc::ptr_fun(handleTimerCB), 33 );
        }
        timerRefCount++;
    }

    ~DocTrack()
    {
        timerRefCount--;
        if ( timerRefCount <= 0 ) {
            refreshTimer.disconnect();
            timerRefCount = 0;
            if ( timer ) {
                timer->stop();
                delete timer;
                timer = nullptr;
            }
        }
        if (doc) {
            gradientRsrcChanged.disconnect();
            defsChanged.disconnect();
            defsModified.disconnect();
        }
    }

    static bool handleTimerCB();

    /**
     * Checks if update should be queued or executed immediately.
     *
     * @return true if the update was queued and should not be immediately executed.
     */
    static bool queueUpdateIfNeeded(SPDocument *doc);

    static Glib::Timer *timer;
    static int timerRefCount;
    static sigc::connection refreshTimer;

    std::unique_ptr<SPDocument> doc;
    bool updatePending;
    double lastGradientUpdate;
    sigc::connection gradientRsrcChanged;
    sigc::connection defsChanged;
    sigc::connection defsModified;

private:
    DocTrack(DocTrack const &) = delete; // no copy
    DocTrack &operator=(DocTrack const &) = delete; // no assign
};

Glib::Timer *DocTrack::timer = nullptr;
int DocTrack::timerRefCount = 0;
sigc::connection DocTrack::refreshTimer;

static const double DOC_UPDATE_THREASHOLD  = 0.090;

bool DocTrack::handleTimerCB()
{
    double now = timer->elapsed();

    std::vector<DocTrack *> needCallback;
    for (auto track : docTrackings) {
        if ( track->updatePending && ( (now - track->lastGradientUpdate) >= DOC_UPDATE_THREASHOLD) ) {
            needCallback.push_back(track);
        }
    }

    for (auto track : needCallback) {
        if ( std::find(docTrackings.begin(), docTrackings.end(), track) != docTrackings.end() ) { // Just in case one gets deleted while we are looping
            // Note: calling handleDefsModified will call queueUpdateIfNeeded and thus update the time and flag.
            SwatchesPanel::handleDefsModified(track->doc.get());
        }
    }

    return true;
}

bool DocTrack::queueUpdateIfNeeded( SPDocument *doc )
{
    bool deferProcessing = false;
    for (auto track : docTrackings) {
        if ( track->doc.get() == doc ) {
            double now = timer->elapsed();
            double elapsed = now - track->lastGradientUpdate;

            if ( elapsed < DOC_UPDATE_THREASHOLD ) {
                deferProcessing = true;
                track->updatePending = true;
            } else {
                track->lastGradientUpdate = now;
                track->updatePending = false;
            }

            break;
        }
    }
    return deferProcessing;
}

void SwatchesPanel::_trackDocument( SwatchesPanel *panel, SPDocument *document )
{
    SPDocument *oldDoc = nullptr;
    if (docPerPanel.find(panel) != docPerPanel.end()) {
        oldDoc = docPerPanel[panel];
        if (!oldDoc) {
            docPerPanel.erase(panel); // Should not be needed, but clean up just in case.
        }
    }
    if (oldDoc != document) {
        if (oldDoc) {
            docPerPanel[panel] = nullptr;
            bool found = false;
            for (std::map<SwatchesPanel*, SPDocument*>::iterator it = docPerPanel.begin(); (it != docPerPanel.end()) && !found; ++it) {
                found = (it->second == document);
            }
            if (!found) {
                for (std::vector<DocTrack*>::iterator it = docTrackings.begin(); it != docTrackings.end(); ++it){
                    if ((*it)->doc.get() == oldDoc) {
                        delete *it;
                        docTrackings.erase(it);
                        break;
                    }
                }
            }
        }

        if (document) {
            bool found = false;
            for (std::map<SwatchesPanel*, SPDocument*>::iterator it = docPerPanel.begin(); (it != docPerPanel.end()) && !found; ++it) {
                found = (it->second == document);
            }
            docPerPanel[panel] = document;
            if (!found) {
                sigc::connection conn1 = document->connectResourcesChanged( "gradient", sigc::bind(sigc::ptr_fun(&SwatchesPanel::handleGradientsChange), document) );
                sigc::connection conn2 = document->getDefs()->connectRelease( sigc::hide(sigc::bind(sigc::ptr_fun(&SwatchesPanel::handleDefsModified), document)) );
                sigc::connection conn3 = document->getDefs()->connectModified( sigc::hide(sigc::hide(sigc::bind(sigc::ptr_fun(&SwatchesPanel::handleDefsModified), document))) );

                DocTrack *dt = new DocTrack(document, conn1, conn2, conn3);
                docTrackings.push_back(dt);

                if (docPalettes.find(document) == docPalettes.end()) {
                    SwatchPage *docPalette = new SwatchPage();
                    docPalette->_name = "Auto";
                    docPalettes[document] = docPalette;
                }
            }
        }
    }
}

void SwatchesPanel::_setDocument( SPDocument *document )
{
    if ( document != _currentDocument ) {
        _trackDocument(this, document);
        _currentDocument = document;

        if (document) {
            handleGradientsChange(document);
        }
    }
}

static void recalcSwatchContents(SPDocument* doc,
                boost::ptr_vector<ColorItem> &tmpColors,
                std::map<ColorItem*, cairo_pattern_t*> &previewMappings,
                std::map<ColorItem*, SPGradient*> &gradMappings)
{
    std::vector<SPGradient*> newList;
    std::vector<SPObject *> gradients = doc->getResourceList("gradient");
    for (auto gradient : gradients) {
        SPGradient* grad = SP_GRADIENT(gradient);
        if ( grad->isSwatch() ) {
            newList.push_back(SP_GRADIENT(gradient));
        }
    }

    if ( !newList.empty() ) {
        std::reverse(newList.begin(), newList.end());
        for (auto grad : newList)
        {
            cairo_surface_t *preview = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                PREVIEW_PIXBUF_WIDTH, VBLOCK);
            cairo_t *ct = cairo_create(preview);

            Glib::ustring name( grad->getId() );
            ColorItem* item = new ColorItem( 0, 0, 0, name );

            cairo_pattern_t *check = ink_cairo_pattern_create_checkerboard();
            cairo_pattern_t *gradient = grad->create_preview_pattern(PREVIEW_PIXBUF_WIDTH);
            cairo_set_source(ct, check);
            cairo_paint(ct);
            cairo_set_source(ct, gradient);
            cairo_paint(ct);

            cairo_destroy(ct);
            cairo_pattern_destroy(gradient);
            cairo_pattern_destroy(check);

            cairo_pattern_t *prevpat = cairo_pattern_create_for_surface(preview);
            cairo_surface_destroy(preview);

            previewMappings[item] = prevpat;

            tmpColors.push_back(item);
            gradMappings[item] = grad;
        }
    }
}

void SwatchesPanel::handleGradientsChange(SPDocument *document)
{
    SwatchPage *docPalette = (docPalettes.find(document) != docPalettes.end()) ? docPalettes[document] : nullptr;
    if (docPalette) {
        boost::ptr_vector<ColorItem> tmpColors;
        std::map<ColorItem*, cairo_pattern_t*> tmpPrevs;
        std::map<ColorItem*, SPGradient*> tmpGrads;
        recalcSwatchContents(document, tmpColors, tmpPrevs, tmpGrads);

        for (auto & tmpPrev : tmpPrevs) {
            tmpPrev.first->setPattern(tmpPrev.second);
            cairo_pattern_destroy(tmpPrev.second);
        }

        for (auto & tmpGrad : tmpGrads) {
            tmpGrad.first->setGradient(tmpGrad.second);
        }

        docPalette->_colors.swap(tmpColors);

        // Figure out which SwatchesPanel instances are affected and update them.

        for (auto & it : docPerPanel) {
            if (it.second == document) {
                SwatchesPanel* swp = it.first;
                std::vector<SwatchPage*> pages = swp->_getSwatchSets();
                SwatchPage* curr = pages[swp->_currentIndex];
                if (curr == docPalette) {
                    swp->_rebuild();
                }
            }
        }
    }
}

void SwatchesPanel::handleDefsModified(SPDocument *document)
{
    SwatchPage *docPalette = (docPalettes.find(document) != docPalettes.end()) ? docPalettes[document] : nullptr;
    if (docPalette && !DocTrack::queueUpdateIfNeeded(document) ) {
        boost::ptr_vector<ColorItem> tmpColors;
        std::map<ColorItem*, cairo_pattern_t*> tmpPrevs;
        std::map<ColorItem*, SPGradient*> tmpGrads;
        recalcSwatchContents(document, tmpColors, tmpPrevs, tmpGrads);

        if ( tmpColors.size() != docPalette->_colors.size() ) {
            handleGradientsChange(document);
        } else {
            int cap = std::min(docPalette->_colors.size(), tmpColors.size());
            for (int i = 0; i < cap; i++) {
                ColorItem *newColor = &tmpColors[i];
                ColorItem *oldColor = &docPalette->_colors[i];
                if ( (newColor->def.getType() != oldColor->def.getType()) ||
                     (newColor->def.getR() != oldColor->def.getR()) ||
                     (newColor->def.getG() != oldColor->def.getG()) ||
                     (newColor->def.getB() != oldColor->def.getB()) ) {
                    oldColor->def.setRGB(newColor->def.getR(), newColor->def.getG(), newColor->def.getB());
                }
                if (tmpGrads.find(newColor) != tmpGrads.end()) {
                    oldColor->setGradient(tmpGrads[newColor]);
                }
                if ( tmpPrevs.find(newColor) != tmpPrevs.end() ) {
                    oldColor->setPattern(tmpPrevs[newColor]);
                }
            }
        }

        for (auto & tmpPrev : tmpPrevs) {
            cairo_pattern_destroy(tmpPrev.second);
        }
    }
}


std::vector<SwatchPage*> SwatchesPanel::_getSwatchSets() const
{
    std::vector<SwatchPage*> tmp;
    if (docPalettes.find(_currentDocument) != docPalettes.end()) {
        tmp.push_back(docPalettes[_currentDocument]);
    }

    tmp.insert(tmp.end(), userSwatchPages.begin(), userSwatchPages.end());
    tmp.insert(tmp.end(), systemSwatchPages.begin(), systemSwatchPages.end());

    return tmp;
}

void SwatchesPanel::_updateFromSelection()
{
    SwatchPage *docPalette = (docPalettes.find(_currentDocument) != docPalettes.end()) ? docPalettes[_currentDocument] : nullptr;
    if ( docPalette ) {
        std::string fillId;
        std::string strokeId;

        SPStyle tmpStyle(_currentDesktop->getDocument());
        int result = sp_desktop_query_style( _currentDesktop, &tmpStyle, QUERY_STYLE_PROPERTY_FILL );
        switch (result) {
            case QUERY_STYLE_SINGLE:
            case QUERY_STYLE_MULTIPLE_AVERAGED:
            case QUERY_STYLE_MULTIPLE_SAME:
            {
                if (tmpStyle.fill.set && tmpStyle.fill.isPaintserver()) {
                    SPPaintServer* server = tmpStyle.getFillPaintServer();
                    if ( SP_IS_GRADIENT(server) ) {
                        SPGradient* target = nullptr;
                        SPGradient* grad = SP_GRADIENT(server);

                        if ( grad->isSwatch() ) {
                            target = grad;
                        } else if ( grad->ref ) {
                            SPGradient *tmp = grad->ref->getObject();
                            if ( tmp && tmp->isSwatch() ) {
                                target = tmp;
                            }
                        }
                        if ( target ) {
                            //XML Tree being used directly here while it shouldn't be
                            gchar const* id = target->getRepr()->attribute("id");
                            if ( id ) {
                                fillId = id;
                            }
                        }
                    }
                }
                break;
            }
        }

        result = sp_desktop_query_style( _currentDesktop, &tmpStyle, QUERY_STYLE_PROPERTY_STROKE );
        switch (result) {
            case QUERY_STYLE_SINGLE:
            case QUERY_STYLE_MULTIPLE_AVERAGED:
            case QUERY_STYLE_MULTIPLE_SAME:
            {
                if (tmpStyle.stroke.set && tmpStyle.stroke.isPaintserver()) {
                    SPPaintServer* server = tmpStyle.getStrokePaintServer();
                    if ( SP_IS_GRADIENT(server) ) {
                        SPGradient* target = nullptr;
                        SPGradient* grad = SP_GRADIENT(server);
                        if ( grad->isSwatch() ) {
                            target = grad;
                        } else if ( grad->ref ) {
                            SPGradient *tmp = grad->ref->getObject();
                            if ( tmp && tmp->isSwatch() ) {
                                target = tmp;
                            }
                        }
                        if ( target ) {
                            //XML Tree being used directly here while it shouldn't be
                            gchar const* id = target->getRepr()->attribute("id");
                            if ( id ) {
                                strokeId = id;
                            }
                        }
                    }
                }
                break;
            }
        }

        for (auto & _color : docPalette->_colors) {
            ColorItem* item = &_color;
            bool isFill = (fillId == item->def.descr);
            bool isStroke = (strokeId == item->def.descr);
            item->setState( isFill, isStroke );
        }
    }
}

void SwatchesPanel::_rebuild()
{
    std::vector<SwatchPage*> pages = _getSwatchSets();
    SwatchPage* curr = pages[_currentIndex];
    _holder->clear();

    if ( curr->_prefWidth > 0 ) {
        _holder->setColumnPref( curr->_prefWidth );
    }
    _holder->freezeUpdates();
    // TODO restore once 'clear' works _holder->addPreview(_clear);
    _holder->addPreview(_remove);
    for (auto & _color : curr->_colors) {
        _holder->addPreview(&_color);
    }
    _holder->thawUpdates();
}

} //namespace Dialog
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
