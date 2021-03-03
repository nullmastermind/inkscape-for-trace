// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * CommandPalette: Class providing Command Palette feature
 *
 * Authors:
 *     Abhay Raj Singh <abhayonlyone@gmail.com>
 *
 * Copyright (C) 2020 Autors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DIALOG_COMMAND_PALETTE_H
#define INKSCAPE_DIALOG_COMMAND_PALETTE_H

#include <giomm/action.h>
#include <giomm/application.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/listboxrow.h>
#include <gtkmm/recentinfo.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchbar.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/viewport.h>
#include <utility>
#include <vector>

#include "inkscape.h"
#include "ui/dialog/align-and-distribute.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

// Enables using switch case
enum class TypeOfVariant
{
    NONE,
    UNKNOWN,
    BOOL,
    INT,
    DOUBLE,
    STRING,
};

enum class CPMode
{
    SEARCH,
    INPUT, // Input arguments
    SHELL,
    HISTORY
};

enum class HistoryType
{
    LPE,
    ACTION,
    OPEN_FILE,
    IMPORT_FILE,
};

struct History
{
    HistoryType history_type;
    std::string data;

    History(HistoryType ht, std::string &&data)
        : history_type(ht)
        , data(data)
    {}
};

class CPHistoryXML
{
public:
    // constructors, asssignment, destructor
    CPHistoryXML();
    ~CPHistoryXML();

    // Handy wrappers for code clearity
    void add_action(const std::string &full_action_name);

    void add_import(const std::string &uri);
    void add_open(const std::string &uri);

    // Remember parameter for action
    void add_action_parameter(const std::string &full_action_name, const std::string &param);

    std::optional<History> get_last_operation();

    // To construct _CPHistory
    std::vector<History> get_operation_history() const;
    // To get parameter history when an action is selected, LIFO stack like so more recent first
    std::vector<std::string> get_action_parameter_history(const std::string &full_action_name) const;

private:
    void save() const;

    void add_operation(const HistoryType history_type, const std::string &data);

    static std::optional<HistoryType> _get_operation_type(Inkscape::XML::Node *operation);

    const std::string _file_path;

    Inkscape::XML::Document *_xml_doc;
    // handy for xml doc child
    Inkscape::XML::Node *_operations;
    Inkscape::XML::Node *_params;
};

class CommandPalette
{
public: // API
    CommandPalette();
    ~CommandPalette() = default;

    CommandPalette(CommandPalette const &) = delete;            // no copy
    CommandPalette &operator=(CommandPalette const &) = delete; // no assignment

    void open();
    void close();
    void toggle();

    Gtk::Box *get_base_widget();

private: // Helpers
    using ActionPtr = Glib::RefPtr<Gio::Action>;
    using ActionPtrName = std::pair<ActionPtr, Glib::ustring>;

    /**
     * Insert actions in _CPSuggestions
     */
    void load_app_actions();
    void load_win_doc_actions();

    void append_recent_file_operation(const Glib::ustring &path, bool is_suggestion, bool is_import = true);
    bool generate_action_operation(const ActionPtrName &action_ptr_name, const bool is_suggestion);

private: // Signal handlers
    void on_search();

    int on_filter_general(Gtk::ListBoxRow *child);
    bool on_filter_full_action_name(Gtk::ListBoxRow *child);
    bool on_filter_recent_file(Gtk::ListBoxRow *child, bool const is_import);

    bool on_key_press_cpfilter_escape(GdkEventKey *evt);
    bool on_key_press_cpfilter_search_mode(GdkEventKey *evt);
    bool on_key_press_cpfilter_input_mode(GdkEventKey *evt, const ActionPtrName &action_ptr_name);
    bool on_key_press_cpfilter_history_mode(GdkEventKey *evt);

    /**
     * when search bar is empty
     */
    void hide_suggestions();

    /**
     * when search bar isn't empty
     */
    void show_suggestions();

    void on_row_activated(Gtk::ListBoxRow *activated_row);
    void on_history_selection_changed(Gtk::ListBoxRow *lb);

    bool operate_recent_file(Glib::ustring const &uri, bool const import);

    void on_action_fullname_clicked(const Glib::ustring &action_fullname);

    /**
     * Implements text matching logic
     */
    bool fuzzy_search(const Glib::ustring &subject, const Glib::ustring &search);
    bool normal_search(const Glib::ustring &subject, const Glib::ustring &search);
    int fuzzy_points(const Glib::ustring &subject, const Glib::ustring &search);
    int on_sort(Gtk::ListBoxRow *row1, Gtk::ListBoxRow *row2);
    void set_mode(CPMode mode);

    /**
     * Color addition in searched character
     */
    void add_color(Gtk::Label *label, const Glib::ustring &search, const Glib::ustring &subject);
    void remove_color(Gtk::Label *label, const Glib::ustring &subject);
    void add_color_description(Gtk::Label *label, const Glib::ustring &search);

    /**
     * Executes Action
     */
    bool ask_action_parameter(const ActionPtrName &action);
    static ActionPtrName get_action_ptr_name(const Glib::ustring &full_action_name);
    bool execute_action(const ActionPtrName &action, const Glib::ustring &value);

    static TypeOfVariant get_action_variant_type(const ActionPtr &action_ptr);

    static std::pair<Gtk::Label *, Gtk::Label *> get_name_desc(Gtk::ListBoxRow *child);
    Gtk::Button *get_full_action_name(Gtk::ListBoxRow *child);

private: // variables
    // Widgets
    Glib::RefPtr<Gtk::Builder> _builder;

    Gtk::Box *_CPBase;
    Gtk::Box *_CPHeader;
    Gtk::Box *_CPListBase;

    Gtk::SearchBar *_CPSearchBar;
    Gtk::SearchEntry *_CPFilter;

    Gtk::ListBox *_CPSuggestions;
    Gtk::ListBox *_CPHistory;

    Gtk::ScrolledWindow *_CPSuggestionsScroll;
    Gtk::ScrolledWindow *_CPHistoryScroll;

    // Data
    const int _max_height_requestable = 360;
    Glib::ustring _search_text;

    // States
    bool _is_open = false;
    bool _win_doc_actions_loaded = false;

    // History
    CPHistoryXML _history_xml;
    /**
     * Remember the mode we are in helps in unecessary signal disconnection and reconnection
     * Used by set_mode()
     */
    CPMode _mode = CPMode::SHELL;
    // Default value other than SEARCH required
    // set_mode() switches between mode hence checks if it already in the target mode.
    // Constructed value is sometimes SEARCH being the first Item for now
    // set_mode() never attaches the on search listener then
    // This initialising value can be any thing ohter than the initial required mode
    // Example currently it's open in search mode

    /**
     * Stores the search connection to deactivate when not needed
     */
    sigc::connection _cpfilter_search_connection;
    /**
     * Stores the key_press connection to deactivate when not needed
     */
    sigc::connection _cpfilter_key_press_connection;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_DIALOG_COMMAND_PALETTE_H

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
