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
    add(_mainBox);

    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _treeView.set_headers_visible(false);
    _scrolledWindow.add(_treeView);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _store = Gtk::ListStore::create(_mColumns);
    _treeView.set_model(_store);

    Inkscape::UI::Widget::AddToIcon * addRenderer = manage(
                new Inkscape::UI::Widget::AddToIcon() );
    addRenderer->property_active() = true;
    _treeView.append_column("type", *addRenderer);
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

    _mainBox.pack_start(_buttonBox, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(*create, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(*del, Gtk::PACK_SHRINK);

    _targetDesktop = getDesktop();
    setDesktop(_targetDesktop);

    /**
     * @brief document
     * If an existing document is opened, its XML representation is obtained
     * and is then used to populate the treeview with the already existing
     * selectors in the style element.
     */
    _styleExists = false;
    _sValue = _populateTree(_getSelectorVec());
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
    Gtk::TreeModel::Row row = *(_store->append());

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
    if ( _desktop->selection->isEmpty() )
        textEditPtr->set_text("Class1");
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
    std::string selectorName = "";
    if ( textEditPtr->get_text().at(0) == '#' ||
         textEditPtr->get_text().at(0) == '.' )
        selectorName = textEditPtr->get_text();
    else
        selectorName = "." + textEditPtr->get_text();

    switch (result) {
    case Gtk::RESPONSE_OK:
        textDialogPtr->hide();
        row[_mColumns._selectorLabel] = selectorName;
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
    if ( _desktop->selection ) {
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        std::string selectorValue;
        for ( unsigned i = 0; i < selected.size(); ++i ) {
            SPObject *obj = selected.at(i);
            std::string style;

            if (obj->getRepr()->attribute("style"))
            {
                for ( List<AttributeRecord const> iter = obj->getRepr()->attributeList();
                      iter; ++iter ) {
                    gchar const * property = g_quark_to_string(iter->key);
                    gchar const * value = iter->value;

                    if ( std::string(property) == "style" )
                    {
                        selectorValue = row[_mColumns._selectorLabel] + "{"
                                + std::string(value) + "}" + "\n";
                    }
                }
            }

            else
            {
                style = " ";
                obj->getRepr()->setAttribute("style", style);
            }

            if ( strcmp(selectorName.substr(0,1).c_str(), ".") == 0 ){
                if (!obj->getRepr()->attribute("class"))
                    obj->getRepr()->setAttribute("class", textEditPtr->get_text()
                                                 .erase(0,1));
                else
                    obj->getRepr()->setAttribute("class", std::string(obj->
                                                                      getRepr()->
                                                                      attribute("class"))
                                                 + " " + textEditPtr->get_text()
                                                 .erase(0,0));
            }

            /**
             * @brief root
             * A new style element is added to the document with value obtained
             * from selectorValue above. If style element already exists, then
             * the new selector's content is appended to its previous content.
             */
            unsigned num = _document->getReprRoot()->childCount();
            Inkscape::XML::Node *styleChild;
            for ( unsigned i = 0; i < num; ++i )
            {
                if ( std::string(_document->getReprRoot()->nthChild(i)->name())
                     == "svg:style" )
                {
                    _styleExists = true;
                    styleChild = _document->getReprRoot()->nthChild(i);
                    break;
                }
                else
                    _styleExists = false;
            }

            if ( _styleExists )
            {
                _sValue = _sValue + selectorValue;
                styleChild->firstChild()->setContent(_sValue.c_str());
            }
            else
            {
                _sValue = selectorValue;
                Inkscape::XML::Node *root = obj->getRepr()->document()->root();
                Inkscape::XML::Node *newChild = obj->getRepr()->document()
                        ->createElement("svg:style");
                Inkscape::XML::Node *smallChildren = obj->getRepr()->document()
                        ->createTextNode(selectorValue.c_str());

                newChild->appendChild(smallChildren);
                Inkscape::GC::release(smallChildren);

                root->addChild(newChild, NULL);
                Inkscape::GC::release(newChild);
            }
            _selectorVec.push_back(std::make_pair(selectorName, selectorValue));
        }
    }
}

/**
 * @brief StyleDialog::_delSelector
 * This function deletes selector when '-' at the bottom is clicked. Currently
 * selectors are deleted from StyleDialog and not from repr of the document.
 */
void StyleDialog::_delSelector()
{
    Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = _treeView.get_selection();
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    std::vector<std::pair<std::string, std::string> >selVec = _getSelectorVec();

    if (iter)
    {
        Gtk::TreeModel::Row row = *iter;
//        for( unsigned it = 0; it < selMap.size(); ++it ) {
//            std::string key = strtok(strdup(_document->getReprRoot()
//                                            ->nthChild(it)
//                                            ->firstChild()->content()), "{");

            /**
             * @brief toDeleteNode
             * The node to be deleted is obtained using nthChild and the selector
             * name and corresponding style node from the repr of document are
             * saved to a map. Keys of this map and labels of selected rows of
             * dialog are compared and deleted.
             */
//            Inkscape::XML::Node *toDeleteNode = _document->getReprRoot()->nthChild(it);
//            std::map<std::string, Inkscape::XML::Node*>toDeleteMap;
//            toDeleteMap[key] = toDeleteNode;

//            for( std::map<std::string, Inkscape::XML::Node*>::iterator i =
//                 toDeleteMap.begin(); i != toDeleteMap.end(); ++i ) {
//                if( row[_mColumns._selectorLabel] == i->first)
//                {
//                    std::cout << i->second->name() << std::endl;
//                /**
//                  This should work but uncommenting it causes a crash currently.
//                  */
//                    //_document->getReprRoot()->removeChild(toDeleteNode);
//                }
//            }
//        }
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
    _document = _targetDesktop->doc();
    unsigned num = _document->getReprRoot()->childCount();

    std::string key, value;
    std::vector<std::pair<std::string, std::string> > selVec;

    for ( unsigned i = 0; i < num; ++i )
    {
        if ( std::string(_document->getReprRoot()->nthChild(i)->name()) == "svg:style" )
        {
            std::stringstream str;
            str << _document->getReprRoot()->nthChild(i)->firstChild()->content();
            std::string sel;

            while(std::getline(str, sel, '\n')){
                REMOVE_SPACES(sel);
                if (!sel.empty())
                {
                    key = strtok(strdup(sel.c_str()), "{");
                    value = strtok(NULL, "}");
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

    for( unsigned it = 0; it < _selectVec.size(); ++it ) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_mColumns._selectorLabel] = _selectVec[it].first;
        std::string selValue = _selectVec[it].first + " { "
                + _selectVec[it].second + " }" + "\n";
        selectorValue.append(selValue.c_str());
    }

    if (_selectVec.size() > 0)
        del->set_sensitive(true);

    return selectorValue;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
