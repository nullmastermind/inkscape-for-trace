#ifndef __INKSCAPE_H__
#define __INKSCAPE_H__

/*
 * Interface to main application
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Liam P. White <inkscapebrony@gmail.com>
 *
 * Copyright (C) 1999-2014 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <map>
#include <list>
#include "selection.h"
#include "color.h"
#include "layer-model.h"
#include <glib.h>
#include <sigc++/signal.h>

class SPDesktop;
class SPDocument;

namespace Inkscape {

struct Application;
namespace UI {
namespace Tools {

class ToolBase;

} // namespace Tools
} // namespace UI

class ActionContext;

namespace XML {
class Node;
struct Document;
} // namespace XML

} // namespace Inkscape

Inkscape::Application * inkscape_ref  (Inkscape::Application * in);
Inkscape::Application * inkscape_unref(Inkscape::Application * in);

#define INKSCAPE inkscape_get_instance()
#define SP_ACTIVE_EVENTCONTEXT (INKSCAPE->active_event_context())
#define SP_ACTIVE_DOCUMENT (INKSCAPE->active_document())
#define SP_ACTIVE_DESKTOP (INKSCAPE->active_desktop())
// \TODO hack
#define inkscape_active_desktop() SP_ACTIVE_DESKTOP

class AppSelectionModel {
    Inkscape::LayerModel _layer_model;
    Inkscape::Selection *_selection;

public:
    AppSelectionModel(SPDocument *doc) {
        _layer_model.setDocument(doc);
        // TODO: is this really how we should manage the lifetime of the selection?
        // I just copied this from the initialization of the Selection in SPDesktop.
        // When and how is it actually released?
        _selection = Inkscape::GC::release(new Inkscape::Selection(&_layer_model, NULL));
    }

    Inkscape::Selection *getSelection() const { return _selection; }
};

namespace Inkscape {

struct Application {
private:
    unsigned refCount;
    gboolean _dialogs_toggle;
    guint _mapalt;
    guint _trackalt;
    gboolean _use_gui; // may want to consider a virtual function
                       // for overriding things like the warning dlg's

public:
    Application();
    ~Application();

    Inkscape::XML::Document *menus;
    std::map<SPDocument *, int> document_set;
    std::map<SPDocument *, AppSelectionModel *> selection_models;
    std::list<SPDesktop *> * desktops;
    gchar *argv0;
    
    // returns the mask of the keyboard modifier to map to Alt, zero if no mapping
    // Needs to be a guint because gdktypes.h does not define a 'no-modifier' value
    guint mapalt() const { return _mapalt; }
    
    // Sets the keyboard modifer to map to Alt. Zero switches off mapping, as does '1', which is the default 
    void mapalt(guint maskvalue);
    
    guint trackalt() const { return _trackalt; }
    void trackalt(guint trackvalue) { _trackalt = trackvalue; }

    gboolean use_gui() const { return _use_gui; }
    void use_gui(gboolean guival) { _use_gui = guival; }

    // signals
    
    // one of selections changed
    sigc::signal<void, Inkscape::Application *, Inkscape::Selection *> signal_selection_changed;
    // one of subselections (text selection, gradient handle, etc) changed
    sigc::signal<void, Inkscape::Application *, SPDesktop *> signal_subselection_changed;
    // one of selections modified
    sigc::signal<void, Inkscape::Application *, Inkscape::Selection *, guint /*flags*/> signal_selection_modified;
    // one of selections set
    sigc::signal<void, Inkscape::Application *, Inkscape::Selection *> signal_selection_set;
    // tool switched
    sigc::signal<void, Inkscape::Application *, Inkscape::UI::Tools::ToolBase * /*eventcontext*/> signal_eventcontext_set;
    // some desktop got focus
    sigc::signal<void, Inkscape::Application *, SPDesktop *> signal_activate_desktop;
    // some desktop lost focus
    sigc::signal<void, Inkscape::Application *, SPDesktop *> signal_deactivate_desktop;
    
    // probably orphaned signals
    sigc::signal<void, Inkscape::Application *, SPDocument *> signal_destroy_document;
    sigc::signal<void, Inkscape::Application *, SPColor *, double /*opacity*/> signal_color_set;
    
    // inkscape is quitting
    sigc::signal<void, Inkscape::Application *> signal_shut_down;
    // user pressed F12
    sigc::signal<void, Inkscape::Application *> signal_dialogs_hide;
    // user pressed F12
    sigc::signal<void, Inkscape::Application *> signal_dialogs_unhide;
    // a document was changed by some external means (undo or XML editor); this
    // may not be reflected by a selection change and thus needs a separate signal
    sigc::signal<void, Inkscape::Application *> signal_external_change;

    // useful functions
    void autosave_init();
    void application_init (const gchar *argv0, gboolean use_gui);
    void load_config (const gchar *filename, Inkscape::XML::Document *config, const gchar *skeleton, 
                      unsigned int skel_size, const gchar *e_notreg, const gchar *e_notxml, 
                      const gchar *e_notsp, const gchar *warn);

    bool load_menus();
    bool save_menus();
    Inkscape::XML::Node * get_menus();
    
    //static Inkscape::Application* get_instance();
    
    Inkscape::UI::Tools::ToolBase * active_event_context();
    SPDocument * active_document();
    SPDesktop * active_desktop();
    
    // Use this function to get selection model etc for a document
    Inkscape::ActionContext action_context_for_document(SPDocument *doc);
    Inkscape::ActionContext active_action_context();
    
    bool sole_desktop_for_document(SPDesktop const &desktop);
    
    // Inkscape desktop stuff
    void add_desktop(SPDesktop * desktop);
    void remove_desktop(SPDesktop* desktop);
    void activate_desktop (SPDesktop * desktop);
    void switch_desktops_next ();
    void switch_desktops_prev ();
    void get_all_desktops (std::list< SPDesktop* >& listbuf);
    void reactivate_desktop (SPDesktop * desktop);
    SPDesktop * find_desktop_by_dkey (unsigned int dkey);
    unsigned int maximum_dkey();
    SPDesktop * next_desktop ();
    SPDesktop * prev_desktop ();
    
    void dialogs_hide ();
    void dialogs_unhide ();
    void dialogs_toggle ();
    
    void external_change ();
    void selection_modified (Inkscape::Selection *selection, guint flags);
    void selection_changed (Inkscape::Selection * selection);
    void subselection_changed (SPDesktop *desktop);
    void selection_set (Inkscape::Selection * selection);
    
    void eventcontext_set (Inkscape::UI::Tools::ToolBase * eventcontext);
    
    // Moved document add/remove functions into public inkscape.h as they are used
    // (rightly or wrongly) by console-mode functions
    void add_document (SPDocument *document);
    bool remove_document (SPDocument *document);
    
    gchar *homedir_path(const char *filename);
    gchar *profile_path(const char *filename);
    
    // fixme: This has to be rethought
    void refresh_display ();
    
    // fixme: This also
    void exit ();
    
    int autosave();

    friend Application * ::inkscape_ref  (Application * in);
    friend Application * ::inkscape_unref(Application * in);
};

} // namespace Inkscape

bool inkscapeIsCrashing();

// gets the current instance and calls autosave()
int inkscape_autosave(gpointer unused);

// hmm, I wonder what this does /s
Inkscape::Application* inkscape_get_instance();

// only temporary until I can properly clean this up
void inkscape_application_init (const gchar *argv0, gboolean use_gui);

#endif

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
