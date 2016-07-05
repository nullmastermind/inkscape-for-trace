/** @file
 * @brief A dialog for CSS selectors
 */
/* Authors:
 *   Kamalpreet Kaur Grewal
 *
 * Copyright (C) Kamalpreet Kaur Grewal 2016 <grewalkamal005@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "styledialog.h"
#include "ui/widget/addtoicon.h"
#include "widgets/icon.h"
#include "verbs.h"
#include "sp-object.h"
#include "selection.h"
#include "xml/attribute-record.h"

using Inkscape::Util::List;
using Inkscape::XML::AttributeRecord;

#define REMOVE_SPACES(x) x.erase(0, x.find_first_not_of(' ')); \
    x.erase(x.find_last_not_of(' ') + 1);

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief StyleDialog::_styleButton
 * @param btn
 * @param iconName
 * @param tooltip
 * This function sets the style of '+' and '-' buttons at the bottom of dialog.
 */
void StyleDialog::_styleButton(Gtk::Button& btn, char const* iconName,
                               char const* tooltip)
{
    GtkWidget *child = sp_icon_new(Inkscape::ICON_SIZE_SMALL_TOOLBAR, iconName);
    gtk_widget_show(child);
    btn.add(*manage(Glib::wrap(child)));
    btn.set_relief(Gtk::RELIEF_NONE);
    btn.set_tooltip_text (tooltip);
}

/**
 * Constructor
 * A treeview and a set of two buttons are added to the dialog. _addSelector
 * adds selectors to treeview. Currently, delete button is disabled.
 */
StyleDialog::StyleDialog() :
    UI::Widget::Panel("", "/dialogs/style", SP_VERB_DIALOG_STYLE),
    _desktop(0)
{
    set_size_request(200, 200);

    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _treeView.set_headers_visible(false);
    _scrolledWindow.add(_treeView);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _store = Gtk::TreeStore::create(_mColumns);
    _treeView.set_model(_store);

    Inkscape::UI::Widget::AddToIcon * addRenderer = manage(
                new Inkscape::UI::Widget::AddToIcon() );
    addRenderer->property_active() = true;

    int addCol = _treeView.append_column("type", *addRenderer) - 1;

    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if ( col ) {
        col->add_attribute( addRenderer->property_active(), _mColumns._colAddRemove );
    }

    _treeView.append_column("Selector Name", _mColumns._selectorLabel);

    create = manage( new Gtk::Button() );
    _styleButton(*create, "list-add", "Add a new CSS Selector");
    create->signal_clicked().connect(sigc::mem_fun(*this,
                                                   &StyleDialog::_addSelector));

    del = manage( new Gtk::Button() );
    _styleButton(*del, "list-remove", "Remove a CSS Selector");
    del->signal_clicked().connect(sigc::mem_fun(*this,
                                                &StyleDialog::_delSelector));
    del->set_sensitive(false);

    _mainBox.pack_end(_buttonBox, Gtk::PACK_SHRINK);

    _buttonBox.pack_start(*create, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(*del, Gtk::PACK_SHRINK);

    _getContents()->pack_start(_mainBox, Gtk::PACK_EXPAND_WIDGET);

    _targetDesktop = getDesktop();
    setDesktop(_targetDesktop);

    /**
     * @brief document
     * If an existing document is opened, its XML representation is obtained
     * and is then used to populate the treeview with the already existing
     * selectors in the style element.
     */
    _styleExists = false;
    _document = _targetDesktop->doc();
    _num = _document->getReprRoot()->childCount();
    _selectorValue = _populateTree(_getSelectorVec());

    _treeView.signal_button_press_event().connect(sigc::mem_fun(*this,
                                                                &StyleDialog::
                                                                _handleButtonEvent),
                                                  false);

    _treeView.signal_row_activated().connect(sigc::mem_fun(*this,
                                                           &StyleDialog::
                                                           _selectedRowCallback));
}

StyleDialog::~StyleDialog()
{
    setDesktop(NULL);
}

void StyleDialog::setDesktop( SPDesktop* desktop )
{
    Panel::setDesktop(desktop);
    _desktop = Panel::getDesktop();
    _desktop->selection->connectChanged(sigc::mem_fun(*this, &StyleDialog::
                                                      _selectRow));
}

/**
 * @brief StyleDialog::_addSelector
 * This function is the slot to the signal emitted when '+' at the bottom of
 * the dialog is clicked.
 */
void StyleDialog::_addSelector()
{
    _row = *(_store->append());

    /**
     * On clicking '+' button, an entrybox with default text opens up. If an
     * object is already selected, a selector with value in the entry
     * is added to a new style element.
     */
    Gtk::Dialog *textDialogPtr =  new Gtk::Dialog();
    Gtk::Entry *textEditPtr = manage ( new Gtk::Entry() );
    textDialogPtr->add_button("Add", Gtk::RESPONSE_OK);
    textDialogPtr->get_vbox()->pack_start(*textEditPtr, Gtk::PACK_SHRINK);

    /**
     * By default, the entrybox contains 'Class1' as text. However, if object(s)
     * is(are) selected and user clicks '+' at the bottom of dialog, the
     * entrybox will have the id(s) of the selected objects as text.
     */
    if (_desktop->selection->isEmpty()) {
        textEditPtr->set_text("Class1");
    }
    else {
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        textEditPtr->set_text(_setClassAttribute(selected));
    }

    textDialogPtr->set_size_request(200, 100);
    textDialogPtr->show_all();
    int result = textDialogPtr->run();

    /**
     * @brief selectorName
     * This string stores selector name. If '#' or a '.' is present in the
     * beginning of string, text from entrybox is saved directly as name for
     * selector. If text like 'red' is written in entrybox, it is prefixed
     * with a dot.
     */
    if (!textEditPtr->get_text().empty()) {
        _selectorName = textEditPtr->get_text();
    }
    else {
        _selectorName = ".Class1";
    }

    del->set_sensitive(true);

    /**
     * The selector name objects is set to the text that the user sets in the
     * entrybox. If the attribute does not exist, it is
     * created. In case the attribute already has a value, the new value entered
     * is appended to the values. If a style attribute does not exist, it is
     * created with an empty value. Also if a class selector is added, then
     * class attribute for the selected object is set too.
     */
    SPObject *obj;
    std::vector<SPObject *> objVec;

    bool objExists = false;
    if (!_desktop->getSelection()->list().empty()) {
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        for (unsigned i = 0; i < selected.size(); ++i) {
            obj = selected.at(i);
            objExists = true;
            std::string style;

            if (obj->getRepr()->attribute("style")) {
                for (List<AttributeRecord const> iter = obj->getRepr()->attributeList();
                     iter; ++iter) {
                    gchar const * property = g_quark_to_string(iter->key);
                    gchar const * value = iter->value;

                    if (std::string(property) == "style") {
                        _selectorValue = _selectorName + "{"
                                + std::string(value) + "}" + "\n";
                    }
                }
            }
            else {
                style = " ";
                obj->getRepr()->setAttribute("style", style);
            }

            if (strcmp(_selectorName.substr(0,1).c_str(), ".") == 0) {
                if (!obj->getRepr()->attribute("class")) {
                    obj->getRepr()->setAttribute("class", textEditPtr->get_text()
                                                 .erase(0,1));
                }
                else {
                    obj->getRepr()->setAttribute("class", std::string(obj->
                                                                      getRepr()->
                                                                      attribute("class"))
                                                 + " " + textEditPtr->get_text()
                                                 .erase(0,0));
                }
            }
        }
    }
    else {
        _selectorValue = _selectorName + "{" + "}" + "\n";
        objExists = false;
    }

    switch (result) {
    case Gtk::RESPONSE_OK:
        textDialogPtr->hide();
        _row[_mColumns._selectorLabel] = _selectorName;
        _row[_mColumns._colAddRemove] = true;
        if (objExists) {
            _row[_mColumns._colObj] = _desktop->selection->list();
            objVec = _row[_mColumns._colObj];
        }
        break;
    default:
        break;
    }

    /**
     * A new style element is added to the document with value obtained
     * from selectorValue above. If style element already exists, then
     * the new selector's content is appended to its previous content.
     */
    _selectorVec.push_back(std::make_pair(std::make_pair(_selectorName, objVec),
                                          _selectorValue));

    if (_styleElementNode()) {
        _styleChild = _styleElementNode();
        _updateStyleContent();
    }
    else if (_styleExists && !_newDrawing) {
        _updateStyleContent();
    }
    else if (!_styleExists) {
        Inkscape::XML::Node *root = _document->getReprDoc()->root();
        Inkscape::XML::Node *newChild = _document->getReprDoc()
                ->createElement("svg:style");
        Inkscape::XML::Node *smallChildren = _document->getReprDoc()
                ->createTextNode(_selectorValue.c_str());

        newChild->appendChild(smallChildren);
        Inkscape::GC::release(smallChildren);

        root->addChild(newChild, NULL);
        Inkscape::GC::release(newChild);
    }
}

/**
 * @brief StyleDialog::_updateStyleContent
 * This function updates the content in style element as new selectors (or
 * objects) are added/removed.
 */
void StyleDialog::_updateStyleContent()
{
    std::string styleContent = "";
    for (unsigned i = 0; i < _selectorVec.size(); ++i) {
        styleContent = styleContent + _selectorVec[i].second;
    }
    _styleChild->firstChild()->setContent(styleContent.c_str());
}

/**
 * @brief StyleDialog::_delSelector
 * This function deletes selector when '-' at the bottom is clicked. The index
 * of selected row is obtained and the corresponding selector and its values are
 * deleted from the selector vector. If a row has no parent, it is directly
 * erased from the vector else the string containing selector row's selector value
 * is updated after parsing.
 */
void StyleDialog::_delSelector()
{
    Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    Gtk::TreeModel::Path path;

    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        path = _treeView.get_model()->get_path(iter);
        int i = atoi(path.to_string().c_str());

        if (_selectorVec.size() != 0) {
            if (!row.parent()) {
                _selectorVec.erase(_selectorVec.begin()+i);
            }
            else {
                std::string sel, key, value;
                for (unsigned it = 0; it < _selectorVec.size(); ++it) {
                    sel = _selectorVec[it].second;
                    REMOVE_SPACES(sel);
                    std::cout << "sel" << sel << std::endl;
                    if (!sel.empty()) {
                        key = strtok(strdup(sel.c_str()), "{");
                        REMOVE_SPACES(key);
                        char *temp = strtok(NULL, "}");
                        if (strtok(temp, "}") != NULL) {
                            value = strtok(temp, "}");
                        }
                        if (key == "#" + row[_mColumns._selectorLabel]) {
                            sel = "";
                        }
                        else {
                            sel = sel;
                        }
                        _selectorVec[it].second = sel;
                    }
                }
            }

            /**
              * If there is a _styleChild which contains the style element, then
              * the content in style element is updated else the _styleChild is
              * obtained and its content is set.
              */
            if (_styleChild) {
                _updateStyleContent();
            }
            else {
                _styleChild = _styleElementNode();
                _updateStyleContent();
            }
        }
        _store->erase(row);
    }
}

/**
 * @brief StyleDialog::_styleElementNode
 * @return
 * This function returns the node containing style element. The document's
 * children are iterated and the repr of the style element that occurs is
 * obtained.
 */
Inkscape::XML::Node* StyleDialog::_styleElementNode()
{
    for (unsigned i = 0; i < _num; ++i) {
        if (std::string(_document->getReprRoot()->nthChild(i)->name())
                == "svg:style") {
            _styleExists = true;
            _newDrawing = true;
            return _document->getReprRoot()->nthChild(i);
        }
    }
    return NULL;
}

/**
 * @brief StyleDialog::_setClassAttribute
 * @param sel
 * @return This function returns the ids of objects selected which are passed
 * to entrybox.
 */
std::string StyleDialog::_setClassAttribute(std::vector<SPObject*> sel)
{
    std::string str = "";
    for ( unsigned i = 0; i < sel.size(); ++i ) {
        SPObject *obj = sel.at(i);
        str = str + "#" + std::string(obj->getId()) + " ";
    }
    return str;
}

/**
 * @brief StyleDialog::_getSelectorVec
 * @return selVec
 * This function returns a vector whose key is the style selector name and value
 * is the style properties. All style selectors are extracted from svg:style
 * element. _newDrawing is flag is set to false check if an existing drawing is
 * opened.
 */
std::vector<_selectorVecType> StyleDialog::_getSelectorVec()
{
    std::string key, value, selId;
    for (unsigned i = 0; i < _num; ++i) {
        if (std::string(_document->getReprRoot()->nthChild(i)->name()) == "svg:style") {
            _styleExists = true;
            _newDrawing = false;
            _styleChild = _document->getReprRoot()->nthChild(i);
            std::stringstream str;
            str << _document->getReprRoot()->nthChild(i)->firstChild()->content();
            std::string sel;

            /**
              * If a selector without any style attribute content is added, the
              * value is set to empty so that its corresponding XML repr is added.
              */
            while(std::getline(str, sel, '\n')) {
                std::vector<SPObject *>objVec;
                REMOVE_SPACES(sel);
                if (!sel.empty()) {
                    key = strtok(strdup(sel.c_str()), "{");
                    _selectorName = key;
                    char *temp = strtok(NULL, "}");
                    if (strtok(temp, "}") != NULL) {
                        value = strtok(temp, "}");
                    }

                    std::stringstream ss(key);
                    std::string token;
                    std::size_t found = key.find(",");
                    if (found!=std::string::npos) {
                        while(std::getline(ss, token, ',')) {
                            REMOVE_SPACES(token);

                            if (strcmp(token.substr(0,1).c_str(), ".") == 0) {
                                token.erase(0, token.find_first_not_of('.'));
                                for (unsigned i = 0; i < _num; ++i) {
                                    if (_document->getReprRoot()->
                                            nthChild(i)->attribute("class") &&
                                            _document->getReprRoot()->nthChild(i)
                                            ->attribute("class") == token) {
                                        selId = _document->getReprRoot()->nthChild(i)
                                                ->attribute("id");
                                        objVec.push_back( _document->getObjectById(selId));
                                    }
                                }
                            }
                            else if (strcmp(token.substr(0,1).c_str(), "#") == 0) {
                                token.erase(0, token.find_first_not_of('#'));
                                selId = token;
                                objVec.push_back(_document->getObjectById(selId));
                            }
                            else {
                                std::cout << "simple text" << std::endl;
                            }
                        }
                    }
                    else {
                        REMOVE_SPACES(key);
                        std::string tkn = key;
                        if (strcmp(tkn.substr(0,1).c_str(), ".") == 0) {
                            tkn.erase(0, tkn.find_first_not_of('.'));
                            for (unsigned i = 0; i < _num; ++i) {
                                if (_document->getReprRoot()->
                                        nthChild(i)->attribute("class") &&
                                        _document->getReprRoot()->nthChild(i)
                                        ->attribute("class") == tkn) {
                                    selId = _document->getReprRoot()->nthChild(i)
                                            ->attribute("id");
                                    objVec.push_back(_document->getObjectById(selId));
                                }
                            }
                        }
                        else if (strcmp(tkn.substr(0,1).c_str(), "#") == 0) {
                            tkn.erase(0, key.find_first_not_of('#'));
                            selId = tkn;
                            objVec.push_back(_document->getObjectById(selId));
                        }
                        else {
                            std::cout << "simple text" << std::endl;
                        }
                    }
                    _selectorValue = _selectorName + "{" + value + "}\n";
                    _selectorVec.push_back(std::make_pair(std::make_pair(key, objVec),
                                                          _selectorValue));
                }
            }
        }
    }
    return _selectorVec;
}

/**
 * @brief StyleDialog::_populateTree
 * @param _selVec
 * This function populates the treeview with selectors available in the
 * stylesheet.
 */
std::string StyleDialog::_populateTree(std::vector<_selectorVecType> _selVec)
{
    _selectorVec = _selVec;
    std::string selectorValue;

    for(unsigned it = 0; it < _selectorVec.size(); ++it) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_mColumns._selectorLabel] = _selectorVec[it].first.first;
        row[_mColumns._colAddRemove] = true;
        row[_mColumns._colObj] = _selectorVec[it].first.second;
        std::string selValue = _selectorVec[it].second;
        selectorValue.append(selValue.c_str());
    }

    if (_selectorVec.size() > 0) {
        del->set_sensitive(true);
    }

    return selectorValue;
}

/**
 * @brief StyleDialog::_handleButtonEvent
 * @param event
 * @return
 * This function handles the event when '+' button in front of a selector name
 * is clicked. The selected objects (if any) is added to the selector as a child
 * in the treeview.
 */
bool StyleDialog::_handleButtonEvent(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        Gtk::TreeViewColumn *col = 0;
        Gtk::TreeModel::Path path;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if (_treeView.get_path_at_pos(x, y, path, col, x2, y2)) {
            if (col == _treeView.get_column(0)) {
                if (_desktop->selection) {
                    std::vector<SPObject *>sel = _desktop->selection->list();
                    for (unsigned i = 0; i < sel.size(); ++i) {
                        SPObject *obj = sel[i];
                        Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =
                                _treeView.get_selection();
                        Gtk::TreeModel::iterator iter = refTreeSelection->
                                get_selected();

                        if (iter)
                        {
                            path = _treeView.get_model()->get_path(iter);
                            int i = atoi(path.to_string().c_str());
                            Gtk::TreeModel::Row row = *iter;
                            std::string childStyle;

                            if (_selectorVec.size() != 0) {
                                if (!row.parent()) {
                                    Gtk::TreeModel::Row childrow;
                                    childrow = *(_store->append(row->children()));
                                    std::cout << "_store->children()  " << _store->children()
                                                 ->children().size() << std::endl;

                                    childrow[_mColumns._selectorLabel] = obj->getId();
                                    childrow[_mColumns._colAddRemove] = false;
                                    childrow[_mColumns._colObj] = _desktop->selection->list();
                                    childStyle = "#" + std::string(obj->getId()) + "{" +
                                            std::string(obj->getAttribute("style")) + "}\n";
                                }
                            }

                            _selectorVec.push_back(std::make_pair(std::make_pair
                                                                  (obj->getId(),
                                                                   sel),
                                                                  childStyle));
                        }
                        if (_styleElementNode()) {
                            _styleChild = _styleElementNode();
                            _updateStyleContent();
                        }
                        else if (_styleExists && !_newDrawing) {
                            _updateStyleContent();
                        }
                    }
                }
            }
        }
    }
    return false;
}

/**
 * @brief StyleDialog::_selected_row_callback
 * @param path
 * This function selects objects in the drawing corresponding to the selector
 * selected in the treeview.
 */
void StyleDialog::_selectedRowCallback(const Gtk::TreeModel::Path& path,
                                         Gtk::TreeViewColumn*  column )
{
    _desktop->selection->clear();
    if (column == _treeView.get_column(1)) {
        Gtk::TreeModel::iterator iter = _store->get_iter(path);
        if (iter) {
            Gtk::TreeModel::Row row = *iter;
            Gtk::TreeModel::Children children = row.children();
            std::vector<SPObject *> objVec = row[_mColumns._colObj];
            for (unsigned i = 0; i < objVec.size(); ++i) {
                SPObject *obj = objVec[i];
                _desktop->selection->add(obj);
            }
            if (children) {
                _checkAllChildren(children);
            }
        }
    }
}

/**
 * @brief StyleDialog::_checkAllChildren
 * @param children
 * This function iterates children of the row selected in treeview and selects
 * the objects corresponding to any selector in child rows.
 */
void StyleDialog::_checkAllChildren(Gtk::TreeModel::Children& children)
{
    for (Gtk::TreeModel::Children::iterator iter = children.begin();
         iter!= children.end(); ++iter) {
        Gtk::TreeModel::Row childrow = *iter;
        std::vector<SPObject *> objVec = childrow[_mColumns._colObj];
        for (unsigned i = 0; i < objVec.size(); ++i) {
            SPObject *obj = objVec[i];
            _desktop->selection->add(obj);
        }
    }
}

/**
 * @brief StyleDialog::_selectRow
 * This function selects the rows in treeview corresponding to an object selected
 * in the drawing.
 */
void StyleDialog::_selectRow(Selection */*sel*/)
{
    if (!_desktop->selection->list().empty()) {
        SPObject *obj;
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        for (unsigned i = 0; i < selected.size(); ++i) {
            obj = selected.at(i);
        }

        Gtk::TreeModel::Children children = _store->children();
        for(Gtk::TreeModel::Children::iterator iter = children.begin();
            iter != children.end(); ++iter) {
            Gtk::TreeModel::Row row = *iter;
            std::vector<SPObject *> objVec = row[_mColumns._colObj];
            std::vector<SPObject *> childObjVec;
            Gtk::TreeModel::Row childRow;
            if (row.children()) {
                for(Gtk::TreeModel::Children::iterator it = row.children().begin();
                    it != row.children().end(); ++it) {
                    childRow = *it;
                    childObjVec = childRow[_mColumns._colObj];
                }
            }
            for (unsigned i = 0; i < objVec.size(); ++i) {
                if (obj->getId() == objVec[i]->getId()) {
                    _treeView.get_selection()->select(row);
                }
            }
            for (unsigned j = 0; j < childObjVec.size(); ++j) {
                if (obj->getId() == childObjVec[j]->getId()) {
                    _treeView.get_selection()->select(childRow);
                }
            }
        }
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
