// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for the about screen
 *
 * Copyright (C) Martin Owens 2019 <doctormo@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <fstream>
#include <streambuf>

#include "startup.h"

#include "file.h"
#include "inkscape.h"
#include "inkscape-application.h"
#include "inkscape-version.h"
#include "preferences.h"
#include "color-rgba.h"

#include "util/units.h"
#include "io/resource.h"
#include "ui/shortcuts.h"
#include "ui/util.h"

#include "object/sp-namedview.h"

using namespace Inkscape::IO;
using namespace Inkscape::UI::View;
using Inkscape::Util::unit_table;

namespace Inkscape {
namespace UI {
namespace Dialog {

class NameIdCols: public Gtk::TreeModel::ColumnRecord {
    public:
        // These types must match those for the model in the .glade file
        NameIdCols() {
            this->add(this->col_name);
            this->add(this->col_id);
        }
        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<Glib::ustring> col_id;
};

class CanvasCols: public Gtk::TreeModel::ColumnRecord {
    public:
        // These types must match those for the model in the .glade file
        CanvasCols() {
            this->add(this->id);
            this->add(this->name);
            this->add(this->icon_filename);
            this->add(this->pagecolor);
            this->add(this->checkered);
            this->add(this->bordercolor);
            this->add(this->shadow);
        }
        Gtk::TreeModelColumn<Glib::ustring> id;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> icon_filename;
        Gtk::TreeModelColumn<Glib::ustring> pagecolor;
        Gtk::TreeModelColumn<bool> checkered;
        Gtk::TreeModelColumn<Glib::ustring> bordercolor;
        Gtk::TreeModelColumn<bool> shadow;
};

class TemplateCols: public Gtk::TreeModel::ColumnRecord {
    public:
        // These types must match those for the model in the .glade file
        TemplateCols() {
            this->add(this->name);
            this->add(this->icon);
            this->add(this->filename);
            this->add(this->width);
            this->add(this->height);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> icon;
        Gtk::TreeModelColumn<Glib::ustring> filename;
        Gtk::TreeModelColumn<Glib::ustring> width;
        Gtk::TreeModelColumn<Glib::ustring> height;
};


class ThemeCols: public Gtk::TreeModel::ColumnRecord {
    public:
        // These types must match those for the model in the .glade file
        ThemeCols() {
            this->add(this->id);
            this->add(this->name);
            this->add(this->theme);
            this->add(this->icons);
            this->add(this->base);
            this->add(this->success);
            this->add(this->warn);
            this->add(this->error);
            this->add(this->dark);
            this->add(this->symbolic);
            this->add(this->smallicons);
        }
        Gtk::TreeModelColumn<Glib::ustring> id;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> theme;
        Gtk::TreeModelColumn<Glib::ustring> icons;
        Gtk::TreeModelColumn<Glib::ustring> base;
        Gtk::TreeModelColumn<Glib::ustring> success;
        Gtk::TreeModelColumn<Glib::ustring> warn;
        Gtk::TreeModelColumn<Glib::ustring> error;
        Gtk::TreeModelColumn<bool> dark;
        Gtk::TreeModelColumn<bool> symbolic;
        Gtk::TreeModelColumn<bool> smallicons;
};

/**
 * Color is store as a string in the form #RRGGBBAA, '0' means "unset"
 *
 * @param color - The string color from glade.
 */
unsigned int get_color_value(const Glib::ustring color)
{
    Gdk::RGBA gdk_color = Gdk::RGBA(color);
    ColorRGBA  sp_color(gdk_color.get_red(), gdk_color.get_green(),
                        gdk_color.get_blue(), gdk_color.get_alpha());
    return sp_color.getIntValue();
}

StartScreen::StartScreen()
    : Gtk::Dialog()
{
    set_can_focus(true);
    grab_focus();
    set_can_default(true);
    grab_default();
    set_urgency_hint(true);  // Draw user's attention to this window!
    set_title("Inkscape 1.1"); // Not actually shown.
    set_modal(true);
    set_position(Gtk::WIN_POS_CENTER_ALWAYS);
    set_default_size(700, 360);

    Glib::ustring gladefile = Resource::get_filename(Resource::UIS, "inkscape-start.glade");

    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_error("Glade file loading failed for boot screen");
        // cleanup?
    }

    // Get window from Glade file.
    builder->get_widget("start-screen-window", window);

    // Get references to various widgets used globally.
    builder->get_widget("tabs", tabs);
    builder->get_widget("kinds", kinds);
    builder->get_widget("banner", banners);
    builder->get_widget("themes", themes);
    builder->get_widget("recent_treeview", recent_treeview);

    // Get references to various widget used locally. (In order of appearance.)
    Gtk::ComboBox* canvas = nullptr;
    Gtk::ComboBox* keys = nullptr;
    Gtk::Button* save = nullptr;
    Gtk::Button* thanks = nullptr;
    Gtk::Button* show_toggle = nullptr;
    Gtk::Button* load = nullptr;
    builder->get_widget("canvas", canvas);
    builder->get_widget("keys", keys);
    builder->get_widget("save", save);
    builder->get_widget("thanks", thanks);
    builder->get_widget("show_toggle", show_toggle);
    builder->get_widget("load", load);

    // Unparent to move to our dialog window.
    auto parent = banners->get_parent();
    parent->remove(*banners);
    parent->remove(*tabs);

    // Add signals and setup things.
    auto prefs = Inkscape::Preferences::get();

    tabs->signal_switch_page().connect(sigc::mem_fun(*this, &StartScreen::notebook_switch));

    // Setup the lists of items
    enlist_recent_files();
    enlist_keys();
    set_active_combo("themes", prefs->getString("/options/boot/theme"));
    set_active_combo("canvas", prefs->getString("/options/boot/canvas"));

    // Welcome! tab
    canvas->signal_changed().connect(sigc::mem_fun(*this, &StartScreen::canvas_changed));
    keys->signal_changed().connect(sigc::mem_fun(*this, &StartScreen::keyboard_changed));
    themes->signal_changed().connect(sigc::mem_fun(*this, &StartScreen::theme_changed));
    save->signal_clicked().connect(sigc::bind<Gtk::Button *>(sigc::mem_fun(*this, &StartScreen::notebook_next), save));

    // "Supported by You" tab
    thanks->signal_clicked().connect(sigc::bind<Gtk::Button *>(sigc::mem_fun(*this, &StartScreen::notebook_next), thanks));

    // "Time to Draw" tab
    recent_treeview->signal_row_activated().connect(sigc::hide(sigc::hide((sigc::mem_fun(*this, &StartScreen::load_now)))));

    for (auto widget : kinds->get_children()) {
        auto container = dynamic_cast<Gtk::Container *>(widget);
        if (container) {
            widget = container->get_children()[0];
        }
        auto template_list = dynamic_cast<Gtk::IconView *>(widget);
        if (template_list) {
            template_list->signal_selection_changed().connect(sigc::mem_fun(*this, &StartScreen::load_now));
        }
    }

    show_toggle->signal_clicked().connect(sigc::mem_fun(*this, &StartScreen::show_toggle));
    load->signal_clicked().connect(sigc::mem_fun(*this, &StartScreen::load_now));

    // Reparent to our dialog window
    set_titlebar(*banners);
    Gtk::Box* box = get_content_area();
    box->add(*tabs);

    // Show the first tab ONLY on the first run for this version
    std::string opt_shown = "/options/boot/shown/ver";
    opt_shown += Inkscape::version_string_without_revision;
    if(!prefs->getBool(opt_shown, false)) {
        tabs->set_current_page(0);
        prefs->setBool(opt_shown, true);
    } else {
        tabs->set_current_page(2);
        notebook_switch(nullptr, 2);
    }

    show();
}

StartScreen::~StartScreen()
{
    // These are "owned" by builder... don't delete them!
    banners->get_parent()->remove(*banners);
    tabs->get_parent()->remove(*tabs);
}

/**
 * Return the active row of the named combo box.
 *
 * @param widget_name - The name of the widget in the glade file
 * @return Gtk Row object ready for use.
 * @throws Three errors depending on where it failed.
 */
Gtk::TreeModel::Row
StartScreen::active_combo(std::string widget_name)
{
    Gtk::ComboBox *combo;
    builder->get_widget(widget_name, combo);
    if (!combo) throw 1;
    Gtk::TreeModel::iterator iter = combo->get_active();
    if (!iter) throw 2;
    Gtk::TreeModel::Row row = *iter;
    if (!row) throw 3;
    return row;
}

/**
 * Set the active item in the combo based on the unique_id (column set in glade)
 *
 * @param widget_name - The name of the widget in the glade file
 * @param unique_id - The column id to activate, sets to first item if blank.
 */
void
StartScreen::set_active_combo(std::string widget_name, std::string unique_id)
{
    Gtk::ComboBox *combo;
    builder->get_widget(widget_name, combo);
    if (combo) {
        if (unique_id.empty()) {
            combo->set_active(0); // Select the first
        } else if(!combo->set_active_id(unique_id)) {
            combo->set_active(-1); // Select nothing
        }
    }
}

/**
 * When a notbook is switched, reveal the right banner image (gtk signal).
 */
void
StartScreen::notebook_switch(Gtk::Widget *tab, guint page_num)
{
    int page = 0;
    for (auto banner : banners->get_children()) {
        auto revealer = dynamic_cast<Gtk::Revealer *>(banner);
        revealer->set_reveal_child(page == page_num);
        page++;
    }
}

void
StartScreen::enlist_recent_files()
{
    NameIdCols cols;
    Gtk::TreeView *recent;
    builder->get_widget("recent_treeview", recent);
    if (!recent) return;
    // We're not sure why we have to ask C for the TreeStore object
    auto store = Glib::wrap(GTK_LIST_STORE(gtk_tree_view_get_model(recent->gobj())));
    store->clear();

    Glib::RefPtr<Gtk::RecentManager> manager = Gtk::RecentManager::get_default();
    for (auto item : manager->get_items()) {
        if (item->has_application(g_get_prgname())
            || item->has_application("org.inkscape.Inkscape")
            || item->has_application("inkscape")
            || item->has_application("inkscape.exe")
           ) {
            std::string path = Glib::filename_from_uri(item->get_uri());
            if (Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR)
                && item->get_mime_type() == "image/svg+xml") {
                Gtk::TreeModel::Row row = *(store->append());
                row[cols.col_name] = item->get_display_name();
                row[cols.col_id] = item->get_uri();
            }
        }
    }
}

/**
 * Called when load button clicked or item is double clicked.
 */
void
StartScreen::load_now()
{
    bool is_template = true;
    Glib::ustring filename = sp_file_default_template_uri();
    Glib::ustring width = "";
    Glib::ustring height = "";

    // Find requested file name.
    Glib::RefPtr<Gio::File> file;
    if (kinds) {

        Gtk::Widget *selected_widget = kinds->get_children()[kinds->get_current_page()];
        auto container = dynamic_cast<Gtk::Container *>(selected_widget);
        if (container) {
            selected_widget = container->get_children()[0];
        }

        auto recent_list = dynamic_cast<Gtk::TreeView *>(selected_widget);
        if (recent_list) {
            auto iter = recent_list->get_selection()->get_selected();
            if (iter) {
                Gtk::TreeModel::Row row = *iter;
                if (row) {
                    NameIdCols cols;
                    Glib::ustring _file = row[cols.col_id];
                    file = Gio::File::create_for_uri(_file);
                    is_template = false;
                }
            }
        }

        auto template_list = dynamic_cast<Gtk::IconView *>(selected_widget);
        if (template_list) {
            auto items = template_list->get_selected_items();
            if (!items.empty()) {
                auto iter = template_list->get_model()->get_iter(items[0]);
                Gtk::TreeModel::Row row = *iter;
                if (row) {
                    is_template = true; // Already set
                    TemplateCols cols;
                    Glib::ustring template_filename = row[cols.filename];
                    if (!(template_filename == "-")) {
                        filename = Resource::get_filename_string(
                            Resource::TEMPLATES, template_filename.c_str(), true);
                    }
                    // This isn't used on opening, just for checking it's existance.
                    file = Gio::File::create_for_path(filename);
                    width = row[cols.width];
                    height = row[cols.height];
                }
            }
        }
    }

    if (!file) {
        // Failure to open, so open up a new document instead.
        file = Gio::File::create_for_path(filename);
        is_template = true;
    }

    if (!file) {
        // We're really messed up... so give up!
        std::cerr << "StartScreen::load_now(): Failed to find: " << filename << std::endl;
        response(GTK_RESPONSE_NONE);
        return;
    }

    // Now we have filename, open document.
    auto app = InkscapeApplication::instance();

    // If it was a template file, modify the document according to user's input.
    if (!is_template) {
        _document = app->document_open (file);
    } else {
        _document = app->document_new (filename);
        auto nv = sp_document_namedview (_document, nullptr);

        if (!width.empty()) {
            // Set the width, height and default display units for the selected template
            auto q_width = unit_table.parseQuantity(width);
            _document->setWidthAndHeight(q_width, unit_table.parseQuantity(height), true);
            nv->setAttribute("inkscape:document-units", q_width.unit->abbr);
        }

        DocumentUndo::clearUndo(_document);
        _document->setModifiedSinceSave(false);
    }

    // We're done, hand back to app.
    response(GTK_RESPONSE_OK);
}

/**
 * When a button needs to go to the next notebook page.
 */
void
StartScreen::notebook_next(Gtk::Widget *button)
{
    int page = tabs->get_current_page();
    if (page == 2) {
        load_now(); // Only occurs from keypress.
    } else {
        tabs->set_current_page(page + 1);
    }
}

/**
 * When a key is pressed in the main window.
 */
bool
StartScreen::on_key_press_event(GdkEventKey* event)
{
    switch (event->keyval) {
        case GDK_KEY_Escape:
            // Prevent loading any selected items
            kinds = nullptr;
            load_now();
            return true;
        case GDK_KEY_Return:
            notebook_next(nullptr);
            return true;
    }

    return false;
}

void
StartScreen::show_toggle()
{
    Gtk::ToggleButton *button;
    builder->get_widget("show_toggle", button);
    if (button) {
        auto prefs = Inkscape::Preferences::get();
        prefs->setBool("/options/boot/enabled", button->get_active());
    } else {
        g_warning("Can't find toggle button widget.");
    }
}

/**
 * Refresh theme in-place so user can see a semi-preview. This theme selection
 * is not meant to be perfect, but hint to the user that they can set the
 * theme if they want.
 *
 * @param theme_name - The name of the theme to load.
 */
void
StartScreen::refresh_theme(Glib::ustring theme_name)
{
    auto const screen = Gdk::Screen::get_default();
    if (INKSCAPE.contrastthemeprovider) {
        Gtk::StyleContext::remove_provider_for_screen(screen, INKSCAPE.contrastthemeprovider);
    }
    auto settings = Gtk::Settings::get_default();

    auto prefs = Inkscape::Preferences::get();

    settings->property_gtk_theme_name() = theme_name;
    settings->property_gtk_application_prefer_dark_theme() = prefs->getBool("/theme/darkTheme", true);
    settings->property_gtk_icon_theme_name() = prefs->getString("/theme/iconTheme");

    if (window) {
        if (prefs->getBool("/theme/symbolicIcons", false)) {
            window->get_style_context()->add_class("symbolic");
            window->get_style_context()->remove_class("regular");
        } else {
            window->get_style_context()->add_class("regular");
            window->get_style_context()->remove_class("symbolic");
        }
    }
    if (INKSCAPE.colorizeprovider) {
        Gtk::StyleContext::remove_provider_for_screen(screen, INKSCAPE.colorizeprovider);
    }
    if (!prefs->getBool("/theme/symbolicDefaultColors", false)) {
        Gtk::CssProvider::create();
        Glib::ustring css_str = INKSCAPE.get_symbolic_colors();
        try {
            INKSCAPE.colorizeprovider->load_from_data(css_str);
        } catch (const Gtk::CssProviderError &ex) {
            g_critical("CSSProviderError::load_from_data(): failed to load '%s'\n(%s)", css_str.c_str(), ex.what().c_str());
        }
        Gtk::StyleContext::add_provider_for_screen(screen, INKSCAPE.colorizeprovider,
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    INKSCAPE.signal_change_theme.emit();
}

/**
 * Set the theme, icon pack and other theme options from a set defined
 * in the glade file. The combo box has a number of columns with the needed
 * data decribing how to set up the theme.
 */
void
StartScreen::theme_changed()
{
    ThemeCols cols;
    try {
        auto row = active_combo("themes");

        auto prefs = Inkscape::Preferences::get();
        prefs->setString("/options/boot/theme", row[cols.id]);

        // Update theme from combo.
        Glib::ustring icons = row[cols.icons];
        prefs->setBool("/toolbox/tools/small", row[cols.smallicons]);
        prefs->setString("/theme/gtkTheme", row[cols.theme]);
        prefs->setString("/theme/iconTheme", icons);
        prefs->setBool("/theme/preferDarkTheme", row[cols.dark]);
        prefs->setBool("/theme/darkTheme", row[cols.dark]);
        prefs->setBool("/theme/symbolicIcons", row[cols.symbolic]);

        // Symbolic icon colours
        if (get_color_value(row[cols.base]) == 0) {
            prefs->setBool("/theme/symbolicDefaultColors", true);
        } else {
            Glib::ustring prefix = "/theme/" + icons;
            prefs->setBool("/theme/symbolicDefaultColors", false);
            prefs->setUInt(prefix + "/symbolicBaseColor", get_color_value(row[cols.base]));
            prefs->setUInt(prefix + "/symbolicSuccessColor", get_color_value(row[cols.success]));
            prefs->setUInt(prefix + "/symbolicWarningColor", get_color_value(row[cols.warn]));
            prefs->setUInt(prefix + "/symbolicErrorColor", get_color_value(row[cols.error]));
        }

        refresh_theme(row[cols.theme]);
    } catch(int e) {
        g_warning("Couldn't find theme value.");
    }
}

/**
 * Called when the canvas dropdown changes.
 */
void
StartScreen::canvas_changed()
{
    CanvasCols cols;
    try {
        auto row = active_combo("canvas");

        auto prefs = Inkscape::Preferences::get();
        prefs->setString("/options/boot/canvas", row[cols.id]);

        Gdk::RGBA gdk_color = Gdk::RGBA(row[cols.pagecolor]);
        SPColor sp_color(gdk_color.get_red(), gdk_color.get_green(), gdk_color.get_blue());
        prefs->setString("/template/base/pagecolor", sp_color.toString());
        prefs->setDouble("/template/base/pageopacity", gdk_color.get_alpha());

        Gdk::RGBA gdk_border = Gdk::RGBA(row[cols.bordercolor]);
        SPColor sp_border(gdk_border.get_red(), gdk_border.get_green(), gdk_border.get_blue());
        prefs->setString("/template/base/bordercolor", sp_border.toString());
        prefs->setDouble("/template/base/borderopacity", gdk_border.get_alpha());

        prefs->setBool("/template/base/pagecheckerboard", row[cols.checkered]);
        prefs->setInt("/template/base/pageshadow", row[cols.shadow] ? 2 : 0);

    } catch(int e) {
        g_warning("Couldn't find canvas value.");
    }
}

void
StartScreen::enlist_keys()
{
    NameIdCols cols;
    Gtk::ComboBox *keys;
    builder->get_widget("keys", keys);
    if (!keys) return;

    auto store = Glib::wrap(GTK_LIST_STORE(gtk_combo_box_get_model(keys->gobj())));
    store->clear();

    for(auto item: Inkscape::Shortcuts::get_file_names()){
        Gtk::TreeModel::Row row = *(store->append());
        row[cols.col_name] = item.first;
        row[cols.col_id] = item.second;
    }

    auto prefs = Inkscape::Preferences::get();
    auto current = prefs->getString("/options/kbshortcuts/shortcutfile");
    if (current.empty()) {
        current = "inkscape.xml";
    }
    keys->set_active_id(current);
}

/**
 * Set the keys file based on the keys set in the enlist above
 */
void
StartScreen::keyboard_changed()
{
    NameIdCols cols;
    try {
        auto row = active_combo("keys");
        auto prefs = Inkscape::Preferences::get();
        prefs->setString("/options/kbshortcuts/shortcutfile", row[cols.col_id]);
    } catch(int e) {
        g_warning("Couldn't find keys value.");
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
