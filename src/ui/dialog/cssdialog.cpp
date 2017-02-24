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
 * Constructor
 * A treeview whose each row corresponds to a CSS property of selector selected.
 * New CSS property can be added by clicking '+' at bottom of the CSS pane. '-'
 * in front of the CSS property row can be clicked to delete the CSS property.
 * Besides clicking on an already selected property row makes the property editable
 * and clicking 'Enter' updates the property with changes reflected in the
 * drawing.
 */
CssDialog::CssDialog():
    UI::Widget::Panel("", "/dialogs/css", SP_VERB_DIALOG_CSS),
    _desktop(0)
{
    set_size_request(20, 15);
    _mainBox.pack_start(_scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    _treeView.set_headers_visible(true);
    _scrolledWindow.add(_treeView);
    _scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    _store = Gtk::ListStore::create(_cssColumns);
    _treeView.set_model(_store);

    Inkscape::UI::Widget::AddToIcon * addRenderer = manage(new Inkscape::UI::Widget::AddToIcon());
    addRenderer->property_active() = false;

    int addCol = _treeView.append_column("", *addRenderer) - 1;
    Gtk::TreeViewColumn *col = _treeView.get_column(addCol);
    if (col) {
        col->add_attribute(addRenderer->property_active(), _cssColumns._colUnsetProp);
    }

    _propRenderer = Gtk::manage(new Gtk::CellRendererText());
    _propRenderer->property_editable() = true;
    int nameColNum = _treeView.append_column("CSS Property", *_propRenderer) - 1;
    _propCol = _treeView.get_column(nameColNum);
    if (_propCol) {
      _propCol->add_attribute(_propRenderer->property_text(), _cssColumns._propertyLabel);
    }

    _sheetRenderer = Gtk::manage(new Gtk::CellRendererText());
    _sheetRenderer->property_editable() = true;
    int sheetColNum = _treeView.append_column("Style Sheet", *_sheetRenderer) - 1;
    _sheetCol = _treeView.get_column(sheetColNum);
    if (_sheetCol) {
      _sheetCol->add_attribute(_sheetRenderer->property_text(), _cssColumns._styleSheetVal);
    }

    _attrRenderer = Gtk::manage(new Gtk::CellRendererText());
    _attrRenderer->property_editable() = false;
    int attrColNum = _treeView.append_column("Style Attribute", *_attrRenderer) - 1;
    _attrCol = _treeView.get_column(attrColNum);
    if (_attrCol) {
      _attrCol->add_attribute(_attrRenderer->property_text(), _cssColumns._styleAttrVal);
    }

    _styleButton(_buttonAddProperty, "list-add", "Add a new property");

    _mainBox.pack_end(_buttonBox, Gtk::PACK_SHRINK);
    _buttonBox.pack_start(_buttonAddProperty, Gtk::PACK_SHRINK);

    _getContents()->pack_start(_mainBox, Gtk::PACK_EXPAND_WIDGET);

    setDesktop(getDesktop());

    _buttonAddProperty.signal_clicked().connect(sigc::mem_fun(*this, &CssDialog::_addProperty));
}


/**
 * @brief CssDialog::~CssDialog
 * Class destructor
 */
CssDialog::~CssDialog()
{
    setDesktop(NULL);
}


/**
 * @brief CssDialog::setDesktop
 * @param desktop
 * This function sets the 'desktop' for the CSS pane.
 */
void CssDialog::setDesktop(SPDesktop* desktop)
{
    _desktop = desktop;
}


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
    btn.set_tooltip_text(tooltip);
}


/**
 * @brief CssDialog::_addProperty
 * This function is a slot to signal_clicked for '+' button at the bottom of CSS
 * panel. A new row is added, double clicking which text for new property can be
 * added.
 */
void CssDialog::_addProperty()
{
    _propRow = *(_store->append());
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape
