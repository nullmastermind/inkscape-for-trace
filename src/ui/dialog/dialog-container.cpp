// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief A widget that manages DialogNotebook's and other widgets inside a horizontal DialogMultipaned.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dialog-container.h"

#include <giomm/file.h>
#include <glibmm/keyfile.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>

#include "enums.h"
#include "inkscape-application.h"
#include "ui/shortcuts.h"
#include "ui/dialog/align-and-distribute.h"
#include "ui/dialog/clonetiler.h"
#include "ui/dialog/dialog-multipaned.h"
#include "ui/dialog/dialog-notebook.h"
#include "ui/dialog/dialog-window.h"
#include "ui/dialog/document-properties.h"
#include "ui/dialog/export.h"
#include "ui/dialog/fill-and-stroke.h"
#include "ui/dialog/filter-effects-dialog.h"
#include "ui/dialog/find.h"
#include "ui/dialog/glyphs.h"
#include "ui/dialog/icon-preview.h"
#include "ui/dialog/inkscape-preferences.h"
#include "ui/dialog/input.h"
#include "ui/dialog/layers.h"
#include "ui/dialog/livepatheffect-editor.h"
#include "ui/dialog/memory.h"
#include "ui/dialog/messages.h"
#include "ui/dialog/object-attributes.h"
#include "ui/dialog/object-properties.h"
#include "ui/dialog/objects.h"
#include "ui/dialog/paint-servers.h"
#include "ui/dialog/prototype.h"
#include "ui/dialog/selectorsdialog.h"
#if WITH_GSPELL
#include "ui/dialog/spellcheck.h"
#endif
#include "ui/dialog/styledialog.h"
#include "ui/dialog/svg-fonts-dialog.h"
#include "ui/dialog/swatches.h"
#include "ui/dialog/symbols.h"
#include "ui/dialog/text-edit.h"
#include "ui/dialog/tile.h"
#include "ui/dialog/tracedialog.h"
#include "ui/dialog/transformation.h"
#include "ui/dialog/undo-history.h"
#include "ui/dialog/xml-tree.h"
#include "ui/icon-names.h"
#include "ui/widget/canvas-grid.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

DialogContainer::DialogContainer()
{
    set_name("DialogContainer");

    // Setup main column
    columns = Gtk::manage(new DialogMultipaned(Gtk::ORIENTATION_HORIZONTAL));

    connections.emplace_back(columns->signal_prepend_drag_data().connect(
        sigc::bind<DialogMultipaned *>(sigc::mem_fun(*this, &DialogContainer::prepend_drop), columns)));

    connections.emplace_back(columns->signal_append_drag_data().connect(
        sigc::bind<DialogMultipaned *>(sigc::mem_fun(*this, &DialogContainer::append_drop), columns)));

    // Setup drop targets.
    target_entries.emplace_back(Gtk::TargetEntry("GTK_NOTEBOOK_TAB"));
    columns->set_target_entries(target_entries);

    add(*columns);

    // Should probably be moved to window.
    connections.emplace_back(signal_unmap().connect(sigc::mem_fun(*this, &DialogContainer::on_unmap)));

    show_all_children();
}

DialogMultipaned *DialogContainer::create_column()
{
    DialogMultipaned *column = Gtk::manage(new DialogMultipaned(Gtk::ORIENTATION_VERTICAL));

    connections.emplace_back(column->signal_prepend_drag_data().connect(
        sigc::bind<DialogMultipaned *>(sigc::mem_fun(*this, &DialogContainer::prepend_drop), column)));

    connections.emplace_back(column->signal_append_drag_data().connect(
        sigc::bind<DialogMultipaned *>(sigc::mem_fun(*this, &DialogContainer::append_drop), column)));

    connections.emplace_back(column->signal_now_empty().connect(
        sigc::bind<DialogMultipaned *>(sigc::mem_fun(*this, &DialogContainer::column_empty), column)));

    column->set_target_entries(target_entries);

    return column;
}

/**
 * Get an instance of a DialogBase dialog using the associated verb code.
 */
DialogBase *DialogContainer::dialog_factory(unsigned int code)
{
    if (code <= 0) {
        return nullptr;
    }

    switch (code) {
        case SP_VERB_DIALOG_ALIGN_DISTRIBUTE:
            return &Inkscape::UI::Dialog::AlignAndDistribute::getInstance();
        case SP_VERB_SELECTION_ARRANGE:
            return &Inkscape::UI::Dialog::ArrangeDialog::getInstance();
        case SP_VERB_DIALOG_CLONETILER:
            return &Inkscape::UI::Dialog::CloneTiler::getInstance();
        case SP_VERB_DIALOG_DEBUG:
            return &Inkscape::UI::Dialog::Messages::getInstance();
        case SP_VERB_DIALOG_DOCPROPERTIES:
            return &Inkscape::UI::Dialog::DocumentProperties::getInstance();
        case SP_VERB_DIALOG_EXPORT:
            return &Inkscape::UI::Dialog::Export::getInstance();
        case SP_VERB_DIALOG_FILL_STROKE:
            return &Inkscape::UI::Dialog::FillAndStroke::getInstance();
        case SP_VERB_DIALOG_FILTER_EFFECTS:
            return &Inkscape::UI::Dialog::FilterEffectsDialog::getInstance();
        case SP_VERB_DIALOG_FIND:
            return &Inkscape::UI::Dialog::Find::getInstance();
        case SP_VERB_DIALOG_GLYPHS:
            return &Inkscape::UI::Dialog::GlyphsPanel::getInstance();
        case SP_VERB_DIALOG_INPUT:
            return &Inkscape::UI::Dialog::InputDialog::getInstance();
        case SP_VERB_DIALOG_LAYERS:
            return &Inkscape::UI::Dialog::LayersPanel::getInstance();
        case SP_VERB_DIALOG_LIVE_PATH_EFFECT:
            return &Inkscape::UI::Dialog::LivePathEffectEditor::getInstance();
        case SP_VERB_DIALOG_ATTR:
            return &Inkscape::UI::Dialog::ObjectAttributes::getInstance();
        case SP_VERB_DIALOG_ITEM:
            return &Inkscape::UI::Dialog::ObjectProperties::getInstance();
        case SP_VERB_DIALOG_OBJECTS:
            return &Inkscape::UI::Dialog::ObjectsPanel::getInstance();
        case SP_VERB_DIALOG_PAINT:
            return &Inkscape::UI::Dialog::PaintServersDialog::getInstance();
        case SP_VERB_DIALOG_PROTOTYPE:
            return &Inkscape::UI::Dialog::Prototype::getInstance();
        case SP_VERB_DIALOG_SELECTORS:
            return &Inkscape::UI::Dialog::SelectorsDialog::getInstance();
#if WITH_GSPELL
        case SP_VERB_DIALOG_SPELLCHECK:
            return &Inkscape::UI::Dialog::SpellCheck::getInstance();
#endif
        case SP_VERB_DIALOG_STYLE:
            return &Inkscape::UI::Dialog::StyleDialog::getInstance();
        case SP_VERB_DIALOG_SVG_FONTS:
            return &Inkscape::UI::Dialog::SvgFontsDialog::getInstance();
        case SP_VERB_DIALOG_SWATCHES:
            return &Inkscape::UI::Dialog::SwatchesPanel::getInstance();
        case SP_VERB_DIALOG_SYMBOLS:
            return &Inkscape::UI::Dialog::SymbolsDialog::getInstance();
        case SP_VERB_DIALOG_TEXT:
            return &Inkscape::UI::Dialog::TextEdit::getInstance();
        case SP_VERB_DIALOG_TRANSFORM:
            return &Inkscape::UI::Dialog::Transformation::getInstance();
        case SP_VERB_DIALOG_UNDO_HISTORY:
            return &Inkscape::UI::Dialog::UndoHistory::getInstance();
        case SP_VERB_DIALOG_XML_EDITOR:
            return &Inkscape::UI::Dialog::XmlTree::getInstance();
        case SP_VERB_HELP_MEMORY:
            return &Inkscape::UI::Dialog::Memory::getInstance();
        case SP_VERB_SELECTION_TRACE:
            return &Inkscape::UI::Dialog::TraceDialog::getInstance();
        case SP_VERB_VIEW_ICON_PREVIEW:
            return &Inkscape::UI::Dialog::IconPreviewPanel::getInstance();
        case SP_VERB_DIALOG_PREFERENCES:
        case SP_VERB_CONTEXT_SELECT_PREFS:
        case SP_VERB_CONTEXT_NODE_PREFS:
        case SP_VERB_CONTEXT_TWEAK_PREFS:
        case SP_VERB_CONTEXT_SPRAY_PREFS:
        case SP_VERB_CONTEXT_RECT_PREFS:
        case SP_VERB_CONTEXT_3DBOX_PREFS:
        case SP_VERB_CONTEXT_ARC_PREFS:
        case SP_VERB_CONTEXT_STAR_PREFS:
        case SP_VERB_CONTEXT_SPIRAL_PREFS:
        case SP_VERB_CONTEXT_PENCIL_PREFS:
        case SP_VERB_CONTEXT_PEN_PREFS:
        case SP_VERB_CONTEXT_CALLIGRAPHIC_PREFS:
        case SP_VERB_CONTEXT_TEXT_PREFS:
        case SP_VERB_CONTEXT_GRADIENT_PREFS:
        case SP_VERB_CONTEXT_MESH_PREFS:
        case SP_VERB_CONTEXT_ZOOM_PREFS:
        case SP_VERB_CONTEXT_MEASURE_PREFS:
        case SP_VERB_CONTEXT_DROPPER_PREFS:
        case SP_VERB_CONTEXT_CONNECTOR_PREFS:
        case SP_VERB_CONTEXT_PAINTBUCKET_PREFS:
        case SP_VERB_CONTEXT_ERASER_PREFS:
        case SP_VERB_CONTEXT_LPETOOL_PREFS:
            return &Inkscape::UI::Dialog::InkscapePreferences::getInstance();
        default:
            return nullptr;
    }
}

// Create the notebook tab
Gtk::Widget *DialogContainer::create_notebook_tab(Glib::ustring label_str, Glib::ustring image_str,
                                                  Gtk::AccelKey key)
{
    Gtk::Label *label = Gtk::manage(new Gtk::Label(label_str));
    Gtk::Image *image = Gtk::manage(new Gtk::Image());
    image->set_from_icon_name(image_str, Gtk::ICON_SIZE_MENU);
    Gtk::Box *tab = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2));
    tab->set_name(label_str);
    tab->pack_start(*image);
    tab->pack_end(*label);
    tab->show_all();

    // Workaround to the fact that Gtk::Box doesn't receive on_button_press event
    Gtk::EventBox *cover = Gtk::manage(new Gtk::EventBox());
    cover->add(*tab);

    // Add shortcut tooltip
    if (!key.is_null()) {
        auto tlabel = Inkscape::Shortcuts::get_label(key);
        int pos = tlabel.find("&", 0);
        if (pos >= 0 && pos < tlabel.length()) {
            tlabel.replace(pos, 1, "&amp;");
        }
        tab->set_tooltip_markup(label_str + " (<b>" + tlabel + "</b>)");
    }

    return cover;
}

/**
 * Add new dialog to the current container or in a floating window, based on preferences.
 */
void DialogContainer::new_dialog(unsigned int code)
{
    // Open all dialogs as floating, if set in preferences
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs == nullptr) {
        return;
    }

    int dockable = prefs->getInt("/options/dialogtype/value", PREFS_DIALOGS_BEHAVIOR_DOCKABLE);
    if (dockable == PREFS_DIALOGS_BEHAVIOR_FLOATING) {
        new_floating_dialog(code);
    } else {
        new_dialog(code, nullptr);
    }
}

/**
 * Overloaded new_dialog
 */
void DialogContainer::new_dialog(unsigned int code, DialogNotebook *notebook)
{
    // Get the verb with that code
    Inkscape::Verb *verb = Inkscape::Verb::get(code);

    // Can't understand the dialog's settings without an associated verb
    if (!verb) {
        return;
    }

    // Limit each container to containing one of any type of dialog.
    auto it = dialogs.find(code);
    if (it != dialogs.end()) {
        // Blink notebook with existing dialog to let user know where it is and show page.
        it->second->blink();
        return;
    }

    // Create the dialog widget
    DialogBase *dialog = dialog_factory(code);

    if (!dialog) {
        std::cerr << "DialogContainer::new_dialog(): couldn't find dialog for: " << verb->get_id() << std::endl;
        return;
    }

    // manage the dialog instance
    dialog = Gtk::manage(dialog);

    // Create the notebook tab
    auto image = verb->get_image();
    Gtk::Widget *tab =
        create_notebook_tab(dialog->get_name(), image ? Glib::ustring(image) : INKSCAPE_ICON("inkscape-logo"),
                            (Inkscape::Shortcuts::getInstance()).get_shortcut_from_verb(verb));

    // If not from notebook menu add at top of last column.
    if (!notebook) {
        // Look to see if last column contains a multipane. If not, add one.
        DialogMultipaned *last_column = dynamic_cast<DialogMultipaned *>(columns->get_last_widget());
        if (!last_column) {
            last_column = create_column();
            columns->append(last_column);
        }

        // Look to see if first widget in column is notebook, if not add one.
        notebook = dynamic_cast<DialogNotebook *>(last_column->get_first_widget());
        if (!notebook) {
            notebook = Gtk::manage(new DialogNotebook(this));
            last_column->prepend(notebook);
        }
    }

    // Add dialog
    notebook->add_page(*dialog, *tab, dialog->get_name());
}

/**
 * Add a new floating dialog
 */
void DialogContainer::new_floating_dialog(unsigned int code)
{
    // Get the verb with that code
    Inkscape::Verb *verb = Inkscape::Verb::get(code);

    // Can't understand the dialog's settings without an associated verb
    if (!verb) {
        return;
    }

    // Create the dialog widget
    DialogBase *dialog = dialog_factory(code);

    if (!dialog) {
        std::cerr << "DialogContainer::new_dialog(): couldn't find dialog for: " << verb->get_id() << std::endl;
        return;
    }

    // manage the dialog instance
    dialog = Gtk::manage(dialog);

    // Create the notebook tab
    auto image = verb->get_image();
    Gtk::Widget *tab =
        create_notebook_tab(dialog->get_name(), image ? Glib::ustring(image) : INKSCAPE_ICON("inkscape-logo"),
                            (Inkscape::Shortcuts::getInstance()).get_shortcut_from_verb(verb));

    // New temporary noteboook
    DialogNotebook *notebook = Gtk::manage(new DialogNotebook(this));
    notebook->add_page(*dialog, *tab, dialog->get_name());

    notebook->pop_tab_callback();
}

void DialogContainer::toggle_dialogs()
{
    columns->toggle_multipaned_children();
}

// Update dialogs
void DialogContainer::update_dialogs()
{
    for_each(dialogs.begin(), dialogs.end(), [&](auto dialog) { dialog.second->update(); });
}

bool DialogContainer::has_dialog_of_type(DialogBase *dialog)
{
    return (dialogs.find(dialog->getVerb()) != dialogs.end());
}

DialogBase *DialogContainer::get_dialog(unsigned int code)
{
    auto found = dialogs.find(code);
    if (found != dialogs.end()) {
        return found->second;
    }

    return nullptr;
}

// Add dialog to list.
void DialogContainer::link_dialog(DialogBase *dialog)
{
    dialogs.insert(std::pair<int, DialogBase *>(dialog->getVerb(), dialog));

    DialogWindow *window = dynamic_cast<DialogWindow *>(get_toplevel());
    if (window) {
        window->update_dialogs();
    }
}

// Remove dialog from list.
void DialogContainer::unlink_dialog(DialogBase *dialog)
{
    if (!dialog) {
        return;
    }

    auto found = dialogs.find(dialog->getVerb());
    if (found != dialogs.end()) {
        dialogs.erase(found);
    }

    DialogWindow *window = dynamic_cast<DialogWindow *>(get_toplevel());
    if (window) {
        window->update_dialogs();
    }
}


/**
 * Load last open window's dialog configuration state.
 *
 * For the keyfile format, check `save_container_state()`.
 */
void DialogContainer::load_container_state()
{
    load_container_state("dialogs-state.ini");
}

/**
 * Load last open window's dialog configuration state.
 *
 * For the keyfile format, check `save_container_state()`.
 */
void DialogContainer::load_container_state(Glib::ustring filename)
{
    // Step 0: check if we want to load the state
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs == nullptr) {
        return;
    }

    int save_state = prefs->getInt("/options/savedialogposition/value", PREFS_DIALOGS_STATE_SAVE);
    if (save_state == PREFS_DIALOGS_STATE_NONE) {
        return; // User has turned off this feature in Preferences
    }

    // if it isn't dockable, all saved docked dialogs are made floating
    bool is_dockable =
        prefs->getInt("/options/dialogtype/value", PREFS_DIALOGS_BEHAVIOR_DOCKABLE) != PREFS_DIALOGS_BEHAVIOR_FLOATING;

    // Step 1: Read from file
    Glib::ustring file = Glib::build_filename(Glib::get_user_cache_dir(), "inkscape", filename);
    Glib::KeyFile *keyfile = new Glib::KeyFile();

    try {
        keyfile->load_from_file(file);
    } catch (Glib::Error &error) {
        std::cerr << "DialogContainer::load_container_state: " << error.what() << std::endl;
        return;
    }

    // Step 2: get the number of windows
    int windows_count = 0;
    try {
        windows_count = keyfile->get_integer("Windows", "Count");
    } catch (Glib::Error &error) {
        std::cerr << "DialogContainer::load_container_state: " << error.what() << std::endl;
    }

    // Step 3: for each window, load its state. Only the first window is not floating (the others are DialogWindow)
    for (int window_idx = 0; window_idx < windows_count; ++window_idx) {
        Glib::ustring group_name = "Window" + std::to_string(window_idx);

        // Step 3.0: read the window parameters
        int column_count = 0;
        bool floating = window_idx != 0;
        try {
            column_count = keyfile->get_integer(group_name, "ColumnCount");
            floating = keyfile->get_boolean(group_name, "Floating");
        } catch (Glib::Error &error) {
            std::cerr << "DialogContainer::load_container_state: " << error.what() << std::endl;
        }

        // Step 3.1: get the window's container columns where we want to create the dialogs
        DialogContainer *active_container = nullptr;
        DialogMultipaned *active_columns = nullptr;
        DialogWindow *dialog_window = nullptr;

        if (is_dockable) {
            if (floating) {
                dialog_window = new DialogWindow(nullptr);
                if (dialog_window) {
                    active_container = dialog_window->get_container();
                    active_columns = dialog_window->get_container()->get_columns();
                }
            } else {
                active_container = this;
                active_columns = columns;
            }

            if (!active_container || !active_columns) {
                continue;
            }
        }

        // Step 3.2: for each column, load its state
        for (int column_idx = 0; column_idx < column_count; ++column_idx) {
            Glib::ustring column_group_name = group_name + "Column" + std::to_string(column_idx);

            // Step 3.2.0: read the column parameters
            int notebook_count = 0;
            bool before_canvas = false;
            try {
                notebook_count = keyfile->get_integer(column_group_name, "NotebookCount");
                before_canvas = keyfile->get_boolean(column_group_name, "BeforeCanvas");
            } catch (Glib::Error &error) {
                std::cerr << "DialogContainer::load_container_state: " << error.what() << std::endl;
            }

            // Step 3.2.1: create the column
            DialogMultipaned *column = nullptr;
            if (is_dockable) {
                column = active_container->create_column();
                if (!column) {
                    continue;
                }

                before_canvas ?  active_columns->prepend(column) : active_columns->append(column);
            }

            // Step 3.2.2: for each noteboook, load its dialogs
            for (int notebook_idx = 0; notebook_idx < notebook_count; ++notebook_idx) {
                Glib::ustring key = "Notebook" + std::to_string(notebook_idx) + "Dialogs";

                // Step 3.2.2.0 read the list of dialog verbs in the current notebook
                std::vector<int> dialogs;
                try {
                    dialogs = keyfile->get_integer_list(column_group_name, key);
                } catch (Glib::Error &error) {
                    std::cerr << "DialogContainer::load_container_state: " << error.what() << std::endl;
                }

                if (!dialogs.size()) {
                    continue;
                }

                DialogNotebook *notebook = nullptr;
                if (is_dockable) {
                    notebook = Gtk::manage(new DialogNotebook(active_container));
                    column->append(notebook);
                }

                // Step 3.2.2.1 create each dialog in the current notebook
                for (auto verb_code : dialogs) {
                    Verb *verb = Inkscape::Verb::get(verb_code);

                    if (verb) {
                        if (is_dockable) {
                            active_container->new_dialog(verb_code, notebook);
                        } else {
                            new_floating_dialog(verb_code);
                        }
                    }
                }
            }
        }

        if (dialog_window) {
            dialog_window->update_window_size_to_fit_children();
        }
    }
}

/**
 * Save container state. The configuration of open dialogs and the relative positions of the notebooks are saved.
 *
 * The structure of such a KeyFile is:
 *
 * There is a "Windows" group that records the number of the windows:
 * [Windows]
 * Count=1
 *
 * A "WindowX" group saves the number of columns the window's container has and whether the window is floating:
 *
 * [Window0]
 * ColumnCount=1
 * Floating=false
 *
 * For each column, we have a "WindowWColumnX" group, where X is the index of the column. "BeforeCanvas" checks
 * if the column is before the canvas or not. "NotebookCount" records how many notebooks are in each column and
 * "NotebookXDialogs" records a list of the verb numbers for the dialogs in notebook X.
 *
 * [Window0Column0]
 * Notebook0Dialogs=262;263;
 * NotebookCount=2
 * BeforeCanvas=false
 *
 */
void DialogContainer::save_container_state()
{
    // Step 0: check if we want to save the state
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs == nullptr) {
        return;
    }

    int save_state = prefs->getInt("/options/savedialogposition/value", PREFS_DIALOGS_STATE_SAVE);
    if (save_state == PREFS_DIALOGS_STATE_NONE) {
        return; // User has turned off this feature in Preferences
    }

    Glib::KeyFile *keyfile = new Glib::KeyFile();
    auto app = InkscapeApplication::instance();

    // Step 1: get all the container columns (in order, from the current container and all DialogWindow containers)
    std::vector<DialogMultipaned *> windows(1, columns);

    for (auto const &window : app->gtk_app()->get_windows()) {
        DialogWindow *dialog_window = dynamic_cast<DialogWindow *>(window);
        if (dialog_window) {
            windows.push_back(dialog_window->get_container()->get_columns());
        }
    }

    // Step 2: save the number of windows
    keyfile->set_integer("Windows", "Count", windows.size());

    // Step 3: for each window, save its data. Only the first window is not floating (the others are DialogWindow)
    for (int window_idx = 0; window_idx < (int)windows.size(); ++window_idx) {
        // Step 3.0: get all the multipanes of the window
        std::vector<DialogMultipaned *> multipanes;

        // used to check if the column is before or after canvas
        std::vector<DialogMultipaned *>::iterator multipanes_it = multipanes.begin();
        bool canvas_seen = window_idx != 0; // no floating windows (window_idx > 0) have a canvas
        int before_canvas_columns_count = 0;

        for (auto const &column : windows[window_idx]->get_children()) {
            if (!canvas_seen) {
                UI::Widget::CanvasGrid *canvas = dynamic_cast<UI::Widget::CanvasGrid *>(column);
                if (canvas) {
                    canvas_seen = true;
                } else {
                    DialogMultipaned *paned = dynamic_cast<DialogMultipaned *>(column);
                    if (paned) {
                        multipanes_it = multipanes.insert(multipanes_it, paned);
                        before_canvas_columns_count++;
                    }
                }
            } else {
                DialogMultipaned *paned = dynamic_cast<DialogMultipaned *>(column);
                if (paned) {
                    multipanes.push_back(paned);
                }
            }
        }

        // Step 3.1: for each non-empty column, save its data.
        int column_count = 0; // non-empty columns count
        for (int column_idx = 0; column_idx < (int)multipanes.size(); ++column_idx) {
            Glib::ustring group_name = "Window" + std::to_string(window_idx) + "Column" + std::to_string(column_idx);
            int notebook_count = 0; // non-empty notebooks count

            // Step 3.1.0: for each notebook, get its dialogs' verbs
            for (auto const &columns_widget : multipanes[column_idx]->get_children()) {
                DialogNotebook *dialog_notebook = dynamic_cast<DialogNotebook *>(columns_widget);

                if (dialog_notebook) {
                    std::vector<int> dialogs;

                    for (auto const &widget : dialog_notebook->get_notebook()->get_children()) {
                        DialogBase *dialog = dynamic_cast<DialogBase *>(widget);
                        if (dialog) {
                            dialogs.push_back(dialog->getVerb());
                        }
                    }

                    // save the dialogs verbs
                    Glib::ustring key = "Notebook" + std::to_string(notebook_count) + "Dialogs";
                    keyfile->set_integer_list(group_name, key, dialogs);

                    // increase the notebook count
                    notebook_count++;
                }
            }

            // Step 3.1.1: increase the column count
            if (notebook_count != 0) {
                column_count++;
            }

            // Step 3.1.2: Save the column's data
            keyfile->set_integer(group_name, "NotebookCount", notebook_count);
            keyfile->set_boolean(group_name, "BeforeCanvas", (column_idx < before_canvas_columns_count));
        }

        // Step 3.2: save the window group
        Glib::ustring group_name = "Window" + std::to_string(window_idx);
        keyfile->set_integer(group_name, "ColumnCount", column_count);
        keyfile->set_boolean(group_name, "Floating", window_idx != 0);
    }

    // Step 4: Write to file
    Glib::ustring path = Glib::build_filename(Glib::get_user_cache_dir(), "inkscape");
    Glib::ustring file = Glib::build_filename(path, "dialogs-state.ini");

    if (!Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
        Gio::File::create_for_path(path)->make_directory_with_parents();
    }

    try {
        keyfile->save_to_file(file);
    } catch (Glib::FileError &error) {
        std::cerr << "DialogContainer::save_container_state(): " << error.what() << std::endl;
    }
}

// Signals -----------------------------------------------------

/**
 * No zombie windows. TODO: Need to work on this as it still leaves Gtk::Window! (?)
 */
void DialogContainer::on_unmap()
{
    // Disconnect all signals
    for_each(connections.begin(), connections.end(), [&](auto c) { c.disconnect(); });

    // Save the state only if you are in an InkscapeWindow
    DialogWindow *window = dynamic_cast<DialogWindow *>(get_toplevel());
    if (!window) {
        std::vector<Gtk::Window *> windows =
            (InkscapeApplication::instance()->gtk_app())->get_windows();
        int inkscape_windows_count =
            std::count_if(windows.begin(), windows.end(), [](auto w) { return !dynamic_cast<DialogWindow *>(w); });

        // First check if this is the last InkscapeWindow, to not save unnecessarily
        if (inkscape_windows_count == 1) {
            save_container_state();
        }
    }

    delete columns;
}

// Create a new notebook and move page.
DialogNotebook *DialogContainer::prepare_drop(const Glib::RefPtr<Gdk::DragContext> context)
{
    Gtk::Widget *source = Gtk::Widget::drag_get_source_widget(context);

    // Find source notebook and page
    Gtk::Notebook *old_notebook = dynamic_cast<Gtk::Notebook *>(source);
    if (!old_notebook) {
        std::cerr << "DialogContainer::prepare_drop: notebook not found!" << std::endl;
        return nullptr;
    }

    // Find page
    Gtk::Widget *page = old_notebook->get_nth_page(old_notebook->get_current_page());
    if (!page) {
        std::cerr << "DialogContainer::prepare_drop: page not found!" << std::endl;
        return nullptr;
    }

    // Create new notebook and move page.
    DialogNotebook *new_notebook = Gtk::manage(new DialogNotebook(this));
    new_notebook->move_page(*page);

    // move_page() takes care of updating dialog lists.
    return new_notebook;
}

// Notebook page dropped on prepend target. Call function to create new notebook and then insert.
void DialogContainer::prepend_drop(const Glib::RefPtr<Gdk::DragContext> context, DialogMultipaned *multipane)
{
    DialogNotebook *new_notebook = prepare_drop(context); // Creates notebook, moves page.
    if (!new_notebook) {
        std::cerr << "DialogContainer::prepend_drop: no new notebook!" << std::endl;
        return;
    }

    if (multipane->get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        // Columns
        // Create column
        DialogMultipaned *column = create_column();
        column->prepend(new_notebook);
        columns->prepend(column);
    } else {
        // Column
        multipane->prepend(new_notebook);
    }

    update_dialogs(); // Always update dialogs on Notebook change
}

// Notebook page dropped on append target. Call function to create new notebook and then insert.
void DialogContainer::append_drop(const Glib::RefPtr<Gdk::DragContext> context, DialogMultipaned *multipane)
{
    DialogNotebook *new_notebook = prepare_drop(context); // Creates notebook, moves page.
    if (!new_notebook) {
        std::cerr << "DialogContainer::append_drop: no new notebook!" << std::endl;
        return;
    }

    if (multipane->get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        // Columns
        // Create column
        DialogMultipaned *column = create_column();
        column->append(new_notebook);
        columns->append(column);
    } else {
        // Column
        multipane->append(new_notebook);
    }

    update_dialogs(); // Always update dialogs on Notebook change
}

/**
 * If a DialogMultipaned column is empty and it can be removed, remove it
 */
void DialogContainer::column_empty(DialogMultipaned *column)
{
    DialogMultipaned *parent = dynamic_cast<DialogMultipaned *>(column->get_parent());
    if (parent) {
        parent->remove(*column);
    }

    DialogWindow *window = dynamic_cast<DialogWindow *>(get_toplevel());
    if (window && parent) {
        auto children = parent->get_children();
        // Close the DialogWindow if you're in an empty one
        if (children.size() == 3 && parent->has_empty_widget()) {
            window->close();
        }
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
