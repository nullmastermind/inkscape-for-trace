// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_DIALOG_MANAGER_H
#define INKSCAPE_UI_DIALOG_MANAGER_H

#include <glibmm/keyfile.h>
#include <gtkmm/window.h>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace Inkscape {
namespace UI {
namespace Dialog {

class DialogWindow;
class DialogBase;
class DialogContainer;

struct window_position_t
{
    int x, y, width, height;
};

// try to read window's geometry
std::optional<window_position_t> dm_get_window_position(Gtk::Window &window);

// restore window's geometry
void dm_restore_window_position(Gtk::Window &window, const window_position_t &position);

class DialogManager
{
public:
    static DialogManager &singleton();

    // store complete dialog window state (including its container state)
    void store_state(DialogWindow &wnd);

    // return true if dialog 'code' should be opened as floating
    bool should_open_floating(unsigned int code);

    // find instance of dialog 'code' in one of currently open floating dialog windows
    DialogBase *find_floating_dialog(unsigned int code);

    // find floating window state hosting dialog 'code', if there was one
    std::shared_ptr<Glib::KeyFile> find_dialog_state(unsigned int code);

    // remove dialog floating state
    void remove_dialog_floating_state(unsigned int code);

    // save configuration of docked and floating dialogs
    void save_dialogs_state(DialogContainer *docking_container);

    // restore state of dialogs
    void restore_dialogs_state(DialogContainer *docking_container, bool include_floating);

private:
    DialogManager() = default;
    ~DialogManager() = default;

    std::vector<unsigned int> count_dialogs(const Glib::KeyFile *state) const;
    void load_transient_state(Glib::KeyFile *keyfile);

    // transient dialog state for floating windows user closes
    std::map<unsigned int, std::shared_ptr<Glib::KeyFile>> floating_dialogs;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif
