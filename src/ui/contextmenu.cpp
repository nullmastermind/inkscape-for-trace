/**
 * @file
 * Context menu
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2012 Kris De Gussem
 * Copyright (C) 2010 authors
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "contextmenu.h"

#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/separatormenuitem.h>

#include "desktop.h"
#include "document.h"
#include "document-undo.h"
#include "helper/action.h"
#include "helper/action-context.h"
#include "inkscape.h"
#include "message-context.h"
#include "message-stack.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "shortcuts.h"

#include "object/sp-anchor.h"
#include "object/sp-clippath.h"
#include "object/sp-image.h"
#include "object/sp-mask.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"

#include "ui/dialog/dialog-manager.h"
#include "ui/dialog/layer-properties.h"
#include "verbs.h"

static bool temporarily_block_actions = false;

ContextMenu::ContextMenu(SPDesktop *desktop, SPItem *item) :
    _item(item),
    MIGroup(),
    MIParent(_("Go to parent"))
{
// g_message("ContextMenu");
    _object = static_cast<SPObject *>(item);
    _desktop = desktop;

    AppendItemFromVerb(Inkscape::Verb::get(SP_VERB_EDIT_UNDO));
    AppendItemFromVerb(Inkscape::Verb::get(SP_VERB_EDIT_REDO));
    AddSeparator();
    AppendItemFromVerb(Inkscape::Verb::get(SP_VERB_EDIT_CUT));
    AppendItemFromVerb(Inkscape::Verb::get(SP_VERB_EDIT_COPY));
    AppendItemFromVerb(Inkscape::Verb::get(SP_VERB_EDIT_PASTE));
    AddSeparator();
    AppendItemFromVerb(Inkscape::Verb::get(SP_VERB_EDIT_DUPLICATE));
    AppendItemFromVerb(Inkscape::Verb::get(SP_VERB_EDIT_DELETE));
    
    positionOfLastDialog = 10; // 9 in front + 1 for the separator in the next if; used to position the dialog menu entries below each other
    /* Item menu */
    if (item!=nullptr) {
        AddSeparator();
        MakeObjectMenu();
    }
    AddSeparator();
    /* Lock/Unock Hide/Unhide*/
    Geom::Rect b(_desktop->point(),_desktop->point() + Geom::Point(1,1));
    std::vector< SPItem * > down_items = _desktop->getDocument()->getItemsPartiallyInBox( _desktop->dkey, b, true, true);
    bool has_down_hidden = false;
    bool has_down_locked = false;
    for(std::vector< SPItem * >::iterator down = down_items.begin(); down != down_items.end(); ++down){
        if((*down)->isHidden()) {
            has_down_hidden = true;
        }
        if((*down)->isLocked()) {
            has_down_locked = true;
        }
    }
    Gtk::MenuItem* mi;

    mi = Gtk::manage(new Gtk::MenuItem(_("Hide selected objects"),true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::HideSelected));
    if (_desktop->selection->isEmpty()) {
        mi->set_sensitive(false);
    }
    mi->show();
    append(*mi);//insert(*mi,positionOfLastDialog++);

    mi = Gtk::manage(new Gtk::MenuItem(_("Unhide objects below"),true));
    mi->signal_activate().connect(sigc::bind<std::vector< SPItem * > >(sigc::mem_fun(*this, &ContextMenu::UnHideBelow), down_items));
    if (!has_down_hidden) {
        mi->set_sensitive(false);
    }
    mi->show();
    append(*mi);//insert(*mi,positionOfLastDialog++);

    mi = Gtk::manage(new Gtk::MenuItem(_("Lock selected objects"),true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::LockSelected));
    if (_desktop->selection->isEmpty()) {
        mi->set_sensitive(false);
    }
    mi->show();
    append(*mi);//insert(*mi,positionOfLastDialog++);

    mi = Gtk::manage(new Gtk::MenuItem(_("Unlock objects below"),true));
    mi->signal_activate().connect(sigc::bind<std::vector< SPItem * > >(sigc::mem_fun(*this, &ContextMenu::UnLockBelow), down_items));
    if (!has_down_locked) {
        mi->set_sensitive(false);
    }
    mi->show();
    append(*mi);//insert(*mi,positionOfLastDialog++);
    /* layer menu */
    SPGroup *group=nullptr;
    if (item) {
        if (SP_IS_GROUP(item)) {
            group = SP_GROUP(item);
        } else if ( item != _desktop->currentRoot() && SP_IS_GROUP(item->parent) ) {
            group = SP_GROUP(item->parent);
        }
    }

    if (( group && group != _desktop->currentLayer() ) ||
        ( _desktop->currentLayer() != _desktop->currentRoot() && _desktop->currentLayer()->parent != _desktop->currentRoot() ) ) {
        AddSeparator();
    }

    if ( group && group != _desktop->currentLayer() ) {
        /* TRANSLATORS: #%1 is the id of the group e.g. <g id="#g7">, not a number. */
        MIGroup.set_label (Glib::ustring::compose(_("Enter group #%1"), group->getId()));
        MIGroup.set_data("group", group);
        MIGroup.signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &ContextMenu::EnterGroup),&MIGroup));
        MIGroup.show();
        append(MIGroup);
    }

    if ( _desktop->currentLayer() != _desktop->currentRoot() ) {
        if ( _desktop->currentLayer()->parent != _desktop->currentRoot() ) {
            MIParent.signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::LeaveGroup));
            MIParent.show();
            append(MIParent);

            /* Pop selection out of group */
            Gtk::MenuItem* miu = Gtk::manage(new Gtk::MenuItem(_("_Pop selection out of group"), true));
            miu->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ActivateUngroupPopSelection));
            miu->show();
            append(*miu);
        }
    }
}

ContextMenu::~ContextMenu(void)
= default;

Gtk::SeparatorMenuItem* ContextMenu::AddSeparator(void)
{
    Gtk::SeparatorMenuItem* sep = Gtk::manage(new Gtk::SeparatorMenuItem());
    sep->show();
    append(*sep);
    return sep;
}

void ContextMenu::EnterGroup(Gtk::MenuItem* mi)
{
    _desktop->setCurrentLayer(reinterpret_cast<SPObject *>(mi->get_data("group")));
    _desktop->selection->clear();
}

void ContextMenu::LeaveGroup(void)
{
    _desktop->setCurrentLayer(_desktop->currentLayer()->parent);
}

void ContextMenu::LockSelected(void) 
{
    auto itemlist = _desktop->selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end(); ++i) {
         (*i)->setLocked(true);
    }
}

void ContextMenu::HideSelected(void)
{
    auto itemlist =_desktop->selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end(); ++i) {
         (*i)->setHidden(true);
    }
}

void ContextMenu::UnLockBelow(std::vector<SPItem *> items)
{
    _desktop->selection->clear();
    for(auto i=items.begin();i!=items.end(); ++i) {
        if ((*i)->isLocked()) {
            (*i)->setLocked(false);
            _desktop->selection->add(*i);
        }
    }
}

void ContextMenu::UnHideBelow(std::vector<SPItem *> items)
{
    _desktop->selection->clear();
    for(auto i=items.begin();i!=items.end(); ++i) {
        if ((*i)->isHidden()) {
            (*i)->setHidden(false);
            _desktop->selection->add(*i);
        }
    }
}

/*
 * Some day when the right-click menus are ready to start working
 * smarter with the verbs, we'll need to change this NULL being
 * sent to sp_action_perform to something useful, or set some kind
 * of global "right-clicked position" variable for actions to
 * investigate when they're called.
 */
static void
context_menu_item_on_my_activate(void */*object*/, SPAction *action)
{
    if (!temporarily_block_actions) {
        sp_action_perform(action, nullptr);
    }
}

static void
context_menu_item_on_my_select(void */*object*/, SPAction *action)
{
    sp_action_get_view(action)->tipsMessageContext()->set(Inkscape::NORMAL_MESSAGE, action->tip);
}

static void
context_menu_item_on_my_deselect(void */*object*/, SPAction *action)
{
    sp_action_get_view(action)->tipsMessageContext()->clear();
}


// TODO: Update this to allow radio items to be used
void ContextMenu::AppendItemFromVerb(Inkscape::Verb *verb)
{
    SPAction *action;
    SPDesktop *view = _desktop;
    
    if (verb->get_code() == SP_VERB_NONE) {
        Gtk::MenuItem *item = AddSeparator();
        item->show();
        append(*item);
    } else {
        action = verb->get_action(Inkscape::ActionContext(view));
        if (!action) {
            return;
        }
        
        auto const item = Gtk::manage(new Gtk::MenuItem(action->name, true));

        sp_shortcut_add_accelerator(GTK_WIDGET(item->gobj()), sp_shortcut_get_primary(verb));

        action->signal_set_sensitive.connect(sigc::mem_fun(*this, &ContextMenu::set_sensitive));
        action->signal_set_name.connect(sigc::mem_fun(*item, &ContextMenu::set_name));

        if (!action->sensitive) {
            item->set_sensitive(FALSE);
        }

        item->set_events(Gdk::KEY_PRESS_MASK);
        item->signal_activate().connect(sigc::bind(sigc::ptr_fun(context_menu_item_on_my_activate),item,action));
        item->signal_select().connect(sigc::bind(sigc::ptr_fun(context_menu_item_on_my_select),item,action));
        item->signal_deselect().connect(sigc::bind(sigc::ptr_fun(context_menu_item_on_my_deselect),item,action));
        item->show_all();
        append(*item);
    }
}

void ContextMenu::MakeObjectMenu(void)
{
    if (SP_IS_ITEM(_object)) {
        MakeItemMenu();
    }

    if (SP_IS_GROUP(_object)) {
        MakeGroupMenu();
    }

    if (SP_IS_ANCHOR(_object)) {
        MakeAnchorMenu();
    }

    if (SP_IS_IMAGE(_object)) {
        MakeImageMenu();
    }

    if (SP_IS_SHAPE(_object)) {
        MakeShapeMenu();
    }

    if (SP_IS_TEXT(_object)) {
        MakeTextMenu();
    }
}

void ContextMenu::MakeItemMenu (void)
{
    Gtk::MenuItem* mi;

    /* Item dialog */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Object Properties..."),true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ItemProperties));
    mi->show();
    append(*mi);//insert(*mi,positionOfLastDialog++);

    AddSeparator();
    
    /* Select item */
    if (Inkscape::Verb::getbyid( "org.inkscape.followlink" )) {
        mi = Gtk::manage(new Gtk::MenuItem(_("_Select This"), true));
        if (_desktop->selection->includes(_item)) {
            mi->set_sensitive(FALSE);
        } else {
            mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ItemSelectThis));
        }
        mi->show();
        append(*mi);
    }


    mi = Gtk::manage(new Gtk::MenuItem(_("Select Same")));
    mi->show();
    Gtk::Menu *select_same_submenu = Gtk::manage(new Gtk::Menu());
    if (_desktop->selection->isEmpty()) {
        mi->set_sensitive(FALSE);
    }
    mi->set_submenu(*select_same_submenu);
    append(*mi);

    /* Select same fill and stroke */
    mi = Gtk::manage(new Gtk::MenuItem(_("Fill and Stroke"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SelectSameFillStroke));
    mi->set_sensitive(!SP_IS_ANCHOR(_item));
    mi->show();
    select_same_submenu->append(*mi);

    /* Select same fill color */
    mi = Gtk::manage(new Gtk::MenuItem(_("Fill Color"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SelectSameFillColor));
    mi->set_sensitive(!SP_IS_ANCHOR(_item));
    mi->show();
    select_same_submenu->append(*mi);

    /* Select same stroke color */
    mi = Gtk::manage(new Gtk::MenuItem(_("Stroke Color"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SelectSameStrokeColor));
    mi->set_sensitive(!SP_IS_ANCHOR(_item));
    mi->show();
    select_same_submenu->append(*mi);

    /* Select same stroke style */
    mi = Gtk::manage(new Gtk::MenuItem(_("Stroke Style"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SelectSameStrokeStyle));
    mi->set_sensitive(!SP_IS_ANCHOR(_item));
    mi->show();
    select_same_submenu->append(*mi);

    /* Select same stroke style */
    mi = Gtk::manage(new Gtk::MenuItem(_("Object type"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SelectSameObjectType));
    mi->set_sensitive(!SP_IS_ANCHOR(_item));
    mi->show();
    select_same_submenu->append(*mi);

    /* Move to layer */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Move to layer ..."), true));
    if (_desktop->selection->isEmpty()) {
        mi->set_sensitive(FALSE);
    } else {
        mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ItemMoveTo));
    }
    mi->show();
    append(*mi);

    /* Create link */
    mi = Gtk::manage(new Gtk::MenuItem(_("Create _Link"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ItemCreateLink));
    mi->set_sensitive(!SP_IS_ANCHOR(_item));
    mi->show();
    append(*mi);
    
    bool ClipRefOK=false;
    bool MaskRefOK=false;
    if (_item){
        if (_item->clip_ref){
            if (_item->clip_ref->getObject()){
                ClipRefOK=true;
            }
        }
    }    
    if (_item){
        if (_item->mask_ref){
            if (_item->mask_ref->getObject()){
                MaskRefOK=true;
            }
        }
    }    
    /* Set mask */
    mi = Gtk::manage(new Gtk::MenuItem(_("Set Mask"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SetMask));
    if (ClipRefOK || MaskRefOK) {
        mi->set_sensitive(FALSE);
    } else {
        mi->set_sensitive(TRUE);
    }
    mi->show();
    append(*mi);
    
    /* Release mask */
    mi = Gtk::manage(new Gtk::MenuItem(_("Release Mask"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ReleaseMask));
    if (MaskRefOK) {
        mi->set_sensitive(TRUE);
    } else {
        mi->set_sensitive(FALSE);
    }
    mi->show();
    append(*mi);

    /*SSet Clip Group */
    mi = Gtk::manage(new Gtk::MenuItem(_("Create Clip G_roup"),true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::CreateGroupClip));
    mi->set_sensitive(TRUE);
    mi->show();
    append(*mi);
    
    /* Set Clip */
    mi = Gtk::manage(new Gtk::MenuItem(_("Set Cl_ip"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SetClip));
    if (ClipRefOK || MaskRefOK) {
        mi->set_sensitive(FALSE);
    } else {
        mi->set_sensitive(TRUE);
    }
    mi->show();
    append(*mi);
    
    /* Release Clip */
    mi = Gtk::manage(new Gtk::MenuItem(_("Release C_lip"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ReleaseClip));
    if (ClipRefOK) {
        mi->set_sensitive(TRUE);
    } else {
        mi->set_sensitive(FALSE);
    }
    mi->show();
    append(*mi);

    /* Group */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Group"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ActivateGroup));
    if (_desktop->selection->isEmpty()) {
        mi->set_sensitive(FALSE);
    } else {
        mi->set_sensitive(TRUE);
    }
    mi->show();
    append(*mi);
}

void ContextMenu::SelectSameFillStroke(void)
{
    sp_select_same_fill_stroke_style(_desktop, true, true, true);
}

void ContextMenu::SelectSameFillColor(void)
{
    sp_select_same_fill_stroke_style(_desktop, true, false, false);
}

void ContextMenu::SelectSameStrokeColor(void)
{
    sp_select_same_fill_stroke_style(_desktop, false, true, false);
}

void ContextMenu::SelectSameStrokeStyle(void)
{
    sp_select_same_fill_stroke_style(_desktop, false, false, true);
}

void ContextMenu::SelectSameObjectType(void)
{
    sp_select_same_object_type(_desktop);
}

void ContextMenu::ItemProperties(void)
{
    _desktop->selection->set(_item);
    _desktop->_dlg_mgr->showDialog("ObjectProperties");
}

void ContextMenu::ItemSelectThis(void)
{
    _desktop->selection->set(_item);
}

void ContextMenu::ItemMoveTo(void)
{
    Inkscape::UI::Dialogs::LayerPropertiesDialog::showMove(_desktop, _desktop->currentLayer());
}



void ContextMenu::ItemCreateLink(void)
{
    Inkscape::XML::Document *xml_doc = _desktop->doc()->getReprDoc();
    Inkscape::XML::Node *repr = xml_doc->createElement("svg:a");
    _item->parent->getRepr()->addChild(repr, _item->getRepr());
    SPObject *object = _item->document->getObjectByRepr(repr);
    g_return_if_fail(SP_IS_ANCHOR(object));

    const char *id = _item->getRepr()->attribute("id");
    Inkscape::XML::Node *child = _item->getRepr()->duplicate(xml_doc);
    _item->deleteObject(false);
    repr->addChild(child, nullptr);
    child->setAttribute("id", id);

    Inkscape::GC::release(repr);
    Inkscape::GC::release(child);

    Inkscape::DocumentUndo::done(object->document, SP_VERB_NONE, _("Create link"));

    _desktop->selection->set(SP_ITEM(object));
    _desktop->_dlg_mgr->showDialog("ObjectAttributes");
}

void ContextMenu::SetMask(void)
{
    _desktop->selection->setMask(false, false);
}

void ContextMenu::ReleaseMask(void)
{
    _desktop->selection->unsetMask(false);
}

void ContextMenu::CreateGroupClip(void)
{
    _desktop->selection->setClipGroup();
}

void ContextMenu::SetClip(void)
{
    _desktop->selection->setMask(true, false);
}


void ContextMenu::ReleaseClip(void)
{
    _desktop->selection->unsetMask(true);
}

void ContextMenu::MakeGroupMenu(void)
{
    /* Ungroup */
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(_("_Ungroup"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ActivateUngroup));
    mi->show();
    append(*mi);
}

void ContextMenu::ActivateGroup(void)
{
    _desktop->selection->group();
}

void ContextMenu::ActivateUngroup(void)
{
	std::vector<SPItem*> children;

    sp_item_group_ungroup(static_cast<SPGroup*>(_item), children);
    _desktop->selection->setList(children);
}
 
void ContextMenu::ActivateUngroupPopSelection(void)
{
    _desktop->selection->popFromGroup();
}


void ContextMenu::MakeAnchorMenu(void)
{
    Gtk::MenuItem* mi;
    
    /* Link dialog */
    mi = Gtk::manage(new Gtk::MenuItem(_("Link _Properties..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::AnchorLinkProperties));
    mi->show();
    insert(*mi,positionOfLastDialog++);
    
    /* Select item */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Follow Link"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::AnchorLinkFollow));
    mi->show();
    append(*mi);
    
    /* Reset transformations */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Remove Link"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::AnchorLinkRemove));
    mi->show();
    append(*mi);
}

void ContextMenu::AnchorLinkProperties(void)
{
    _desktop->_dlg_mgr->showDialog("ObjectAttributes");
}

void ContextMenu::AnchorLinkFollow(void)
{

    if (_desktop->selection->isEmpty()) {
        _desktop->selection->set(_item);
    }
    // Opening the selected links with a python extension
    Inkscape::Verb *verb = Inkscape::Verb::getbyid( "org.inkscape.followlink" );
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(_desktop));
        if (action) {
            sp_action_perform(action, nullptr);
        }
    }
}

void ContextMenu::AnchorLinkRemove(void)
{
	std::vector<SPItem*> children;
    sp_item_group_ungroup(static_cast<SPAnchor*>(_item), children, false);
    Inkscape::DocumentUndo::done(_desktop->doc(), SP_VERB_NONE, _("Remove link"));
}

void ContextMenu::MakeImageMenu (void)
{
    Gtk::MenuItem* mi;
    Inkscape::XML::Node *ir = _object->getRepr();
    const gchar *href = ir->attribute("xlink:href");

    /* Image properties */
    mi = Gtk::manage(new Gtk::MenuItem(_("Image _Properties..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ImageProperties));
    mi->show();
    insert(*mi,positionOfLastDialog++);

    /* Edit externally */
    mi = Gtk::manage(new Gtk::MenuItem(_("Edit Externally..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ImageEdit));
    mi->show();
    insert(*mi,positionOfLastDialog++);
    if ( (!href) || ((strncmp(href, "data:", 5) == 0)) ) {
        mi->set_sensitive( FALSE );
    }

    /* Trace Bitmap */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Trace Bitmap..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ImageTraceBitmap));
    mi->show();
    insert(*mi,positionOfLastDialog++);
    if (_desktop->selection->isEmpty()) {
        mi->set_sensitive(FALSE);
    }

    /* Trace Pixel Art */
    mi = Gtk::manage(new Gtk::MenuItem(_("Trace Pixel Art"), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ImageTracePixelArt));
    mi->show();
    insert(*mi,positionOfLastDialog++);
    if (_desktop->selection->isEmpty()) {
        mi->set_sensitive(FALSE);
    }

    /* Embed image */
    if (Inkscape::Verb::getbyid( "org.ekips.filter.embedselectedimages" )) {
        mi = Gtk::manage(new Gtk::MenuItem(C_("Context menu", "Embed Image")));
        mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ImageEmbed));
        mi->show();
        insert(*mi,positionOfLastDialog++);
        if ( (!href) || ((strncmp(href, "data:", 5) == 0)) ) {
            mi->set_sensitive( FALSE );
        }
    }

    /* Extract image */
    if (Inkscape::Verb::getbyid( "org.ekips.filter.extractimage" )) {
        mi = Gtk::manage(new Gtk::MenuItem(C_("Context menu", "Extract Image...")));
        mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::ImageExtract));
        mi->show();
        insert(*mi,positionOfLastDialog++);
        if ( (!href) || ((strncmp(href, "data:", 5) != 0)) ) {
            mi->set_sensitive( FALSE );
        }
    }
}

void ContextMenu::ImageProperties(void)
{
    _desktop->_dlg_mgr->showDialog("ObjectAttributes");
}

Glib::ustring ContextMenu::getImageEditorName(bool is_svg) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring value;
    if (!is_svg) {
        Glib::ustring choices = prefs->getString("/options/bitmapeditor/value");
        if (!choices.empty()) {
            value = choices;
        }
        else {
            value = "gimp";
        }
    } else {
        Glib::ustring choices = prefs->getString("/options/svgeditor/value");
        if (!choices.empty()) {
            value = choices;
        }
        else {
            value = "inkscape";
        }
    }
    return value;
}

void ContextMenu::ImageEdit(void)
{
    if (_desktop->selection->isEmpty()) {
        _desktop->selection->set(_item);
    }

    GError* errThing = nullptr;
    Glib::ustring bmpeditor = getImageEditorName();
    Glib::ustring cmdline = bmpeditor;
    Glib::ustring name;
    Glib::ustring fullname;

#ifdef WIN32
    // g_spawn_command_line_sync parsing is done according to Unix shell rules,
    // not Windows command interpreter rules. Thus we need to enclose the
    // executable path with single quotes.
    int index = cmdline.find(".exe");
    if ( index < 0 ) index = cmdline.find(".bat");
    if ( index < 0 ) index = cmdline.find(".com");
    if ( index >= 0 ) {
        Glib::ustring editorBin = cmdline.substr(0, index + 4).c_str();
        Glib::ustring args = cmdline.substr(index + 4, cmdline.length()).c_str();
        editorBin.insert(0, "'");
        editorBin.append("'");
        cmdline = editorBin;
        cmdline.append(args);
    } else {
        // Enclose the whole command line if no executable path can be extracted.
        cmdline.insert(0, "'");
        cmdline.append("'");
    }
#endif

    auto itemlist= _desktop->selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        Inkscape::XML::Node *ir = (*i)->getRepr();
        const gchar *href = ir->attribute("xlink:href");
        
        if (strncmp (href,"file:",5) == 0) {
        // URI to filename conversion
          name = g_filename_from_uri(href, nullptr, nullptr);
        } else {
          name.append(href);
        }

        if (Glib::path_is_absolute(name)) {
            fullname = name;
        } else if (SP_ACTIVE_DOCUMENT->getBase()) {
            fullname = Glib::build_filename(SP_ACTIVE_DOCUMENT->getBase(), name);
        } else {
            fullname = Glib::build_filename(Glib::get_current_dir(), name);
        }
        if (name.substr(name.find_last_of(".") + 1) == "SVG" ||
            name.substr(name.find_last_of(".") + 1) == "svg"   ) 
        {
            cmdline.erase(0, bmpeditor.length());
            Glib::ustring svgeditor = getImageEditorName(true);
            cmdline = svgeditor.append(cmdline);
        }
        cmdline.append(" '");
        cmdline.append(fullname.c_str());
        cmdline.append("'");
    }

    //g_warning("##Command line: %s\n", cmdline.c_str());

    g_spawn_command_line_async(cmdline.c_str(), &errThing); 

    if ( errThing ) {
        g_warning("Problem launching editor (%d). %s", errThing->code, errThing->message);
        (_desktop->messageStack())->flash(Inkscape::ERROR_MESSAGE, errThing->message);
        g_error_free(errThing);
        errThing = nullptr;
    }
}

void ContextMenu::ImageTraceBitmap(void)
{
    INKSCAPE.dialogs_unhide();
    _desktop->_dlg_mgr->showDialog("Trace");
}

void ContextMenu::ImageTracePixelArt(void)
{
    INKSCAPE.dialogs_unhide();
    _desktop->_dlg_mgr->showDialog("PixelArt");
}

void ContextMenu::ImageEmbed(void)
{
    if (_desktop->selection->isEmpty()) {
        _desktop->selection->set(_item);
    }

    Inkscape::Verb *verb = Inkscape::Verb::getbyid( "org.ekips.filter.embedselectedimages" );
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(_desktop));
        if (action) {
            sp_action_perform(action, nullptr);
        }
    }
}

void ContextMenu::ImageExtract(void)
{
    if (_desktop->selection->isEmpty()) {
        _desktop->selection->set(_item);
    }

    Inkscape::Verb *verb = Inkscape::Verb::getbyid( "org.ekips.filter.extractimage" );
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(_desktop));
        if (action) {
            sp_action_perform(action, nullptr);
        }
    }
}

void ContextMenu::MakeShapeMenu (void)
{
    Gtk::MenuItem* mi;
    
    /* Item dialog */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Fill and Stroke..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::FillSettings));
    mi->show();
    insert(*mi,positionOfLastDialog++);
}

void ContextMenu::FillSettings(void)
{
    if (_desktop->selection->isEmpty()) {
        _desktop->selection->set(_item);
    }

    _desktop->_dlg_mgr->showDialog("FillAndStroke");
}

void ContextMenu::MakeTextMenu (void)
{
    Gtk::MenuItem* mi;

    /* Fill and Stroke dialog */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Fill and Stroke..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::FillSettings));
    mi->show();
    insert(*mi,positionOfLastDialog++);
    
    /* Edit Text dialog */
    mi = Gtk::manage(new Gtk::MenuItem(_("_Text and Font..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::TextSettings));
    mi->show();
    insert(*mi,positionOfLastDialog++);

    /* Spellcheck dialog */
    mi = Gtk::manage(new Gtk::MenuItem(_("Check Spellin_g..."), true));
    mi->signal_activate().connect(sigc::mem_fun(*this, &ContextMenu::SpellcheckSettings));
    mi->show();
    insert(*mi,positionOfLastDialog++);
}

void ContextMenu::TextSettings (void)
{
    if (_desktop->selection->isEmpty()) {
        _desktop->selection->set(_item);
    }

    _desktop->_dlg_mgr->showDialog("TextFont");
}

void ContextMenu::SpellcheckSettings (void)
{
    if (_desktop->selection->isEmpty()) {
        _desktop->selection->set(_item);
    }

    _desktop->_dlg_mgr->showDialog("SpellCheck");
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
