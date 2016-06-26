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

#define REMOVE_SPACES(x) x.erase(std::remove(x.begin(), x.end(), ' '), x.end());

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
    _sValue = _populateTree(_getSelectorVec());

    _treeView.signal_button_press_event().connect(sigc::mem_fun(*this,
                                                                &StyleDialog::
                                                                _handleButtonEvent),
                                                  false);
}

StyleDialog::~StyleDialog()
{
    setDesktop(NULL);
}

void StyleDialog::setDesktop( SPDesktop* desktop )
{
    Panel::setDesktop(desktop);
    _desktop = Panel::getDesktop();
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
        if (textEditPtr->get_text().at(0) == '#' ||
             textEditPtr->get_text().at(0) == '.') {
            _selectorName = textEditPtr->get_text();
        }
        else {
            _selectorName = "." + textEditPtr->get_text();
        }
    }
    else {
        _selectorName = ".Class1";
    }

    switch (result) {
    case Gtk::RESPONSE_OK:
        textDialogPtr->hide();
        _row[_mColumns._selectorLabel] = _selectorName;
        _row[_mColumns._colAddRemove] = true;
        break;
    default:
        break;
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
    if (!_desktop->getSelection()->list().empty()) {
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        for (unsigned i = 0; i < selected.size(); ++i) {
            obj = selected.at(i);
            std::string style;

            if (obj->getRepr()->attribute("style")) {
                for (List<AttributeRecord const> iter = obj->getRepr()->attributeList();
                      iter; ++iter) {
                    gchar const * property = g_quark_to_string(iter->key);
                    gchar const * value = iter->value;

                    if (std::string(property) == "style") {
                        _selectorValue = _row[_mColumns._selectorLabel] + "{"
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
    }

    /**
     * @brief root
     * A new style element is added to the document with value obtained
     * from selectorValue above. If style element already exists, then
     * the new selector's content is appended to its previous content.
     */
    for (unsigned i = 0; i < _num; ++i) {
        if (std::string(_document->getReprRoot()->nthChild(i)->name())
             == "svg:style") {
            _styleExists = true;
            _styleChild = _document->getReprRoot()->nthChild(i);
            break;
        }
        else {
            _styleExists = false;
        }
    }

    if (_styleExists) {
        _sValue = _sValue + _selectorValue;
        _styleChild->firstChild()->setContent(_sValue.c_str());
    }
    else {
        _sValue = _selectorValue;
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
    _selectorVec.push_back(std::make_pair(_selectorName, _selectorValue));
}

/**
 * @brief StyleDialog::_delSelector
 * This function deletes selector when '-' at the bottom is clicked. The index
 * of selected row is obtained and the corresponding selector and its values are
 * deleted from the selector vector. Then _sValue's content is reset and contains
 * only selectors remaining in the selVec (or treeview).
 */
void StyleDialog::_delSelector()
{
    Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    std::vector<std::pair<std::string, std::string> >selVec = _getSelectorVec();
    Gtk::TreeModel::Path path;

    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        path = _treeView.get_model()->get_path(iter);
        int i = atoi(path.to_string().c_str());

        if (selVec.size() != 0) {
            selVec.erase(selVec.begin()+i);
            _sValue.clear();

            for (unsigned i = 0; i < selVec.size(); ++i) {
                std::string selValue = (selVec[i].first + "{"
                                        + selVec[i].second + " }\n");
                _sValue.append(selValue.c_str());
            }

            /**
              * Only if a value exists in _sValue and there is a _styleChild
              * which contains the style element, then the content in style
              * element is updated else the _styleChild is set and its content
              * is set to an empty string.
              */
            if (!_sValue.empty() && _styleChild) {
                _styleChild->firstChild()->setContent(_sValue.c_str());
            }
            else {
                for (unsigned i = 0; i < _num; ++i) {
                    if (std::string(_document->getReprRoot()->nthChild(i)->name())
                         == "svg:style") {
                        _styleChild = _document->getReprRoot()->nthChild(i);
                    }
                }
                _styleChild->firstChild()->setContent("");
            }
        }
        _store->erase(row);
    }
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
 * element.
 */
std::vector<std::pair<std::string, std::string> >StyleDialog::_getSelectorVec()
{
    std::string key, value;
    std::vector<std::pair<std::string, std::string> > selVec;

    for (unsigned i = 0; i < _num; ++i) {
        if (std::string(_document->getReprRoot()->nthChild(i)->name()) == "svg:style") {
            std::stringstream str;
            str << _document->getReprRoot()->nthChild(i)->firstChild()->content();
            std::string sel;

            /**
              * If a selector without any style attribute content is added, the
              * value is set to empty so that its corresponding XML repr is added.
              */
            while(std::getline(str, sel, '\n')) {
                REMOVE_SPACES(sel);
                if (!sel.empty()) {
                    key = strtok(strdup(sel.c_str()), "{");
                    char *temp = strtok(NULL, "}");
                    if (strtok(temp, "}") != NULL) {
                        value = strtok(temp, "}");
                    }
                    selVec.push_back(std::make_pair(key, value));
                }
            }
        }
    }
    return selVec;
}

/**
 * @brief StyleDialog::_populateTree
 * @param _selVec
 * This function populates the treeview with selectors available in the
 * stylesheet.
 */
std::string StyleDialog::_populateTree(std::vector<std::pair<std::string,
                                       std::string> > _selVec)
{
    std::vector<std::pair<std::string, std::string> > _selectVec = _selVec;
    std::string selectorValue;

    for(unsigned it = 0; it < _selectVec.size(); ++it) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_mColumns._selectorLabel] = _selectVec[it].first;
        std::string selValue = _selectVec[it].first + " { "
                + _selectVec[it].second + " }" + "\n";
        selectorValue.append(selValue.c_str());
    }

    if (_selectVec.size() > 0) {
        del->set_sensitive(true);
    }

    for (unsigned i = 0; i < _num; ++i) {
        if (std::string(_document->getReprRoot()->nthChild(i)->name())
             == "svg:style") {
            _styleChild = _document->getReprRoot()->nthChild(i);
        }
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
 * TODO: Appending children in XML repr.
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

                        if (iter) {
                            Gtk::TreeModel::Row row = *iter;
                            typedef Gtk::TreeModel::Children type_children;
                            type_children children = row->children();
                            Gtk::TreeModel::Row childrow;

                            if (children.size() == 0 && row->parent() == 0) {
                                childrow = *(_store->append(row->children()));
                                childrow[_mColumns._selectorLabel] = obj->getId();
                                childrow[_mColumns._colAddRemove] = false;
                            }
                            else {
                                for(type_children::iterator it = children.begin();
                                    it != children.end(); ++it) {
                                    Gtk::TreeModel::iterator i = it;
                                    path = _treeView.get_model()->get_path(i);

                                    if((*it)->get_value(_mColumns._selectorLabel)
                                            != obj->getId()) {
                                        childrow = *(_store->append(row.children()));
                                        childrow[_mColumns._selectorLabel] = obj->getId();
                                        childrow[_mColumns._colAddRemove] = false;
                                    }
                                    else {
                                        std::cout << "Do nothing" << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
