// SPDX-License-Identifier: GPL-2.0-or-later

#include "dialog-manager.h"

#include <gdkmm/monitor.h>
#include <limits>

#include "io/resource.h"

#include "dialog-base.h"
#include "dialog-container.h"
#include "dialog-window.h"
#include "enums.h"
#include "preferences.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

std::optional<window_position_t> dm_get_window_position(Gtk::Window &window)
{
    std::optional<window_position_t> position = std::nullopt;

    const int max = std::numeric_limits<int>::max();
    int x = max;
    int y = max;
    int width = 0;
    int height = 0;
    // gravity NW to include window decorations
    window.property_gravity() = Gdk::GRAVITY_NORTH_WEST;
    window.get_position(x, y);
    window.get_size(width, height);

    if (x != max && y != max && width > 0 && height > 0) {
        position = window_position_t{x, y, width, height};
    }

    return position;
}

void dm_restore_window_position(Gtk::Window &window, const window_position_t &position)
{
    // note: Gtk window methods are recommended over low-level Gdk ones to resize and position window
    window.property_gravity() = Gdk::GRAVITY_NORTH_WEST;
    window.set_default_size(position.width, position.height);
    // move & resize positions window on the screen making sure it is not clipped
    // (meaning it is visible; this works with two monitors too)
    window.move(position.x, position.y);
    window.resize(position.width, position.height);
}

DialogManager &DialogManager::singleton()
{
    static DialogManager dm;
    return dm;
}

// store complete dialog window state (including its container state)
void DialogManager::store_state(DialogWindow &wnd)
{
    // get window's size and position
    if (auto pos = dm_get_window_position(wnd)) {
        if (auto container = wnd.get_container()) {
            // get container's state
            auto state = container->get_container_state(&*pos);

            // find dialogs hosted in this window
            for (auto dlg : *container->get_dialogs()) {
                floating_dialogs[dlg.first] = state;
            }
        }
    }
}

bool DialogManager::should_open_floating(unsigned int code)
{
    return floating_dialogs.count(code) > 0;
}

DialogBase *DialogManager::find_floating_dialog(unsigned int code)
{
    std::vector<Gtk::Window *> windows = InkscapeApplication::instance()->gtk_app()->get_windows();

    for (auto wnd : windows) {
        if (auto dlg_wnd = dynamic_cast<DialogWindow *>(wnd)) {
            if (auto container = dlg_wnd->get_container()) {
                if (auto dlg = container->get_dialog(code)) {
                    return dlg;
                }
            }
        }
    }

    return nullptr;
}

std::shared_ptr<Glib::KeyFile> DialogManager::find_dialog_state(unsigned int code)
{
    auto it = floating_dialogs.find(code);
    if (it != floating_dialogs.end()) {
        return it->second;
    }
    return nullptr;
}

const char dialogs_state[] = "dialogs-state.ini";
const char save_dialog_position[] = "/options/savedialogposition/value";
const char transient_group[] = "transient";

// list of dialogs sharing the same state
std::vector<unsigned int> DialogManager::count_dialogs(const Glib::KeyFile *state) const
{
    std::vector<unsigned int> dialogs;
    for (auto dlg : floating_dialogs) {
        if (dlg.second.get() == state) {
            dialogs.push_back(dlg.first);
        }
    }
    return dialogs;
}

void DialogManager::save_dialogs_state(DialogContainer *docking_container)
{
    if (!docking_container)
        return;

    // check if we want to save the state
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int save_state = prefs->getInt(save_dialog_position, PREFS_DIALOGS_STATE_SAVE);
    if (save_state == PREFS_DIALOGS_STATE_NONE)
        return;

    // save state of docked dialogs and currently open floating ones
    auto keyfile = docking_container->save_container_state();

    // save transient state of floating dialogs that user might have opened interacting with the app
    std::set<Glib::KeyFile *> files;
    for (auto dlg : floating_dialogs) {
        files.insert(dlg.second.get());
    }

    int idx = 1;
    for (auto state : files) {
        auto index = std::to_string(idx++);
        keyfile->set_string(transient_group, "state" + index, state->to_data());
        auto dialogs = count_dialogs(state);
        keyfile->set_integer_list("transient", "dialogs" + index, dialogs);
    }
    keyfile->set_integer(transient_group, "count", files.size());

    std::string filename = Glib::build_filename(Inkscape::IO::Resource::profile_path(), dialogs_state);
    try {
        keyfile->save_to_file(filename);
    } catch (Glib::FileError &error) {
        std::cerr << G_STRFUNC << ": " << error.what() << std::endl;
    }
}

// load transient dialog state - it includes state of floating dialogs that may or may not be open
void DialogManager::load_transient_state(Glib::KeyFile *file)
{
    int count = file->get_integer(transient_group, "count");
    for (int i = 0; i < count; ++i) {
        auto index = std::to_string(i + 1);
        auto dialogs = file->get_integer_list(transient_group, "dialogs" + index);
        auto state = file->get_string(transient_group, "state" + index);

        auto keyfile = std::make_shared<Glib::KeyFile>();
        keyfile->load_from_data(state);
        for (auto code : dialogs) {
            floating_dialogs[code] = keyfile;
        }
    }
}

// restore state of dialogs; populate docking container and open visible floating dialogs
void DialogManager::restore_dialogs_state(DialogContainer *docking_container, bool include_floating)
{
    if (!docking_container)
        return;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int save_state = prefs->getInt(save_dialog_position, PREFS_DIALOGS_STATE_SAVE);
    if (save_state == PREFS_DIALOGS_STATE_NONE)
        return;

    try {
        auto keyfile = std::make_unique<Glib::KeyFile>();
        std::string filename = Glib::build_filename(Inkscape::IO::Resource::profile_path(), dialogs_state);
        if (keyfile->load_from_file(filename)) {
            // restore visible dialogs first; that state is up-to-date
            docking_container->load_container_state(keyfile.get(), include_floating);

            // then load transient data too; it may be older than above
            if (include_floating) {
                try {
                    load_transient_state(keyfile.get());
                } catch (Glib::Error &error) {
                    std::cerr << G_STRFUNC << ": transient state not loaded - " << error.what() << std::endl;
                }
            }
        }
    } catch (Glib::Error &error) {
        std::cerr << G_STRFUNC << ": dialogs state not loaded - " << error.what() << std::endl;
    }
}

void DialogManager::remove_dialog_floating_state(unsigned int code) {
    auto it = floating_dialogs.find(code);
    if (it != floating_dialogs.end()) {
        floating_dialogs.erase(it);
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
