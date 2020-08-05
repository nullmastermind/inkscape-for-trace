// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief A widget that manages InkNotebook's and other widgets inside a horizontal InkMultipane.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dialog-container.h"

#include <gtkmm/image.h>

#include "ui/dialog/align-and-distribute.h"
#include "ui/dialog/clonetiler.h"
#include "ui/dialog/dialog-multipaned.h"
#include "ui/dialog/dialog-notebook.h"
#include "ui/dialog/dialog-window.h"
#include "ui/dialog/document-properties.h"
#include "ui/dialog/export.h"
#include "ui/dialog/extension-editor.h"
#include "ui/dialog/fill-and-stroke.h"
#include "ui/dialog/filter-editor.h"
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

// Helper used because switch doesn't support strings
// Source: https://stackoverflow.com/a/16388610/4679160
constexpr unsigned int str2int(const char *str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

/**
 * Get an instance of a DialogBase dialog using and encoding name.
 */
DialogBase *DialogContainer::dialog_factory(Glib::ustring name)
{
    if (name.empty()) {
        return nullptr;
    }

    switch (str2int(name.c_str())) {
        case str2int("DialogAlignDistribute"):
            return &Inkscape::UI::Dialog::AlignAndDistribute::getInstance();
        case str2int("DialogArrange"):
            return &Inkscape::UI::Dialog::ArrangeDialog::getInstance();
        case str2int("DialogClonetiler"):
            return &Inkscape::UI::Dialog::CloneTiler::getInstance();
        case str2int("DialogDebug"):
            return &Inkscape::UI::Dialog::Messages::getInstance();
        case str2int("DialogDocumentProperties"):
            return &Inkscape::UI::Dialog::DocumentProperties::getInstance();
        case str2int("DialogExport"):
            return &Inkscape::UI::Dialog::Export::getInstance();
        case str2int("DialogExtensionEditor"):
            return &Inkscape::UI::Dialog::ExtensionEditor::getInstance();
        case str2int("DialogFillStroke"):
            return &Inkscape::UI::Dialog::FillAndStroke::getInstance();
        case str2int("DialogFilterEffects"):
            return &Inkscape::UI::Dialog::FilterEffectsDialog::getInstance();
        case str2int("DialogFind"):
            return &Inkscape::UI::Dialog::Find::getInstance();
        case str2int("DialogGlyphs"):
            return &Inkscape::UI::Dialog::GlyphsPanel::getInstance();
        case str2int("DialogInput"):
            return &Inkscape::UI::Dialog::InputDialog::getInstance();
        case str2int("DialogLayers"):
            return &Inkscape::UI::Dialog::LayersPanel::getInstance();
        case str2int("DialogLivePathEffect"):
            return &Inkscape::UI::Dialog::LivePathEffectEditor::getInstance();
        case str2int("DialogObjectAttributes"):
            return &Inkscape::UI::Dialog::ObjectAttributes::getInstance();
        case str2int("DialogObjectProperties"):
            return &Inkscape::UI::Dialog::ObjectProperties::getInstance();
        case str2int("DialogObjects"):
            return &Inkscape::UI::Dialog::ObjectsPanel::getInstance();
        case str2int("DialogPaintServers"):
            return &Inkscape::UI::Dialog::PaintServersDialog::getInstance();
        case str2int("DialogPrototype"):
            return &Inkscape::UI::Dialog::Prototype::getInstance();
        case str2int("DialogSelectors"):
            return &Inkscape::UI::Dialog::SelectorsDialog::getInstance();
#if WITH_GSPELL
        case str2int("DialogSpellcheck"):
            return &Inkscape::UI::Dialog::SpellCheck::getInstance();
#endif
        case str2int("DialogStyle"):
            return &Inkscape::UI::Dialog::StyleDialog::getInstance();
        case str2int("DialogSVGFonts"):
            return &Inkscape::UI::Dialog::SvgFontsDialog::getInstance();
        case str2int("DialogSwatches"):
            return &Inkscape::UI::Dialog::SwatchesPanel::getInstance();
        case str2int("DialogSymbols"):
            return &Inkscape::UI::Dialog::SymbolsDialog::getInstance();
        case str2int("DialogText"):
            return &Inkscape::UI::Dialog::TextEdit::getInstance();
        case str2int("DialogTransform"):
            return &Inkscape::UI::Dialog::Transformation::getInstance();
        case str2int("DialogUndoHistory"):
            return &Inkscape::UI::Dialog::UndoHistory::getInstance();
        case str2int("DialogXMLEditor"):
            return &Inkscape::UI::Dialog::XmlTree::getInstance();
        case str2int("FilterEditorDialog"):
            return &Inkscape::UI::Dialog::FilterEditorDialog::getInstance();
        case str2int("HelpAboutMemory"):
            return &Inkscape::UI::Dialog::Memory::getInstance();
        case str2int("InkscapePreferences"):
        case str2int("SelectPrefs"):
        case str2int("NodePrefs"):
        case str2int("TweakPrefs"):
        case str2int("SprayPrefs"):
        case str2int("RectPrefs"):
        case str2int("3DBoxPrefs"):
        case str2int("ArcPrefs"):
        case str2int("StarPrefs"):
        case str2int("SpiralPrefs"):
        case str2int("PencilPrefs"):
        case str2int("PenPrefs"):
        case str2int("CalligraphicPrefs"):
        case str2int("TextPrefs"):
        case str2int("GradientPrefs"):
        case str2int("Mesh_Prefs"):
        case str2int("ZoomPrefs"):
        case str2int("MeasurePrefs"):
        case str2int("DropperPrefs"):
        case str2int("ConnectorPrefs"):
        case str2int("PaintBucketPrefs"):
        case str2int("EraserPrefs"):
        case str2int("LPEToolPrefs"):
            return &Inkscape::UI::Dialog::InkscapePreferences::getInstance();
        case str2int("SelectionTrace"):
            return &Inkscape::UI::Dialog::TraceDialog::getInstance();
        case str2int("ViewIconPreview"):
            return &Inkscape::UI::Dialog::IconPreviewPanel::getInstance();
        default:
            return nullptr;
    }
}

// Create the notebook tab
Gtk::Widget *DialogContainer::create_notebook_tab(Glib::ustring label_str, Glib::ustring image_str)
{
    Gtk::Label *label = Gtk::manage(new Gtk::Label(label_str));
    label->set_use_underline();
    Gtk::Image *image = Gtk::manage(new Gtk::Image());
    if (image_str.size() > 0) {
        image->set_from_icon_name(image_str, Gtk::ICON_SIZE_MENU);
    } else {
        image->set_from_icon_name(INKSCAPE_ICON("inkscape-logo"), Gtk::ICON_SIZE_MENU);
    }
    Gtk::Box *tab = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2));
    tab->set_name(label->get_label());
    tab->pack_start(*image);
    tab->pack_end(*label);
    tab->show_all();

    return tab;
}

// Add new dialog (in response to menu)
void DialogContainer::new_dialog(Glib::ustring name)
{
    // Get the verb with that id
    Glib::ustring id = name == "InkscapePreferences" ? "DialogPreferences" : name; // TODO: might be improved
    Inkscape::Verb *verb = Inkscape::Verb::getbyid(id.c_str());

    // Can't understand the dialog's settings without an associated verb
    if (!verb) {
        return;
    }

    // Limit each container to containing one of any type of dialog.
    auto it = dialogs.find(verb->get_code());
    if (it != dialogs.end()) {
        std::cerr << "DialogContainer::new_dialog: Already has a \"" << name << "\" dialog!" << std::endl;

        // Blink notebook with existing dialog to let user know where it is and show page.
        it->second->blink();
        return;
    }

    // Create the dialog widget
    DialogBase *dialog = dialog_factory(name);

    if (!dialog) {
        std::cerr << "DialogContainer::new_dialog(): couldn't find dialog for: " << name << std::endl;
        return;
    }

    // Create the notebook tab
    Gtk::Widget *tab = create_notebook_tab(dialog->get_name(), verb->get_image());

    // Find or create notebook to insert dialog
    DialogNotebook *notebook = nullptr;

    // Check if request came from notebook menu
    Gtk::Window *window = dynamic_cast<Gtk::Window *>(get_toplevel());
    if (window) {
        Gtk::Widget *focus = window->get_focus();
        if (focus) {
            Gtk::Notebook *gtknotebook = dynamic_cast<Gtk::Notebook *>(focus->get_ancestor(GTK_TYPE_NOTEBOOK));
            if (gtknotebook) {
                notebook = dynamic_cast<DialogNotebook *>(gtknotebook->get_parent());
            }
        }
    }

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

void DialogContainer::new_floating_dialog(Glib::ustring name)
{
    // Get the verb with that id
    Glib::ustring id = name == "InkscapePreferences" ? "DialogPreferences" : name; // TODO: might be improved
    Inkscape::Verb *verb = Inkscape::Verb::getbyid(id.c_str());

    // Can't understand the dialog's settings without an associated verb
    if (!verb) {
        return;
    }

    // Create the dialog widget
    DialogBase *dialog = dialog_factory(name);

    if (!dialog) {
        std::cerr << "DialogContainer::new_dialog(): couldn't find dialog for: " << name << std::endl;
        return;
    }

    // Create the notebook tab
    Gtk::Widget *tab = create_notebook_tab(dialog->get_name(), verb->get_image());

    // New temporary noteboook
    DialogNotebook *notebook = Gtk::manage(new DialogNotebook(this));
    notebook->add_page(*dialog, *tab, dialog->get_name());

    notebook->move_tab_callback();
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

/**
 * Get dialog by name - used by specific functionalities in Inkscape where a specific
 * state of a specific dialog is needed. Create the dialog if it doesn't exist.
 */
DialogBase *DialogContainer::get_dialog(Glib::ustring name)
{
    // Get the verb with that id
    Glib::ustring id = name == "InkscapePreferences" ? "DialogPreferences" : name;
    Inkscape::Verb *verb = Inkscape::Verb::getbyid(id.c_str());

    DialogBase *dialog = get_dialog(verb->get_code());

    if (dialog) {
        return dialog;
    }

    new_dialog(name);

    auto found = dialogs.find(verb->get_code());
    if (found != dialogs.end()) {
        return found->second;
    }
    return nullptr;
}

DialogBase *DialogContainer::get_dialog(int verb_code)
{
    auto found = dialogs.find(verb_code);
    if (found != dialogs.end()) {
        return found->second;
    }

    return nullptr;
}

// Add dialog to list.
void DialogContainer::link_dialog(DialogBase *dialog)
{
    dialogs.insert(std::pair<int, DialogBase *>(dialog->getVerb(), dialog));
}

// Remove dialog from list.
void DialogContainer::unlink_dialog(DialogBase *dialog)
{
    // We can't use find here as we want to remove a specific dialog
    // and find can return any dialog with the same type.
    for (auto it = dialogs.begin(); it != dialogs.end(); ++it) {
        if (it->second == dialog) {
            dialogs.erase(it);
            break;
        }
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
    // We want to keep the column alive if it is the only DialogMultipaned at the end.
    // This always gives the user one column to drop dialogs into.

    // If it is not the last column, delete it.
    if (column != columns->get_last_widget()) {
        columns->remove(*column);
        return;
    }

    // OK, it's the last column
    DialogMultipaned *parent = dynamic_cast<DialogMultipaned *>(column->get_parent());
    if (parent && !parent->is_only_final_multipaned(column)) {
        parent->remove(*column);
    }

    // Close the DialogWindow if you're in an empty one
    if (columns->get_first_widget() == columns->get_last_widget()) {
        DialogWindow *window = dynamic_cast<DialogWindow *>(get_toplevel());
        if (window) {
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
