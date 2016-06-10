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
#include "widgets/icon.h"
#include "verbs.h"
#include "sp-object.h"
#include "selection.h"
#include "xml/attribute-record.h"

using Inkscape::Util::List;
using Inkscape::XML::AttributeRecord;

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
    _treeView.append_column("Selector Name", _mColumns._selectorLabel);

    Gtk::Button* create = manage( new Gtk::Button() );
    _styleButton(*create, "list-add", "Add a new CSS Selector");
    create->signal_clicked().connect(sigc::mem_fun(*this,
                                                   &StyleDialog::_addSelector));

    Gtk::Button* del = manage( new Gtk::Button() );
    _styleButton(*del, "list-remove", "Remove a CSS Selector");
    del->set_sensitive(false);

    _mainBox.pack_start(_buttonBox, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(*create, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(*del, Gtk::PACK_SHRINK);

    SPDesktop* targetDesktop = getDesktop();
    setDesktop(targetDesktop);

    /**
     * @brief document
     * If an existing document is opened, its XML representation is obtained
     * and if it contains any style element with a style selector, the selector
     * name and its value is extracted and saved to a map. This map is then used
     * to populate the treeview with the already existing selectors.
     */
    SPDocument * document = targetDesktop->doc();
    unsigned num = document->getReprRoot()->childCount();

    std::string key, value;
    std::map<std::string, std::string> selMap;

    for ( unsigned i = 0; i < num; ++i )
    {
        if ( std::string(document->getReprRoot()->nthChild(i)->name()) == "svg:style" )
        {
            char *str = strdup(document->getReprRoot()->nthChild(i)->firstChild()
                               ->content());
            key = strtok(str, " ");
            value = strtok(NULL, "");
            selMap[key] = value;
        }
    }
    _populateTree(selMap);
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

    /**
     * The selector name objects is set to the text that the user sets in the
     * entrybox. If the attribute does not exist, it is
     * created. In case the attribute already has a value, the new value entered
     * is appended to the values.
     */
    if ( _desktop->selection ) {
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        std::string selectorValue;
        for ( unsigned i = 0; i < selected.size(); ++i ) {
            SPObject *obj = selected.at(i);

            std::string style = std::string(obj->getRepr()->attribute("style"));
            style = row[_mColumns._selectorLabel] + ";" + style;

            for ( List<AttributeRecord const> iter = obj->getRepr()->attributeList();
                  iter; ++iter ) {
                gchar const * property = g_quark_to_string(iter->key);
                gchar const * value = iter->value;

                if ( std::string(property) == "style" )
                {
                    selectorValue = row[_mColumns._selectorLabel] + "{" +
                            "\n" + std::string(value) + "\n" + "}";
                }
            }

            /**
             * @brief root
             * A new style element is added to the document with value obtained
             * from selectorValue above.
             */
            Inkscape::XML::Node *root = obj->getRepr()->document()->root();
            Inkscape::XML::Node *newChild = obj->getRepr()->document()
                    ->createElement("svg:style");

            Inkscape::XML::Node *smallChildren = obj->getRepr()->document()
                    ->createTextNode(selectorValue.c_str());

            newChild->appendChild(smallChildren);
            Inkscape::GC::release(smallChildren);

            root->addChild(newChild, NULL);
            Inkscape::GC::release(newChild);

            _selectorMap[selectorName] = selectorValue;
        }
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
 * @brief StyleDialog::_populateTree
 * @param _selMap
 * This function populates the treeview with selectors available in the
 * stylesheet.
 */
void StyleDialog::_populateTree(std::map<std::string, std::string> _selMap)
{
    std::map<std::string, std::string> _selectMap = _selMap;

    for( std::map<std::string, std::string>::iterator it = _selectMap.begin();
         it != _selectMap.end(); ++it ) {
        Gtk::TreeModel::Row row = *(_store->append());
        row[_mColumns._selectorLabel] = it->first;
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
