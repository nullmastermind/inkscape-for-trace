// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>
#include <memory>
#include "document.h"
#include "view.h"
#include "message-stack.h"
#include "message-context.h"
#include "verbs.h"
#include "inkscape.h"

namespace Inkscape {
namespace UI {
namespace View {

static void 
_onResized (double x, double y, View* v)
{
    v->onResized (x,y);
}

static void 
_onRedrawRequested (View* v)
{
    v->onRedrawRequested();
}

static void 
_onStatusMessage (Inkscape::MessageType type, gchar const *message, View* v)
{
    v->onStatusMessage (type, message);
}

static void 
_onDocumentURISet (gchar const* uri, View* v)
{
    v->onDocumentURISet (uri);
}

static void 
_onDocumentResized (double x, double y, View* v)
{
    v->onDocumentResized (x,y);
}

//--------------------------------------------------------------------
View::View()
:  _doc(nullptr)
{
    _message_stack = std::make_shared<Inkscape::MessageStack>();
    _tips_message_context = std::unique_ptr<Inkscape::MessageContext>(new Inkscape::MessageContext(_message_stack));

    _resized_connection = _resized_signal.connect (sigc::bind (sigc::ptr_fun (&_onResized), this));
    _redraw_requested_connection = _redraw_requested_signal.connect (sigc::bind (sigc::ptr_fun (&_onRedrawRequested), this));
    
    _message_changed_connection = _message_stack->connectChanged (sigc::bind (sigc::ptr_fun (&_onStatusMessage), this));
}

View::~View()
{
    _close();
}

void View::_close() {
    _message_changed_connection.disconnect();

    _tips_message_context = nullptr;

    _message_stack = nullptr;

    if (_doc) {
        _document_uri_set_connection.disconnect();
        _document_resized_connection.disconnect();
        if (INKSCAPE.remove_document(_doc)) {
            // this was the last view of this document, so delete it
            // delete _doc;  Delete now handled in Inkscape::Application
        }
        _doc = nullptr;
    }
    
   Inkscape::Verb::delete_all_view (this);
}

void View::emitResized (double width, double height)
{
    _resized_signal.emit (width, height);
}

void View::requestRedraw() 
{
    _redraw_requested_signal.emit();
}

void View::setDocument(SPDocument *doc) {
    g_return_if_fail(doc != nullptr);

    if (_doc) {
        _document_uri_set_connection.disconnect();
        _document_resized_connection.disconnect();
        if (INKSCAPE.remove_document(_doc)) {
            // this was the last view of this document, so delete it
            // delete _doc; Delete now handled in Inkscape::Application
        }
    }

    INKSCAPE.add_document(doc);

    _doc = doc;
    _document_uri_set_connection = 
        _doc->connectURISet(sigc::bind(sigc::ptr_fun(&_onDocumentURISet), this));
    _document_resized_connection = 
        _doc->connectResized(sigc::bind(sigc::ptr_fun(&_onDocumentResized), this));
    _document_uri_set_signal.emit( _doc->getDocumentURI() );
}

}}}

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
