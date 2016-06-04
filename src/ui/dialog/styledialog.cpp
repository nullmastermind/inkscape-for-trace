#include "styledialog.h"
#include "widgets/icon.h"
#include "verbs.h"
#include "sp-object.h"
#include "selection.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

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
    _treeView.append_column("Selector Number", _mColumns._selectorNumber);
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

void StyleDialog::_addSelector()
{
    Gtk::TreeModel::Row row = *(_store->append());

    /**
     * On clicking '+' button, an entrybox with default text opens up. If an
     * object is already selected, 'class' attribute with value in the entry
     * is added to the selected object.
     */
    Gtk::Dialog *textDialogPtr =  new Gtk::Dialog();
    Gtk::Entry *textEditPtr = manage ( new Gtk::Entry() );
    textDialogPtr->add_button("Add", Gtk::RESPONSE_OK);
    textDialogPtr->get_vbox()->pack_start(*textEditPtr, Gtk::PACK_SHRINK);
    textEditPtr->set_text("Class1");

    textDialogPtr->set_size_request(200, 100);
    textDialogPtr->show_all();
    int result = textDialogPtr->run();
    static int number = 1;

    switch (result) {
    case Gtk::RESPONSE_OK:
        textDialogPtr->hide();
        row[_mColumns._selectorNumber] = number;
        row[_mColumns._selectorLabel] = textEditPtr->get_text();
        number++;
        break;
    default:
        break;
    }

    if (_desktop->selection) {
        std::vector<SPObject*> selected = _desktop->getSelection()->list();
        for (int i = 0; i < selected.size(); ++i ) {
            SPObject *obj = selected.at(i);
            obj->setAttribute("class", textEditPtr->get_text());
        }
    }
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
