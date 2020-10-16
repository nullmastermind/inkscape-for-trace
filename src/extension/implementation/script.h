// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Code for handling extensions (i.e., scripts)
 *
 * Authors:
 *   Bryce Harrington <bryce@osdl.org>
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_IMPEMENTATION_SCRIPT_H_SEEN
#define INKSCAPE_EXTENSION_IMPEMENTATION_SCRIPT_H_SEEN

#include "implementation.h"
#include <gtkmm/enums.h>
#include <gtkmm/window.h>
#include <glibmm/main.h>
#include <glibmm/spawn.h>
#include <glibmm/fileutils.h>

namespace Inkscape {
namespace XML {
class Node;
} // namespace XML

namespace Extension {
namespace Implementation {

/**
 * Utility class used for loading and launching script extensions
 */
class Script : public Implementation {
public:

    Script();
    ~Script() override;
    bool load(Inkscape::Extension::Extension *module) override;
    void unload(Inkscape::Extension::Extension *module) override;
    bool check(Inkscape::Extension::Extension *module) override;

    ImplementationDocumentCache * newDocCache(Inkscape::Extension::Extension * ext, Inkscape::UI::View::View * view) override;

    Gtk::Widget *prefs_input(Inkscape::Extension::Input *module, gchar const *filename) override;
    SPDocument *open(Inkscape::Extension::Input *module, gchar const *filename) override;
    Gtk::Widget *prefs_output(Inkscape::Extension::Output *module) override;
    void save(Inkscape::Extension::Output *module, SPDocument *doc, gchar const *filename) override;
    void export_raster(Inkscape::Extension::Output *module, std::string const png_file, gchar const *filename) override;
    void effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View *doc, ImplementationDocumentCache * docCache) override;
    bool cancelProcessing () override;

private:
    bool _canceled;
    Glib::Pid _pid;
    Glib::RefPtr<Glib::MainLoop> _main_loop;

    /**
     * The command that has been derived from
     * the configuration file with appropriate directories
     */
    std::list<std::string> command;

     /**
      * This is the extension that will be used
      * as the helper to read in or write out the
      * data
      */
    Glib::ustring helper_extension;

     /**
      * The window which should be considered as "parent window" of the script execution,
      * e.g. when showin warning messages
      *
      * If set to NULL the main window of the currently active document is used.
      */
    Gtk::Window *parent_window;

    void copy_doc(Inkscape::XML::Node * olddoc, Inkscape::XML::Node * newdoc);
    void checkStderr (Glib::ustring const& filename, Gtk::MessageType type, Glib::ustring const& message);

    class file_listener {
        Glib::ustring _string;
        sigc::connection _conn;
        Glib::RefPtr<Glib::IOChannel> _channel;
        Glib::RefPtr<Glib::MainLoop> _main_loop;
        bool _dead;

    public:
        file_listener () : _dead(false) { };
        virtual ~file_listener () {
            _conn.disconnect();
        };

        bool isDead () { return _dead; }
        void init(int fd, Glib::RefPtr<Glib::MainLoop> main);
        bool read(Glib::IOCondition condition);
        Glib::ustring string () { return _string; };
        bool toFile(const Glib::ustring &name);
        bool toFile(const std::string &name);
    };

    int execute (const std::list<std::string> &in_command,
                 const std::list<std::string> &in_params,
                 const Glib::ustring &filein,
                 file_listener &fileout);

    void pump_events();

    /** \brief  A definition of an interpreter, which can be specified
        in the INX file, but we need to know what to call */
    struct interpreter_t {
	std::string              prefstring;   /**< The preferences key that can override the default */
	std::vector<std::string> defaultvals;  /**< The default values to check if the preferences are wrong */
    };
    static const std::map<std::string, interpreter_t> interpreterTab;

    std::string resolveInterpreterExecutable(const Glib::ustring &interpNameArg);

}; // class Script
}  // namespace Implementation
}  // namespace Extension
}  // namespace Inkscape

#endif // INKSCAPE_EXTENSION_IMPEMENTATION_SCRIPT_H_SEEN

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
