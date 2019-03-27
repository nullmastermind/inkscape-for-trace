// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Per-desktop selection container
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Andrius R. <knutux@gmail.com>
 *   Abhishek Sharma
 *   Adrian Boguszewski
 *
 * Copyright (C) 2016 Adrian Boguszewski
 * Copyright (C) 2006 Andrius R.
 * Copyright (C) 2004-2005 MenTaLguY
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#endif

#include "inkscape.h"
#include "preferences.h"
#include "desktop.h"
#include "document.h"
#include "ui/tools/node-tool.h"
#include "ui/tool/multi-path-manipulator.h"
#include "ui/tool/path-manipulator.h"
#include "ui/tool/control-point-selection.h"
#include "object/sp-path.h"
#include "object/sp-defs.h"
#include "object/sp-shape.h"
#include "xml/repr.h"

#define SP_SELECTION_UPDATE_PRIORITY (G_PRIORITY_HIGH_IDLE + 1)

namespace Inkscape {

Selection::Selection(LayerModel *layers, SPDesktop *desktop):
    ObjectSet(desktop),
    _layers(layers),
    _selection_context(nullptr),
    _flags(0),
    _idle(0)
{
}

Selection::~Selection() {
    _layers = nullptr;
    if (_idle) {
        g_source_remove(_idle);
        _idle = 0;
    }
}

/* Handler for selected objects "modified" signal */

void Selection::_schedule_modified(SPObject */*obj*/, guint flags) {
    if (!this->_idle) {
        /* Request handling to be run in _idle loop */
        this->_idle = g_idle_add_full(SP_SELECTION_UPDATE_PRIORITY, GSourceFunc(&Selection::_emit_modified), this, nullptr);
    }

    /* Collect all flags */
    this->_flags |= flags;
}

gboolean Selection::_emit_modified(Selection *selection)
{
    /* force new handler to be created if requested before we return */
    selection->_idle = 0;
    guint flags = selection->_flags;
    selection->_flags = 0;

    selection->_emitModified(flags);

    /* drop this handler */
    return FALSE;
}

void Selection::_emitModified(guint flags) {
    INKSCAPE.selection_modified(this, flags);
    _modified_signal.emit(this, flags);
}

void Selection::_emitChanged(bool persist_selection_context/* = false */) {
    if (persist_selection_context) {
        if (nullptr == _selection_context) {
            _selection_context = _layers->currentLayer();
            sp_object_ref(_selection_context, nullptr);
            _context_release_connection = _selection_context->connectRelease(sigc::mem_fun(*this, &Selection::_releaseContext));
        }
    } else {
        _releaseContext(_selection_context);
    }

    INKSCAPE.selection_changed(this);
    _changed_signal.emit(this);
}

void Selection::_releaseContext(SPObject *obj)
{
    if (nullptr == _selection_context || _selection_context != obj)
        return;

    _context_release_connection.disconnect();

    sp_object_unref(_selection_context, nullptr);
    _selection_context = nullptr;
}

SPObject *Selection::activeContext() {
    if (nullptr != _selection_context)
        return _selection_context;
    return _layers->currentLayer();
}

std::vector<Inkscape::SnapCandidatePoint> Selection::getSnapPoints(SnapPreferences const *snapprefs) const {
    std::vector<Inkscape::SnapCandidatePoint> p;

    if (snapprefs != nullptr){
        SnapPreferences snapprefs_dummy = *snapprefs; // create a local copy of the snapping prefs
        snapprefs_dummy.setTargetSnappable(Inkscape::SNAPTARGET_ROTATION_CENTER, false); // locally disable snapping to the item center
        auto items = const_cast<Selection *>(this)->items();
        for (auto iter = items.begin(); iter != items.end(); ++iter) {
            SPItem *this_item = *iter;
            this_item->getSnappoints(p, &snapprefs_dummy);

            //Include the transformation origin for snapping
            //For a selection or group only the overall center is considered, not for each item individually
            if (snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_ROTATION_CENTER)) {
                p.emplace_back(this_item->getCenter(), SNAPSOURCE_ROTATION_CENTER);
            }
        }
    }

    return p;
}

SPObject *Selection::_objectForXMLNode(Inkscape::XML::Node *repr) const {
    g_return_val_if_fail(repr != nullptr, NULL);
    gchar const *id = repr->attribute("id");
    g_return_val_if_fail(id != nullptr, NULL);
    SPObject *object=_layers->getDocument()->getObjectById(id);
    g_return_val_if_fail(object != nullptr, NULL);
    return object;
}

size_t Selection::numberOfLayers() {
    auto items = this->items();
    std::set<SPObject*> layers;
    for (auto iter = items.begin(); iter != items.end(); ++iter) {
        SPObject *layer = _layers->layerForObject(*iter);
        layers.insert(layer);
    }

    return layers.size();
}

size_t Selection::numberOfParents() {
    auto items = this->items();
    std::set<SPObject*> parents;
    for (auto iter = items.begin(); iter != items.end(); ++iter) {
        SPObject *parent = (*iter)->parent;
        parents.insert(parent);
    }
    return parents.size();
}

void Selection::_emitSignals() {
    _emitChanged();
}

void Selection::_connectSignals(SPObject *object) {
    _modified_connections[object] = object->connectModified(sigc::mem_fun(*this, &Selection::_schedule_modified));
}

void Selection::_releaseSignals(SPObject *object) {
    _modified_connections[object].disconnect();
    _modified_connections.erase(object);
}

void
Selection::emptyBackup(){
    _selected_ids.clear();
    _seldata.clear();
    params.clear();
}

void
Selection::setBackup ()
{
    SPDesktop *desktop = this->desktop();
    SPDocument *document = SP_ACTIVE_DOCUMENT;
    Inkscape::UI::Tools::NodeTool *tool = nullptr;
    if (desktop) {
        Inkscape::UI::Tools::ToolBase *ec = desktop->event_context;
        if (INK_IS_NODE_TOOL(ec)) {
            tool = static_cast<Inkscape::UI::Tools::NodeTool*>(ec);
        }
    }
    _selected_ids.clear();
    _seldata.clear();
    params.clear();
    auto items = const_cast<Selection *>(this)->items();
    for (auto iter = items.begin(); iter != items.end(); ++iter) {
        SPItem *item = *iter;
        std::string selected_id;
        selected_id += "--id=";
        selected_id += item->getId();
        params.push_back(selected_id);
        _selected_ids.emplace_back(item->getId());
    }
    if(tool){
        Inkscape::UI::ControlPointSelection *cps = tool->_selected_nodes;
        std::list<Inkscape::UI::SelectableControlPoint *> points_list = cps->_points_list;
        for (auto & i : points_list) {
            Inkscape::UI::Node *node = dynamic_cast<Inkscape::UI::Node*>(i);
            if (node) {
                std::string id = node->nodeList().subpathList().pm().item()->getId();

                int sp = 0;
                bool found_sp = false;
                for(Inkscape::UI::SubpathList::iterator i = node->nodeList().subpathList().begin(); i != node->nodeList().subpathList().end(); ++i,++sp){
                    if(&**i == &(node->nodeList())){
                        found_sp = true;
                        break;
                    }
                }
                int nl=0;
                bool found_nl = false;
                for (Inkscape::UI::NodeList::iterator j = node->nodeList().begin(); j != node->nodeList().end(); ++j, ++nl){
                    if(&*j==node){
                        found_nl = true;
                        break;
                    }
                }
                std::ostringstream ss;
                ss<< "--selected-nodes=" << id << ":" << sp << ":" << nl;
                Glib::ustring selected_nodes = ss.str();

                if(found_nl && found_sp) {
                    _seldata.emplace_back(id,std::make_pair(sp,nl));
                    params.push_back(selected_nodes);
                } else {
                    g_warning("Something went wrong while trying to pass selected nodes to extension. Please report a bug.");
                }
            }
        }
    }//end add selected nodes
}

void
Selection::restoreBackup()
{
    SPDesktop *desktop = this->desktop();
    SPDocument *document = SP_ACTIVE_DOCUMENT;
    Inkscape::UI::Tools::NodeTool *tool = nullptr;
    if (desktop) {
        Inkscape::UI::Tools::ToolBase *ec = desktop->event_context;
        if (INK_IS_NODE_TOOL(ec)) {
            tool = static_cast<Inkscape::UI::Tools::NodeTool*>(ec);
        }
    }
    clear();
    std::vector<std::string>::iterator it = _selected_ids.begin();
    for (; it!= _selected_ids.end(); ++it){
        SPItem * item = dynamic_cast<SPItem *>(document->getObjectById(it->c_str()));
        SPDefs * defs = document->getDefs();
        if (item && !defs->isAncestorOf(item)) {
            add(item);
        }
    }
    if (tool) {
        Inkscape::UI::ControlPointSelection *cps = tool->_selected_nodes;
        cps->selectAll();
        std::list<Inkscape::UI::SelectableControlPoint *> points_list = cps->_points_list;
        cps->clear();
        Inkscape::UI::Node * node = dynamic_cast<Inkscape::UI::Node*>(*points_list.begin());
        if (node) {
            Inkscape::UI::SubpathList sp = node->nodeList().subpathList();
            for (auto & l : _seldata) {
                SPPath * path = dynamic_cast<SPPath *>(document->getObjectById(l.first));
                gint sp_count = 0;
                for (Inkscape::UI::SubpathList::iterator j = sp.begin(); j != sp.end(); ++j, ++sp_count) {
                    if(sp_count == l.second.first) {
                        gint nt_count = 0;
                        for (Inkscape::UI::NodeList::iterator k = (*j)->begin(); k != (*j)->end(); ++k, ++nt_count) {
                            if(nt_count == l.second.second) {
                                cps->insert(k.ptr());
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
        points_list.clear();
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
