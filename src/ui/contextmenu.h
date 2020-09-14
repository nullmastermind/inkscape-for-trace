// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CONTEXTMENU_H
#define SEEN_CONTEXTMENU_H

/*
 * Context menu
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2012 Kris De Gussem
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/menu.h>

class SPDesktop;
class SPItem;
class SPObject;

namespace Gtk {
class SeparatorMenuItem;
}

namespace Inkscape {
class Verb;
}

/**
 * Implements the Inkscape context menu.
 * 
 * For the context menu implementation, the ContextMenu class stores the object
 * that was selected in a private data member. This should be fairly safe to do
 * and a pointer to the SPItem as well as SPObject class are kept.
 * All callbacks of the context menu entries are implemented as private
 * functions.
 * 
 * @todo add callbacks to destroy the context menu when it is closed (=key or mouse button pressed out of the scope of the context menu)
 */
class ContextMenu : public Gtk::Menu
{
    public:
        /**
         * The ContextMenu constructor contains all code to create and show the
         * menu entries (aka child widgets).
         * 
         * @param desktop pointer to the desktop the user is currently working on.
         * @param item SPItem pointer to the object selected at the time the ContextMenu is created.
         */
        ContextMenu(SPDesktop *desktop, SPItem *item);
        ~ContextMenu() override;

    private:
        SPItem *_item; // pointer to the object selected at the time the ContextMenu is created
        SPObject *_object; // pointer to the object selected at the time the ContextMenu is created
        SPDesktop *_desktop; //pointer to the desktop the user was currently working on at the time the ContextMenu is created
        
        int positionOfLastDialog;

        Gtk::MenuItem MIGroup; //menu entry to enter a group
        SPObject *_MIGroup_group; // Group to enter when MIGroup is activated
        Gtk::MenuItem MIParent; //menu entry to leave a group
        
        /**
         * auxiliary function that adds a separator line in the context menu
         */
        Gtk::SeparatorMenuItem* AddSeparator();
        
        /**
         * Appends a custom menu UI from a verb.
         * 
         * c++ified version of sp_ui_menu_append_item.
         * @see sp_ui_menu_append_item_from_verb and synchronize/drop that function when c++ifying other code in interface.cpp
         * 
         * @param show_icon True if an icon should be displayed before the menu item's label
         * 
         */
        void AppendItemFromVerb(Inkscape::Verb *verb, bool show_icon = false);
        
        /**
         * main function which is responsible for creating the context sensitive menu items,
         * calls subfunctions below to create the menu entry widgets.
         */
        void MakeObjectMenu (); 
        /**
         * creates menu entries for an SP_TYPE_ITEM object
         */
        void MakeItemMenu   ();
        /**
         * creates menu entries for a grouped object
         */
        void MakeGroupMenu  ();
        /**
         * creates menu entries for an anchor object
         */
        void MakeAnchorMenu ();
        /**
         * creates menu entries for a bitmap image object
         */
        void MakeImageMenu  ();
        /**
         * creates menu entries for a shape object
         */
        void MakeShapeMenu  ();
        /**
         * creates menu entries for a text object
         */
        void MakeTextMenu   ();
        
        void EnterGroup(Gtk::MenuItem* mi);
        void LeaveGroup();
        void LockSelected();
        void HideSelected();
        void UnLockBelow(std::vector<SPItem *> items);
        void UnHideBelow(std::vector<SPItem *> items);
        //////////////////////////////////////////
        //callbacks for the context menu entries of an SP_TYPE_ITEM object
        void ItemProperties();
        void ItemSelectThis();
        void ItemMoveTo();
        void SelectSameFillStroke();
        void SelectSameFillColor();
        void SelectSameStrokeColor();
        void SelectSameStrokeStyle();
        void SelectSameObjectType();
        void ItemCreateLink();
        void CreateGroupClip();
        void SetMask();
        void ReleaseMask();
        void SetClip();
        void ReleaseClip();
        //////////////////////////////////////////
        
        
        /**
         * callback, is executed on clicking the anchor "Group" and "Ungroup" menu entry
         */
        void ActivateUngroupPopSelection();
        void ActivateUngroup();
        void ActivateGroup();
        
        void AnchorLinkProperties();
        /**
         * placeholder for callback to be executed on clicking the anchor "Follow link" context menu entry
         * @todo add code to follow link externally
         */
        void AnchorLinkFollow();
        
        /**
         * callback, is executed on clicking the anchor "Link remove" menu entry
         */
        void AnchorLinkRemove();
        
        
        /**
         * callback, opens the image properties dialog and is executed on clicking the context menu entry with similar name
         */
        void ImageProperties();
        
        /**
         * callback, is executed on clicking the image "Edit Externally" menu entry
         */
        void ImageEdit();
        
        /**
         * auxiliary function that loads the external image editor name from the settings.
         */
        Glib::ustring getImageEditorName(bool is_svg = false);
        
        /**
         * callback, is executed on clicking the "Embed Image" menu entry
         */
        void ImageEmbed();
        
        /**
         * callback, is executed on clicking the "Trace Bitmap" menu entry
         */
        void ImageTraceBitmap();

        /**
         * callback, is executed on clicking the "Extract Image" menu entry
         */
        void ImageExtract();
        
        
        /**
         * callback, is executed on clicking the "Fill and Stroke" menu entry
         */
        void FillSettings();
        
        
        /**
         * callback, is executed on clicking the "Text and Font" menu entry
         */
        void TextSettings();
        
        /**
         * callback, is executed on clicking the "Check spelling" menu entry
         */
        void SpellcheckSettings();
};
#endif // SEEN_CONTEXT_MENU_H

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
