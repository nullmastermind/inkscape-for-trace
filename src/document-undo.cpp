// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Undo/Redo stack implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007  MenTaLguY <mental@rydia.net>
 * Copyright (C) 1999-2003 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Using the split document model gives sodipodi a very simple and clean
 * undo implementation. Whenever mutation occurs in the XML tree,
 * SPObject invokes one of the five corresponding handlers of its
 * container document. This writes down a generic description of the
 * given action, and appends it to the recent action list, kept by the
 * document. There will be as many action records as there are mutation
 * events, which are all kept and processed together in the undo
 * stack. Two methods exist to indicate that the given action is completed:
 *
 * \verbatim
   void sp_document_done( SPDocument *document );
   void sp_document_maybe_done( SPDocument *document, const unsigned char *key ) \endverbatim
 *
 * Both move the recent action list into the undo stack and clear the
 * list afterwards.  While the first method does an unconditional push,
 * the second one first checks the key of the most recent stack entry. If
 * the keys are identical, the current action list is appended to the
 * existing stack entry, instead of pushing it onto its own.  This
 * behaviour can be used to collect multi-step actions (like winding the
 * Gtk spinbutton) from the UI into a single undoable step.
 *
 * For controls implemented by Sodipodi itself, implementing undo as a
 * single step is usually done in a more efficient way. Most controls have
 * the abstract model of grab, drag, release, and change user
 * action. During the grab phase, all modifications are done to the
 * SPObject directly - i.e. they do not change XML tree, and thus do not
 * generate undo actions either.  Only at the release phase (normally
 * associated with releasing the mousebutton), changes are written back
 * to the XML tree, thus generating only a single set of undo actions.
 * (Lauris Kaplinski)
 */

#include <string>
#include "xml/repr.h"
#include "inkscape.h"
#include "document-undo.h"
#include "debug/event-tracker.h"
#include "debug/simple-event.h"
#include "debug/timestamp.h"
#include "event.h"


/*
 * Undo & redo
 */

void Inkscape::DocumentUndo::setUndoSensitive(SPDocument *doc, bool sensitive)
{
	g_assert (doc != nullptr);

	if ( sensitive == doc->sensitive )
		return;

	if (sensitive) {
		sp_repr_begin_transaction (doc->rdoc);
	} else {
		doc->partial = sp_repr_coalesce_log (
			doc->partial,
			sp_repr_commit_undoable (doc->rdoc)
		);
	}

	doc->sensitive = sensitive;
}

bool Inkscape::DocumentUndo::getUndoSensitive(SPDocument const *document) {
	g_assert(document != nullptr);

	return document->sensitive;
}

void Inkscape::DocumentUndo::done(SPDocument *doc, const unsigned int event_type, Glib::ustring const &event_description)
{
    maybeDone(doc, nullptr, event_type, event_description);
}

void Inkscape::DocumentUndo::resetKey( SPDocument *doc )
{
    doc->actionkey.clear();
}

namespace {

using Inkscape::Debug::Event;
using Inkscape::Debug::SimpleEvent;
using Inkscape::Debug::timestamp;
using Inkscape::Verb;

typedef SimpleEvent<Event::INTERACTION> InteractionEvent;

class CommitEvent : public InteractionEvent {
public:

    CommitEvent(SPDocument *doc, const gchar *key, const unsigned int type)
    : InteractionEvent("commit")
    {
        _addProperty("timestamp", timestamp());
        _addProperty("document", doc->serial());
        Verb *verb = Verb::get(type);
        if (verb) {
            _addProperty("context", verb->get_id());
        }
        if (key) {
            _addProperty("merge-key", key);
        }
    }
};

}

void Inkscape::DocumentUndo::maybeDone(SPDocument *doc, const gchar *key, const unsigned int event_type,
                                       Glib::ustring const &event_description)
{
	g_assert (doc != nullptr);
        g_assert (doc->sensitive);
        if ( key && !*key ) {
            g_warning("Blank undo key specified.");
        }

        Inkscape::Debug::EventTracker<CommitEvent> tracker(doc, key, event_type);

	doc->collectOrphans();

	doc->ensureUpToDate();

	DocumentUndo::clearRedo(doc);

	Inkscape::XML::Event *log = sp_repr_coalesce_log (doc->partial, sp_repr_commit_undoable (doc->rdoc));
	doc->partial = nullptr;

	if (!log) {
		sp_repr_begin_transaction (doc->rdoc);
		return;
	}

	if (key && !doc->actionkey.empty() && (doc->actionkey == key) && !doc->undo.empty()) {
                (doc->undo.back())->event =
                    sp_repr_coalesce_log ((doc->undo.back())->event, log);
	} else {
                Inkscape::Event *event = new Inkscape::Event(log, event_type, event_description);
                doc->undo.push_back(event);
		doc->history_size++;
		doc->undoStackObservers.notifyUndoCommitEvent(event);
	}

        if ( key ) {
            doc->actionkey = key;
        } else {
            doc->actionkey.clear();
        }

	doc->virgin = FALSE;
        doc->setModifiedSinceSave();

	sp_repr_begin_transaction (doc->rdoc);

  doc->commit_signal.emit();
}

void Inkscape::DocumentUndo::cancel(SPDocument *doc)
{
	g_assert (doc != nullptr);
        g_assert (doc->sensitive);

	sp_repr_rollback (doc->rdoc);

	if (doc->partial) {
		sp_repr_undo_log (doc->partial);
                doc->emitReconstructionFinish();
		sp_repr_free_log (doc->partial);
		doc->partial = nullptr;
	}

	sp_repr_begin_transaction (doc->rdoc);
}

// Member function for friend access to SPDocument privates.
void Inkscape::DocumentUndo::finish_incomplete_transaction(SPDocument &doc) {
	Inkscape::XML::Event *log=sp_repr_commit_undoable(doc.rdoc);
	if (log || doc.partial) {
		g_warning ("Incomplete undo transaction:");
		doc.partial = sp_repr_coalesce_log(doc.partial, log);
		sp_repr_debug_print_log(doc.partial);
                Inkscape::Event *event = new Inkscape::Event(doc.partial);
		doc.undo.push_back(event);
                doc.undoStackObservers.notifyUndoCommitEvent(event);
		doc.partial = nullptr;
	}
}

// Member function for friend access to SPDocument privates.
void Inkscape::DocumentUndo::perform_document_update(SPDocument &doc) {
    sp_repr_begin_transaction(doc.rdoc);
    doc.ensureUpToDate();

    Inkscape::XML::Event *update_log=sp_repr_commit_undoable(doc.rdoc);
    doc.emitReconstructionFinish();

    if (update_log != nullptr) {
        g_warning("Document was modified while being updated after undo operation");
        sp_repr_debug_print_log(update_log);

        //Coalesce the update changes with the last action performed by user
        if (!doc.undo.empty()) {
            Inkscape::Event* undo_stack_top = doc.undo.back();
            undo_stack_top->event = sp_repr_coalesce_log(undo_stack_top->event, update_log);
        } else {
            sp_repr_free_log(update_log);
        }
    }
}

gboolean Inkscape::DocumentUndo::undo(SPDocument *doc)
{
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::SimpleEvent;

    gboolean ret;

    EventTracker<SimpleEvent<Inkscape::Debug::Event::DOCUMENT> > tracker("undo");

    g_assert (doc != nullptr);
    g_assert (doc->sensitive);

    doc->sensitive = FALSE;
        doc->seeking = true;

    doc->actionkey.clear();

    finish_incomplete_transaction(*doc);

    if (! doc->undo.empty()) {
	    Inkscape::Event *log = doc->undo.back();
	    doc->undo.pop_back();
	    sp_repr_undo_log (log->event);
	    perform_document_update(*doc);

	    doc->redo.push_back(log);

                doc->setModifiedSinceSave();
                doc->undoStackObservers.notifyUndoEvent(log);

	    ret = TRUE;
    } else {
	    ret = FALSE;
    }

    sp_repr_begin_transaction (doc->rdoc);

    doc->sensitive = TRUE;
        doc->seeking = false;

    if (ret) INKSCAPE.external_change();

    return ret;
}

gboolean Inkscape::DocumentUndo::redo(SPDocument *doc)
{
	using Inkscape::Debug::EventTracker;
	using Inkscape::Debug::SimpleEvent;

	gboolean ret;

	EventTracker<SimpleEvent<Inkscape::Debug::Event::DOCUMENT> > tracker("redo");

	g_assert (doc != nullptr);
        g_assert (doc->sensitive);

	doc->sensitive = FALSE;
        doc->seeking = true;

	doc->actionkey.clear();

	finish_incomplete_transaction(*doc);

	if (! doc->redo.empty()) {
		Inkscape::Event *log = doc->redo.back();
		doc->redo.pop_back();
		sp_repr_replay_log (log->event);
		doc->undo.push_back(log);

                doc->setModifiedSinceSave();
		doc->undoStackObservers.notifyRedoEvent(log);

		ret = TRUE;
	} else {
		ret = FALSE;
	}

	sp_repr_begin_transaction (doc->rdoc);

	doc->sensitive = TRUE;
        doc->seeking = false;

	if (ret) {
		INKSCAPE.external_change();
                doc->emitReconstructionFinish();
        }

	return ret;
}

void Inkscape::DocumentUndo::clearUndo(SPDocument *doc)
{
    if (! doc->undo.empty())
        doc->undoStackObservers.notifyClearUndoEvent();
    while (! doc->undo.empty()) {
        Inkscape::Event *e = doc->undo.back();
        doc->undo.pop_back();
        delete e;
        doc->history_size--;
    }
}

void Inkscape::DocumentUndo::clearRedo(SPDocument *doc)
{
        if (!doc->redo.empty())
                doc->undoStackObservers.notifyClearRedoEvent();

    while (! doc->redo.empty()) {
        Inkscape::Event *e = doc->redo.back();
        doc->redo.pop_back();
        delete e;
        doc->history_size--;
    }
}

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
