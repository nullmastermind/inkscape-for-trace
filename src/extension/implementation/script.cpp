// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Code for handling extensions (i.e. scripts).
 *
 * Authors:
 *   Bryce Harrington <bryce@osdl.org>
 *   Ted Gould <ted@gould.cx>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/main.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <glibmm/miscutils.h>
#include <glibmm/convert.h>
#include <unistd.h>

#include <cerrno>
#include <glib/gstdio.h>


#include "desktop.h"
#include "ui/dialog-events.h"
#include "extension/effect.h"
#include "extension/execution-env.h"
#include "extension/output.h"
#include "extension/input.h"
#include "extension/db.h"
#include "inkscape.h"
#include "io/resource.h"
#include "preferences.h"
#include "script.h"
#include "selection.h"
#include "object/sp-namedview.h"
#include "object/sp-path.h"
#include "extension/system.h"
#include "ui/view/view.h"
#include "xml/node.h"
#include "xml/attribute-record.h"
#include "ui/tools/node-tool.h"
#include "ui/tool/multi-path-manipulator.h"
#include "ui/tool/path-manipulator.h"
#include "ui/tool/control-point-selection.h"


#include "path-prefix.h"

#ifdef _WIN32
#include <windows.h>
#include <sys/stat.h>
#endif

/** This is the command buffer that gets allocated from the stack */
#define BUFSIZE (255)

/* Namespaces */
namespace Inkscape {
namespace Extension {
namespace Implementation {

/** \brief  Make GTK+ events continue to come through a little bit

    This just keeps coming the events through so that we'll make the GUI
    update and look pretty.
*/
void Script::pump_events () {
    while ( Gtk::Main::events_pending() ) {
        Gtk::Main::iteration();
    }
    return;
}


/** \brief  A table of what interpreters to call for a given language

    This table is used to keep track of all the programs to execute a
    given script.  It also tracks the preference to use to overwrite
    the given interpreter to a custom one per user.
*/
Script::interpreter_t const Script::interpreterTab[] = {
#ifdef _WIN32
        {"perl",   "perl-interpreter",   "wperl"   },
        {"python", "python-interpreter", "pythonw" },
#else
        {"perl",   "perl-interpreter",   "perl"   },
        {"python", "python-interpreter", "python" },
#endif
        {"ruby",   "ruby-interpreter",   "ruby"   },
        {"shell",  "shell-interpreter",  "sh"     },
        { nullptr,    nullptr,                  nullptr    }
};



/** \brief Look up an interpreter name, and translate to something that
    is executable
    \param interpNameArg  The name of the interpreter that we're looking
    for, should be an entry in interpreterTab
*/
std::string Script::resolveInterpreterExecutable(const Glib::ustring &interpNameArg)
{
    interpreter_t const *interp = nullptr;
    bool foundInterp = false;
    for (interp =  interpreterTab ; interp->identity ; interp++ ){
        if (interpNameArg == interp->identity) {
            foundInterp = true;
            break;
        }
    }

    // Do we have a supported interpreter type?
    if (!foundInterp) {
        g_critical("Script::resolveInterpreterExecutable(): unknown script interpreter '%s'", interpNameArg.c_str());
        return "";
    }
    std::string interpreter_path = Glib::filename_from_utf8(interp->defaultval);

    // 1.  Check preferences for an override.
    // Note: this must be an absolute path.
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring prefInterp = prefs->getString("/extensions/" + Glib::ustring(interp->prefstring));

    if (!prefInterp.empty()) {
        interpreter_path = Glib::filename_from_utf8(prefInterp);
    }

    // 2. Search the path.
    // Do this on all systems, for consistency.
    // PATH is set up to contain the Python and Perl binary directories
    // on Windows, so no extra code is necessary.
    if (!Glib::path_is_absolute(interpreter_path)) {
        std::string found_path = Glib::find_program_in_path(interpreter_path);
        if (found_path.empty()) { 
            g_critical("Script::resolveInterpreterExecutable(): failed to locate script interpreter '%s'; "
                       "'%s' not found on PATH", interpNameArg.c_str(), interpreter_path.c_str());
        }
        interpreter_path = found_path;
    }
    return interpreter_path;
}

/** \brief     This function creates a script object and sets up the
               variables.
    \return    A script object

   This function just sets the command to NULL.  It should get built
   officially in the load function.  This allows for less allocation
   of memory in the unloaded state.
*/
Script::Script()
    : Implementation()
    , _canceled(false)
    , parent_window(nullptr)
{
}

/**
 *   \brief     Destructor
 */
Script::~Script()
= default;



/**
    \return    A string with the complete string with the relative directory expanded
    \brief     This function takes in a Repr that contains a reldir entry
               and returns that data with the relative directory expanded.
               Mostly it is here so that relative directories all get used
               the same way.
    \param     reprin   The Inkscape::XML::Node with the reldir in it.

    Basically this function looks at an attribute of the Repr, and makes
    a decision based on that.  Currently, it is only working with the
    'extensions' relative directory, but there will be more of them.
    One thing to notice is that this function always returns an allocated
    string.  This means that the caller of this function can always
    free what they are given (and should do it too!).
*/
std::string Script::solve_reldir(Inkscape::XML::Node *reprin) {

    gchar const *s = reprin->attribute("reldir");

    // right now the only recognized relative directory is "extensions"
    if (!s || Glib::ustring(s) != "extensions") {
        Glib::ustring str = reprin->firstChild()->content();
        return str;
    }
    using namespace Inkscape::IO::Resource;
    return get_filename(EXTENSIONS, reprin->firstChild()->content());
}



/**
    \return   Whether the command given exists, including in the path
    \brief    This function is used to find out if something exists for
              the check command.  It can look in the path if required.
    \param    command   The command or file that should be looked for

    The first thing that this function does is check to see if the
    incoming file name has a directory delimiter in it.  This would
    mean that it wants to control the directories, and should be
    used directly.

    If not, the path is used.  Each entry in the path is stepped through,
    attached to the string, and then tested.  If the file is found
    then a TRUE is returned.  If we get all the way through the path
    then a FALSE is returned, the command could not be found.
*/
bool Script::check_existence(const std::string &command)
{
    // Check the simple case first
    if (command.empty()) {
        return false;
    }

    //Don't search when it is an absolute path. */
    if (Glib::path_is_absolute(command)) {
        return Glib::file_test(command, Glib::FILE_TEST_EXISTS);
    }

    // First search in the current directory
    std::string path = G_SEARCHPATH_SEPARATOR_S;
    path.append(";");
    // And then in the PATH environment variable.
    path.append(Glib::getenv("PATH"));

    std::string::size_type pos  = 0;
    std::string::size_type pos2 = 0;
    while ( pos < path.size() ) {

        std::string localPath;

        pos2 = path.find(G_SEARCHPATH_SEPARATOR, pos);
        if (pos2 == path.npos) {
            localPath = path.substr(pos);
            pos = path.size();
        } else {
            localPath = path.substr(pos, pos2-pos);
            pos = pos2+1;
        }

        //printf("### %s\n", localPath.c_str());
        std::string candidatePath =
                      Glib::build_filename(localPath, command);

        if (Glib::file_test(candidatePath,
                      Glib::FILE_TEST_EXISTS)) {
            return true;
        }

    }

    return false;
}





/**
    \return   none
    \brief    This function 'loads' an extension, basically it determines
              the full command for the extension and stores that.
    \param    module  The extension to be loaded.

    The most difficult part about this function is finding the actual
    command through all of the Reprs.  Basically it is hidden down a
    couple of layers, and so the code has to move down too.  When
    the command is actually found, it has its relative directory
    solved.

    At that point all of the loops are exited, and there is an
    if statement to make sure they didn't exit because of not finding
    the command.  If that's the case, the extension doesn't get loaded
    and should error out at a higher level.
*/

bool Script::load(Inkscape::Extension::Extension *module)
{
    if (module->loaded()) {
        return true;
    }

    helper_extension = "";

    /* This should probably check to find the executable... */
    Inkscape::XML::Node *child_repr = module->get_repr()->firstChild();
    while (child_repr != nullptr) {
        if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "script")) {
            for (child_repr = child_repr->firstChild(); child_repr != nullptr; child_repr = child_repr->next()) {
                if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "command")) {
                    const gchar *interpretstr = child_repr->attribute("interpreter");
                    if (interpretstr != nullptr) {
                        std::string interpString = resolveInterpreterExecutable(interpretstr);
                        if (interpString.empty()) {
                            continue; // can't have a script extension with empty interpreter
                        } else {
                            command.push_back(interpString);
                        }
                    }
                    command.push_back(solve_reldir(child_repr));
                }
                if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "helper_extension")) {
                    helper_extension = child_repr->firstChild()->content();
                }
            }

            break;
        }
        child_repr = child_repr->next();
    }

    // TODO: Currently this causes extensions to fail silently, see comment in Extension::set_state()
    g_return_val_if_fail(command.size() > 0, false);

    return true;
}


/**
    \return   None.
    \brief    Unload this puppy!
    \param    module  Extension to be unloaded.

    This function just sets the module to unloaded.  It free's the
    command if it has been allocated.
*/
void Script::unload(Inkscape::Extension::Extension */*module*/)
{
    command.clear();
    helper_extension = "";
}




/**
    \return   Whether the check passed or not
    \brief    Check every dependency that was given to make sure we should keep this extension
    \param    module  The Extension in question

*/
bool Script::check(Inkscape::Extension::Extension *module)
{
    int script_count = 0;
    Inkscape::XML::Node *child_repr = module->get_repr()->firstChild();
    while (child_repr != nullptr) {
        if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "script")) {
            script_count++;
            child_repr = child_repr->firstChild();
            while (child_repr != nullptr) {
                if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "check")) {
                    std::string command_text = solve_reldir(child_repr);
                    if (!command_text.empty()) {
                        /* I've got the command */
                        bool existance = check_existence(command_text);
                        if (!existance) {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }

                if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "helper_extension")) {
                    gchar const *helper = child_repr->firstChild()->content();
                    if (Inkscape::Extension::db.get(helper) == nullptr) {
                        return false;
                    }
                }

                child_repr = child_repr->next();
            }

            break;
        }
        child_repr = child_repr->next();
    }

    if (script_count == 0) {
        return false;
    }

    return true;
}

class ScriptDocCache : public ImplementationDocumentCache {
    friend class Script;
protected:
    std::string _filename;
    int _tempfd;
public:
    ScriptDocCache (Inkscape::UI::View::View * view);
    ~ScriptDocCache ( ) override;
};

ScriptDocCache::ScriptDocCache (Inkscape::UI::View::View * view) :
    ImplementationDocumentCache(view),
    _filename(""),
    _tempfd(0)
{
    try {
        _tempfd = Glib::file_open_tmp(_filename, "ink_ext_XXXXXX.svg");
    } catch (...) {
        /// \todo Popup dialog here
        return;
    }

    SPDesktop *desktop = (SPDesktop *) view;
    sp_namedview_document_from_window(desktop);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/options/svgoutput/disable_optimizations", true);
    Inkscape::Extension::save(
              Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE),
              view->doc(), _filename.c_str(), false, false, false, Inkscape::Extension::FILE_SAVE_METHOD_TEMPORARY);
    prefs->setBool("/options/svgoutput/disable_optimizations", false);
    return;
}

ScriptDocCache::~ScriptDocCache ( )
{
    close(_tempfd);
    unlink(_filename.c_str());
}

ImplementationDocumentCache *Script::newDocCache( Inkscape::Extension::Extension * /*ext*/, Inkscape::UI::View::View * view ) {
    return new ScriptDocCache(view);
}


/**
    \return   A dialog for preferences
    \brief    A stub function right now
    \param    module    Module who's preferences need getting
    \param    filename  Hey, the file you're getting might be important

    This function should really do something, right now it doesn't.
*/
Gtk::Widget *Script::prefs_input(Inkscape::Extension::Input *module,
                    const gchar */*filename*/)
{
    return module->autogui(nullptr, nullptr);
}



/**
    \return   A dialog for preferences
    \brief    A stub function right now
    \param    module    Module whose preferences need getting

    This function should really do something, right now it doesn't.
*/
Gtk::Widget *Script::prefs_output(Inkscape::Extension::Output *module)
{
    return module->autogui(nullptr, nullptr);
}

/**
    \return  A new document that has been opened
    \brief   This function uses a filename that is put in, and calls
             the extension's command to create an SVG file which is
             returned.
    \param   module   Extension to use.
    \param   filename File to open.

    First things first, this function needs a temporary file name.  To
    create one of those the function Glib::file_open_tmp is used with
    the header of ink_ext_.

    The extension is then executed using the 'execute' function
    with the filename assigned and then the temporary filename.  
    After execution the SVG should be in the temporary file.

    Finally, the temporary file is opened using the SVG input module and
    a document is returned.  That document has its filename set to
    the incoming filename (so that it's not the temporary filename).
    That document is then returned from this function.
*/
SPDocument *Script::open(Inkscape::Extension::Input *module,
             const gchar *filenameArg)
{
    std::list<std::string> params;
    module->paramListString(params);

    std::string tempfilename_out;
    int tempfd_out = 0;
    try {
        tempfd_out = Glib::file_open_tmp(tempfilename_out, "ink_ext_XXXXXX.svg");
    } catch (...) {
        /// \todo Popup dialog here
        return nullptr;
    }

    std::string lfilename = Glib::filename_from_utf8(filenameArg);

    file_listener fileout;
    int data_read = execute(command, params, lfilename, fileout);
    fileout.toFile(tempfilename_out);

    SPDocument * mydoc = nullptr;
    if (data_read > 10) {
        if (helper_extension.size()==0) {
            mydoc = Inkscape::Extension::open(
                  Inkscape::Extension::db.get(SP_MODULE_KEY_INPUT_SVG),
                  tempfilename_out.c_str());
        } else {
            mydoc = Inkscape::Extension::open(
                  Inkscape::Extension::db.get(helper_extension.c_str()),
                  tempfilename_out.c_str());
        }
    } // data_read

    if (mydoc != nullptr) {
        mydoc->setDocumentBase(nullptr);
        mydoc->changeUriAndHrefs(filenameArg);
    }

    // make sure we don't leak file descriptors from Glib::file_open_tmp
    close(tempfd_out);

    unlink(tempfilename_out.c_str());

    return mydoc;
} // open



/**
    \return   none
    \brief    This function uses an extension to save a document.  It first
              creates an SVG file of the document, and then runs it through
              the script.
    \param    module    Extension to be used
    \param    doc       Document to be saved
    \param    filename  The name to save the final file as
    \return   false in case of any failure writing the file, otherwise true

    Well, at some point people need to save - it is really what makes
    the entire application useful.  And, it is possible that someone
    would want to use an extension for this, so we need a function to
    do that, eh?

    First things first, the document is saved to a temporary file that
    is an SVG file.  To get the temporary filename Glib::file_open_tmp is used with
    ink_ext_ as a prefix.  Don't worry, this file gets deleted at the
    end of the function.

    After we have the SVG file, then Script::execute is called with
    the temporary file name and the final output filename.  This should
    put the output of the script into the final output file.  We then
    delete the temporary file.
*/
void Script::save(Inkscape::Extension::Output *module,
             SPDocument *doc,
             const gchar *filenameArg)
{
    std::list<std::string> params;
    module->paramListString(params);

    std::string tempfilename_in;
    int tempfd_in = 0;
    try {
        tempfd_in = Glib::file_open_tmp(tempfilename_in, "ink_ext_XXXXXX.svg");
    } catch (...) {
        /// \todo Popup dialog here
        throw Inkscape::Extension::Output::save_failed();
    }

    if (helper_extension.size() == 0) {
        Inkscape::Extension::save(
                   Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE),
                   doc, tempfilename_in.c_str(), false, false, false,
                   Inkscape::Extension::FILE_SAVE_METHOD_TEMPORARY);
    } else {
        Inkscape::Extension::save(
                   Inkscape::Extension::db.get(helper_extension.c_str()),
                   doc, tempfilename_in.c_str(), false, false, false,
                   Inkscape::Extension::FILE_SAVE_METHOD_TEMPORARY);
    }


    file_listener fileout;
    int data_read = execute(command, params, tempfilename_in, fileout);

    bool success = false;

    if (data_read > 0) {
        std::string lfilename = Glib::filename_from_utf8(filenameArg);
        success = fileout.toFile(lfilename);
    }

    // make sure we don't leak file descriptors from Glib::file_open_tmp
    close(tempfd_in);
    // FIXME: convert to utf8 (from "filename encoding") and unlink_utf8name
    unlink(tempfilename_in.c_str());

    if (success == false) {
        throw Inkscape::Extension::Output::save_failed();
    }

    return;
}



/**
    \return    none
    \brief     This function uses an extension as an effect on a document.
    \param     module   Extension to effect with.
    \param     doc      Document to run through the effect.

    This function is a little bit trickier than the previous two.  It
    needs two temporary files to get its work done.  Both of these
    files have random names created for them using the Glib::file_open_temp function
    with the ink_ext_ prefix in the temporary directory.  Like the other
    functions, the temporary files are deleted at the end.

    To save/load the two temporary documents (both are SVG) the internal
    modules for SVG load and save are used.  They are both used through
    the module system function by passing their keys into the functions.

    The command itself is built a little bit differently than in other
    functions because the effect support selections.  So on the command
    line a list of all the ids that are selected is included.  Currently,
    this only works for a single selected object, but there will be more.
    The command string is filled with the data, and then after the execution
    it is freed.

    The execute function is used at the core of this function
    to execute the Script on the two SVG documents (actually only one
    exists at the time, the other is created by that script).  At that
    point both should be full, and the second one is loaded.
*/
void Script::effect(Inkscape::Extension::Effect *module,
               Inkscape::UI::View::View *doc,
               ImplementationDocumentCache * docCache)
{
    if (docCache == nullptr) {
        docCache = newDocCache(module, doc);
    }
    ScriptDocCache * dc = dynamic_cast<ScriptDocCache *>(docCache);
    if (dc == nullptr) {
        printf("TOO BAD TO LIVE!!!");
        exit(1);
    }
    if (doc == nullptr)
    {
        g_warning("Script::effect: View not defined");
        return;
    }

    SPDesktop *desktop = reinterpret_cast<SPDesktop *>(doc);
    sp_namedview_document_from_window(desktop);

    std::list<std::string> params;
    module->paramListString(params);

    parent_window = module->get_execution_env()->get_working_dialog();

    if (module->no_doc) {
        // this is a no-doc extension, e.g. a Help menu command;
        // just run the command without any files, ignoring errors

        Glib::ustring empty;
        file_listener outfile;
        execute(command, params, empty, outfile);

        return;
    }

    std::string tempfilename_out;
    int tempfd_out = 0;
    try {
        tempfd_out = Glib::file_open_tmp(tempfilename_out, "ink_ext_XXXXXX.svg");
    } catch (...) {
        /// \todo Popup dialog here
        return;
    }

    if (desktop) {
        Inkscape::Selection * selection = desktop->getSelection();
        if (selection) {
            params = selection->params;
            module->paramListString(params);
            selection->clear();
        }
    }

    file_listener fileout;
    int data_read = execute(command, params, dc->_filename, fileout);
    fileout.toFile(tempfilename_out);

    pump_events();

    SPDocument * mydoc = nullptr;
    if (data_read > 10) {
        try {
            mydoc = Inkscape::Extension::open(
                  Inkscape::Extension::db.get(SP_MODULE_KEY_INPUT_SVG),
                  tempfilename_out.c_str());
        } catch (const Inkscape::Extension::Input::open_failed &e) {
            g_warning("Extension returned output that could not be parsed: %s", e.what());
            Gtk::MessageDialog warning(
                    _("The output from the extension could not be parsed."),
                    false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
            warning.set_transient_for( parent_window ? *parent_window : *(INKSCAPE.active_desktop()->getToplevel()) );
            warning.run();
        }
    } // data_read

    pump_events();

    // make sure we don't leak file descriptors from Glib::file_open_tmp
    close(tempfd_out);

    g_unlink(tempfilename_out.c_str());

    if (mydoc) {
        SPDocument* vd=doc->doc();
        if (vd != nullptr)
        {
            vd->emitReconstructionStart();
            copy_doc(vd->rroot, mydoc->rroot);
            vd->emitReconstructionFinish();

            // Getting the named view from the document generated by the extension
            SPNamedView *nv = sp_document_namedview(mydoc, nullptr);
            
            //Check if it has a default layer set up
            SPObject *layer = nullptr;
            if ( nv != nullptr)
            {
                if( nv->default_layer_id != 0 ) {
                    SPDocument *document = desktop->doc();
                    //If so, get that layer
                    if (document != nullptr)
                    {
                        layer = document->getObjectById(g_quark_to_string(nv->default_layer_id));
                    }
                }
                desktop->showGrids(nv->grids_visible);
            }
            
            sp_namedview_update_layers_from_document(desktop);
            //If that layer exists,
            if (layer) {
                //set the current layer
                desktop->setCurrentLayer(layer);
            }
        }
        mydoc->release();
    }

    return;
}



/**
    \brief  A function to replace all the elements in an old document
            by those from a new document.
            document and repinserts them into an emptied old document.
    \param  oldroot  The root node of the old (destination) document.
    \param  newroot  The root node of the new (source) document.

    This function first deletes all the root attributes in the old document followed
    by copying all the root attributes from the new document to the old document.

    It then deletes all the elements in the old document by
    making two passes, the first to create a list of the old elements and
    the second to actually delete them. This two pass approach removes issues
    with the list being changed while parsing through it... lots of nasty bugs.

    Then, it copies all the element in the new document into the old document.

    Finally, it copies the attributes in namedview.
*/
void Script::copy_doc (Inkscape::XML::Node * oldroot, Inkscape::XML::Node * newroot)
{
    if ((oldroot == nullptr) ||(newroot == nullptr))
    {
        g_warning("Error on copy_doc: NULL pointer input.");
        return;
    }

    // For copying attributes in root and in namedview
    using Inkscape::Util::List;
    using Inkscape::XML::AttributeRecord;
    std::vector<gchar const *> attribs;

    // Must explicitly copy root attributes. This must be done first since
    // copying grid lines calls "SPGuide::set()" which needs to know the
    // width, height, and viewBox of the root element.

    // Make a list of all attributes of the old root node.
    for (List<AttributeRecord const> iter = oldroot->attributeList(); iter; ++iter) {
        attribs.push_back(g_quark_to_string(iter->key));
    }

    // Delete the attributes of the old root node.
    for (std::vector<gchar const *>::const_iterator it = attribs.begin(); it != attribs.end(); ++it) {
        oldroot->setAttribute(*it, nullptr);
    }

    // Set the new attributes.
    for (List<AttributeRecord const> iter = newroot->attributeList(); iter; ++iter) {
        gchar const *name = g_quark_to_string(iter->key);
        oldroot->setAttribute(name, newroot->attribute(name));
    }

    // Question: Why is the "sodipodi:namedview" special? Treating it as a normal
    // elmement results in crashes.
    // Seems to be a bug:
    // http://inkscape.13.x6.nabble.com/Effect-that-modifies-the-document-properties-tt2822126.html

    std::vector<Inkscape::XML::Node *> delete_list;

    // Make list
    for (Inkscape::XML::Node * child = oldroot->firstChild();
            child != nullptr;
            child = child->next()) {
        if (!strcmp("sodipodi:namedview", child->name())) {
            for (Inkscape::XML::Node * oldroot_namedview_child = child->firstChild();
                    oldroot_namedview_child != nullptr;
                    oldroot_namedview_child = oldroot_namedview_child->next()) {
                delete_list.push_back(oldroot_namedview_child);
            }
            break;
        }
    }

    // Unparent (delete)
    for (auto & i : delete_list) {
        sp_repr_unparent(i);
    }
    attribs.clear();
    oldroot->mergeFrom(newroot, "id", true, true);
}

/**  \brief  This function checks the stderr file, and if it has data,
             shows it in a warning dialog to the user
     \param  filename  Filename of the stderr file
*/
void Script::checkStderr (const Glib::ustring &data,
                           Gtk::MessageType type,
                     const Glib::ustring &message)
{
    Gtk::MessageDialog warning(message, false, type, Gtk::BUTTONS_OK, true);
    warning.set_resizable(true);
    GtkWidget *dlg = GTK_WIDGET(warning.gobj());
    if (parent_window) {
        warning.set_transient_for(*parent_window);
    } else {
        sp_transientize(dlg);
    }

    auto vbox = warning.get_content_area();

    /* Gtk::TextView * textview = new Gtk::TextView(Gtk::TextBuffer::create()); */
    Gtk::TextView * textview = new Gtk::TextView();
    textview->set_editable(false);
    textview->set_wrap_mode(Gtk::WRAP_WORD);
    textview->show();

    textview->get_buffer()->set_text(data.c_str());

    Gtk::ScrolledWindow * scrollwindow = new Gtk::ScrolledWindow();
    scrollwindow->add(*textview);
    scrollwindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrollwindow->set_shadow_type(Gtk::SHADOW_IN);
    scrollwindow->show();
    scrollwindow->set_size_request(0, 60);

    vbox->pack_start(*scrollwindow, true, true, 5 /* fix these */);

    warning.run();

    delete textview;
    delete scrollwindow;

    return;
}

bool Script::cancelProcessing () {
    _canceled = true;
    _main_loop->quit();
    Glib::spawn_close_pid(_pid);

    return true;
}


/** \brief    This is the core of the extension file as it actually does
              the execution of the extension.
    \param    in_command  The command to be executed
    \param    filein      Filename coming in
    \param    fileout     Filename of the out file
    \return   Number of bytes that were read into the output file.

    The first thing that this function does is build the command to be
    executed.  This consists of the first string (in_command) and then
    the filename for input (filein).  This file is put on the command
    line.

    The next thing that this function does is open a pipe to the
    command and get the file handle in the ppipe variable.  It then
    opens the output file with the output file handle.  Both of these
    operations are checked extensively for errors.

    After both are opened, then the data is copied from the output
    of the pipe into the file out using \a fread and \a fwrite.  These two
    functions are used because of their primitive nature - they make
    no assumptions about the data.  A buffer is used in the transfer,
    but the output of \a fread is stored so the exact number of bytes
    is handled gracefully.

    At the very end (after the data has been copied) both of the files
    are closed, and we return to what we were doing.
*/
int Script::execute (const std::list<std::string> &in_command,
                 const std::list<std::string> &in_params,
                 const Glib::ustring &filein,
                 file_listener &fileout)
{
    g_return_val_if_fail(!in_command.empty(), 0);

    std::vector<std::string> argv;

    bool interpreted = (in_command.size() == 2);
    std::string program = in_command.front();
    std::string script = interpreted ? in_command.back() : "";
    std::string working_directory = "";

    // Use Glib::find_program_in_path instead of the equivalent
    // Glib::spawn_* functionality, because _wspawnp is broken on Windows:
    // it doesn't work when PATH contains Unicode directories
    //
    // Note:
    //  - We already have an absolute path here for interpreted scripts, see Script::resolveInterpreterExecutable()
    //  - For "normal" scripts we might not as we only use the rudimentary functionality in Script::solve_reldir().
    //    However we already track dependencies down in a much more reliable way in Dependency::check(), so it might
    //    make sense to eventually store and use that result instead of duplicating partial functionality here.
    if (!Glib::path_is_absolute(program)) {
        std::string found_program = Glib::find_program_in_path(program);
        if (found_program.empty()) {
            g_critical("Script::execute(): failed to locate program '%s' on PATH", program.c_str());
            return 0;
        }
        program = found_program;
    }
    argv.push_back(program);

    if (interpreted) {
        // On Windows, Python garbles Unicode command line parameters
        // in an useless way. This means extensions fail when Inkscape
        // is run from an Unicode directory.
        // As a workaround, we set the working directory to the one
        // containing the script.
        working_directory = Glib::path_get_dirname(script);
        script = Glib::path_get_basename(script);
        #ifdef G_OS_WIN32
        // ANNOYING: glibmm does not wrap g_win32_locale_filename_from_utf8
        gchar *workdir_s = g_win32_locale_filename_from_utf8(working_directory.data());
        working_directory = workdir_s;
        g_free(workdir_s);
        #endif

        argv.push_back(script);
    }

    // assemble the rest of argv
    std::copy(in_params.begin(), in_params.end(), std::back_inserter(argv));
    if (!filein.empty()) {
        if(Glib::path_is_absolute(filein))
            argv.push_back(filein);
        else {
            std::vector<std::string> buildargs;
            buildargs.push_back(Glib::get_current_dir());
            buildargs.push_back(filein);
            argv.push_back(Glib::build_filename(buildargs));
        }
    }

    //for(int i=0;i<argv.size(); ++i){printf("%s ",argv[i].c_str());}printf("\n");

    int stdout_pipe, stderr_pipe;

    try {
        Glib::spawn_async_with_pipes(working_directory, // working directory
                                     argv,  // arg v
                                     static_cast<Glib::SpawnFlags>(0), // no flags
                                     sigc::slot<void>(),
                                     &_pid,          // Pid
                                     nullptr,           // STDIN
                                     &stdout_pipe,   // STDOUT
                                     &stderr_pipe);  // STDERR
    } catch (Glib::Error &e) {
        g_critical("Script::execute(): failed to execute program '%s'.\n\tReason: %s", program.c_str(), e.what().data());
        return 0;
    }

    // Create a new MainContext for the loop so that the original context sources are not run here,
    // this enforces that only the file_listeners should be read in this new MainLoop
    Glib::RefPtr<Glib::MainContext> _main_context = Glib::MainContext::create();
    _main_loop = Glib::MainLoop::create(_main_context, false);

    file_listener fileerr;
    fileout.init(stdout_pipe, _main_loop);
    fileerr.init(stderr_pipe, _main_loop);

    _canceled = false;
    _main_loop->run();

    // Ensure all the data is out of the pipe
    while (!fileout.isDead()) {
        fileout.read(Glib::IO_IN);
    }
    while (!fileerr.isDead()) {
        fileerr.read(Glib::IO_IN);
    }

    if (_canceled) {
        // std::cout << "Script Canceled" << std::endl;
        return 0;
    }

    Glib::ustring stderr_data = fileerr.string();
    if (stderr_data.length() != 0 &&
        INKSCAPE.use_gui()
       ) {
        checkStderr(stderr_data, Gtk::MESSAGE_INFO,
                                 _("Inkscape has received additional data from the script executed.  "
                                   "The script did not return an error, but this may indicate the results will not be as expected."));
    }

    Glib::ustring stdout_data = fileout.string();
    if (stdout_data.length() == 0) {
        return 0;
    }

    // std::cout << "Finishing Execution." << std::endl;
    return stdout_data.length();
}


void Script::file_listener::init(int fd, Glib::RefPtr<Glib::MainLoop> main) {
    _channel = Glib::IOChannel::create_from_fd(fd);
    _channel->set_encoding();
    _conn = main->get_context()->signal_io().connect(sigc::mem_fun(*this, &file_listener::read), _channel, Glib::IO_IN | Glib::IO_HUP | Glib::IO_ERR);
    _main_loop = main;

    return;
}

bool Script::file_listener::read(Glib::IOCondition condition) {
    if (condition != Glib::IO_IN) {
        _main_loop->quit();
        return false;
    }

    Glib::IOStatus status;
    Glib::ustring out;
    status = _channel->read_line(out);
    _string += out;

    if (status != Glib::IO_STATUS_NORMAL) {
        _main_loop->quit();
        _dead = true;
        return false;
    }

    return true;
}

bool Script::file_listener::toFile(const Glib::ustring &name) {
    try {
        Glib::RefPtr<Glib::IOChannel> stdout_file = Glib::IOChannel::create_from_file(name, "w");
        stdout_file->set_encoding();
        stdout_file->write(_string);
    } catch (Glib::FileError &e) {
        return false;
    }
    return true;
}

}  // namespace Implementation
}  // namespace Extension
}  // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
