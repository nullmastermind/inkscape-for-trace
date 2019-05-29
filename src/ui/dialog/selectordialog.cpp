// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for CSS selectors
 */
/* Authors:
 *   Kamalpreet Kaur Grewal
 *   Tavmjong Bah
 *
 * Copyright (C) Kamalpreet Kaur Grewal 2016 <grewalkamal005@gmail.com>
 * Copyright (C) Tavmjong Bah 2017 <tavmjong@free.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "selectordialog.h"
#include "verbs.h"
#include "selection.h"
#include "attribute-rel-svg.h"
#include "inkscape.h"
#include "document-undo.h"

#include "ui/icon-loader.h"
#include "ui/widget/iconrenderer.h"

#include "xml/attribute-record.h"
#include "xml/node-observer.h"

#include <glibmm/i18n.h>
#include <glibmm/regex.h>

#include <map>
#include <utility>

//#define DEBUG_SELECTORDIALOG
//#define G_LOG_DOMAIN "SELECTORDIALOG"

using Inkscape::DocumentUndo;
using Inkscape::Util::List;
using Inkscape::XML::AttributeRecord;

/**
 * This macro is used to remove spaces around selectors or any strings when
 * parsing is done to update XML style element or row labels in this dialog.
 */
#define REMOVE_SPACES(x) x.erase(0, x.find_first_not_of(' ')); \
    x.erase(x.find_last_not_of(' ') + 1);

namespace Inkscape {
namespace UI {
namespace Dialog {

// Keeps a watch on style element
class SelectorDialog::NodeObserver : public Inkscape::XML::NodeObserver {
public:
  NodeObserver(SelectorDialog *selectordialog)
      : _selectordialog(selectordialog)
  {
      g_debug("SelectorDialog::NodeObserver: Constructor");
    };

    void notifyContentChanged(Inkscape::XML::Node &node,
                                      Inkscape::Util::ptr_shared old_content,
                                      Inkscape::Util::ptr_shared new_content) override;

    SelectorDialog *_selectordialog;
};


void
SelectorDialog::NodeObserver::notifyContentChanged(
    Inkscape::XML::Node &/*node*/,
    Inkscape::Util::ptr_shared /*old_content*/,
    Inkscape::Util::ptr_shared /*new_content*/ ) {

    g_debug("SelectorDialog::NodeObserver::notifyContentChanged");
    _selectordialog->_updating = false;
    _selectordialog->_readStyleElement();
    _selectordialog->_selectRow();
}


// Keeps a watch for new/removed/changed nodes
// (Must update objects that selectors match.)
class SelectorDialog::NodeWatcher : public Inkscape::XML::NodeObserver {
public:
  NodeWatcher(SelectorDialog *selectordialog, Inkscape::XML::Node *repr)
      : _selectordialog(selectordialog)
      , _repr(repr)
  {
      g_debug("SelectorDialog::NodeWatcher: Constructor");
    };

    void notifyChildAdded( Inkscape::XML::Node &/*node*/,
                                   Inkscape::XML::Node &child,
                                   Inkscape::XML::Node */*prev*/ ) override
    {
        if (_selectordialog && _repr) {
            _selectordialog->_nodeAdded(child);
        }
    }

    void notifyChildRemoved( Inkscape::XML::Node &/*node*/,
                                     Inkscape::XML::Node &child,
                                     Inkscape::XML::Node */*prev*/ ) override
    {
        if (_selectordialog && _repr) {
            _selectordialog->_nodeRemoved(child);
        }
    }

    void notifyAttributeChanged( Inkscape::XML::Node &node,
                                         GQuark qname,
                                         Util::ptr_shared /*old_value*/,
                                         Util::ptr_shared /*new_value*/ ) override {
        if (_selectordialog && _repr) {

            // For the moment only care about attributes that are directly used in selectors.
            const gchar * cname = g_quark_to_string (qname );
            Glib::ustring name;
            if (cname) {
                name = cname;
            }

            if ( name == "id" || name == "class" ) {
                _selectordialog->_nodeChanged(node);
            }
        }
    }

    SelectorDialog *_selectordialog;
    Inkscape::XML::Node * _repr;  // Need to track if document changes.
};

void
SelectorDialog::_nodeAdded( Inkscape::XML::Node &node ) {

    SelectorDialog::NodeWatcher *w = new SelectorDialog::NodeWatcher (this, &node);
    node.addObserver (*w);
    _nodeWatchers.push_back(w);

    _readStyleElement();
    _selectRow();
}

void
SelectorDialog::_nodeRemoved( Inkscape::XML::Node &repr ) {

    for (auto it = _nodeWatchers.begin(); it != _nodeWatchers.end(); ++it) {
        if ( (*it)->_repr == &repr ) {
            (*it)->_repr->removeObserver (**it);
            _nodeWatchers.erase( it );
            break;
        }
    }

    _readStyleElement();
    _selectRow();
}

void
SelectorDialog::_nodeChanged( Inkscape::XML::Node &object ) {

    _readStyleElement();
    _selectRow();
}

SelectorDialog::TreeStore::TreeStore()
= default;


/**
 * Allow dragging only selectors.
 */
bool
SelectorDialog::TreeStore::row_draggable_vfunc(const Gtk::TreeModel::Path& path) const
{
    g_debug("SelectorDialog::TreeStore::row_draggable_vfunc");

    auto unconstThis = const_cast<SelectorDialog::TreeStore*>(this);
    const_iterator iter = unconstThis->get_iter(path);
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        bool is_draggable =
            row[_selectordialog->_mColumns._colType] == SELECTOR || row[_selectordialog->_mColumns._colType] == UNHANDLED;
        return is_draggable;
    }
    return Gtk::TreeStore::row_draggable_vfunc(path);
}


/**
 * Allow dropping only in between other selectors.
 */
bool
SelectorDialog::TreeStore::row_drop_possible_vfunc(const Gtk::TreeModel::Path& dest,
                                                const Gtk::SelectionData& selection_data) const
{
    g_debug("SelectorDialog::TreeStore::row_drop_possible_vfunc");

    Gtk::TreeModel::Path dest_parent = dest;
    dest_parent.up();
    return dest_parent.empty();
}


// This is only here to handle updating style element after a drag and drop.
void
SelectorDialog::TreeStore::on_row_deleted(const TreeModel::Path& path)
{
    if (_selectordialog->_updating) return;  // Don't write if we deleted row (other than from DND)

    g_debug("on_row_deleted");

    _selectordialog->_writeStyleElement();
}


Glib::RefPtr<SelectorDialog::TreeStore> SelectorDialog::TreeStore::create(SelectorDialog *selectordialog)
{
    SelectorDialog::TreeStore * store = new SelectorDialog::TreeStore();
    store->_selectordialog = selectordialog;
    store->set_column_types( store->_selectordialog->_mColumns );
    return Glib::RefPtr<SelectorDialog::TreeStore>( store );
}

/**
 * Constructor
 * A treeview and a set of two buttons are added to the dialog. _addSelector
 * adds selectors to treeview. _delSelector deletes the selector from the dialog.
 * Any addition/deletion of the selectors updates XML style element accordingly.
 */
SelectorDialog::SelectorDialog() :
    UI::Widget::Panel("/dialogs/style", SP_VERB_DIALOG_STYLE),
    _updating(false),
    _textNode(nullptr),
    _desktopTracker()
{
    g_debug("SelectorDialog::SelectorDialog");

    // Tree
    Inkscape::UI::Widget::IconRenderer * addRenderer = manage(
                new Inkscape::UI::Widget::IconRenderer() );
    addRenderer->add_icon("edit-delete");
    addRenderer->add_icon("list-add");
    addRenderer->add_icon("object-locked");

    _store = TreeStore::create(this);
    _treeView.set_model(_store);

    _treeView.set_headers_visible(true);
    _treeView.enable_model_drag_source();
    _treeView.enable_model_drag_dest( Gdk::ACTION_MOVE );
    int addCol = _treeView.append_column("", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if ( col ) {
        col->add_attribute(addRenderer->property_icon(), _mColumns._colType);
    }
    _treeView.append_column("CSS Selector", _mColumns._colSelector);
    _treeView.set_expander_column(*(_treeView.get_column(1)));

    // Pack widgets
    _paned.set_orientation(Gtk::ORIENTATION_VERTICAL);
    _paned.pack1(_mainBox, Gtk::SHRINK);
    _mainBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _scrolledWindow.add(_treeView);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    create = manage( new Gtk::Button() );
    _styleButton(*create, "list-add", "Add a new CSS Selector");
    create->signal_clicked().connect(sigc::mem_fun(*this, &SelectorDialog::_addSelector));

    del = manage( new Gtk::Button() );
    _styleButton(*del, "list-remove", "Remove a CSS Selector");
    del->signal_clicked().connect(sigc::mem_fun(*this, &SelectorDialog::_delSelector));
    del->hide();
    _mainBox.pack_end(_buttonBox, Gtk::PACK_SHRINK);

    _buttonBox.pack_start(*create, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(*del, Gtk::PACK_SHRINK);
    _getContents()->pack_start(_paned, Gtk::PACK_EXPAND_WIDGET);


    // Signal handlers
    _treeView.signal_button_release_event().connect(   // Needs to be release, not press.
        sigc::mem_fun(*this,  &SelectorDialog::_handleButtonEvent),
        false);

    _treeView.signal_button_release_event().connect_notify(
        sigc::mem_fun(*this, &SelectorDialog::_buttonEventsSelectObjs),
        false);

    _treeView.signal_row_expanded().connect(sigc::mem_fun(*this, &SelectorDialog::_rowExpand));

    _treeView.signal_row_collapsed().connect(sigc::mem_fun(*this, &SelectorDialog::_rowCollapse));

    // Document & Desktop
    _desktop_changed_connection = _desktopTracker.connectDesktopChanged(
        sigc::mem_fun(*this, &SelectorDialog::_handleDesktopChanged) );
    _desktopTracker.connect(GTK_WIDGET(gobj()));

    _document_replaced_connection = getDesktop()->connectDocumentReplaced(
        sigc::mem_fun(this, &SelectorDialog::_handleDocumentReplaced));

    _selection_changed_connection = getDesktop()->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &SelectorDialog::_handleSelectionChanged)));

    // Add watchers
    _updateWatchers();

    // Load tree
    _readStyleElement();
    _selectRow();

    if (!_store->children().empty()) {
        del->show();
    }

}


/**
 * Class destructor
 */
SelectorDialog::~SelectorDialog()
{
    g_debug("SelectorDialog::~SelectorDialog");
    _desktop_changed_connection.disconnect();
    _document_replaced_connection.disconnect();
    _selection_changed_connection.disconnect();
}


/**
 * @return Inkscape::XML::Node* pointing to a style element's text node.
 * Returns the style element's text node. If there is no style element, one is created.
 * Ditto for text node.
 */
Inkscape::XML::Node* SelectorDialog::_getStyleTextNode()
{

    Inkscape::XML::Node *styleNode = nullptr;
    Inkscape::XML::Node *textNode = nullptr;

    Inkscape::XML::Node *root = SP_ACTIVE_DOCUMENT->getReprRoot();
    for (unsigned i = 0; i < root->childCount(); ++i) {
        if (Glib::ustring(root->nthChild(i)->name()) == "svg:style") {

            styleNode = root->nthChild(i);

            for (unsigned j = 0; j < styleNode->childCount(); ++j) {
                if (styleNode->nthChild(j)->type() == Inkscape::XML::TEXT_NODE) {
                    textNode = styleNode->nthChild(j);
                }
            }

            if (textNode == nullptr) {
                // Style element found but does not contain text node!
                std::cerr << "SelectorDialog::_getStyleTextNode(): No text node!" << std::endl;
                textNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");
                styleNode->appendChild(textNode);
                Inkscape::GC::release(textNode);
            }
        }
    }

    if (styleNode == nullptr) {
        // Style element not found, create one
        styleNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createElement("svg:style");
        textNode  = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");

        root->addChild(styleNode, nullptr);
        Inkscape::GC::release(styleNode);

        styleNode->appendChild(textNode);
        Inkscape::GC::release(textNode);
    }

    if (_textNode != textNode) {
        _textNode = textNode;
        NodeObserver *no = new NodeObserver(this);
        textNode->addObserver(*no);
    }

    return textNode;
}


/**
 * Fill the Gtk::TreeStore from the svg:style element.
 */
void SelectorDialog::_readStyleElement()
{
    g_debug("SelectorDialog::_readStyleElement: updating %s", (_updating ? "true" : "false"));

    if (_updating) return; // Don't read if we wrote style element.
    _updating = true;

    Inkscape::XML::Node * textNode = _getStyleTextNode();
    if (textNode == nullptr) {
        std::cerr << "SelectorDialog::_readStyleElement: No text node!" << std::endl;
    }

    // Get content from style text node.
    std::string content = (textNode->content() ? textNode->content() : "");

    // Remove end-of-lines (check it works on Windoze).
    content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());

    // Remove comments (/* xxx */)
    /*     while(content.find("/*") != std::string::npos) {
            size_t start = content.find("/*");
            content.erase(start, (content.find("*\/", start) - start) +2);
        } */

    // First split into selector/value chunks.
    // An attempt to use Glib::Regex failed. A C++11 version worked but
    // reportedly has problems on Windows. Using split_simple() is simpler
    // and probably faster.
    //
    // Glib::RefPtr<Glib::Regex> regex1 =
    //   Glib::Regex::create("([^\\{]+)\\{([^\\{]+)\\}");
    //
    // Glib::MatchInfo minfo;
    // regex1->match(content, minfo);

    // Split on curly brackets. Even tokens are selectors, odd are values.
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[}{]", content);

    // If text node is empty, return (avoids problem with negative below).
    if (tokens.size() == 0) {
        _updating = false;
        return;
    }
    std::vector<std::pair<Glib::ustring, bool>> expanderstatus;
    for (unsigned i = 0; i < tokens.size() - 1; i += 2) {
        Glib::ustring selector = tokens[i];
        REMOVE_SPACES(selector); // Remove leading/trailing spaces
        for (auto &row : _store->children()) {
            Glib::ustring selectorold = row[_mColumns._colSelector];
            if (selectorold == selector) {
                expanderstatus.emplace_back(selector, row[_mColumns._colExpand]);
            }
        }
    }
    _store->clear();

    for (unsigned i = 0; i < tokens.size()-1; i += 2) {

        Glib::ustring selector = tokens[i];
        REMOVE_SPACES(selector); // Remove leading/trailing spaces
        std::vector<Glib::ustring> tokensplus = Glib::Regex::split_simple("[,]+", selector);
        coltype colType = SELECTOR;
        for (auto tok : tokensplus) {
            REMOVE_SPACES(tok);
            if (SPAttributeRelSVG::isSVGElement(tok) || tok.find(" ") != -1 || tok[0] == '>' || tok[0] == '+' ||
                tok[0] == '~' || tok[0] == '*' || tok.erase(0, 1).find(".") != -1) {
                colType = UNHANDLED;
            }
        }
        // Get list of objects selector matches
        std::vector<SPObject *> objVec = _getObjVec( selector );

        Glib::ustring properties;
        // Check to make sure we do have a value to match selector.
        if ((i+1) < tokens.size()) {
            properties = tokens[i+1];
        } else {
            std::cerr << "SelectorDialog::_readStyleElement: Missing values "
                "for last selector!" << std::endl;
        }
        REMOVE_SPACES(properties);
        bool colExpand = false;
        for (auto rowstatus : expanderstatus) {
            if (selector == rowstatus.first) {
                colExpand = rowstatus.second;
            }
        }
        std::vector<Glib::ustring> properties_data = Glib::Regex::split_simple(";", properties);
        Gtk::TreeModel::Row row = *(_store->append());
        row[_mColumns._colSelector]   = selector;
        row[_mColumns._colExpand] = colExpand;
        row[_mColumns._colType] = colType;
        row[_mColumns._colObj]        = objVec;
        row[_mColumns._colProperties] = properties;
        row[_mColumns._colVisible] = true;
        // Add as children, objects that match selector.
        for (auto &obj : objVec) {
            Gtk::TreeModel::Row childrow = *(_store->append(row->children()));
            childrow[_mColumns._colSelector] = "#" + Glib::ustring(obj->getId());
            childrow[_mColumns._colExpand] = false;
            childrow[_mColumns._colType] = colType == UNHANDLED ? UNHANDLED : OBJECT;
            ;
            childrow[_mColumns._colObj] = std::vector<SPObject *>(1, obj);
            childrow[_mColumns._colProperties] = ""; // Unused
            childrow[_mColumns._colVisible] = true;  // Unused
        }
    }


    _updating = false;
}

void SelectorDialog::_rowExpand(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path)
{
    g_debug("SelectorDialog::_row_expand()");
    Gtk::TreeModel::Row row = *iter;
    row[_mColumns._colExpand] = true;
}

void SelectorDialog::_rowCollapse(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path)
{
    g_debug("SelectorDialog::_row_collapse()");
    Gtk::TreeModel::Row row = *iter;
    row[_mColumns._colExpand] = false;
}
/**
 * Update the content of the style element as selectors (or objects) are added/removed.
 */
void SelectorDialog::_writeStyleElement()
{
    if (_updating) {
        return;
    }
    _updating = true;

    Glib::ustring styleContent;
    for (auto& row: _store->children()) {
        Glib::ustring selector = row[_mColumns._colSelector];
        /*
                REMOVE_SPACES(selector);
        /*         size_t len = selector.size();
                if(selector[len-1] == ','){
                    selector.erase(len-1);
                }
                row[_mColumns._colSelector] =  selector; */
        styleContent = styleContent + selector + " { " + row[_mColumns._colProperties] + " }\n";
    }
    // We could test if styleContent is empty and then delete the style node here but there is no
    // harm in keeping it around ...

    Inkscape::XML::Node *textNode = _getStyleTextNode();
    textNode->setContent(styleContent.c_str());

    DocumentUndo::done(SP_ACTIVE_DOCUMENT, SP_VERB_DIALOG_STYLE, _("Edited style element."));

    _updating = false;
    g_debug("SelectorDialog::_writeStyleElement(): | %s |", styleContent.c_str());
}


void SelectorDialog::_addWatcherRecursive(Inkscape::XML::Node *node) {

    g_debug("SelectorDialog::_addWatcherRecursive()");

    SelectorDialog::NodeWatcher *w = new SelectorDialog::NodeWatcher(this, node);
    node->addObserver(*w);
    _nodeWatchers.push_back(w);

    for (unsigned i = 0; i < node->childCount(); ++i) {
        _addWatcherRecursive(node->nthChild(i));
    }
}

/**
 * Update the watchers on objects.
 */
void SelectorDialog::_updateWatchers()
{
    _updating = true;

    // Remove old document watchers
    while (!_nodeWatchers.empty()) {
        SelectorDialog::NodeWatcher *w = _nodeWatchers.back();
        w->_repr->removeObserver(*w);
        _nodeWatchers.pop_back();
        delete w;
    }

    // Recursively add new watchers
    Inkscape::XML::Node *root = SP_ACTIVE_DOCUMENT->getReprRoot();
    _addWatcherRecursive(root);

    g_debug("SelectorDialog::_updateWatchers(): %d", (int)_nodeWatchers.size());

    _updating = false;
}


/**
 * @param row
 * Add selected objects on the desktop to the selector corresponding to 'row'.
 */
void SelectorDialog::_addToSelector(Gtk::TreeModel::Row row)
{
    g_debug("SelectorDialog::_addToSelector: Entrance");
    if (*row) {

        Glib::ustring selector = row[_mColumns._colSelector];

        if (selector[0] == '#') {
            // 'id' selector... add selected object's id's to list.
            Inkscape::Selection* selection = getDesktop()->getSelection();
            for (auto& obj: selection->objects()) {

                Glib::ustring id = (obj->getId()?obj->getId():"");

                std::vector<SPObject *> objVec = row[_mColumns._colObj];
                bool found = false;
                for (auto& obj: objVec) {
                    if (id == obj->getId()) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    // Update row
                    objVec.push_back(obj); // Adding to copy so need to update tree
                    row[_mColumns._colObj]      = objVec;
                    row[_mColumns._colSelector] = _getIdList( objVec );
                    row[_mColumns._colExpand] = true;
                    // Add child row
                    Gtk::TreeModel::Row childrow = *(_store->append(row->children()));
                    childrow[_mColumns._colSelector]   = "#" + Glib::ustring(obj->getId());
                    childrow[_mColumns._colType] = OBJECT;
                    childrow[_mColumns._colObj]        = std::vector<SPObject *>(1, obj);
                    childrow[_mColumns._colProperties] = "";  // Unused
                    childrow[_mColumns._colVisible] = true;   // Unused
                }
            }
        }

        else if (selector[0] == '.') {
            // 'class' selector... add value to class attribute of selected objects.

            // Get first class (split on white space or comma)
            std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[,\\s]+", selector);
            Glib::ustring className = tokens[0];
            className.erase(0,1);
            Inkscape::Selection* selection = getDesktop()->getSelection();
            std::vector<SPObject *> sel_obj(selection->objects().begin(), selection->objects().end());
            _insertClass(sel_obj, className);
            std::vector<SPObject *> objVec = _getObjVec(selector);
            ;
            for (auto &obj : sel_obj) {

                Glib::ustring id = (obj->getId() ? obj->getId() : "");
                bool found = false;
                for (auto &obj : objVec) {
                    if (id == obj->getId()) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    // Update row
                    objVec.push_back(obj); // Adding to copy so need to update tree
                    row[_mColumns._colObj] = objVec;
                    row[_mColumns._colExpand] = true;

                    // Update row
                    Gtk::TreeModel::Row childrow = *(_store->append(row->children()));
                    childrow[_mColumns._colSelector] = "#" + Glib::ustring(obj->getId());
                    childrow[_mColumns._colExpand] = false;
                    childrow[_mColumns._colType] = OBJECT;
                    childrow[_mColumns._colObj] = std::vector<SPObject *>(1, obj);
                    childrow[_mColumns._colProperties] = ""; // Unused
                    childrow[_mColumns._colVisible] = true;  // Unused
                }
            }
        }

        else {
            // Do nothing for element selectors.
            // std::cout << "  Element selector... doing nothing!" << std::endl;
        }
    }

    _writeStyleElement();
}


/**
 * @param row
 * Remove the object corresponding to 'row' from the parent selector.
 */
void SelectorDialog::_removeFromSelector(Gtk::TreeModel::Row row)
{
    g_debug("SelectorDialog::_removeFromSelector: Entrance");
    if (*row) {

        Glib::ustring objectLabel = row[_mColumns._colSelector];
        if (row[_mColumns._colType] == UNHANDLED) {
            return;
        };
        Gtk::TreeModel::iterator iter = row->parent();
        if (iter) {
            Gtk::TreeModel::Row parent = *iter;
            Glib::ustring selector = parent[_mColumns._colSelector];
            REMOVE_SPACES(selector);
            if (selector[0] == '#') {
                // 'id' selector... remove selected object's id's to list.

                // Erase from selector label.
                auto i = selector.find(objectLabel);
                if (i != Glib::ustring::npos) {
                    selector.erase(i, objectLabel.length());
                }
                // Erase any comma/space
                REMOVE_SPACES(selector);
                if (i != Glib::ustring::npos && selector[i] == ',') {
                    selector.erase(i, 1);
                }
                if (i != Glib::ustring::npos && selector[i] == ' ') {
                    selector.erase(i, 1);
                }
                REMOVE_SPACES(selector);
                if (selector[selector.size() - 1] == ',') {
                    selector.erase(selector.size() - 1, 1);
                }

                // Update store
                if (selector.empty()) {
                    _store->erase(parent);
                } else {
                    // Save new selector and update object vector.
                    parent[_mColumns._colSelector] = selector;
                    parent[_mColumns._colObj]      = _getObjVec( selector );
                    parent[_mColumns._colExpand] = true;
                    _store->erase(row);
                }
            }

            else if (selector[0] == '.') {
                // 'class' selector... remove value to class attribute of selected objects.

                std::vector<SPObject *> objVec = row[_mColumns._colObj]; // Just one
                // Get first class (split on white space or comma)
                std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[,\\s]+", selector);
                Glib::ustring className = tokens[0];
                className.erase(0, 1);
                // Erase class name from 'class' attribute.
                Glib::ustring classAttr = objVec[0]->getRepr()->attribute("class");
                auto i = classAttr.find( className );
                if (i != Glib::ustring::npos) {
                    classAttr.erase(i, className.length());
                }
                if (i != Glib::ustring::npos && classAttr[i] == ' ') {
                    classAttr.erase(i, 1);
                }
                _store->erase(row);
                objVec[0]->getRepr()->setAttribute("class", classAttr);
                parent[_mColumns._colExpand] = true;
            } else {
                // Do nothing for element selectors.
                // std::cout << "  Element selector... doing nothing!" << std::endl;
            }
        }
    }
    _writeStyleElement();
}


/**
 * @param sel
 * @return This function returns a comma separated list of ids for objects in input vector.
 * It is used in creating an 'id' selector. It relies on objects having 'id's.
 */
Glib::ustring SelectorDialog::_getIdList(std::vector<SPObject*> sel)
{
    Glib::ustring str;
    for (auto& obj: sel) {
        str += "#" + Glib::ustring(obj->getId()) + ", ";
    }
    if (!str.empty()) {
        str.erase(str.size()-1); // Remove space at end. c++11 has pop_back() but not ustring.
        str.erase(str.size()-1); // Remove comma at end.
    }
    return str;
}

/**
 * @param selector: a valid CSS selector string.
 * @return objVec: a vector of pointers to SPObject's the selector matches.
 * Return a vector of all objects that selector matches.
 */
std::vector<SPObject *> SelectorDialog::_getObjVec(Glib::ustring selector) {

    g_debug("SelectorDialog::_getObjVec: | %s |", selector.c_str());
    std::vector<SPObject *> objVec;
    std::vector<Glib::ustring> tokensplus = Glib::Regex::split_simple("[,]+", selector);
    bool unhandled = false;
    for (auto tok : tokensplus) {
        REMOVE_SPACES(tok);
        if (SPAttributeRelSVG::isSVGElement(tok) || tok.find(" ") != -1 || tok[0] == '>' || tok[0] == '+' ||
            tok[0] == '~' || tok[0] == '*' || tok.erase(0, 1).find(".") != -1) {
            unhandled = true;
            std::vector<SPObject *> objVecSplited = SP_ACTIVE_DOCUMENT->getObjectsBySelector(tok);
            objVec.insert(objVec.end(), objVecSplited.begin(), objVecSplited.end());
        }
    }
    if (!unhandled) {
        objVec = SP_ACTIVE_DOCUMENT->getObjectsBySelector(selector);
    }

    for (auto& obj: objVec) {
        g_debug("  %s", obj->getId() ? obj->getId() : "null");
    }

    return objVec;
}


/**
 * @param objs: list of objects to insert class
 * @param class: class to insert
 * Insert a class name into objects' 'class' attribute.
 */
void SelectorDialog::_insertClass(const std::vector<SPObject *>& objVec, const Glib::ustring& className) {

    for (auto& obj: objVec) {

        if (!obj->getRepr()->attribute("class")) {
            // 'class' attribute does not exist, create it.
            obj->getRepr()->setAttribute("class", className);
        } else {
            // 'class' attribute exists, append.
            Glib::ustring classAttr = obj->getRepr()->attribute("class");

            // Split on white space.
            std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s+", classAttr);
            bool add = true;
            for (auto& token: tokens) {
                if (token == className) {
                    add = false; // Might be useful to still add...
                    break;
                }
            }
            if (add) {
                obj->getRepr()->setAttribute("class", classAttr + " " + className );
            }
        }
    }
 }


/**
 * @param eventX
 * @param eventY
 * This function selects objects in the drawing corresponding to the selector
 * selected in the treeview.
 */
void SelectorDialog::_selectObjects(int eventX, int eventY)
{
    g_debug("SelectorDialog::_selectObjects: %d, %d", eventX, eventY);
    getDesktop()->selection->clear();
    Gtk::TreeViewColumn *col = _treeView.get_column(1);
    Gtk::TreeModel::Path path;
    int x2 = 0;
    int y2 = 0;
    // To do: We should be able to do this via passing in row.
    if (_treeView.get_path_at_pos(eventX, eventY, path, col, x2, y2)) {
        if (col == _treeView.get_column(1)) {
            Gtk::TreeModel::iterator iter = _store->get_iter(path);
            if (iter) {
                Gtk::TreeModel::Row row = *iter;
                Gtk::TreeModel::Children children = row.children();
                if (children.empty() || children.size() == 1) {
                    del->show();
                }
                std::vector<SPObject *> objVec = row[_mColumns._colObj];

                for (auto obj : objVec) {
                    getDesktop()->selection->add(obj);
                }
            }
        }
    }
}


/**
 * This function opens a dialog to add a selector. The dialog is prefilled
 * with an 'id' selector containing a list of the id's of selected objects
 * or with a 'class' selector if no objects are selected.
 */
void SelectorDialog::_addSelector()
{
    g_debug("SelectorDialog::_addSelector: Entrance");

    // Store list of selected elements on desktop (not to be confused with selector).
    Inkscape::Selection* selection = getDesktop()->getSelection();
    std::vector<SPObject *> objVec( selection->objects().begin(),
                                    selection->objects().end() );

    // ==== Create popup dialog ====
    Gtk::Dialog *textDialogPtr =  new Gtk::Dialog();
    textDialogPtr->add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    textDialogPtr->add_button(_("Add"),    Gtk::RESPONSE_OK);

    Gtk::Entry *textEditPtr = manage ( new Gtk::Entry() );
    textEditPtr->signal_activate().connect(
        sigc::bind<Gtk::Dialog *>(sigc::mem_fun(*this, &SelectorDialog::_closeDialog), textDialogPtr));
    textDialogPtr->get_content_area()->pack_start(*textEditPtr, Gtk::PACK_SHRINK);

    Gtk::Label *textLabelPtr = manage ( new Gtk::Label(
      _("Invalid entry: Not an id (#), class (.), or element CSS selector.")
    ) );
    textDialogPtr->get_content_area()->pack_start(*textLabelPtr, Gtk::PACK_SHRINK);

    /**
     * By default, the entrybox contains 'Class1' as text. However, if object(s)
     * is(are) selected and user clicks '+' at the bottom of dialog, the
     * entrybox will have the id(s) of the selected objects as text.
     */
    if (getDesktop()->getSelection()->isEmpty()) {
        textEditPtr->set_text(".Class1");
    } else {
        textEditPtr->set_text(_getIdList(objVec));
    }

    Gtk::Requisition sreq1, sreq2;
    textDialogPtr->get_preferred_size(sreq1, sreq2);
    int minWidth = 200;
    int minHeight = 100;
    minWidth  = (sreq2.width  > minWidth  ? sreq2.width  : minWidth );
    minHeight = (sreq2.height > minHeight ? sreq2.height : minHeight);
    textDialogPtr->set_size_request(minWidth, minHeight);
    textEditPtr->show();
    textLabelPtr->hide();
    textDialogPtr->show();


    // ==== Get response ====
    int result = -1;
    bool invalid = true;
    Glib::ustring selectorValue;
    bool handled = true;
    while (invalid) {
        result = textDialogPtr->run();
        if (result != Gtk::RESPONSE_OK) { // Cancel, close dialog, etc.
            textDialogPtr->hide();
            delete textDialogPtr;
            return;
        }
        /**
         * @brief selectorName
         * This string stores selector name. The text from entrybox is saved as name
         * for selector. If the entrybox is empty, the text (thus selectorName) is
         * set to ".Class1"
         */
        selectorValue = textEditPtr->get_text();
        del->show();
        std::vector<Glib::ustring> tokensplus = Glib::Regex::split_simple("[,]+", selectorValue);
        bool unhandled = false;
        bool partialinvalid = false;
        for (auto tok : tokensplus) {
            REMOVE_SPACES(tok);
            if (SPAttributeRelSVG::isSVGElement(tok) || tok.find(" ") != -1 || tok[0] == '>' || tok[0] == '+' ||
                tok[0] == '~' || tok[0] == '*' || tok.erase(0, 1).find(".") != -1) {
                unhandled = true;
                Glib::ustring firstWord = tok.substr(0, tok.find_first_of(" >+~"));
                if (firstWord != tok) {
                    handled = false;
                }
                if (!partialinvalid &&
                    (tok[0] == '.' || tok[0] == '#' || tok[0] == '*' || SPAttributeRelSVG::isSVGElement(tok))) {
                    partialinvalid = false;
                } else {
                    partialinvalid = true;
                }
            }
        }
        if (!unhandled) {
            Glib::ustring firstWord = selectorValue.substr(0, selectorValue.find_first_of(" >+~"));
            if (firstWord != selectorValue) {
                handled = false;
            }
            if (selectorValue[0] == '.' || selectorValue[0] == '#' || selectorValue[0] == '*' ||
                SPAttributeRelSVG::isSVGElement(selectorValue)) {
                invalid = false;
            } else {
                textLabelPtr->show();
            }
        } else if (partialinvalid) {
            textLabelPtr->show();
        } else {
            invalid = false;
        }
    }
    delete textDialogPtr;
    // ==== Handle response ====

    // If class selector, add selector name to class attribute for each object
    if (selectorValue[0] == '.' && handled) {
        std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("[,\\s]+", selectorValue);
        Glib::ustring originClassName = tokens[0];
        originClassName.erase(0, 1);
        std::vector<Glib::ustring> classes = Glib::Regex::split_simple("[\\.]+", originClassName);
        if (classes.size() == 1) {
            _insertClass(objVec, classes[0]);
        } else {
            handled = false;
        }
    }

    // Generate a new object vector (we could have an element selector,
    // the user could have edited the id selector list, etc.).
    objVec = _getObjVec( selectorValue );

    // Add entry to GUI tree
    Gtk::TreeModel::Row row = *(_store->append());
    row[_mColumns._colSelector] = selectorValue;
    row[_mColumns._colExpand] = true;
    row[_mColumns._colType] = handled ? SELECTOR : UNHANDLED;
    row[_mColumns._colObj] = objVec;

    // Add as children objects that match selector.
    if (handled) {
        for (auto &obj : objVec) {
            Gtk::TreeModel::Row childrow = *(_store->append(row->children()));
            childrow[_mColumns._colSelector] = "#" + Glib::ustring(obj->getId());
            childrow[_mColumns._colExpand] = false;
            childrow[_mColumns._colType] = OBJECT;
            childrow[_mColumns._colObj] = std::vector<SPObject *>(1, obj);
        }
    }

    // Add entry to style element
    _writeStyleElement();
}

void SelectorDialog::_closeDialog(Gtk::Dialog *textDialogPtr) { textDialogPtr->response(Gtk::RESPONSE_OK); }

/**
 * This function deletes selector when '-' at the bottom is clicked.
 * Note: If deleting a class selector, class attributes are NOT changed.
 */
void SelectorDialog::_delSelector()
{
    g_debug("SelectorDialog::_delSelector");

    Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
    _treeView.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        if (row.children().size() > 2) {
            return;
        }
        _updating = true;
        _store->erase(iter);
        _updating = false;
        _writeStyleElement();
        del->hide();
    }
}

/**
 * @param event
 * @return
 * Handles the event when '+' button in front of a selector name is clicked or when a '-' button in
 * front of a child object is clicked. In the first case, the selected objects on the desktop (if
 * any) are added as children of the selector in the treeview. In the latter case, the object
 * corresponding to the row is removed from the selector.
 */
bool SelectorDialog::_handleButtonEvent(GdkEventButton *event)
{
    g_debug("SelectorDialog::_handleButtonEvent: Entrance");
    if (event->type == GDK_BUTTON_RELEASE && event->button == 1) {
        Gtk::TreeViewColumn *col = nullptr;
        Gtk::TreeModel::Path path;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;

        if (_treeView.get_path_at_pos(x, y, path, col, x2, y2)) {
            if (col == _treeView.get_column(0)) {
                bool remove_parent = false;
                Gtk::TreeModel::iterator iter = _store->get_iter(path);
                Gtk::TreeModel::Row row = *iter;
                Glib::RefPtr<Gtk::TreeSelection> sel = _treeView.get_selection();
                sel->select(row);
                // Add or remove objects from a
                Gtk::TreeModel::Row row_to_sel;
                if (!row.parent()) {
                    row_to_sel = row;
                    _addToSelector(row);
                } else {
                    row_to_sel = *row.parent();
                    _removeFromSelector(row);
                }
            }
        }
    }
    return false;
}

// -------------------------------------------------------------------

class PropertyData
{
public:
    PropertyData() = default;;
    PropertyData(Glib::ustring name) : _name(std::move(name)) {};

    void _setSheetValue(Glib::ustring value) { _sheetValue = value; };
    void _setAttrValue(Glib::ustring value)  { _attrValue  = value; };
    Glib::ustring _getName()       { return _name;       };
    Glib::ustring _getSheetValue() { return _sheetValue; };
    Glib::ustring _getAttrValue()  { return _attrValue;  };

private:
    Glib::ustring _name;
    Glib::ustring _sheetValue;
    Glib::ustring _attrValue;
};

// -------------------------------------------------------------------


/**
 * Handle document replaced. (Happens when a default document is immediately replaced by another
 * document in a new window.)
 */
void
SelectorDialog::_handleDocumentReplaced(SPDesktop *desktop, SPDocument * /* document */)
{
    g_debug("SelectorDialog::handleDocumentReplaced()");

    _selection_changed_connection.disconnect();

    _selection_changed_connection = desktop->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &SelectorDialog::_handleSelectionChanged)));

    _updateWatchers();
    _readStyleElement();
    _selectRow();
}


/*
 * When a dialog is floating, it is connected to the active desktop.
 */
void
SelectorDialog::_handleDesktopChanged(SPDesktop* desktop) {
    g_debug("SelectorDialog::handleDesktopReplaced()");

    if (getDesktop() == desktop) {
        // This will happen after construction of dialog. We've already
        // set up signals so just return.
        return;
    }

    _selection_changed_connection.disconnect();
    _document_replaced_connection.disconnect();

    setDesktop( desktop );

    _selection_changed_connection = desktop->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &SelectorDialog::_handleSelectionChanged)));
    _document_replaced_connection = desktop->connectDocumentReplaced(
        sigc::mem_fun(this, &SelectorDialog::_handleDocumentReplaced));

    _updateWatchers();
    _readStyleElement();
    _selectRow();
}


/*
 * Handle a change in which objects are selected in a document.
 */
void
SelectorDialog::_handleSelectionChanged() {
    g_debug("SelectorDialog::_handleSelectionChanged()");
    _treeView.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    _selectRow();
}


/**
 * @param event
 * This function detects single or double click on a selector in any row. Clicking
 * on a selector selects the matching objects on the desktop. A double click will
 * in addition open the CSS dialog.
 */
void SelectorDialog::_buttonEventsSelectObjs(GdkEventButton* event )
{
    g_debug("SelectorDialog::_buttonEventsSelectObjs");
    _treeView.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    _updating = true;
    del->show();
    if (event->type == GDK_BUTTON_RELEASE && event->button == 1) {
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        _selectObjects(x, y);
    }
    _updating = false;
}


/**
 * This function selects the row in treeview corresponding to an object selected
 * in the drawing. If more than one row matches, the first is chosen.
 */
void SelectorDialog::_selectRow()
{
    g_debug("SelectorDialog::_selectRow: updating: %s", (_updating ? "true" : "false"));
    del->hide();
    std::vector<Gtk::TreeModel::Path> selectedrows = _treeView.get_selection()->get_selected_rows();
    if (selectedrows.size() == 1) {
        Gtk::TreeModel::Row row = *_store->get_iter(selectedrows[0]);
        if (!row->parent() && row->children().size() < 2) {
            del->show();
        }
    } else if (selectedrows.size() == 0) {
        del->show();
    }
    if (_updating || !getDesktop()) return; // Avoid updating if we have set row via dialog.
    if (SP_ACTIVE_DESKTOP != getDesktop()) {
        std::cerr << "SelectorDialog::_selectRow: SP_ACTIVE_DESKTOP != getDesktop()" << std::endl;
        return;
    }

    _treeView.get_selection()->unselect_all();
    Gtk::TreeModel::Children children = _store->children();
    Inkscape::Selection* selection = getDesktop()->getSelection();
    SPObject *obj = nullptr;
    if (!selection->isEmpty()) {
        obj = selection->objects().back();
    }

    for (auto row : children) {
        std::vector<SPObject *> objVec = row[_mColumns._colObj];
        if (obj) {
            for (auto & i : objVec) {
                if (obj->getId() == i->getId()) {
                    _treeView.get_selection()->select(row);
                    row[_mColumns._colVisible] = true;
                    break;
                }
            }
        }
        if (row[_mColumns._colExpand]) {
            _treeView.expand_to_path(Gtk::TreePath(row));
        }
    }
}

/**
 * @param btn
 * @param iconName
 * @param tooltip
 * Set the style of '+' and '-' buttons at the bottom of dialog.
 */
void SelectorDialog::_styleButton(Gtk::Button& btn, char const* iconName,
                               char const* tooltip)
{
    GtkWidget *child = sp_get_icon_image(iconName, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(child);
    btn.add(*manage(Glib::wrap(child)));
    btn.set_relief(Gtk::RELIEF_NONE);
    btn.set_tooltip_text (tooltip);
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
