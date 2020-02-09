// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gradient vector widget
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <gtkmm/treeview.h>
#include <vector>

#include "document-undo.h"
#include "document.h"
#include "gradient-chemistry.h"
#include "gradient-vector.h"
#include "id-clash.h"
#include "inkscape.h"
#include "paint-selector.h"
#include "preferences.h"
#include "verbs.h"

#include "object/sp-defs.h"
#include "style.h"

#include "helper/action.h"
#include "ui/icon-loader.h"

#include "ui/icon-names.h"

enum {
    GRABBED,
    DRAGGED,
    RELEASED,
    CHANGED,
    LAST_SIGNAL
};


static void sp_gradient_selector_dispose(GObject *object);

/* Signal handlers */
static void sp_gradient_selector_vector_set (SPGradientVectorSelector *gvs, SPGradient *gr, SPGradientSelector *sel);
static void sp_gradient_selector_edit_vector_clicked (GtkWidget *w, SPGradientSelector *sel);
static void sp_gradient_selector_add_vector_clicked (GtkWidget *w, SPGradientSelector *sel);
static void sp_gradient_selector_delete_vector_clicked (GtkWidget *w, SPGradientSelector *sel);

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(SPGradientSelector, sp_gradient_selector, GTK_TYPE_BOX);

static void sp_gradient_selector_class_init(SPGradientSelectorClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    signals[GRABBED] =  g_signal_new ("grabbed",
                                        G_TYPE_FROM_CLASS(object_class),
                                        (GSignalFlags)(G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE),
                                        G_STRUCT_OFFSET (SPGradientSelectorClass, grabbed),
					nullptr, nullptr,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);
    signals[DRAGGED] =  g_signal_new ("dragged",
                                        G_TYPE_FROM_CLASS(object_class),
                                        (GSignalFlags)(G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE),
                                        G_STRUCT_OFFSET (SPGradientSelectorClass, dragged),
					nullptr, nullptr,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);
    signals[RELEASED] = g_signal_new ("released",
                                        G_TYPE_FROM_CLASS(object_class),
                                        (GSignalFlags)(G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE),
                                        G_STRUCT_OFFSET (SPGradientSelectorClass, released),
					nullptr, nullptr,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);
    signals[CHANGED] =  g_signal_new ("changed",
                                        G_TYPE_FROM_CLASS(object_class),
                                        (GSignalFlags)(G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE),
                                        G_STRUCT_OFFSET (SPGradientSelectorClass, changed),
					nullptr, nullptr,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);

    object_class->dispose = sp_gradient_selector_dispose;
}

static void gradsel_style_button(GtkWidget *gtkbtn, char const *iconName)
{
    Gtk::Button *btn = Glib::wrap(GTK_BUTTON(gtkbtn));
    GtkWidget *child = sp_get_icon_image(iconName, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(child);
    btn->add(*manage(Glib::wrap(child)));
    btn->set_relief(Gtk::RELIEF_NONE);
}

static void sp_gradient_selector_init(SPGradientSelector *sel)
{
    sel->safelyInit = true;
    sel->blocked = false;

    gtk_orientable_set_orientation(GTK_ORIENTABLE(sel), GTK_ORIENTATION_VERTICAL);

    new (&sel->nonsolid) std::vector<GtkWidget*>();
    new (&sel->swatch_widgets) std::vector<GtkWidget*>();

    sel->mode = SPGradientSelector::MODE_LINEAR;

    sel->gradientUnits = SP_GRADIENT_UNITS_USERSPACEONUSE;
    sel->gradientSpread = SP_GRADIENT_SPREAD_PAD;

    /* Vectors */
    sel->vectors = sp_gradient_vector_selector_new (nullptr, nullptr);
    SPGradientVectorSelector *gvs = SP_GRADIENT_VECTOR_SELECTOR(sel->vectors);
    sel->store = gvs->store;
    sel->columns = gvs->columns;

    sel->treeview = Gtk::manage(new Gtk::TreeView());
    sel->treeview->set_model(gvs->store);
    sel->treeview->set_headers_clickable (true);
    sel->treeview->set_search_column(1);
    sel->treeview->set_vexpand();
    sel->icon_renderer = Gtk::manage(new Gtk::CellRendererPixbuf());
    sel->text_renderer = Gtk::manage(new Gtk::CellRendererText());

    sel->treeview->append_column(_("Gradient"), *sel->icon_renderer);
    Gtk::TreeView::Column* icon_column = sel->treeview->get_column(0);
    icon_column->add_attribute(sel->icon_renderer->property_pixbuf(), sel->columns->pixbuf);
    icon_column->set_sort_column(sel->columns->color);
    icon_column->set_clickable(true);

    sel->treeview->append_column(_("Name"), *sel->text_renderer);
    Gtk::TreeView::Column* name_column = sel->treeview->get_column(1);
    sel->text_renderer->property_editable() = true;
    name_column->add_attribute(sel->text_renderer->property_text(), sel->columns->name);
    name_column->set_min_width(180);
    name_column->set_clickable(true);
    name_column->set_resizable(true);

    sel->treeview->append_column("#", sel->columns->refcount);
    Gtk::TreeView::Column* count_column = sel->treeview->get_column(2);
    count_column->set_clickable(true);
    count_column->set_resizable(true);

    sel->treeview->signal_key_press_event().connect(sigc::mem_fun(*sel, &SPGradientSelector::onKeyPressEvent), false);

    sel->treeview->show();

    icon_column->signal_clicked().connect( sigc::mem_fun(*sel, &SPGradientSelector::onTreeColorColClick) );
    name_column->signal_clicked().connect( sigc::mem_fun(*sel, &SPGradientSelector::onTreeNameColClick) );
    count_column->signal_clicked().connect( sigc::mem_fun(*sel, &SPGradientSelector::onTreeCountColClick) );

    gvs->tree_select_connection = sel->treeview->get_selection()->signal_changed().connect( sigc::mem_fun(*sel, &SPGradientSelector::onTreeSelection) );
    sel->text_renderer->signal_edited().connect( sigc::mem_fun(*sel, &SPGradientSelector::onGradientRename) );

    sel->scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
    sel->scrolled_window->add(*sel->treeview);
    sel->scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    sel->scrolled_window->set_shadow_type(Gtk::SHADOW_IN);
    sel->scrolled_window->set_size_request(0, 180);
    sel->scrolled_window->set_hexpand();
    sel->scrolled_window->show();

    gtk_box_pack_start (GTK_BOX (sel), GTK_WIDGET(sel->scrolled_window->gobj()), TRUE, TRUE, 4);


    /* Create box for buttons */
    auto hb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(hb), FALSE);
    gtk_box_pack_start( GTK_BOX(sel), hb, FALSE, FALSE, 0 );

    sel->add = gtk_button_new();
    gradsel_style_button(sel->add, INKSCAPE_ICON("list-add"));

    sel->nonsolid.push_back(sel->add);
    gtk_box_pack_start (GTK_BOX (hb), sel->add, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (sel->add), "clicked", G_CALLBACK (sp_gradient_selector_add_vector_clicked), sel);
    gtk_widget_set_sensitive (sel->add, FALSE);
    gtk_button_set_relief(GTK_BUTTON(sel->add), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text( sel->add, _("Create a duplicate gradient"));

    sel->edit = gtk_button_new();
    gradsel_style_button(sel->edit, INKSCAPE_ICON("edit"));

    sel->nonsolid.push_back(sel->edit);
    gtk_box_pack_start (GTK_BOX (hb), sel->edit, FALSE, FALSE, 0);
    g_signal_connect (G_OBJECT (sel->edit), "clicked", G_CALLBACK (sp_gradient_selector_edit_vector_clicked), sel);
    gtk_widget_set_sensitive (sel->edit, FALSE);
    gtk_button_set_relief(GTK_BUTTON(sel->edit), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text( sel->edit, _("Edit gradient"));

    sel->del = gtk_button_new ();
    gradsel_style_button(sel->del, INKSCAPE_ICON("list-remove"));

    sel->swatch_widgets.push_back(sel->del);
    gtk_box_pack_start (GTK_BOX (hb), sel->del, FALSE, FALSE, 0);
    g_signal_connect (G_OBJECT (sel->del), "clicked", G_CALLBACK (sp_gradient_selector_delete_vector_clicked), sel);
    gtk_widget_set_sensitive (sel->del, FALSE);
    gtk_button_set_relief(GTK_BUTTON(sel->del), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text( sel->del, _("Delete swatch"));

    gtk_widget_show_all(hb);
}

static void sp_gradient_selector_dispose(GObject *object)
{
    SPGradientSelector *sel = SP_GRADIENT_SELECTOR( object );

    if ( sel->safelyInit ) {
        sel->safelyInit = false;
        sel->nonsolid.~vector<GtkWidget*>();
        sel->swatch_widgets.~vector<GtkWidget*>();
    }

    if (sel->icon_renderer) {
        delete sel->icon_renderer;
        sel->icon_renderer = nullptr;
    }
    if (sel->text_renderer) {
        delete sel->text_renderer;
        sel->text_renderer = nullptr;
    }

    if ((G_OBJECT_CLASS(sp_gradient_selector_parent_class))->dispose) {
        (G_OBJECT_CLASS(sp_gradient_selector_parent_class))->dispose(object);
    }
}

void SPGradientSelector::setSpread(SPGradientSpread spread)
{
    gradientSpread = spread;
    //gtk_combo_box_set_active (GTK_COMBO_BOX(this->spread), gradientSpread);
}


GtkWidget *sp_gradient_selector_new()
{
    SPGradientSelector *sel = SP_GRADIENT_SELECTOR(g_object_new (SP_TYPE_GRADIENT_SELECTOR, nullptr));

    return GTK_WIDGET(sel);
}

void SPGradientSelector::setMode(SelectorMode mode)
{
    if (mode != this->mode) {
        this->mode = mode;
        if (mode == MODE_SWATCH) {
            for (auto & it : nonsolid)
            {
                gtk_widget_hide(it);
            }
            for (auto & swatch_widget : swatch_widgets)
            {
                gtk_widget_show_all(swatch_widget);
            }

            Gtk::TreeView::Column* icon_column = treeview->get_column(0);
            icon_column->set_title(_("Swatch"));

            SPGradientVectorSelector* vs = SP_GRADIENT_VECTOR_SELECTOR(vectors);
            vs->setSwatched();
        } else {
            for (auto & it : nonsolid)
            {
                gtk_widget_show_all(it);
            }
            for (auto & swatch_widget : swatch_widgets)
            {
                gtk_widget_hide(swatch_widget);
            }
            Gtk::TreeView::Column* icon_column = treeview->get_column(0);
            icon_column->set_title(_("Gradient"));

        }
    }
}

void SPGradientSelector::setUnits(SPGradientUnits units)
{
    gradientUnits = units;
}

SPGradientUnits SPGradientSelector::getUnits()
{
    return gradientUnits;
}

SPGradientSpread SPGradientSelector::getSpread()
{
    return gradientSpread;
}

void SPGradientSelector::onGradientRename( const Glib::ustring& path_string, const Glib::ustring& new_text)
{
    Gtk::TreePath path(path_string);
    Gtk::TreeModel::iterator iter = store->get_iter(path);

    if( iter )
    {
        Gtk::TreeModel::Row row = *iter;
        if ( row ) {
            SPObject* obj = row[columns->data];
            if ( obj ) {
                row[columns->name] = gr_prepare_label(obj);
                if (!new_text.empty() && new_text != row[columns->name]) {
                  rename_id(obj, new_text );
                  Inkscape::DocumentUndo::done(obj->document, SP_VERB_CONTEXT_GRADIENT,
                                     _("Rename gradient"));
                }
            }
        }
    }
}

void SPGradientSelector::onTreeColorColClick() {
    Gtk::TreeView::Column* column = treeview->get_column(0);
    column->set_sort_column(columns->color);
}

void SPGradientSelector::onTreeNameColClick() {
    Gtk::TreeView::Column* column = treeview->get_column(1);
    column->set_sort_column(columns->name);
}


void SPGradientSelector::onTreeCountColClick() {
    Gtk::TreeView::Column* column = treeview->get_column(2);
    column->set_sort_column(columns->refcount);
}

void SPGradientSelector::moveSelection(int amount, bool down, bool toEnd)
{
    Glib::RefPtr<Gtk::TreeSelection> select = treeview->get_selection();
    auto iter = select->get_selected();

    if (amount < 0) {
        down = !down;
        amount = -amount;
    }

    auto canary = iter;
    if (down) {
        ++canary;
    } else {
        --canary;
    }
    while (canary && (toEnd || amount > 0)) {
        --amount;
        if (down) {
            ++canary;
            ++iter;
        } else {
            --canary;
            --iter;
        }
    }

    select->select(iter);
    treeview->scroll_to_row(store->get_path(iter), 0.5);
}

bool SPGradientSelector::onKeyPressEvent(GdkEventKey *event)
{
    bool consume = false;
    auto display = Gdk::Display::get_default();
    auto keymap = display->get_keymap();
    guint key = 0;
    gdk_keymap_translate_keyboard_state(keymap, event->hardware_keycode,
                                        static_cast<GdkModifierType>(event->state),
                                        0, &key, 0, 0, 0);

    switch (key) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up: {
            moveSelection(-1);
            consume = true;
            break;
        }
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down: {
            moveSelection(1);
            consume = true;
            break;
        }
        case GDK_KEY_Page_Up:
        case GDK_KEY_KP_Page_Up: {
            moveSelection(-5);
            consume = true;
            break;
        }

        case GDK_KEY_Page_Down:
        case GDK_KEY_KP_Page_Down: {
            moveSelection(5);
            consume = true;
            break;
        }

        case GDK_KEY_End:
        case GDK_KEY_KP_End: {
            moveSelection(0, true, true);
            consume = true;
            break;
        }

        case GDK_KEY_Home:
        case GDK_KEY_KP_Home: {
            moveSelection(0, false, true);
            consume = true;
            break;
        }
    }
    return consume;
}

void SPGradientSelector::onTreeSelection()
{
    if (!treeview) {
        return;
    }

    if (blocked) {
        return;
    }

    if (!treeview->has_focus()) {
        /* Workaround for GTK bug on Windows/OS X
         * When the treeview initially doesn't have focus and is clicked
         * sometimes get_selection()->signal_changed() has the wrong selection
         */
        treeview->grab_focus();
    }

    const Glib::RefPtr<Gtk::TreeSelection> sel = treeview->get_selection();
    if (!sel) {
        return;
    }

    SPGradient *obj = nullptr;
    /* Single selection */
    Gtk::TreeModel::iterator iter = sel->get_selected();
    if ( iter ) {
        Gtk::TreeModel::Row row = *iter;
        obj = row[columns->data];
    }

    if (obj) {
        sp_gradient_selector_vector_set (nullptr, SP_GRADIENT(obj), this);
    }
}

bool SPGradientSelector::_checkForSelected(const Gtk::TreePath &path, const Gtk::TreeIter& iter, SPGradient *vector)
{
    bool found = false;

    Gtk::TreeModel::Row row = *iter;
    if ( vector == row[columns->data] )
    {
        treeview->scroll_to_row(path, 0.5);
        Glib::RefPtr<Gtk::TreeSelection> select = treeview->get_selection();
        bool wasBlocked = blocked;
        blocked = true;
        select->select(iter);
        blocked = wasBlocked;
        found = true;
    }

    return found;
}

void SPGradientSelector::selectGradientInTree(SPGradient *vector)
{
    store->foreach( sigc::bind<SPGradient*>(sigc::mem_fun(*this, &SPGradientSelector::_checkForSelected), vector) );
}

void SPGradientSelector::setVector(SPDocument *doc, SPGradient *vector)
{
    g_return_if_fail(!vector || SP_IS_GRADIENT(vector));
    g_return_if_fail(!vector || (vector->document == doc));

    if (vector && !vector->hasStops()) {
        return;
    }

    sp_gradient_vector_selector_set_gradient(SP_GRADIENT_VECTOR_SELECTOR(vectors), doc, vector);

    selectGradientInTree(vector);

    if (vector) {
        if ( (mode == MODE_SWATCH) && vector->isSwatch() ) {
            if ( vector->isSolid() ) {
                for (auto & it : nonsolid)
                {
                    gtk_widget_hide(it);
                }
            } else {
                for (auto & it : nonsolid)
                {
                    gtk_widget_show_all(it);
                }
            }
        } else if (mode != MODE_SWATCH) {

            for (auto & swatch_widget : swatch_widgets)
            {
                gtk_widget_hide(swatch_widget);
            }
            for (auto & it : nonsolid)
            {
                gtk_widget_show_all(it);
            }

        }

        if (edit) {
            gtk_widget_set_sensitive(edit, TRUE);
        }
        if (add) {
            gtk_widget_set_sensitive(add, TRUE);
        }
        if (del) {
            gtk_widget_set_sensitive(del, TRUE);
        }
    } else {
        if (edit) {
            gtk_widget_set_sensitive(edit, FALSE);
        }
        if (add) {
            gtk_widget_set_sensitive(add, (doc != nullptr));
        }
        if (del) {
            gtk_widget_set_sensitive(del, FALSE);
        }
    }
}

SPGradient *SPGradientSelector::getVector()
{
    /* fixme: */
    return SP_GRADIENT_VECTOR_SELECTOR(vectors)->gr;
}


static void sp_gradient_selector_vector_set(SPGradientVectorSelector * /*gvs*/, SPGradient *gr, SPGradientSelector *sel)
{

    if (!sel->blocked) {
        sel->blocked = TRUE;
        gr = sp_gradient_ensure_vector_normalized (gr);
        sel->setVector((gr) ? gr->document : nullptr, gr);
        g_signal_emit (G_OBJECT (sel), signals[CHANGED], 0, gr);
        sel->blocked = FALSE;
    }
}


static void
sp_gradient_selector_delete_vector_clicked (GtkWidget */*w*/, SPGradientSelector *sel)
{
    const Glib::RefPtr<Gtk::TreeSelection> selection = sel->treeview->get_selection();
    if (!selection) {
        return;
    }

    SPGradient *obj = nullptr;
    /* Single selection */
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if ( iter ) {
        Gtk::TreeModel::Row row = *iter;
        obj = row[sel->columns->data];
    }

    if (obj) {
        sp_gradient_unset_swatch(SP_ACTIVE_DESKTOP, obj->getId());
    }

}

static void
sp_gradient_selector_edit_vector_clicked (GtkWidget */*w*/, SPGradientSelector *sel)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/dialogs/gradienteditor/showlegacy", false)) {
        // Legacy gradient dialog
        GtkWidget *dialog;
        dialog = sp_gradient_vector_editor_new (SP_GRADIENT_VECTOR_SELECTOR (sel->vectors)->gr);
        gtk_widget_show (dialog);
    } else {
        // Invoke the gradient tool
        Inkscape::Verb *verb = Inkscape::Verb::get( SP_VERB_CONTEXT_GRADIENT );
        if ( verb ) {
            SPAction *action = verb->get_action( Inkscape::ActionContext( ( Inkscape::UI::View::View * ) SP_ACTIVE_DESKTOP ) );
            if ( action ) {
                sp_action_perform( action, nullptr );
            }
        }
    }
}

static void
sp_gradient_selector_add_vector_clicked (GtkWidget */*w*/, SPGradientSelector *sel)
{
    SPDocument *doc = sp_gradient_vector_selector_get_document (SP_GRADIENT_VECTOR_SELECTOR (sel->vectors));

    if (!doc)
        return;

    SPGradient *gr = sp_gradient_vector_selector_get_gradient( SP_GRADIENT_VECTOR_SELECTOR (sel->vectors));
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    Inkscape::XML::Node *repr = nullptr;

    if (gr) {
        repr = gr->getRepr()->duplicate(xml_doc);
        // Rename the new gradients id to be similar to the cloned gradients
        Glib::ustring old_id = gr->getId();
        rename_id(gr, old_id);
        doc->getDefs()->getRepr()->addChild(repr, nullptr);
    } else {
        repr = xml_doc->createElement("svg:linearGradient");
        Inkscape::XML::Node *stop = xml_doc->createElement("svg:stop");
        stop->setAttribute("offset", "0");
        stop->setAttribute("style", "stop-color:#000;stop-opacity:1;");
        repr->appendChild(stop);
        Inkscape::GC::release(stop);
        stop = xml_doc->createElement("svg:stop");
        stop->setAttribute("offset", "1");
        stop->setAttribute("style", "stop-color:#fff;stop-opacity:1;");
        repr->appendChild(stop);
        Inkscape::GC::release(stop);
        doc->getDefs()->getRepr()->addChild(repr, nullptr);
        gr = SP_GRADIENT(doc->getObjectByRepr(repr));
    }
    
    sp_gradient_vector_selector_set_gradient( SP_GRADIENT_VECTOR_SELECTOR (sel->vectors), doc, gr);

    sel->selectGradientInTree(gr);

    Inkscape::GC::release(repr);
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
