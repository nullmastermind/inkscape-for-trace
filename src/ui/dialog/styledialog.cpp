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

    _paned.pack1(_mainBox, Gtk::SHRINK);
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
    _treeView.set_expander_column(*(_treeView.get_column(1)));

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

    _getContents()->pack_start(_paned, Gtk::PACK_EXPAND_WIDGET);

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
    _selectorValue = _populateTree(_getSelectorVec());

    _treeView.signal_button_press_event().connect(sigc::mem_fun(*this,
                                                                &StyleDialog::
                                                                _handleButtonEvent),
                                                  false);

    _treeView.signal_button_press_event().connect_notify(sigc::mem_fun
                                                         (*this, &StyleDialog::
                                                          _buttonEventsSelectObjs),
                                                         false);

    _cssPane = new CssDialog;
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
            if (!obj->getRepr()->attribute("style")) {
                obj->getRepr()->setAttribute("style", NULL);
            }

            if (obj->getAttribute("style") == NULL) {
                _selectorValue = _selectorName + "{" + "}" + "\n";
            }
            else {
                _selectorValue = _selectorName + "{"
                        + obj->getAttribute("style") + "}" + "\n";
            }

            if (_selectorName[0] == '.') {
                if (!obj->getRepr()->attribute("class")) {
                    obj->getRepr()->setAttribute("class", textEditPtr->get_text()
                                                 .erase(0,1));
                }
                else {
                    obj->getRepr()->setAttribute("class", std::string(obj->
                                                                      getRepr()->
                                                                      attribute("class"))
                                                 + " " + textEditPtr->get_text()
                                                 .erase(0,1));
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
    inkSelector._selector = _selectorName;
    inkSelector._matchingObjs = objVec;
    inkSelector._xmlContent = _selectorValue;
    _selectorVec.push_back(inkSelector);

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
        styleContent = styleContent + _selectorVec[i]._xmlContent;
    }
    _styleChild->firstChild()->setContent(styleContent.c_str());
}

/**
 * @brief StyleDialog::_delSelector
 * This function deletes selector when '-' at the bottom is clicked. The index
 * of selected row is obtained and the corresponding selector and its values are
 * deleted from the selector vector. If a row has no parent, it is directly
 * erased from the vector along with its child rows. The style element is updated
 * accordingly.
 */
void StyleDialog::_delSelector()
{
    Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        std::string sel, key, value;
        std::vector<InkSelector>::iterator it;
        for (it = _selectorVec.begin(); it != _selectorVec.end();) {
            sel = (*it)._xmlContent;
            REMOVE_SPACES(sel);
            if (!sel.empty()) {
                key = strtok((char*)sel.c_str(), "{");
                REMOVE_SPACES(key);
                char *temp = strtok(NULL, "}");
                if (strtok(temp, "}") != NULL) {
                    value = strtok(temp, "}");
                }
            }

            Glib::ustring selectedRowLabel = row[_mColumns._selectorLabel];
            std::string matchSelector = selectedRowLabel;
            REMOVE_SPACES(matchSelector);
            if (key == matchSelector) {
                if (!row.children().empty()) {
                    for (Gtk::TreeModel::Children::iterator child = row.children().begin();
                         child != row.children().end(); ++child) {
                        Gtk::TreeModel::Row childrow = *child;
                        std::string childSel, childKey;
                        std::vector<InkSelector>::iterator i;
                        for (i = _selectorVec.begin(); i != _selectorVec.end();) {
                            childSel = (*i)._xmlContent;
                            REMOVE_SPACES(childSel);
                            if (!childSel.empty()) {
                                childKey = strtok((char*)childSel.c_str(), "{");
                                REMOVE_SPACES(childKey);
                            }
                            Glib::ustring selectedChildRowLabel =
                                    childrow[_mColumns._selectorLabel];
                            std::string matchChildSelector = selectedChildRowLabel;
                            REMOVE_SPACES(matchChildSelector);
                            if (childKey == matchChildSelector) {
                                i = _selectorVec.erase(i);
                            }
                            else {
                                ++i;
                            }
                        }
                    }
                }
                it = _selectorVec.erase(it);
                _store->erase(row);
            }
            else {
                ++it;
            }

            /**
              * The _stylechild is obtained which contains the style element and
              * the content in style element is updated. If _selectorVec is
              * empty, the style element is removed from the XML repr else
              * the content is updated simply using _updateStyleContent().
              */
            _styleChild = _styleElementNode();
            if (_selectorVec.size() == 0) {
                _document->getReprRoot()->removeChild(_styleChild);
                _styleExists = false;
            }
            else {
                _updateStyleContent();
            }
        }
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
    for (unsigned i = 0; i < _document->getReprRoot()->childCount(); ++i) {
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
std::vector<StyleDialog::InkSelector> StyleDialog::_getSelectorVec()
{
    for (unsigned i = 0; i < _document->getReprRoot()->childCount(); ++i) {
        if (std::string(_document->getReprRoot()->nthChild(i)->name()) == "svg:style") {
            _styleExists = true;
            _newDrawing = false;
            _styleChild = _document->getReprRoot()->nthChild(i);

            // Get content from first style element.
            std::string content = _styleChild->firstChild()->content();

            // Remove end-of-lines (check it works on Windoze).
            content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());

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
            std::vector<std::string> tokens = Glib::Regex::split_simple("[}{]", content);

            for (unsigned i = 0; i < tokens.size()-1; i += 2) {
                std::string selectors = tokens[i];
                REMOVE_SPACES(selectors); // Remove leading/trailing spaces

                /** Make a list of all objects that selector matches. This is
                 * currently limited to simple id, class, and element selectors.
                 * Expanding this would take integrating a true CSS parser.
                 */
                std::vector<SPObject *>objVec;

                // Split selector string into individual selectors (which are comma separated).
                std::vector<std::string> tokens2 = Glib::Regex::split_simple
                        ("\\s*,\\s*", selectors );

                for(unsigned i = 0; i < tokens2.size(); ++i) {
                    std::string token2 = tokens2[i];

                    // Find objects that match class selector
                    if (token2[0] == '.') {
                        token2.erase(0,1);
                        std::vector<SPObject *> objects = _document
                                ->getObjectsByClass(token2);
                        objVec.insert(objVec.end(), objects.begin(), objects.end());
                    }

                    // Find objects that match id selector
                    else if (token2[0] == '#') {
                        token2.erase(0,1);
                        SPObject * object = _document->getObjectById(token2);
                        if (object) {
                            objVec.push_back(object);
                        }
                    }

                    // Find objects that match element selector
                    else {
                        std::vector<SPObject *> objects = _document->
                                getObjectsByElement(token2);
                        objVec.insert(objVec.end(), objects.begin(), objects.end());
                    }
                }

                std::string values;
                // Check to make sure we do have a value to match selector.
                if ((i+1) < tokens.size()) {
                    values = tokens[i+1];
                } else {
                    std::cerr << "StyleDialog::_getSelectorVec: Missing values "
                                 "for last selector!" << std::endl;
                }

                _selectorValue = selectors + "{" + values + "}\n";
                inkSelector._selector = selectors;
                inkSelector._matchingObjs = objVec;
                inkSelector._xmlContent = _selectorValue;
                _selectorVec.push_back(inkSelector);
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
std::string StyleDialog::_populateTree(std::vector<InkSelector> _selVec)
{
    _selectorVec = _selVec;
    std::string selectorValue;

    for(unsigned it = 0; it < _selectorVec.size(); ++it) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_mColumns._selectorLabel] = _selectorVec[it]._selector;
        row[_mColumns._colAddRemove] = true;
        row[_mColumns._colObj] = _selectorVec[it]._matchingObjs;
        std::string selValue = _selectorVec[it]._xmlContent;
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
                Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =
                        _treeView.get_selection();
                Gtk::TreeModel::iterator iter = refTreeSelection->
                        get_selected();
                Gtk::TreeModel::Row row = *iter;

                /**
                  * This adds child rows to selected rows. If the parent row is
                  * a class selector, then the class attribute of object added
                  * to child row is appended with class in the parent row. The
                  * else below deletes objects from selectors when 'delete' button
                  * in front of child row is clicked. The class attribute is updated
                  * by removing the parent row's class selector name.
                  */
                if (!row.parent()) {
                    if (_desktop->selection) {
                        std::vector<SPObject *>sel = _desktop->selection->list();
                        for (unsigned i = 0; i < sel.size(); ++i) {
                            SPObject *obj = sel[i];
                            std::string childStyle;
                            if (iter) {
                                path = _treeView.get_model()->get_path(iter);
                                if (_selectorVec.size() != 0) {
                                    Gtk::TreeModel::Row childrow;
                                    childrow = *(_store->append(row->children()));
                                    childrow[_mColumns._selectorLabel] = "#" +
                                            std::string(obj->getId());
                                    childrow[_mColumns._colAddRemove] = false;
                                    childrow[_mColumns._colObj] = _desktop->selection->list();
                                    if (obj->getAttribute("style") != NULL) {
                                        childStyle = "#" + std::string(obj->getId()) + "{" +
                                                std::string(obj->getAttribute("style")) + "}\n";
                                    }
                                    else {
                                        childStyle = "#" + std::string(obj->getId())
                                                + "{" + "}\n";
                                    }
                                    Glib::ustring key = row[_mColumns._selectorLabel];
                                    if (key[0] == '.') {
                                        if (!obj->getRepr()->attribute("class")) {
                                            obj->setAttribute("class", key.erase(0,1));
                                        }
                                        else {
                                            obj->setAttribute("class", std::string
                                                              (obj->getRepr()->
                                                               attribute("class"))
                                                              + " " + key
                                                              .erase(0,1));
                                        }
                                    }
                                }
                            }

                            inkSelector._selector = obj->getId();
                            inkSelector._matchingObjs = sel;
                            inkSelector._xmlContent = childStyle;
                            _selectorVec.push_back(inkSelector);
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

                else {
                    std::string sel, key, value;
                    std::vector<InkSelector>::iterator it;
                    for (it = _selectorVec.begin(); it != _selectorVec.end(); ++it ) {
                        sel = (*it)._xmlContent;
                        REMOVE_SPACES(sel);
                        if (!sel.empty()) {
                            key = strtok((char*)sel.c_str(), "{");
                            REMOVE_SPACES(key);
                            char *temp = strtok(NULL, "}");
                            if (strtok(temp, "}") != NULL) {
                                value = strtok(temp, "}");
                            }
                        }
                    }
                    if (key == row[_mColumns._selectorLabel]) {
                        Gtk::TreeModel::Row parentRow = *(row.parent());
                        Glib::ustring parentKey = parentRow[_mColumns._selectorLabel];
                        if (parentKey[0] == '.') {
                            std::vector<SPObject *> objVec = row[_mColumns._colObj];
                            for (unsigned i = 0; i < objVec.size(); ++i) {
                                SPObject *obj = objVec[i];
                                std::string classAttr = std::string(obj->getRepr()
                                                                    ->attribute("class"));
                                std::size_t found = classAttr.find(parentKey.erase(0,1));
                                if (found != std::string::npos) {
                                    classAttr.erase(found, parentKey.length()+1);
                                    obj->getRepr()->setAttribute("class", classAttr);
                                }
                            }
                        }
                        _selectorVec.erase(it);
                    }

                    if (_styleChild) {
                        _updateStyleContent();
                    }
                    else {
                        _styleChild = _styleElementNode();
                        _updateStyleContent();
                    }
                    _store->erase(row);
                }
            }
        }
    }
    return false;
}

/**
 * @brief StyleDialog::_selectObjects
 * @param event
 * This function detects single or double click on a selector in any row. Single
 * click on a selector selects the matching objects. A double click on any
 * selector selects the matching objects as well as will open CSS dialog. It
 * calls _selectObjects to add objects to selection.
 */
void StyleDialog::_buttonEventsSelectObjs(GdkEventButton* event )
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        _selectObjects(x, y);
    }
    else if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        _selectObjects(x, y);

        //Open CSS dialog here.
        if (!_cssPane->get_visible()) {
            _paned.pack2(*_cssPane, Gtk::SHRINK);
            _cssPane->show_all();
        }

        Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
        Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
        if (iter) {
            Gtk::TreeModel::Row row = *iter;
            std::string sel, key, value;
            std::vector<InkSelector>::iterator it;
            for (it = _selectorVec.begin(); it != _selectorVec.end(); ++it) {
                sel = (*it)._xmlContent;
                REMOVE_SPACES(sel);
                if (!sel.empty()) {
                    key = strtok((char*)sel.c_str(), "{");
                    REMOVE_SPACES(key);
                    char *temp = strtok(NULL, "}");
                    if (strtok(temp, "}") != NULL) {
                        value = strtok(temp, "}");
                    }
                }

                Glib::ustring selectedRowLabel = row[_mColumns._selectorLabel];
                std::string matchSelector = selectedRowLabel;
                REMOVE_SPACES(matchSelector);

                if (key == matchSelector) {
                    _cssPane->_store->clear();
                    std::stringstream ss(value);
                    std::string token;
                    std::size_t found = value.find(";");
                    if (found!=std::string::npos) {
                        while(std::getline(ss, token, ';')) {
                            REMOVE_SPACES(token);
                            if (!token.empty()) {
                                _cssPane->_propRow = *(_cssPane->_store->append());
                                _cssPane->_propRow[_cssPane->_cssColumns._colUnsetProp] = false;
                                _cssPane->_propRow[_cssPane->_cssColumns._propertyLabel] = token;
                                _cssPane->_propCol->add_attribute(_cssPane->_textRenderer
                                                                  ->property_text(),
                                                                  _cssPane->_cssColumns
                                                                  ._propertyLabel);
                            }
                        }
                    }
                }
            }
            _cssPane->_textRenderer->signal_edited().connect(sigc::mem_fun(*this,
                                                                           &StyleDialog::
                                                                           _handleEdited));
        }
        else {
            _cssPane->_store->clear();
            _cssPane->hide();
        }
    }
}

/**
 * @brief StyleDialog::_handleEdited
 * @param path
 * @param new_text
 * This function edits CSS properties of the selector chosen. new_text is used
 * to update the property in XML repr. The value from selected selector is
 * obtained and modified as per value of new_text. Later _updateStyleContent() is
 * called to update XML repr and hence changes are reflected in the drawing too.
 */
void StyleDialog::_handleEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
    Gtk::TreeModel::iterator iterCss = _cssPane->_treeView.get_model()->get_iter(path);
    if (iterCss) {
        Gtk::TreeModel::Row row = *iterCss;
        row[_cssPane->_cssColumns._propertyLabel] = new_text;
        _cssPane->_editedProp = new_text;
    }

    Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        std::string sel, key, value;
        std::vector<InkSelector>::iterator it;
        for (it = _selectorVec.begin(); it != _selectorVec.end(); ++it) {
            sel = (*it)._xmlContent;
            REMOVE_SPACES(sel);
            if (!sel.empty()) {
                key = strtok((char*)sel.c_str(), "{");
                REMOVE_SPACES(key);
                char *temp = strtok(NULL, "}");
                if (strtok(temp, "}") != NULL) {
                    value = strtok(temp, "}");
                }
            }

            Glib::ustring selectedRowLabel = row[_mColumns._selectorLabel];
            std::string matchSelector = selectedRowLabel;
            REMOVE_SPACES(matchSelector);

            if (key == matchSelector) {
                std::stringstream ss(value);
                std::string token, editedToken;
                std::size_t found = value.find(";");
                if (found!=std::string::npos) {
                    while(std::getline(ss, token, ';')) {
                        REMOVE_SPACES(token);
                        if (!token.empty()) {
                            if (token.substr(0, token.find(":")) == _cssPane
                                    ->_editedProp.substr(0, _cssPane->_editedProp
                                                         .find(":"))) {
                                editedToken = _cssPane->_editedProp;
                                size_t startPos = value.find(token);
                                value.replace(startPos, token.length(), editedToken);
                                (*it)._xmlContent = key + "{" + value + "}\n";
                                _updateStyleContent();
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief StyleDialog::_selectObjects
 * @param eventX
 * @param eventY
 * This function selects objects in the drawing corresponding to the selector
 * selected in the treeview.
 */
void StyleDialog::_selectObjects(int eventX, int eventY)
{
    _desktop->selection->clear();
    Gtk::TreeViewColumn *col = _treeView.get_column(1);
    Gtk::TreeModel::Path path;
    int x = eventX;
    int y = eventY;
    int x2 = 0;
    int y2 = 0;
    if (_treeView.get_path_at_pos(x, y, path, col, x2, y2)) {
        if (col == _treeView.get_column(1)) {
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
    SPObject *obj = NULL;
    if (!_desktop->selection->list().empty()) {
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        obj = selected.back();
    }

    /**
      * If obj has some SPObject, then it is added to desktop's selection. If a
      * row in treeview has children, those rows are checked too against selected
      * object's id. If an object which is not present in any selector is selected,
      * the treeview's selections are unselected.
      */
    if (obj != NULL) {
        Gtk::TreeModel::Children children = _store->children();
        for(Gtk::TreeModel::Children::iterator iter = children.begin();
            iter != children.end(); ++iter) {
            Gtk::TreeModel::Row row = *iter;
            std::vector<SPObject *> objVec = row[_mColumns._colObj];
            std::vector<SPObject *> childObjVec;
            Gtk::TreeModel::Row childRow;

            for (unsigned i = 0; i < objVec.size(); ++i) {
                if (obj->getId() == objVec[i]->getId()) {
                    _treeView.get_selection()->select(row);
                }
            }

            if (row.children()) {
                for(Gtk::TreeModel::Children::iterator it = row.children().begin();
                    it != row.children().end(); ++it) {
                    childRow = *it;
                    childObjVec = childRow[_mColumns._colObj];
                }
                for (unsigned j = 0; j < childObjVec.size(); ++j) {
                    if (obj->getId() == childObjVec[j]->getId()) {
                        _treeView.get_selection()->select(childRow);
                    }
                }
            }
        }
    }
    else {
        _treeView.get_selection()->unselect_all();
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
