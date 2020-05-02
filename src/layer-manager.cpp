// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LayerManager - a view of a document's layers, relative
 *                          to a particular desktop
 *
 * Copyright 2006  MenTaLguY  <mental@rydia.net>
 *   Abhishek Sharma
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <set>

#include <sigc++/functors/mem_fun.h>
#include <sigc++/adaptors/hide.h>


#include "desktop.h"
#include "document.h"
#include "gc-finalized.h"
#include "layer-manager.h"
#include "selection.h"

#include "inkgc/gc-managed.h"

#include "object/sp-item-group.h"

#include "xml/node-observer.h"

namespace Inkscape {


using Inkscape::XML::Node;

class LayerManager::LayerWatcher : public Inkscape::XML::NodeObserver {
public:
    LayerWatcher(LayerManager* mgr, SPObject* obj) :
        _mgr(mgr),
        _obj(obj),
        _lockedAttr(g_quark_from_string("sodipodi:insensitive")),
        _labelAttr(g_quark_from_string("inkscape:label"))
    {
        _connection = _obj->connectModified(sigc::mem_fun(*mgr, &LayerManager::_objectModified));
        _obj->getRepr()->addObserver(*this);
    }

    ~LayerWatcher() override
    {
        _connection.disconnect();

        if (_obj) {
            Node *node = _obj->getRepr();
            if (node) {
                node->removeObserver(*this);
            }
        }
    }

    void notifyChildAdded( Node &/*node*/, Node &/*child*/, Node */*prev*/ ) override {}
    void notifyChildRemoved( Node &/*node*/, Node &/*child*/, Node */*prev*/ ) override {}
    void notifyChildOrderChanged( Node &/*node*/, Node &/*child*/, Node */*old_prev*/, Node */*new_prev*/ ) override {}
    void notifyContentChanged( Node &/*node*/, Util::ptr_shared /*old_content*/, Util::ptr_shared /*new_content*/ ) override {}
    void notifyAttributeChanged( Node &/*node*/, GQuark name, Util::ptr_shared /*old_value*/, Util::ptr_shared /*new_value*/ ) override {
        if ( name == _lockedAttr || name == _labelAttr ) {
            if ( _mgr && _obj ) {
                _mgr->_objectModified( _obj, 0 );
            }
        }
    }

    LayerManager* _mgr;
    SPObject* _obj;
    sigc::connection _connection;
    GQuark _lockedAttr;
    GQuark _labelAttr;
};

/*
namespace {

Util::ptr_shared stringify_node(Node const &node);

Util::ptr_shared stringify_obj(SPObject const &obj) {
    gchar *string;

    if (obj.id) {
        string = g_strdup_printf("SPObject(%p)=%s  repr(%p)", &obj, obj.id, obj.repr);
    } else {
        string = g_strdup_printf("SPObject(%p) repr(%p)", &obj, obj.repr);
    }

    Util::ptr_shared result=Util::share_string(string);
    g_free(string);
    return result;

}

typedef Debug::SimpleEvent<Debug::Event::OTHER> DebugLayer;

class DebugLayerNote : public DebugLayer {
public:
    DebugLayerNote(Util::ptr_shared descr)
        : DebugLayer(Util::share_static_string("layer-note"))
    {
        _addProperty("descr", descr);
    }
};

class DebugLayerRebuild : public DebugLayer {
public:
    DebugLayerRebuild()
        : DebugLayer(Util::share_static_string("rebuild-layers"))
    {
    }
};

class DebugLayerObj : public DebugLayer {
public:
    DebugLayerObj(SPObject const& obj, Util::ptr_shared name)
        : DebugLayer(name)
    {
        _addProperty("layer", stringify_obj(obj));
    }
};

class DebugAddLayer : public DebugLayerObj {
public:
    DebugAddLayer(SPObject const &obj)
        : DebugLayerObj(obj, Util::share_static_string("add-layer"))
    {
    }
};


} // end of namespace
*/

LayerManager::LayerManager(SPDesktop *desktop)
: _desktop(desktop), _document(nullptr)
{
    _layer_connection = desktop->connectCurrentLayerChanged( sigc::mem_fun(*this, &LayerManager::_selectedLayerChanged) );

    sigc::bound_mem_functor1<void, Inkscape::LayerManager, SPDocument*> first = sigc::mem_fun(*this, &LayerManager::_setDocument);

    // This next line has problems on gcc 4.0.2
    sigc::slot<void, SPDocument*> base2 = first;

    sigc::slot<void,SPDesktop*,SPDocument*> slot2 = sigc::hide<0>( base2 );
    _document_connection = desktop->connectDocumentReplaced( slot2 );

    _setDocument(desktop->doc());
}

LayerManager::~LayerManager()
{
    _layer_connection.disconnect();
    _document_connection.disconnect();
    _resource_connection.disconnect();
    _document = nullptr;
}

void LayerManager::setCurrentLayer( SPObject* obj )
{
    //g_return_if_fail( _desktop->currentRoot() );
    if ( _desktop->currentRoot() ) {
        _desktop->setCurrentLayer( obj );

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (prefs->getBool("/options/selection/layerdeselect", true)) {
            _desktop->getSelection()->clear();
        }
    }
}

/*
 * Return a unique layer name similar to param label
 * A unique name is made by substituting or appending the label's number suffix with
 * the next unique larger number suffix not already used for any layer name
 */
Glib::ustring LayerManager::getNextLayerName( SPObject* obj, gchar const *label)
{
    Glib::ustring incoming( label ? label : "Layer 1" );
    Glib::ustring result(incoming);
    Glib::ustring base(incoming);
    Glib::ustring split(" ");
    guint startNum = 1;

    gint pos = base.length()-1;
    while (pos >= 0 && g_ascii_isdigit(base[pos])) {
        pos-- ;
    }

    gchar* numpart = g_strdup(base.substr(pos+1).c_str());
    if ( numpart ) {
        gchar* endPtr = nullptr;
        guint64 val = g_ascii_strtoull( numpart, &endPtr, 10);
        if ( ((val > 0) || (endPtr != numpart)) && (val < 65536) ) {
            base.erase( pos+1);
            result = incoming;
            startNum = static_cast<int>(val);
            split = "";
        }
        g_free(numpart);
    }

    std::set<Glib::ustring> currentNames;
    std::vector<SPObject *> layers = _document->getResourceList("layer");
    SPObject *root=_desktop->currentRoot();
    if ( root ) {
        for (auto layer : layers) { 
            if (layer != obj)
                currentNames.insert( layer->label() ? Glib::ustring(layer->label()) : Glib::ustring() );
        }
    }

    // Not sure if we need to cap it, but we'll just be paranoid for the moment
    // Intentionally unsigned
    guint endNum = startNum + 3000;
    for ( guint i = startNum; (i < endNum) && (currentNames.find(result) != currentNames.end()); i++ ) {
        result = Glib::ustring::format(base, split, i);
    }

    return result;
}

void LayerManager::renameLayer( SPObject* obj, gchar const *label, bool uniquify )
{
    Glib::ustring incoming( label ? label : "" );
    Glib::ustring result(incoming);

    if (uniquify) {
        result = getNextLayerName(obj, label);
    }

    obj->setLabel( result.c_str() );
}



void LayerManager::_setDocument(SPDocument *document) {
    if (_document) {
        _resource_connection.disconnect();
    }
    _document = document;
    if (document) {
        _resource_connection = document->connectResourcesChanged("layer", sigc::mem_fun(*this, &LayerManager::_rebuild));
    }
    _rebuild();
}

void LayerManager::_objectModified( SPObject* obj, guint /*flags*/ )
{
    _details_changed_signal.emit( obj );
}

void LayerManager::_rebuild() {
//     Debug::EventTracker<DebugLayerRebuild> tracker1();

    _watchers.clear();

    _clear();

    if (!_document) // http://sourceforge.net/mailarchive/forum.php?thread_name=5747bce9a7ed077c1b4fc9f0f4f8a5e0%40localhost&forum_name=inkscape-devel
        return;

    std::vector<SPObject *> layers = _document->getResourceList("layer");

    SPObject *root=_desktop->currentRoot();
    if ( root ) {
        _addOne(root);

        std::set<SPGroup*> layersToAdd;

        for ( std::vector<SPObject *>::const_iterator iter = layers.begin(); iter != layers.end(); ++iter ) {
            SPObject *layer = *iter;
            bool needsAdd = false;
            std::set<SPGroup*> additional;

            if ( root->isAncestorOf(layer) ) {
                needsAdd = true;
                for ( SPObject* curr = layer; curr && (curr != root) && needsAdd; curr = curr->parent ) {
                    if ( SP_IS_GROUP(curr) ) {
                        SPGroup* group = SP_GROUP(curr);
                        if ( group->layerMode() == SPGroup::LAYER ) {
                            // If we have a layer-group as the one or a parent, ensure it is listed as a valid layer.
                            needsAdd &= ( std::find(layers.begin(),layers.end(),curr) != layers.end() );
							// XML Tree being used here directly while it shouldn't be...
                            if ( (!(group->getRepr())) || (!(group->getRepr()->parent())) ) {
                                needsAdd = false;
                            }
                        } else {
                            // If a non-layer group is a parent of layer groups, then show it also as a layer.
                            // TODO add the magic Inkscape group mode?
							// XML Tree being used directly while it shouldn't be...
                            if ( group->getRepr() && group->getRepr()->parent() ) {
                                additional.insert(group);
                            } else {
                                needsAdd = false;
                            }
                        }
                    }
                }
            }
            if ( needsAdd ) {
                if ( !includes(layer) ) {
                    layersToAdd.insert(SP_GROUP(layer));
                }
                for (auto it : additional) {
                    if ( !includes(it) ) {
                        layersToAdd.insert(it);
                    }
                }
            }
        }

        for (auto layer : layersToAdd) {
            // Filter out objects in the middle of being deleted

            // Such may have been the cause of bug 1339397.
            // See http://sourceforge.net/tracker/index.php?func=detail&aid=1339397&group_id=93438&atid=604306

            SPObject const *higher = layer;
            while ( higher && (higher->parent != root) ) {
                higher = higher->parent;
            }
            Node const* node = higher ? higher->getRepr() : nullptr;
            if ( node && node->parent() ) {
//                 Debug::EventTracker<DebugAddLayer> tracker(*layer);

                _watchers.emplace_back(new LayerWatcher(this, layer));

                _addOne(layer);
            }
        }
    }
}

// Connected to the desktop's CurrentLayerChanged signal
void LayerManager::_selectedLayerChanged(SPObject *layer)
{
    // notify anyone who's listening to this instead of directly to the desktop
    _layer_changed_signal.emit(layer);
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
