// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007-2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/dialog.h>
#include <gtkmm/messagedialog.h>

#include "execution-env.h"
#include "prefdialog/prefdialog.h"
#include "implementation/implementation.h"

#include "selection.h"
#include "effect.h"
#include "document.h"
#include "desktop.h"
#include "inkscape.h"
#include "document-undo.h"
#include "desktop.h"
#include "object/sp-namedview.h"

#include "display/sp-canvas.h"

namespace Inkscape {
namespace Extension {

/** \brief  Create an execution environment that will allow the effect
            to execute independently.
    \param effect  The effect that we should execute
    \param doc     The Document to execute on
    \param docCache  The cache created for that document
    \param show_working  Show the working dialog
    \param show_error    Show the error dialog (not working)

    Grabs the selection of the current document so that it can get
    restored.  Will generate a document cache if one isn't provided.
*/
ExecutionEnv::ExecutionEnv (Effect * effect, Inkscape::UI::View::View * doc, Implementation::ImplementationDocumentCache * docCache, bool show_working, bool show_errors) :
    _state(ExecutionEnv::INIT),
    _visibleDialog(nullptr),
    _mainloop(nullptr),
    _doc(doc),
    _docCache(docCache),
    _effect(effect),
    _show_working(show_working),
    _show_errors(show_errors)
{
    genDocCache();

    return;
}

/** \brief  Destroy an execution environment

    Destroys the dialog if created and the document cache.
*/
ExecutionEnv::~ExecutionEnv () {
    if (_visibleDialog != nullptr) {
        _visibleDialog->hide();
        delete _visibleDialog;
        _visibleDialog = nullptr;
    }
    killDocCache();
    return;
}

/** \brief  Generate a document cache if needed

    If there isn't one we create a new one from the implementation
    from the effect's implementation.
*/
void
ExecutionEnv::genDocCache () {
    if (_docCache == nullptr) {
        // printf("Gen Doc Cache\n");
        _docCache = _effect->get_imp()->newDocCache(_effect, _doc);
    }
    return;
}

/** \brief  Destroy a document cache

    Just delete it.
*/
void
ExecutionEnv::killDocCache () {
    if (_docCache != nullptr) {
        // printf("Killed Doc Cache\n");
        delete _docCache;
        _docCache = nullptr;
    }
    return;
}

/** \brief  Create the working dialog

    Builds the dialog with a message saying that the effect is working.
    And make sure to connect to the cancel.
*/
void
ExecutionEnv::createWorkingDialog () {
    if (_visibleDialog != nullptr) {
        _visibleDialog->hide();
        delete _visibleDialog;
        _visibleDialog = nullptr;
    }

    SPDesktop *desktop = (SPDesktop *)_doc;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(desktop->canvas));
    if (!toplevel || !gtk_widget_is_toplevel (toplevel))
        return;
    Gtk::Window *window = Glib::wrap(GTK_WINDOW(toplevel), false);

    gchar * dlgmessage = g_strdup_printf(_("'%s' working, please wait..."), _(_effect->get_name()));
    _visibleDialog = new Gtk::MessageDialog(*window,
                               dlgmessage,
                               false, // use markup
                               Gtk::MESSAGE_INFO,
                               Gtk::BUTTONS_CANCEL,
                               true); // modal
    _visibleDialog->signal_response().connect(sigc::mem_fun(this, &ExecutionEnv::workingCanceled));
    g_free(dlgmessage);

    Gtk::Dialog *dlg = _effect->get_pref_dialog();
    if (dlg) {
        _visibleDialog->set_transient_for(*dlg);
    } else {
        // ToDo: Do we need to make the window transient for the main window here?
        //       Currently imossible to test because of GUI freezing during save,
        //       see https://bugs.launchpad.net/inkscape/+bug/967416
    }
    _visibleDialog->show_now();

    return;
}

void
ExecutionEnv::workingCanceled( const int /*resp*/) {
    cancel();
    undo();
    return;
}

void
ExecutionEnv::cancel () {
    SPDesktop *desktop = (SPDesktop *)_doc;
    desktop->clearWaitingCursor();
    _effect->get_imp()->cancelProcessing();
    return;
}

void
ExecutionEnv::undo () {
    DocumentUndo::cancel(_doc->doc());
    return;
}

void
ExecutionEnv::commit () {
    DocumentUndo::done(_doc->doc(), SP_VERB_NONE, _(_effect->get_name()));
    Effect::set_last_effect(_effect);
    _effect->get_imp()->commitDocument();
    killDocCache();
    return;
}

void
ExecutionEnv::reselect () {
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if(desktop) {
        Inkscape::Selection *selection = desktop->getSelection();
        if (selection) {
            selection->restoreBackup();
        }
    }
    return;
}

void
ExecutionEnv::run () {
    _state = ExecutionEnv::RUNNING;
    if (_show_working) {
        createWorkingDialog();
    }
    SPDesktop *desktop = (SPDesktop *)_doc;
    Inkscape::Selection *selection = desktop->getSelection();
    selection->setBackup();
    desktop->setWaitingCursor();
    _effect->get_imp()->effect(_effect, _doc, _docCache);
    desktop->clearWaitingCursor();
    _state = ExecutionEnv::COMPLETE;
    selection->restoreBackup();
    // _runComplete.signal();
    return;
}

void
ExecutionEnv::runComplete () {
    _mainloop->quit();
}

bool
ExecutionEnv::wait () {
    if (_state != ExecutionEnv::COMPLETE) {
        if (_mainloop) {
            _mainloop = Glib::MainLoop::create(false);
        }

        sigc::connection conn = _runComplete.connect(sigc::mem_fun(this, &ExecutionEnv::runComplete));
        _mainloop->run();

        conn.disconnect();
    }

    return true;
}



} }  /* namespace Inkscape, Extension */



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
