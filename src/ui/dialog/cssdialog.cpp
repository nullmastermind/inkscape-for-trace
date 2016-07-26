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

#include "cssdialog.h"
#include "ui/widget/addtoicon.h"
#include "widgets/icon.h"
#include "verbs.h"
#include "sp-object.h"
#include "selection.h"
#include "xml/attribute-record.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief CssDialog::_styleButton
 * @param btn
 * @param iconName
 * @param tooltip
 * This function sets the style of '+'button at the bottom of dialog.
 */
void CssDialog::_styleButton(Gtk::Button& btn, char const* iconName,
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
 * A treeview whose each row corresponds to a CSS property of selector selected.
 * TODO: Further, buttons to add and delete properties will be added.
 */
CssDialog::CssDialog():
    UI::Widget::Panel("", "/dialogs/css", SP_VERB_DIALOG_CSS),
    _desktop(0)
{
    set_size_request(20, 15);
    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _treeView.set_headers_visible(false);
    _scrolledWindow.add(_treeView);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _store = Gtk::ListStore::create(_cssColumns);
    _treeView.set_model(_store);

    Inkscape::UI::Widget::AddToIcon * addRenderer = manage(
                new Inkscape::UI::Widget::AddToIcon() );
    addRenderer->property_active() = false;

    int addCol = _treeView.append_column("Unset Property", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if ( col ) {
        col->add_attribute( addRenderer->property_active(), _cssColumns._colUnsetProp);
    }
    _textRenderer = Gtk::manage(new Gtk::CellRendererText());
    _textRenderer->property_editable() = true;

    int nameColNum = _treeView.append_column("Property", *_textRenderer) - 1;
    _propCol = _treeView.get_column(nameColNum);

    create = manage(new Gtk::Button());
    _styleButton(*create, "list-add", "Add a new property");

    _mainBox.pack_end(_buttonBox, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(*create, Gtk::PACK_SHRINK);

    _getContents()->pack_start(_mainBox, Gtk::PACK_EXPAND_WIDGET);

    _targetDesktop = getDesktop();
    setDesktop(_targetDesktop);

    create->signal_clicked().connect(sigc::mem_fun(*this, &CssDialog::_addProperty));
}

CssDialog::~CssDialog()
{
    setDesktop(NULL);
}

void CssDialog::setDesktop( SPDesktop* desktop )
{
    _desktop = desktop;
}

/**
 * @brief CssDialog::_addProperty
 * This function is a slot to signal_clicked for '+' button at the bottom of CSS
 * panel. A new row is added, double clicking which text for new property can be
 * added. _newProperty is set to true in which case the value is appended.
 */
void CssDialog::_addProperty()
{
    _propRow = *(_store->append());
    _newProperty = true;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
