/** @file
 * @brief Color swatches dialog
 */
/* Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2005 Jon A. Cruz
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SEEN_DIALOGS_SWATCHES_H
#define SEEN_DIALOGS_SWATCHES_H

#include "ui/widget/panel.h"
#include "enums.h"
#include "sp-object.h"
#include "sp-item-group.h"
#include <gtk/gtk.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/table.h>
#include <gtkmm/box.h>
#include <gtkmm/menu.h>
#include <gtkmm/notebook.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treemodel.h>
#include "sp-gradient.h"
#include "xml/node-observer.h"

namespace Inkscape {
namespace UI {

namespace Dialogs {

/**
 * A panel that displays paint swatches.
 */
class SwatchesPanel : public Inkscape::UI::Widget::Panel
{
public:
    SwatchesPanel(gchar const* prefsPath = "/dialogs/swatches", bool showLabels = true);
    virtual ~SwatchesPanel();
    
    static SwatchesPanel& getInstance();

    virtual void setDesktop( SPDesktop* desktop );
    virtual SPDesktop* getDesktop() {return _currentDesktop;}

protected:

    virtual void _setDocument( SPDesktop* desktop, SPDocument *document );

private:
    class ModelColumns;
    class ModelColumnsDoc;
    
    class StopWatcher;
    class GradientWatcher;
    class SwatchWatcher;
    
    SwatchesPanel(SwatchesPanel const &); // no copy
    SwatchesPanel &operator=(SwatchesPanel const &); // no assign
    
    void _styleButton( Gtk::Button& btn, SPDesktop *desktop, unsigned int code, char const* iconName, char const* fallback );

    void _setSelectionSwatch(SPGradient* swatchItem, bool isStroke);

    void NoLinkToggled();
    void NewGroup();
    void NewGroupBI();
    void _addBIButtonClicked(GdkEventButton* event);
    void Duplicate(SPGroup* page);
    void SaveAs();
    void AddSelectedFill(SPGroup* page);
    void AddSelectedStroke(SPGroup* page);
    void MoveUp(SPGroup* page);
    void MoveDown(SPGroup* page);
    void Remove(SPGroup* page);
    
    void SetSelectedFill(SPGradient* swatch);
    void SetSelectedStroke(SPGradient* swatch);
    void MoveSwatchUp(SPGradient* swatch);
    void MoveSwatchDown(SPGradient* swatch);
    void RemoveSwatch(SPGradient* swatch);
    
    void _lblClick(GdkEventButton* event, SPGroup* page);
    void _addSwatchButtonClicked(SPGroup* swatch, bool recurse);
    void _defsChanged();
    void _importButtonClicked(bool addToDoc, bool addToBI);
    void _deleteButtonClicked();
    void _addButtonClicked(GdkEventButton* event);
    void _swatchClicked(GdkEventButton* event, SPGradient* swatchItem);
    
    Gtk::MenuItem* _addSwatchGroup(SPGroup* group, Gtk::TreeModel::Row* parentRow);
    void _swatchesChanged();
    
    bool _handleButtonEvent(GdkEventButton *event);
    bool _handleKeyEvent(GdkEventKey *event);
    
    void _handleEdited(const Glib::ustring& path, const Glib::ustring& new_text);
    void _handleEditingCancelled();
    void _renameObject(Gtk::TreeModel::Row row, const Glib::ustring& name);
    
    void _setCollapsed(SPGroup * group);
    void _setExpanded(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& /*path*/, bool isexpanded);
    
    void _deleteButtonClickedDoc();
    bool _handleButtonEventDoc(GdkEventButton* event);
    bool _handleKeyEventDoc(GdkEventKey *event);

    void _handleEditedDoc(const Glib::ustring& path, const Glib::ustring& new_text);
    void _handleEditingCancelledDoc();
    void _renameObjectDoc(Gtk::TreeModel::Row row, const Glib::ustring& name);
    
    bool _handleDragDropDoc(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
    bool _handleDragDrop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
    
    Gtk::Menu *popUpMenu;
    Gtk::Menu *popUpImportMenu;

    Gtk::ScrolledWindow _scroller;
    Gtk::Table _insideTable;
    Gtk::VBox _insideV;
    Gtk::HBox _insideH;
    Gtk::HBox _buttonsRow;
    Gtk::VBox _outsideV;
    Gtk::CheckButton _noLink;
    bool _showlabels;

    SwatchesPanel::ModelColumns* _model;
    Glib::RefPtr<Gtk::TreeStore> _store;
    
    Gtk::CellRendererText *_text_rendererBI;
    Gtk::TreeView::Column *_name_columnBI;
    Gtk::ScrolledWindow _scrollerBI;
    Gtk::TreeView _editBI;
    Gtk::HBox _buttonsRowBI;
    Gtk::VBox _editBIV;

    SwatchesPanel::ModelColumnsDoc* _modelDoc;
    Glib::RefPtr<Gtk::TreeStore> _storeDoc;
    
    Gtk::CellRendererText *_text_rendererDoc;
    Gtk::CellRendererPixbuf *_pixbuf_rendererDoc;
    Gtk::TreeView::Column *_name_columnDoc;
    Gtk::TreeView::Column *_pixbuf_columnDoc;
    Gtk::ScrolledWindow _scrollerDoc;
    Gtk::TreeView _editDoc;
    Gtk::HBox _buttonsRowDoc;
    Gtk::VBox _editDocV;

    Gtk::Notebook _notebook;
    
    SwatchWatcher* rootWatcher;
    std::vector<Inkscape::XML::NodeObserver*> rootWatchers;
    
    std::vector<Inkscape::XML::NodeObserver*> docWatchers;
    SwatchWatcher* docWatcher;

    SPDesktop*  _currentDesktop;
    SPDocument* _currentDocument;
    
    bool _dnd_into;
    SPObject* _dnd_source;
    SPObject* _dnd_target;

    sigc::connection _documentConnection;
    sigc::connection _selChanged;
};

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape



#endif // SEEN_SWATCHES_H

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
