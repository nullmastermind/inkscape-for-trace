// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path offsets.
 *//*
 * Authors:
 * see git history
 *  Created by fred on Fri Dec 05 2003.
 *  tweaked endlessly by bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/*
 * contains lots of stitched pieces of path-chemistry.c
 */

#include <vector>

#include <glibmm/i18n.h>

#include "path-offset.h"
#include "path-util.h"

#include "message-stack.h"
#include "path-chemistry.h"     // copy_object_properties()
#include "selection.h"

#include "display/curve.h"

#include "livarot/Path.h"
#include "livarot/Shape.h"

#include "object/sp-flowtext.h"
#include "object/sp-path.h"
#include "object/sp-text.h"

#include "style.h"

#define MIN_OFFSET 0.01

using Inkscape::DocumentUndo;

void sp_selected_path_do_offset(SPDesktop *desktop, bool expand, double prefOffset);
void sp_selected_path_create_offset_object(SPDesktop *desktop, int expand, bool updating);

void
sp_selected_path_offset(SPDesktop *desktop)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double prefOffset = prefs->getDouble("/options/defaultoffsetwidth/value", 1.0, "px");

    sp_selected_path_do_offset(desktop, true, prefOffset);
}
void
sp_selected_path_inset(SPDesktop *desktop)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double prefOffset = prefs->getDouble("/options/defaultoffsetwidth/value", 1.0, "px");

    sp_selected_path_do_offset(desktop, false, prefOffset);
}

void
sp_selected_path_offset_screen(SPDesktop *desktop, double pixels)
{
    sp_selected_path_do_offset(desktop, true,  pixels / desktop->current_zoom());
}

void
sp_selected_path_inset_screen(SPDesktop *desktop, double pixels)
{
    sp_selected_path_do_offset(desktop, false,  pixels / desktop->current_zoom());
}


void sp_selected_path_create_offset_object_zero(SPDesktop *desktop)
{
    sp_selected_path_create_offset_object(desktop, 0, false);
}

void sp_selected_path_create_offset(SPDesktop *desktop)
{
    sp_selected_path_create_offset_object(desktop, 1, false);
}
void sp_selected_path_create_inset(SPDesktop *desktop)
{
    sp_selected_path_create_offset_object(desktop, -1, false);
}

void sp_selected_path_create_updating_offset_object_zero(SPDesktop *desktop)
{
    sp_selected_path_create_offset_object(desktop, 0, true);
}

void sp_selected_path_create_updating_offset(SPDesktop *desktop)
{
    sp_selected_path_create_offset_object(desktop, 1, true);
}

void sp_selected_path_create_updating_inset(SPDesktop *desktop)
{
    sp_selected_path_create_offset_object(desktop, -1, true);
}

void sp_selected_path_create_offset_object(SPDesktop *desktop, int expand, bool updating)
{
    Inkscape::Selection *selection = desktop->getSelection();
    SPItem *item = selection->singleItem();

    if (auto shape = dynamic_cast<SPShape const *>(item)) {
        if (!shape->curve())
            return;
    } else if (auto text = dynamic_cast<SPText const *>(item)) {
        if (!text->getNormalizedBpath())
            return;
    } else {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Selected object is <b>not a path</b>, cannot inset/outset."));
        return;
    }

    Geom::Affine const transform(item->transform);
    auto scaling_factor = item->i2doc_affine().descrim();

    item->doWriteTransform(Geom::identity());

    // remember the position of the item
    gint pos = item->getRepr()->position();

    // remember parent
    Inkscape::XML::Node *parent = item->getRepr()->parent();

    float o_width = 0;
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        o_width = prefs->getDouble("/options/defaultoffsetwidth/value", 1.0, "px");
        o_width /= scaling_factor;
        if (scaling_factor == 0 || o_width < MIN_OFFSET) {
            o_width = MIN_OFFSET;
        }
    }

    Path *orig = Path_for_item(item, true, false);
    if (orig == nullptr)
    {
        return;
    }

    Path *res = new Path;
    res->SetBackData(false);

    {
        Shape *theShape = new Shape;
        Shape *theRes = new Shape;

        orig->ConvertWithBackData(1.0);
        orig->Fill(theShape, 0);

        SPCSSAttr *css = sp_repr_css_attr(item->getRepr(), "style");
        gchar const *val = sp_repr_css_property(css, "fill-rule", nullptr);
        if (val && strcmp(val, "nonzero") == 0)
        {
            theRes->ConvertToShape(theShape, fill_nonZero);
        }
        else if (val && strcmp(val, "evenodd") == 0)
        {
            theRes->ConvertToShape(theShape, fill_oddEven);
        }
        else
        {
            theRes->ConvertToShape(theShape, fill_nonZero);
        }

        Path *originaux[1];
        originaux[0] = orig;
        theRes->ConvertToForme(res, 1, originaux);

        delete theShape;
        delete theRes;
    }

    if (res->descr_cmd.size() <= 1)
    {
        // pas vraiment de points sur le resultat
        // donc il ne reste rien
        DocumentUndo::done(desktop->getDocument(), 
                           (updating ? SP_VERB_SELECTION_LINKED_OFFSET 
                            : SP_VERB_SELECTION_DYNAMIC_OFFSET),
                           (updating ? _("Create linked offset")
                            : _("Create dynamic offset")));
        selection->clear();

        delete res;
        delete orig;
        return;
    }

    {
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

        if (!updating) {
            Inkscape::copy_object_properties(repr, item->getRepr());
        } else {
            gchar const *style = item->getRepr()->attribute("style");
            repr->setAttribute("style", style);
        }

        repr->setAttribute("sodipodi:type", "inkscape:offset");
        sp_repr_set_svg_double(repr, "inkscape:radius", ( expand > 0
                                                          ? o_width
                                                          : expand < 0
                                                          ? -o_width
                                                          : 0 ));

        gchar *str = res->svg_dump_path();
        repr->setAttribute("inkscape:original", str);
        g_free(str);
        str = nullptr;

        if ( updating ) {

			//XML Tree being used directly here while it shouldn't be
            item->doWriteTransform(transform);
            char const *id = item->getRepr()->attribute("id");
            char const *uri = g_strdup_printf("#%s", id);
            repr->setAttribute("xlink:href", uri);
            g_free((void *) uri);
        } else {
            repr->removeAttribute("inkscape:href");
            // delete original
            item->deleteObject(false);
        }

        // add the new repr to the parent
        // move to the saved position
        parent->addChildAtPos(repr, pos);

        SPItem *nitem = reinterpret_cast<SPItem *>(desktop->getDocument()->getObjectByRepr(repr));

        if ( !updating ) {
            // apply the transform to the offset
            nitem->doWriteTransform(transform);
        }

        // The object just created from a temporary repr is only a seed.
        // We need to invoke its write which will update its real repr (in particular adding d=)
        nitem->updateRepr();

        Inkscape::GC::release(repr);

        selection->set(nitem);
    }

    DocumentUndo::done(desktop->getDocument(), 
                       (updating ? SP_VERB_SELECTION_LINKED_OFFSET 
                        : SP_VERB_SELECTION_DYNAMIC_OFFSET),
                       (updating ? _("Create linked offset")
                        : _("Create dynamic offset")));

    delete res;
    delete orig;
}

/**
 * Apply offset to selected paths
 * @param desktop Targetted desktop
 * @param expand True if offset expands, False if it shrinks paths
 * @param prefOffset Size of offset in pixels
 */
void
sp_selected_path_do_offset(SPDesktop *desktop, bool expand, double prefOffset)
{
    Inkscape::Selection *selection = desktop->getSelection();

    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>path(s)</b> to inset/outset."));
        return;
    }

    bool did = false;
    std::vector<SPItem*> il(selection->items().begin(), selection->items().end());
    for (auto item : il){
        if (auto shape = dynamic_cast<SPShape const *>(item)) {
            if (!shape->curve())
                continue;
        } else if (auto text = dynamic_cast<SPText const *>(item)) {
            if (!text->getNormalizedBpath())
                continue;
        } else if (auto text = dynamic_cast<SPFlowtext const *>(item)) {
            if (!text->getNormalizedBpath())
                continue;
        } else {
            continue;
        }

        Geom::Affine const transform(item->transform);
        auto scaling_factor = item->i2doc_affine().descrim();

        item->doWriteTransform(Geom::identity());

        float o_width = 0;
        float o_miter = 0;
        JoinType o_join = join_straight;
        //ButtType o_butt = butt_straight;

        {
            SPStyle *i_style = item->style;
            int jointype = i_style->stroke_linejoin.value;

            switch (jointype) {
                case SP_STROKE_LINEJOIN_MITER:
                    o_join = join_pointy;
                    break;
                case SP_STROKE_LINEJOIN_ROUND:
                    o_join = join_round;
                    break;
                default:
                    o_join = join_straight;
                    break;
            }

            // scale to account for transforms and document units
            o_width = prefOffset / scaling_factor;

            if (scaling_factor == 0 || o_width < MIN_OFFSET) {
                o_width = MIN_OFFSET;
            }
            o_miter = i_style->stroke_miterlimit.value * o_width;
        }

        Path *orig = Path_for_item(item, false);
        if (orig == nullptr) {
            continue;
        }

        Path *res = new Path;
        res->SetBackData(false);

        {
            Shape *theShape = new Shape;
            Shape *theRes = new Shape;

            orig->ConvertWithBackData(0.03);
            orig->Fill(theShape, 0);

            SPCSSAttr *css = sp_repr_css_attr(item->getRepr(), "style");
            gchar const *val = sp_repr_css_property(css, "fill-rule", nullptr);
            if (val && strcmp(val, "nonzero") == 0)
            {
                theRes->ConvertToShape(theShape, fill_nonZero);
            }
            else if (val && strcmp(val, "evenodd") == 0)
            {
                theRes->ConvertToShape(theShape, fill_oddEven);
            }
            else
            {
                theRes->ConvertToShape(theShape, fill_nonZero);
            }

            // et maintenant: offset
            // methode inexacte
/*			Path *originaux[1];
			originaux[0] = orig;
			theRes->ConvertToForme(res, 1, originaux);

			if (expand) {
                        res->OutsideOutline(orig, 0.5 * o_width, o_join, o_butt, o_miter);
			} else {
                        res->OutsideOutline(orig, -0.5 * o_width, o_join, o_butt, o_miter);
			}

			orig->ConvertWithBackData(1.0);
			orig->Fill(theShape, 0);
			theRes->ConvertToShape(theShape, fill_positive);
			originaux[0] = orig;
			theRes->ConvertToForme(res, 1, originaux);

			if (o_width >= 0.5) {
                        //     res->Coalesce(1.0);
                        res->ConvertEvenLines(1.0);
                        res->Simplify(1.0);
			} else {
                        //      res->Coalesce(o_width);
                        res->ConvertEvenLines(1.0*o_width);
                        res->Simplify(1.0 * o_width);
			}    */
            // methode par makeoffset

            if (expand)
            {
                theShape->MakeOffset(theRes, o_width, o_join, o_miter);
            }
            else
            {
                theShape->MakeOffset(theRes, -o_width, o_join, o_miter);
            }
            theRes->ConvertToShape(theShape, fill_positive);

            res->Reset();
            theRes->ConvertToForme(res);

            res->ConvertEvenLines(0.1);
            res->Simplify(0.1);

            delete theShape;
            delete theRes;
        }

        did = true;

        // remember the position of the item
        gint pos = item->getRepr()->position();
        // remember parent
        Inkscape::XML::Node *parent = item->getRepr()->parent();

        selection->remove(item);

        Inkscape::XML::Node *repr = nullptr;

        if (res->descr_cmd.size() > 1) { // if there's 0 or 1 node left, drop this path altogether
            Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
            repr = xml_doc->createElement("svg:path");

            Inkscape::copy_object_properties(repr, item->getRepr());
        }

        item->deleteObject(false);

        if (repr) {
            gchar *str = res->svg_dump_path();
            repr->setAttribute("d", str);
            g_free(str);

            // add the new repr to the parent
            // move to the saved position
            parent->addChildAtPos(repr, pos);

            SPItem *newitem = (SPItem *) desktop->getDocument()->getObjectByRepr(repr);

            // reapply the transform
            newitem->doWriteTransform(transform);

            selection->add(repr);

            Inkscape::GC::release(repr);
        }

        delete orig;
        delete res;
    }

    if (did) {
        DocumentUndo::done(desktop->getDocument(), 
                           (expand ? SP_VERB_SELECTION_OFFSET : SP_VERB_SELECTION_INSET),
                           (expand ? _("Outset path") : _("Inset path")));
    } else {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No paths</b> to inset/outset in the selection."));
        return;
    }
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
