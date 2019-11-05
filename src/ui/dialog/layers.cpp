// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple panel for layers
 *
 * Authors:
 *   Jon A. Cruz
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "layers.h"

#include <gtkmm/icontheme.h>
#include <gtkmm/separatormenuitem.h>
#include <glibmm/main.h>

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "layer-fns.h"
#include "layer-manager.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "helper/action.h"
#include "ui/icon-loader.h"

#include "include/gtkmm_version.h"

#include "object/sp-root.h"

#include "svg/css-ostringstream.h"

#include "ui/contextmenu.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/imagetoggler.h"

//#define DUMP_LAYERS 1

namespace Inkscape {
namespace UI {
namespace Dialog {

LayersPanel& LayersPanel::getInstance()
{
    return *new LayersPanel();
}

enum {
    COL_VISIBLE = 1,
    COL_LOCKED
};

enum {
    BUTTON_NEW = 0,
    BUTTON_RENAME,
    BUTTON_TOP,
    BUTTON_BOTTOM,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_DUPLICATE,
    BUTTON_DELETE,
    BUTTON_SOLO,
    BUTTON_SHOW_ALL,
    BUTTON_HIDE_ALL,
    BUTTON_LOCK_OTHERS,
    BUTTON_LOCK_ALL,
    BUTTON_UNLOCK_ALL,
    DRAGNDROP
};

class LayersPanel::InternalUIBounce
{
public:
    int _actionCode;
    SPObject* _target;
};

void LayersPanel::_styleButton( Gtk::Button& btn, SPDesktop *desktop, unsigned int code, char const* iconName, char const* fallback )
{
    bool set = false;

    if ( iconName ) {
        GtkWidget *child = sp_get_icon_image(iconName, GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_widget_show( child );
        btn.add( *Gtk::manage(Glib::wrap(child)) );
        btn.set_relief(Gtk::RELIEF_NONE);
        set = true;
    }

    if ( desktop ) {
        Verb *verb = Verb::get( code );
        if ( verb ) {
            SPAction *action = verb->get_action(Inkscape::ActionContext(desktop));
            if ( !set && action && action->image ) {
                GtkWidget *child = sp_get_icon_image(action->image, GTK_ICON_SIZE_SMALL_TOOLBAR);
                gtk_widget_show( child );
                btn.add( *Gtk::manage(Glib::wrap(child)) );
                set = true;
            }

            if ( action && action->tip ) {
                btn.set_tooltip_text (action->tip);
            }
        }
    }

    if ( !set && fallback ) {
        btn.set_label( fallback );
    }
}


Gtk::MenuItem& LayersPanel::_addPopupItem( SPDesktop *desktop, unsigned int code, int id )
{
    Verb *verb = Verb::get( code );
    g_assert(verb);
    SPAction *action = verb->get_action(Inkscape::ActionContext(desktop));

    Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem());

    Gtk::Label *label = Gtk::manage(new Gtk::Label(action->name, true));
    label->set_xalign(0.0);

    if (_show_contextmenu_icons && action->image) {
        item->set_name("ImageMenuItem");  // custom name to identify our "ImageMenuItems"
        Gtk::Image *icon = Gtk::manage(sp_get_icon_image(action->image, Gtk::ICON_SIZE_MENU));

        // Create a box to hold icon and label as Gtk::MenuItem derives from GtkBin and can only hold one child
        Gtk::Box *box = Gtk::manage(new Gtk::Box());
        box->pack_start(*icon, false, false, 0);
        box->pack_start(*label, true,  true,  0);
        item->add(*box);
    } else {
        item->add(*label);
    }

    item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &LayersPanel::_takeAction), id));
    _popupMenu.append(*item);

    return *item;
}

void LayersPanel::_fireAction( unsigned int code )
{
    if ( _desktop ) {
        Verb *verb = Verb::get( code );
        if ( verb ) {
            SPAction *action = verb->get_action(Inkscape::ActionContext(_desktop));
            if ( action ) {
                sp_action_perform( action, nullptr );
//             } else {
//                 g_message("no action");
            }
//         } else {
//             g_message("no verb for %u", code);
        }
//     } else {
//         g_message("no active desktop");
    }
}

//     SP_VERB_LAYER_NEXT,
//     SP_VERB_LAYER_PREV,
void LayersPanel::_takeAction( int val )
{
    if ( !_pending ) {
        _pending = new InternalUIBounce();
        _pending->_actionCode = val;
        _pending->_target = _selectedLayer();
        Glib::signal_timeout().connect( sigc::mem_fun(*this, &LayersPanel::_executeAction), 0 );
    }
}

bool LayersPanel::_executeAction()
{
    // Make sure selected layer hasn't changed since the action was triggered
    if ( _pending
         && (
             (_pending->_actionCode == BUTTON_NEW || _pending->_actionCode == DRAGNDROP)
             || !( (_desktop && _desktop->currentLayer())
                   && (_desktop->currentLayer() != _pending->_target)
                 )
             )
        ) {
        int val = _pending->_actionCode;
//        SPObject* target = _pending->_target;

        switch ( val ) {
            case BUTTON_NEW:
            {
                _fireAction( SP_VERB_LAYER_NEW );
            }
            break;
            case BUTTON_RENAME:
            {
                _fireAction( SP_VERB_LAYER_RENAME );
            }
            break;
            case BUTTON_TOP:
            {
                _fireAction( SP_VERB_LAYER_TO_TOP );
            }
            break;
            case BUTTON_BOTTOM:
            {
                _fireAction( SP_VERB_LAYER_TO_BOTTOM );
            }
            break;
            case BUTTON_UP:
            {
                _fireAction( SP_VERB_LAYER_RAISE );
            }
            break;
            case BUTTON_DOWN:
            {
                _fireAction( SP_VERB_LAYER_LOWER );
            }
            break;
            case BUTTON_DUPLICATE:
            {
                _fireAction( SP_VERB_LAYER_DUPLICATE );
            }
            break;
            case BUTTON_DELETE:
            {
                _fireAction( SP_VERB_LAYER_DELETE );
            }
            break;
            case BUTTON_SOLO:
            {
                _fireAction( SP_VERB_LAYER_SOLO );
            }
            break;
            case BUTTON_SHOW_ALL:
            {
                _fireAction( SP_VERB_LAYER_SHOW_ALL );
            }
            break;
            case BUTTON_HIDE_ALL:
            {
                _fireAction( SP_VERB_LAYER_HIDE_ALL );
            }
            break;
            case BUTTON_LOCK_OTHERS:
            {
                _fireAction( SP_VERB_LAYER_LOCK_OTHERS );
            }
            break;
            case BUTTON_LOCK_ALL:
            {
                _fireAction( SP_VERB_LAYER_LOCK_ALL );
            }
            break;
            case BUTTON_UNLOCK_ALL:
            {
                _fireAction( SP_VERB_LAYER_UNLOCK_ALL );
            }
            break;
            case DRAGNDROP:
            {
                _doTreeMove( );
            }
            break;
        }

        delete _pending;
        _pending = nullptr;
    }

    return false;
}

class LayersPanel::ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:

    ModelColumns()
    {
        add(_colObject);
        add(_colVisible);
        add(_colLocked);
        add(_colLabel);
    }
    ~ModelColumns() override = default;

    Gtk::TreeModelColumn<SPObject*> _colObject;
    Gtk::TreeModelColumn<Glib::ustring> _colLabel;
    Gtk::TreeModelColumn<bool> _colVisible;
    Gtk::TreeModelColumn<bool> _colLocked;
};

void LayersPanel::_updateLayer( SPObject *layer ) {
    _store->foreach( sigc::bind<SPObject*>(sigc::mem_fun(*this, &LayersPanel::_checkForUpdated), layer) );
}

bool LayersPanel::_checkForUpdated(const Gtk::TreePath &/*path*/, const Gtk::TreeIter& iter, SPObject* layer)
{
    bool stopGoing = false;
    Gtk::TreeModel::Row row = *iter;
    if ( layer == row[_model->_colObject] )
    {
        /*
         * We get notified of layer update here (from layer->setLabel()) before layer->label() is set
         * with the correct value (sp-object bug?). So use the inkscape:label attribute instead which
         * has the correct value (bug #168351)
         */
        //row[_model->_colLabel] = layer->label() ? layer->label() : layer->defaultLabel();
        gchar const *label = layer->getAttribute("inkscape:label");
        row[_model->_colLabel] = label ? label : layer->defaultLabel();
        row[_model->_colVisible] = SP_IS_ITEM(layer) ? !SP_ITEM(layer)->isHidden() : false;
        row[_model->_colLocked] = SP_IS_ITEM(layer) ? SP_ITEM(layer)->isLocked() : false;

        stopGoing = true;
    }

    return stopGoing;
}

void LayersPanel::_selectLayer( SPObject *layer ) {
    if ( !layer || (_desktop && _desktop->doc() && (layer == _desktop->doc()->getRoot())) ) {
        if ( _tree.get_selection()->count_selected_rows() != 0 ) {
            _tree.get_selection()->unselect_all();
        }
    } else {
        _store->foreach( sigc::bind<SPObject*>(sigc::mem_fun(*this, &LayersPanel::_checkForSelected), layer) );
    }

    _checkTreeSelection();
}

bool LayersPanel::_checkForSelected(const Gtk::TreePath &path, const Gtk::TreeIter& iter, SPObject* layer)
{
    bool stopGoing = false;

    Gtk::TreeModel::Row row = *iter;
    if ( layer == row[_model->_colObject] )
    {
        _tree.expand_to_path( path );

        Glib::RefPtr<Gtk::TreeSelection> select = _tree.get_selection();

        select->select(iter);

        stopGoing = true;
    }

    return stopGoing;
}

void LayersPanel::_layersChanged()
{
//    g_message("_layersChanged()");
    if (_desktop) {
        SPDocument* document = _desktop->doc();
        g_return_if_fail(document != nullptr); // bug #158: Crash on File>Quit
        SPRoot* root = document->getRoot();
        if ( root ) {
            _selectedConnection.block();
            if ( _desktop->layer_manager && _desktop->layer_manager->includes( root ) ) {
                SPObject* target = _desktop->currentLayer();
                _store->clear();

    #if DUMP_LAYERS
                g_message("root:%p  {%s}   [%s]", root, root->id, root->label() );
    #endif // DUMP_LAYERS
                _addLayer( document, root, nullptr, target, 0 );
            }
            _selectedConnection.unblock();
        }
    }
}

void LayersPanel::_addLayer( SPDocument* doc, SPObject* layer, Gtk::TreeModel::Row* parentRow, SPObject* target, int level )
{
    if ( _desktop && _desktop->layer_manager && layer && (level < _maxNestDepth) ) {
        unsigned int counter = _desktop->layer_manager->childCount(layer);
        for ( unsigned int i = 0; i < counter; i++ ) {
            SPObject *child = _desktop->layer_manager->nthChildOf(layer, i);
            if ( child ) {
#if DUMP_LAYERS
                g_message(" %3d    layer:%p  {%s}   [%s]", level, child, child->getId(), child->label() );
#endif // DUMP_LAYERS

                Gtk::TreeModel::iterator iter = parentRow ? _store->prepend(parentRow->children()) : _store->prepend();
                Gtk::TreeModel::Row row = *iter;
                row[_model->_colObject] = child;
                row[_model->_colLabel] = child->defaultLabel();
                row[_model->_colVisible] = SP_IS_ITEM(child) ? !SP_ITEM(child)->isHidden() : false;
                row[_model->_colLocked] = SP_IS_ITEM(child) ? SP_ITEM(child)->isLocked() : false;

                if ( target && child == target ) {
                    _tree.expand_to_path( _store->get_path(iter) );

                    Glib::RefPtr<Gtk::TreeSelection> select = _tree.get_selection();
                    select->select(iter);

                    _checkTreeSelection();
                }

                _addLayer( doc, child, &row, target, level + 1 );
            }
        }
    }
}

SPObject* LayersPanel::_selectedLayer()
{
    SPObject* obj = nullptr;

    Gtk::TreeModel::iterator iter = _tree.get_selection()->get_selected();
    if ( iter ) {
        Gtk::TreeModel::Row row = *iter;
        obj = row[_model->_colObject];
    }

    return obj;
}

void LayersPanel::_pushTreeSelectionToCurrent()
{
    // TODO hunt down the possible API abuse in getting NULL
    if ( _desktop && _desktop->layer_manager && _desktop->currentRoot() ) {
        SPObject* inTree = _selectedLayer();
        if ( inTree ) {
            SPObject* curr = _desktop->currentLayer();
            if ( curr != inTree ) {
                _desktop->layer_manager->setCurrentLayer( inTree );
            }
        } else {
            _desktop->layer_manager->setCurrentLayer( _desktop->doc()->getRoot() );
        }
    }
}

void LayersPanel::_checkTreeSelection()
{
    bool sensitive = false;
    bool sensitiveNonTop = false;
    bool sensitiveNonBottom = false;
    if ( _tree.get_selection()->count_selected_rows() > 0 ) {
        sensitive = true;

        SPObject* inTree = _selectedLayer();
        if ( inTree ) {

            sensitiveNonTop = (Inkscape::next_layer(inTree->parent, inTree) != nullptr);
            sensitiveNonBottom = (Inkscape::previous_layer(inTree->parent, inTree) != nullptr);

        }
    }


    for (auto & it : _watching) {
        it->set_sensitive( sensitive );
    }
    for (auto & it : _watchingNonTop) {
        it->set_sensitive( sensitiveNonTop );
    }
    for (auto & it : _watchingNonBottom) {
        it->set_sensitive( sensitiveNonBottom );
    }
}

void LayersPanel::_preToggle( GdkEvent const *event )
{

    if ( _toggleEvent ) {
        gdk_event_free(_toggleEvent);
        _toggleEvent = nullptr;
    }

    if ( event && (event->type == GDK_BUTTON_PRESS) ) {
        // Make a copy so we can keep it around.
        _toggleEvent = gdk_event_copy(const_cast<GdkEvent*>(event));
    }
}

void LayersPanel::_toggled( Glib::ustring const& str, int targetCol )
{
    g_return_if_fail(_desktop != nullptr);

    Gtk::TreeModel::Children::iterator iter = _tree.get_model()->get_iter(str);
    Gtk::TreeModel::Row row = *iter;

    Glib::ustring tmp = row[_model->_colLabel];

    SPObject* obj = row[_model->_colObject];
    SPItem* item = ( obj && SP_IS_ITEM(obj) ) ? SP_ITEM(obj) : nullptr;
    if ( item ) {
        switch ( targetCol ) {
            case COL_VISIBLE:
            {
                bool newValue = !row[_model->_colVisible];
                row[_model->_colVisible] = newValue;
                item->setHidden( !newValue  );
                item->updateRepr();
                DocumentUndo::done( _desktop->doc() , SP_VERB_DIALOG_LAYERS,
                                    newValue? _("Unhide layer") : _("Hide layer"));
            }
            break;

            case COL_LOCKED:
            {
                bool newValue = !row[_model->_colLocked];
                row[_model->_colLocked] = newValue;
                item->setLocked( newValue );
                item->updateRepr();
                DocumentUndo::done( _desktop->doc() , SP_VERB_DIALOG_LAYERS,
                                    newValue? _("Lock layer") : _("Unlock layer"));
            }
            break;
        }
    }
    Inkscape::SelectionHelper::fixSelection(_desktop);
}

bool LayersPanel::_handleButtonEvent(GdkEventButton* event)
{
    static unsigned doubleclick = 0;

    if ( (event->type == GDK_BUTTON_PRESS) && (event->button == 3) ) {
        // TODO - fix to a better is-popup function
        Gtk::TreeModel::Path path;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        if ( _tree.get_path_at_pos( x, y, path ) ) {
            _checkTreeSelection();

            _popupMenu.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
        }
    }

    if ( (event->type == GDK_BUTTON_PRESS) && (event->button == 1)
            && (event->state & GDK_MOD1_MASK)) {
        // Alt left click on the visible/lock columns - eat this event to keep row selection
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col = nullptr;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if ( _tree.get_path_at_pos( x, y, path, col, x2, y2 ) ) {
            if (col == _tree.get_column(COL_VISIBLE-1) ||
                    col == _tree.get_column(COL_LOCKED-1)) {
                return true;
            }
        }
    }

    // TODO - ImageToggler doesn't seem to handle Shift/Alt clicks - so we deal with them here.
    if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == 1)
            && (event->state & (GDK_SHIFT_MASK | GDK_MOD1_MASK))) {

        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col = nullptr;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if ( _tree.get_path_at_pos( x, y, path, col, x2, y2 ) ) {
            if (event->state & GDK_SHIFT_MASK) {
                // Shift left click on the visible/lock columns toggles "solo" mode
                if (col == _tree.get_column(COL_VISIBLE - 1)) {
                    _takeAction(BUTTON_SOLO);
                } else if (col == _tree.get_column(COL_LOCKED - 1)) {
                    _takeAction(BUTTON_LOCK_OTHERS);
                }
            } else if (event->state & GDK_MOD1_MASK) {
                // Alt+left click on the visible/lock columns toggles "solo" mode and preserves selection
                Gtk::TreeModel::iterator iter = _store->get_iter(path);
                if (_store->iter_is_valid(iter)) {
                    Gtk::TreeModel::Row row = *iter;
                    SPObject *obj = row[_model->_colObject];
                    if (col == _tree.get_column(COL_VISIBLE - 1)) {
                        _desktop->toggleLayerSolo( obj );
                        DocumentUndo::maybeDone(_desktop->doc(), "layer:solo", SP_VERB_LAYER_SOLO, _("Toggle layer solo"));
                    } else if (col == _tree.get_column(COL_LOCKED - 1)) {
                        _desktop->toggleLockOtherLayers( obj );
                        DocumentUndo::maybeDone(_desktop->doc(), "layer:lockothers", SP_VERB_LAYER_LOCK_OTHERS, _("Lock other layers"));
                    }
                }
            }
        }
    }


    if ( (event->type == GDK_2BUTTON_PRESS) && (event->button == 1) ) {
        doubleclick = 1;
    }

    return false;
}

/*
 * Drap and drop within the tree
 * Save the drag source and drop target SPObjects and if its a drag between layers or into (sublayer) a layer
 */
bool LayersPanel::_handleDragDrop(const Glib::RefPtr<Gdk::DragContext>& /*context*/, int x, int y, guint /*time*/)
{
    int cell_x = 0, cell_y = 0;
    Gtk::TreeModel::Path target_path;
    Gtk::TreeView::Column *target_column;
    SPObject *selected = _selectedLayer();

    _dnd_into = false;
    _dnd_target = nullptr;
    _dnd_source = ( selected && SP_IS_ITEM(selected) ) ? SP_ITEM(selected) : nullptr;

    if (_tree.get_path_at_pos (x, y, target_path, target_column, cell_x, cell_y)) {
        // Are we before, inside or after the drop layer
        Gdk::Rectangle rect;
        _tree.get_background_area (target_path, *target_column, rect);
        int cell_height = rect.get_height();
        _dnd_into = (cell_y > (int)(cell_height * 1/3) && cell_y <= (int)(cell_height * 2/3));
        if (cell_y > (int)(cell_height * 2/3)) {
            Gtk::TreeModel::Path next_path = target_path;
            next_path.next();
            if (_store->iter_is_valid(_store->get_iter(next_path))) {
                target_path = next_path;
            } else {
                // Dragging to the "end"
                Gtk::TreeModel::Path up_path = target_path;
                up_path.up();
                if (_store->iter_is_valid(_store->get_iter(up_path))) {
                    // Drop into parent
                    target_path = up_path;
                    _dnd_into = true;
                } else {
                    // Drop into the top level
                    _dnd_target = nullptr;
                }
            }
        }
        Gtk::TreeModel::iterator iter = _store->get_iter(target_path);
        if (_store->iter_is_valid(iter)) {
            Gtk::TreeModel::Row row = *iter;
            SPObject *obj = row[_model->_colObject];
            _dnd_target = ( obj && SP_IS_ITEM(obj) ) ? SP_ITEM(obj) : nullptr;
        }
    }

    _takeAction(DRAGNDROP);

    return false;
}

/*
 * Move a layer in response to a drag & drop action
 */
void LayersPanel::_doTreeMove( )
{
    if (_dnd_source &&  _dnd_source->getRepr() ) {
        if(!_dnd_target){
            _dnd_source->doWriteTransform(_dnd_source->i2doc_affine() * _dnd_source->document->getRoot()->i2doc_affine().inverse());
        }else{
            SPItem* parent = _dnd_into ? _dnd_target : dynamic_cast<SPItem*>(_dnd_target->parent);
            if(parent){
                Geom::Affine move = _dnd_source->i2doc_affine() * parent->i2doc_affine().inverse();
                _dnd_source->doWriteTransform(move);
            }
        }
        _dnd_source->moveTo(_dnd_target, _dnd_into);
        _selectLayer(_dnd_source);
        _dnd_source = nullptr;
        DocumentUndo::done( _desktop->doc() , SP_VERB_NONE,
                                            _("Move layer"));
    }
}


void LayersPanel::_handleEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
    Gtk::TreeModel::iterator iter = _tree.get_model()->get_iter(path);
    Gtk::TreeModel::Row row = *iter;

    _renameLayer(row, new_text);
}

void LayersPanel::_renameLayer(Gtk::TreeModel::Row row, const Glib::ustring& name)
{
    if ( row && _desktop && _desktop->layer_manager) {
        SPObject* obj = row[_model->_colObject];
        if ( obj ) {
            gchar const* oldLabel = obj->label();
            if ( !name.empty() && (!oldLabel || name != oldLabel) ) {
                _desktop->layer_manager->renameLayer( obj, name.c_str(), FALSE );
                DocumentUndo::done( _desktop->doc() , SP_VERB_NONE,
                                                    _("Rename layer"));
            }

        }
    }
}

bool LayersPanel::_rowSelectFunction( Glib::RefPtr<Gtk::TreeModel> const & /*model*/, Gtk::TreeModel::Path const & /*path*/, bool currentlySelected )
{
    bool val = true;
    if ( !currentlySelected && _toggleEvent )
    {
        GdkEvent* event = gtk_get_current_event();
        if ( event ) {
            // (keep these checks separate, so we know when to call gdk_event_free()
            if ( event->type == GDK_BUTTON_PRESS ) {
                GdkEventButton const* target = reinterpret_cast<GdkEventButton const*>(_toggleEvent);
                GdkEventButton const* evtb = reinterpret_cast<GdkEventButton const*>(event);

                if ( (evtb->window == target->window)
                     && (evtb->send_event == target->send_event)
                     && (evtb->time == target->time)
                     && (evtb->state == target->state)
                    )
                {
                    // Ooooh! It's a magic one
                    val = false;
                }
            }
            gdk_event_free(event);
        }
    }
    return val;
}

/**
 * Constructor
 */
LayersPanel::LayersPanel() :
    UI::Widget::Panel("/dialogs/layers", SP_VERB_DIALOG_LAYERS),
    deskTrack(),
    _maxNestDepth(20),
    _desktop(nullptr),
    _model(nullptr),
    _pending(nullptr),
    _toggleEvent(nullptr),
    _compositeSettings(SP_VERB_DIALOG_LAYERS, "layers",
                       UI::Widget::SimpleFilterModifier::ISOLATION |
                       UI::Widget::SimpleFilterModifier::BLEND |
                       UI::Widget::SimpleFilterModifier::OPACITY |
                       UI::Widget::SimpleFilterModifier::BLUR),
    desktopChangeConn()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _maxNestDepth = prefs->getIntLimited("/dialogs/layers/maxDepth", 20, 1, 1000);

    ModelColumns *zoop = new ModelColumns();
    _model = zoop;

    _store = Gtk::TreeStore::create( *zoop );

    _tree.set_model( _store );
    _tree.set_headers_visible(false);
    _tree.set_reorderable(true);
    _tree.enable_model_drag_dest (Gdk::ACTION_MOVE);

    Inkscape::UI::Widget::ImageToggler *eyeRenderer = Gtk::manage( new Inkscape::UI::Widget::ImageToggler(
        INKSCAPE_ICON("object-visible"), INKSCAPE_ICON("object-hidden")) );
    int visibleColNum = _tree.append_column("vis", *eyeRenderer) - 1;
    eyeRenderer->signal_pre_toggle().connect( sigc::mem_fun(*this, &LayersPanel::_preToggle) );
    eyeRenderer->signal_toggled().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_toggled), (int)COL_VISIBLE) );
    eyeRenderer->property_activatable() = true;
    Gtk::TreeViewColumn* col = _tree.get_column(visibleColNum);
    if ( col ) {
        col->add_attribute( eyeRenderer->property_active(), _model->_colVisible );
    }


    Inkscape::UI::Widget::ImageToggler * renderer = Gtk::manage( new Inkscape::UI::Widget::ImageToggler(
        INKSCAPE_ICON("object-locked"), INKSCAPE_ICON("object-unlocked")) );
    int lockedColNum = _tree.append_column("lock", *renderer) - 1;
    renderer->signal_pre_toggle().connect( sigc::mem_fun(*this, &LayersPanel::_preToggle) );
    renderer->signal_toggled().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_toggled), (int)COL_LOCKED) );
    renderer->property_activatable() = true;
    col = _tree.get_column(lockedColNum);
    if ( col ) {
        col->add_attribute( renderer->property_active(), _model->_colLocked );
    }

    _text_renderer = Gtk::manage(new Gtk::CellRendererText());
    int nameColNum = _tree.append_column("Name", *_text_renderer) - 1;
    _name_column = _tree.get_column(nameColNum);
    _name_column->add_attribute(_text_renderer->property_text(), _model->_colLabel);

    _tree.set_expander_column( *_tree.get_column(nameColNum) );
    _tree.set_search_column(nameColNum + 1);
	
    _compositeSettings.setSubject(&_subject);

    _selectedConnection = _tree.get_selection()->signal_changed().connect( sigc::mem_fun(*this, &LayersPanel::_pushTreeSelectionToCurrent) );
    _tree.get_selection()->set_select_function( sigc::mem_fun(*this, &LayersPanel::_rowSelectFunction) );

    _tree.signal_drag_drop().connect( sigc::mem_fun(*this, &LayersPanel::_handleDragDrop), false);

    _text_renderer->property_editable() = true;
    _text_renderer->signal_edited().connect( sigc::mem_fun(*this, &LayersPanel::_handleEdited) );

    _tree.signal_button_press_event().connect( sigc::mem_fun(*this, &LayersPanel::_handleButtonEvent), false );
    _tree.signal_button_release_event().connect( sigc::mem_fun(*this, &LayersPanel::_handleButtonEvent), false );

    _scroller.add( _tree );
    _scroller.set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
    _scroller.set_shadow_type(Gtk::SHADOW_IN);
    Gtk::Requisition sreq;
    Gtk::Requisition sreq_natural;
    _scroller.get_preferred_size(sreq_natural, sreq);
    int minHeight = 70;
    if (sreq.height < minHeight) {
        // Set a min height to see the layers when used with Ubuntu liboverlay-scrollbar
        _scroller.set_size_request(sreq.width, minHeight);
    }

    _watching.push_back( &_compositeSettings );

    _layersPage.pack_start( _scroller, Gtk::PACK_EXPAND_WIDGET );
    _layersPage.pack_end(_compositeSettings, Gtk::PACK_SHRINK);
    _layersPage.pack_end(_buttonsRow, Gtk::PACK_SHRINK);

    _getContents()->pack_start(_layersPage, Gtk::PACK_EXPAND_WIDGET);

    SPDesktop* targetDesktop = getDesktop();

    Gtk::Button* btn = Gtk::manage( new Gtk::Button() );
    _styleButton( *btn, targetDesktop, SP_VERB_LAYER_NEW, INKSCAPE_ICON("list-add"), C_("Layers", "New") );
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_takeAction), (int)BUTTON_NEW) );
    _buttonsSecondary.pack_start(*btn, Gtk::PACK_SHRINK);

    btn = Gtk::manage( new Gtk::Button() );
    _styleButton( *btn, targetDesktop, SP_VERB_LAYER_TO_BOTTOM, INKSCAPE_ICON("go-bottom"), C_("Layers", "Bot") );
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_takeAction), (int)BUTTON_BOTTOM) );
    _watchingNonBottom.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton( *btn, targetDesktop, SP_VERB_LAYER_LOWER, INKSCAPE_ICON("go-down"), C_("Layers", "Dn") );
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_takeAction), (int)BUTTON_DOWN) );
    _watchingNonBottom.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton( *btn, targetDesktop, SP_VERB_LAYER_RAISE, INKSCAPE_ICON("go-up"), C_("Layers", "Up") );
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_takeAction), (int)BUTTON_UP) );
    _watchingNonTop.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);
    
    btn = Gtk::manage( new Gtk::Button() );
    _styleButton( *btn, targetDesktop, SP_VERB_LAYER_TO_TOP, INKSCAPE_ICON("go-top"), C_("Layers", "Top") );
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_takeAction), (int)BUTTON_TOP) );
    _watchingNonTop.push_back( btn );
    _buttonsPrimary.pack_end(*btn, Gtk::PACK_SHRINK);

//     btn = Gtk::manage( new Gtk::Button("Dup") );
//     btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_takeAction), (int)BUTTON_DUPLICATE) );
//     _buttonsRow.add( *btn );

    btn = Gtk::manage( new Gtk::Button() );
    _styleButton( *btn, targetDesktop, SP_VERB_LAYER_DELETE, INKSCAPE_ICON("list-remove"), _("X") );
    btn->signal_clicked().connect( sigc::bind( sigc::mem_fun(*this, &LayersPanel::_takeAction), (int)BUTTON_DELETE) );
    _watching.push_back( btn );
    _buttonsSecondary.pack_start(*btn, Gtk::PACK_SHRINK);
    
    _buttonsRow.pack_start(_buttonsSecondary, Gtk::PACK_EXPAND_WIDGET);
    _buttonsRow.pack_end(_buttonsPrimary, Gtk::PACK_EXPAND_WIDGET);



    // -------------------------------------------------------
    {
        _show_contextmenu_icons = prefs->getBool("/theme/menuIcons_layers", true);

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_NEW, (int)BUTTON_NEW ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_RENAME, (int)BUTTON_RENAME ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_SOLO, (int)BUTTON_SOLO ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_SHOW_ALL, (int)BUTTON_SHOW_ALL ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_HIDE_ALL, (int)BUTTON_HIDE_ALL ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_LOCK_OTHERS, (int)BUTTON_LOCK_OTHERS ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_LOCK_ALL, (int)BUTTON_LOCK_ALL ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_UNLOCK_ALL, (int)BUTTON_UNLOCK_ALL ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        _watchingNonTop.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_RAISE, (int)BUTTON_UP ) );
        _watchingNonBottom.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_LOWER, (int)BUTTON_DOWN ) );

        _popupMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_DUPLICATE, (int)BUTTON_DUPLICATE ) );
        _watching.push_back( &_addPopupItem( targetDesktop, SP_VERB_LAYER_DELETE, (int)BUTTON_DELETE ) );

        _popupMenu.show_all_children();

        // Install CSS to shift icons into the space reserved for toggles (i.e. check and radio items).
        _popupMenu.signal_map().connect(sigc::mem_fun(static_cast<ContextMenu*>(&_popupMenu), &ContextMenu::ShiftIcons));
    }
    // -------------------------------------------------------



    for (auto & it : _watching) {
        it->set_sensitive( false );
    }
    for (auto & it : _watchingNonTop) {
        it->set_sensitive( false );
    }
    for (auto & it : _watchingNonBottom) {
        it->set_sensitive( false );
    }

    setDesktop( targetDesktop );

    show_all_children();

    // restorePanelPrefs();

    // Connect this up last
    desktopChangeConn = deskTrack.connectDesktopChanged( sigc::mem_fun(*this, &LayersPanel::setDesktop) );
    deskTrack.connect(GTK_WIDGET(gobj()));
}

LayersPanel::~LayersPanel()
{
    setDesktop(nullptr);

    _compositeSettings.setSubject(nullptr);

    if ( _model )
    {
        delete _model;
        _model = nullptr;
    }

    if (_pending) {
        delete _pending;
        _pending = nullptr;
    }

    if ( _toggleEvent )
    {
        gdk_event_free( _toggleEvent );
        _toggleEvent = nullptr;
    }

    desktopChangeConn.disconnect();
    deskTrack.disconnect();
}


void LayersPanel::setDesktop( SPDesktop* desktop )
{
    Panel::setDesktop(desktop);

    if ( desktop != _desktop ) {
        _layerChangedConnection.disconnect();
        _layerUpdatedConnection.disconnect();
        _changedConnection.disconnect();
        if ( _desktop ) {
            _desktop = nullptr;
        }

        _desktop = Panel::getDesktop();
        if ( _desktop ) {
            //setLabel( _desktop->doc()->name );

            LayerManager *mgr = _desktop->layer_manager;
            if ( mgr ) {
                _layerChangedConnection = mgr->connectCurrentLayerChanged( sigc::mem_fun(*this, &LayersPanel::_selectLayer) );
                _layerUpdatedConnection = mgr->connectLayerDetailsChanged( sigc::mem_fun(*this, &LayersPanel::_updateLayer) );
                _changedConnection = mgr->connectChanged( sigc::mem_fun(*this, &LayersPanel::_layersChanged) );
            }

            _layersChanged();
        }
    }
    deskTrack.setBase(desktop);
}



} //namespace Dialogs
} //namespace UI
} //namespace Inkscape


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
