// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Document metadata dialog, Gtkmm-style.
 */
/* Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon Phillips <jon@rejon.org>
 *   Ralf Stephan <ralf@ark.in-berlin.de> (Gtkmm)
 *
 * Copyright (C) 2000 - 2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "document-metadata.h"
#include "desktop.h"
#include "rdf.h"
#include "verbs.h"

#include "object/sp-namedview.h"

#include "ui/widget/entity-entry.h"
#include "xml/node-event-vector.h"


namespace Inkscape {
namespace UI {
namespace Dialog {

#define SPACE_SIZE_X 15
#define SPACE_SIZE_Y 15

//===================================================

//---------------------------------------------------

static void on_repr_attr_changed (Inkscape::XML::Node *, gchar const *, gchar const *, gchar const *, bool, gpointer);

static Inkscape::XML::NodeEventVector const _repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    on_repr_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};


DocumentMetadata &
DocumentMetadata::getInstance()
{
    DocumentMetadata &instance = *new DocumentMetadata();
    instance.init();
    return instance;
}


DocumentMetadata::DocumentMetadata()
    : UI::Widget::Panel("/dialogs/documentmetadata", SP_VERB_DIALOG_METADATA)
{
    hide();
    _getContents()->set_spacing (4);
    _getContents()->pack_start(_notebook, true, true);

    _page_metadata1.set_border_width(4);
    _page_metadata2.set_border_width(4);
   
    _page_metadata1.set_column_spacing(2);
    _page_metadata2.set_column_spacing(2);
    _page_metadata1.set_row_spacing(2);
    _page_metadata2.set_row_spacing(2);
    
    _notebook.append_page(_page_metadata1, _("Metadata"));
    _notebook.append_page(_page_metadata2, _("License"));

    signalDocumentReplaced().connect(sigc::mem_fun(*this, &DocumentMetadata::_handleDocumentReplaced));
    signalActivateDesktop().connect(sigc::mem_fun(*this, &DocumentMetadata::_handleActivateDesktop));
    signalDeactiveDesktop().connect(sigc::mem_fun(*this, &DocumentMetadata::_handleDeactivateDesktop));

    build_metadata();
}

void
DocumentMetadata::init()
{
    update();

    Inkscape::XML::Node *repr = getDesktop()->getNamedView()->getRepr();
    repr->addListener (&_repr_events, this);

    show_all_children();
}

DocumentMetadata::~DocumentMetadata()
{
    Inkscape::XML::Node *repr = getDesktop()->getNamedView()->getRepr();
    repr->removeListenerByData (this);

    for (auto & it : _rdflist)
        delete it;
}

// TODO: This duplicates code in document-properties.cpp
void
DocumentMetadata::build_metadata()
{
    using Inkscape::UI::Widget::EntityEntry;

    _page_metadata1.show();

    Gtk::Label *label = Gtk::manage (new Gtk::Label);
    label->set_markup (_("<b>Dublin Core Entities</b>"));
    label->set_halign(Gtk::ALIGN_START);
    label->set_valign(Gtk::ALIGN_CENTER);

    _page_metadata1.attach(*label, 0, 0, 2, 1);

     /* add generic metadata entry areas */
    struct rdf_work_entity_t * entity;
    int row = 1;
    for (entity = rdf_work_entities; entity && entity->name; entity++, row++) {
        if ( entity->editable == RDF_EDIT_GENERIC ) {
            EntityEntry *w = EntityEntry::create (entity, _wr);
            _rdflist.push_back (w);

            w->_label.set_halign(Gtk::ALIGN_START);
            w->_label.set_valign(Gtk::ALIGN_CENTER);
            _page_metadata1.attach(w->_label, 0, row, 1, 1);

            w->_packable->set_hexpand();
            w->_packable->set_valign(Gtk::ALIGN_CENTER);
            _page_metadata1.attach(*w->_packable, 1, row, 1, 1);
        }
    }

    _page_metadata2.show();

    row = 0;
    Gtk::Label *llabel = Gtk::manage (new Gtk::Label);
    llabel->set_markup (_("<b>License</b>"));
    llabel->set_halign(Gtk::ALIGN_START);
    llabel->set_valign(Gtk::ALIGN_CENTER);
    _page_metadata2.attach(*llabel, 0, row, 2, 1);

    /* add license selector pull-down and URI */
    ++row;
    _licensor.init (_wr);

    _licensor.set_hexpand();
    _licensor.set_valign(Gtk::ALIGN_CENTER);
    _page_metadata2.attach(_licensor, 1, row, 1, 1);
}

/**
 * Update dialog widgets from desktop.
 */
void DocumentMetadata::update()
{
    if (_wr.isUpdating()) return;

    _wr.setUpdating (true);
    set_sensitive (true);

    //-----------------------------------------------------------meta pages
    /* update the RDF entities */
    for (auto & it : _rdflist)
        it->update (SP_ACTIVE_DOCUMENT);

    _licensor.update (SP_ACTIVE_DOCUMENT);

    _wr.setUpdating (false);
}

void 
DocumentMetadata::_handleDocumentReplaced(SPDesktop* desktop, SPDocument *)
{
    Inkscape::XML::Node *repr = desktop->getNamedView()->getRepr();
    repr->addListener (&_repr_events, this);
    update();
}

void 
DocumentMetadata::_handleActivateDesktop(SPDesktop *desktop)
{
    Inkscape::XML::Node *repr = desktop->getNamedView()->getRepr();
    repr->addListener(&_repr_events, this);
    update();
}

void
DocumentMetadata::_handleDeactivateDesktop(SPDesktop *desktop)
{
    Inkscape::XML::Node *repr = desktop->getNamedView()->getRepr();
    repr->removeListenerByData(this);
}

//--------------------------------------------------------------------

/**
 * Called when XML node attribute changed; updates dialog widgets.
 */
static void on_repr_attr_changed(Inkscape::XML::Node *, gchar const *, gchar const *, gchar const *, bool, gpointer data)
{
    if (DocumentMetadata *dialog = static_cast<DocumentMetadata *>(data))
	dialog->update();
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
