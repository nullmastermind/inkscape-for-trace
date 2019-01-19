// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file
 * Drag and drop of drawings onto canvas.
 */

/* Authors:
 *
 * Copyright (C) Tavmjong Bah 2019
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "drag-and-drop.h"

#include <glibmm/i18n.h>  // Internationalization

#include "desktop-style.h"
#include "document-undo.h"
#include "gradient-drag.h"
#include "file.h"
#include "inkscape.h" // SP_ACTIVE_DESKTOP
#include "splivarot.h"
#include "style.h"

#include "display/sp-canvas.h" // window to world transform

#include "extension/db.h"
#include "extension/find_extension_by_mime.h"

#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "object/sp-flowtext.h"

#include "svg/svg-color.h" // write color

#include "ui/clipboard.h"
#include "ui/interface.h"
#include "ui/tools/tool-base.h"

#include "widgets/ege-paint-def.h"

using Inkscape::DocumentUndo;

/* Drag and Drop */
enum ui_drop_target_info {
    URI_LIST,
    SVG_XML_DATA,
    SVG_DATA,
    PNG_DATA,
    JPEG_DATA,
    IMAGE_DATA,
    APP_X_INKY_COLOR,
    APP_X_COLOR,
    APP_OSWB_COLOR,
    APP_X_INK_PASTE
};

static GtkTargetEntry ui_drop_target_entries [] = {
    {(gchar *)"text/uri-list",                0, URI_LIST        },
    {(gchar *)"image/svg+xml",                0, SVG_XML_DATA    },
    {(gchar *)"image/svg",                    0, SVG_DATA        },
    {(gchar *)"image/png",                    0, PNG_DATA        },
    {(gchar *)"image/jpeg",                   0, JPEG_DATA       },
#if ENABLE_MAGIC_COLORS
    {(gchar *)"application/x-inkscape-color", 0, APP_X_INKY_COLOR},
#endif // ENABLE_MAGIC_COLORS
    {(gchar *)"application/x-oswb-color",     0, APP_OSWB_COLOR  },
    {(gchar *)"application/x-color",          0, APP_X_COLOR     },
    {(gchar *)"application/x-inkscape-paste", 0, APP_X_INK_PASTE }
};

static GtkTargetEntry *completeDropTargets = nullptr;
static int completeDropTargetsCount = 0;

static guint nui_drop_target_entries = G_N_ELEMENTS(ui_drop_target_entries);

/* Drag and Drop */
void
sp_ui_drag_data_received(GtkWidget *widget,
                         GdkDragContext *drag_context,
                         gint x, gint y,
                         GtkSelectionData *data,
                         guint info,
                         guint /*event_time*/,
                         gpointer /*user_data*/)
{
    SPDocument *doc = SP_ACTIVE_DOCUMENT;
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    switch (info) {
#if ENABLE_MAGIC_COLORS
        case APP_X_INKY_COLOR:
        {
            int destX = 0;
            int destY = 0;
            gtk_widget_translate_coordinates( widget, &(desktop->canvas->widget), x, y, &destX, &destY );
            Geom::Point where( sp_canvas_window_to_world( desktop->canvas, Geom::Point( destX, destY ) ) );

            SPItem *item = desktop->getItemAtPoint( where, true );
            if ( item )
            {
                bool fillnotstroke = (drag_context->action != GDK_ACTION_MOVE);

                if ( data->length >= 8 ) {
                    cmsHPROFILE srgbProf = cmsCreate_sRGBProfile();

                    gchar c[64] = {0};
                    // Careful about endian issues.
                    guint16* dataVals = (guint16*)data->data;
                    sp_svg_write_color( c, sizeof(c),
                                        SP_RGBA32_U_COMPOSE(
                                            0x0ff & (dataVals[0] >> 8),
                                            0x0ff & (dataVals[1] >> 8),
                                            0x0ff & (dataVals[2] >> 8),
                                            0xff // can't have transparency in the color itself
                                            //0x0ff & (data->data[3] >> 8),
                                            ));
                    SPCSSAttr *css = sp_repr_css_attr_new();
                    bool updatePerformed = false;

                    if ( data->length > 14 ) {
                        int flags = dataVals[4];

                        // piggie-backed palette entry info
                        int index = dataVals[5];
                        Glib::ustring palName;
                        for ( int i = 0; i < dataVals[6]; i++ ) {
                            palName += (gunichar)dataVals[7+i];
                        }

                        // Now hook in a magic tag of some sort.
                        if ( !palName.empty() && (flags & 1) ) {
                            gchar* str = g_strdup_printf("%d|", index);
                            palName.insert( 0, str );
                            g_free(str);
                            str = 0;

                            item->setAttribute(
                                fillnotstroke ? "inkscape:x-fill-tag":"inkscape:x-stroke-tag",
                                palName.c_str(),
                                false );
                            item->updateRepr();

                            sp_repr_css_set_property( css, fillnotstroke ? "fill":"stroke", c );
                            updatePerformed = true;
                        }
                    }

                    if ( !updatePerformed ) {
                        sp_repr_css_set_property( css, fillnotstroke ? "fill":"stroke", c );
                    }

                    sp_desktop_apply_css_recursive( item, css, true );
                    item->updateRepr();

                    SPDocumentUndo::done( doc , SP_VERB_NONE,
                                      _("Drop color"));

                    if ( srgbProf ) {
                        cmsCloseProfile( srgbProf );
                    }
                }
            }
        }
        break;
#endif // ENABLE_MAGIC_COLORS

        case APP_X_COLOR:
        {
            int destX = 0;
            int destY = 0;
            gtk_widget_translate_coordinates( widget, GTK_WIDGET(desktop->canvas), x, y, &destX, &destY );
            Geom::Point where( sp_canvas_window_to_world( desktop->canvas, Geom::Point( destX, destY ) ) );
            Geom::Point const button_dt(desktop->w2d(where));
            Geom::Point const button_doc(desktop->dt2doc(button_dt));

            if ( gtk_selection_data_get_length (data) == 8 ) {
                gchar colorspec[64] = {0};
                // Careful about endian issues.
                guint16* dataVals = (guint16*)gtk_selection_data_get_data (data);
                sp_svg_write_color( colorspec, sizeof(colorspec),
                                    SP_RGBA32_U_COMPOSE(
                                        0x0ff & (dataVals[0] >> 8),
                                        0x0ff & (dataVals[1] >> 8),
                                        0x0ff & (dataVals[2] >> 8),
                                        0xff // can't have transparency in the color itself
                                        //0x0ff & (data->data[3] >> 8),
                                        ));

                SPItem *item = desktop->getItemAtPoint( where, true );

                bool consumed = false;
                if (desktop->event_context && desktop->event_context->get_drag()) {
                    consumed = desktop->event_context->get_drag()->dropColor(item, colorspec, button_dt);
                    if (consumed) {
                        DocumentUndo::done( doc , SP_VERB_NONE, _("Drop color on gradient") );
                        desktop->event_context->get_drag()->updateDraggers();
                    }
                }

                //if (!consumed && tools_active(desktop, TOOLS_TEXT)) {
                //    consumed = sp_text_context_drop_color(c, button_doc);
                //    if (consumed) {
                //        SPDocumentUndo::done( doc , SP_VERB_NONE, _("Drop color on gradient stop"));
                //    }
                //}

                if (!consumed && item) {
                    bool fillnotstroke = (gdk_drag_context_get_actions (drag_context) != GDK_ACTION_MOVE);
                    if (fillnotstroke &&
                        (SP_IS_SHAPE(item) || SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item))) {
                        Path *livarot_path = Path_for_item(item, true, true);
                        livarot_path->ConvertWithBackData(0.04);

                        boost::optional<Path::cut_position> position = get_nearest_position_on_Path(livarot_path, button_doc);
                        if (position) {
                            Geom::Point nearest = get_point_on_Path(livarot_path, position->piece, position->t);
                            Geom::Point delta = nearest - button_doc;
                            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                            delta = desktop->d2w(delta);
                            double stroke_tolerance =
                                ( !item->style->stroke.isNone() ?
                                  desktop->current_zoom() *
                                  item->style->stroke_width.computed *
                                  item->i2dt_affine().descrim() * 0.5
                                  : 0.0)
                                + prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

                            if (Geom::L2 (delta) < stroke_tolerance) {
                                fillnotstroke = false;
                            }
                        }
                        delete livarot_path;
                    }

                    SPCSSAttr *css = sp_repr_css_attr_new();
                    sp_repr_css_set_property( css, fillnotstroke ? "fill":"stroke", colorspec );

                    sp_desktop_apply_css_recursive( item, css, true );
                    item->updateRepr();

                    DocumentUndo::done( doc , SP_VERB_NONE,
                                        _("Drop color") );
                }
            }
        }
        break;

        case APP_OSWB_COLOR:
        {
            bool worked = false;
            Glib::ustring colorspec;
            if ( gtk_selection_data_get_format (data) == 8 ) {
                ege::PaintDef color;
                worked = color.fromMIMEData("application/x-oswb-color",
                                            reinterpret_cast<char const *>(gtk_selection_data_get_data (data)),
                                            gtk_selection_data_get_length (data),
                                            gtk_selection_data_get_format (data));
                if ( worked ) {
                    if ( color.getType() == ege::PaintDef::CLEAR ) {
                        colorspec = ""; // TODO check if this is sufficient
                    } else if ( color.getType() == ege::PaintDef::NONE ) {
                        colorspec = "none";
                    } else {
                        unsigned int r = color.getR();
                        unsigned int g = color.getG();
                        unsigned int b = color.getB();

                        SPGradient* matches = nullptr;
                        std::vector<SPObject *> gradients = doc->getResourceList("gradient");
                        for (std::vector<SPObject *>::const_iterator item = gradients.begin(); item != gradients.end(); ++item) {
                            SPGradient* grad = SP_GRADIENT(*item);
                            if ( color.descr == grad->getId() ) {
                                if ( grad->hasStops() ) {
                                    matches = grad;
                                    break;
                                }
                            }
                        }
                        if (matches) {
                            colorspec = "url(#";
                            colorspec += matches->getId();
                            colorspec += ")";
                        } else {
                            gchar* tmp = g_strdup_printf("#%02x%02x%02x", r, g, b);
                            colorspec = tmp;
                            g_free(tmp);
                        }
                    }
                }
            }
            if ( worked ) {
                int destX = 0;
                int destY = 0;
                gtk_widget_translate_coordinates( widget, GTK_WIDGET(desktop->canvas), x, y, &destX, &destY );
                Geom::Point where( sp_canvas_window_to_world( desktop->canvas, Geom::Point( destX, destY ) ) );
                Geom::Point const button_dt(desktop->w2d(where));
                Geom::Point const button_doc(desktop->dt2doc(button_dt));

                SPItem *item = desktop->getItemAtPoint( where, true );

                bool consumed = false;
                if (desktop->event_context && desktop->event_context->get_drag()) {
                    consumed = desktop->event_context->get_drag()->dropColor(item, colorspec.c_str(), button_dt);
                    if (consumed) {
                        DocumentUndo::done( doc , SP_VERB_NONE, _("Drop color on gradient") );
                        desktop->event_context->get_drag()->updateDraggers();
                    }
                }

                if (!consumed && item) {
                    bool fillnotstroke = (gdk_drag_context_get_actions (drag_context) != GDK_ACTION_MOVE);
                    if (fillnotstroke &&
                        (SP_IS_SHAPE(item) || SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item))) {
                        Path *livarot_path = Path_for_item(item, true, true);
                        livarot_path->ConvertWithBackData(0.04);

                        boost::optional<Path::cut_position> position = get_nearest_position_on_Path(livarot_path, button_doc);
                        if (position) {
                            Geom::Point nearest = get_point_on_Path(livarot_path, position->piece, position->t);
                            Geom::Point delta = nearest - button_doc;
                            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                            delta = desktop->d2w(delta);
                            double stroke_tolerance =
                                ( !item->style->stroke.isNone() ?
                                  desktop->current_zoom() *
                                  item->style->stroke_width.computed *
                                  item->i2dt_affine().descrim() * 0.5
                                  : 0.0)
                                + prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

                            if (Geom::L2 (delta) < stroke_tolerance) {
                                fillnotstroke = false;
                            }
                        }
                        delete livarot_path;
                    }

                    SPCSSAttr *css = sp_repr_css_attr_new();
                    sp_repr_css_set_property( css, fillnotstroke ? "fill":"stroke", colorspec.c_str() );

                    sp_desktop_apply_css_recursive( item, css, true );
                    item->updateRepr();

                    DocumentUndo::done( doc , SP_VERB_NONE,
                                        _("Drop color") );
                }
            }
        }
        break;

        case SVG_DATA:
        case SVG_XML_DATA: {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setBool("/options/onimport", true);
            gchar *svgdata = (gchar *)gtk_selection_data_get_data (data);

            Inkscape::XML::Document *rnewdoc = sp_repr_read_mem(svgdata, gtk_selection_data_get_length (data), SP_SVG_NS_URI);

            if (rnewdoc == nullptr) {
                sp_ui_error_dialog(_("Could not parse SVG data"));
                return;
            }

            Inkscape::XML::Node *repr = rnewdoc->root();
            gchar const *style = repr->attribute("style");

            Inkscape::XML::Node *newgroup = rnewdoc->createElement("svg:g");
            newgroup->setAttribute("style", style);

            Inkscape::XML::Document * xml_doc =  doc->getReprDoc();
            for (Inkscape::XML::Node *child = repr->firstChild(); child != nullptr; child = child->next()) {
                Inkscape::XML::Node *newchild = child->duplicate(xml_doc);
                newgroup->appendChild(newchild);
            }

            Inkscape::GC::release(rnewdoc);

            // Add it to the current layer

            // Greg's edits to add intelligent positioning of svg drops
            SPObject *new_obj = nullptr;
            new_obj = desktop->currentLayer()->appendChildRepr(newgroup);

            Inkscape::Selection *selection = desktop->getSelection();
            selection->set(SP_ITEM(new_obj));

            // move to mouse pointer
            {
                desktop->getDocument()->ensureUpToDate();
                Geom::OptRect sel_bbox = selection->visualBounds();
                if (sel_bbox) {
                    Geom::Point m( desktop->point() - sel_bbox->midpoint() );
                    selection->moveRelative(m, false);
                }
            }

            Inkscape::GC::release(newgroup);
            DocumentUndo::done( doc, SP_VERB_NONE,
                                _("Drop SVG") );
            prefs->setBool("/options/onimport", false);
            break;
        }

        case URI_LIST: {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setBool("/options/onimport", true);
            gchar *uri = (gchar *)gtk_selection_data_get_data (data);
            sp_ui_import_files(uri);
            prefs->setBool("/options/onimport", false);
            break;
        }

        case APP_X_INK_PASTE: {
            Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
            cm->paste(desktop);
            DocumentUndo::done( doc, SP_VERB_NONE, _("Drop Symbol") );
            break;
        }

        case PNG_DATA:
        case JPEG_DATA:
        case IMAGE_DATA: {
            Inkscape::Extension::Extension *ext = Inkscape::Extension::find_by_mime((info == JPEG_DATA ? "image/jpeg" : "image/png"));
            bool save = (strcmp(ext->get_param_optiongroup("link"), "embed") == 0);
            ext->set_param_optiongroup("link", "embed");
            ext->set_gui(false);

            gchar *filename = g_build_filename( g_get_tmp_dir(), "inkscape-dnd-import", NULL );
            g_file_set_contents(filename,
                reinterpret_cast<gchar const *>(gtk_selection_data_get_data (data)),
                gtk_selection_data_get_length (data),
                nullptr);
            file_import(doc, filename, ext);
            g_free(filename);

            ext->set_param_optiongroup("link", save ? "embed" : "link");
            ext->set_gui(true);
            DocumentUndo::done( doc , SP_VERB_NONE,
                                _("Drop bitmap image") );
            break;
        }
    }
}

#include "ui/tools/gradient-tool.h"

void sp_ui_drag_motion( GtkWidget */*widget*/,
                        GdkDragContext */*drag_context*/,
                        gint /*x*/, gint /*y*/,
                        GtkSelectionData */*data*/,
                        guint /*info*/,
                        guint /*event_time*/,
                        gpointer /*user_data*/)
{
//     SPDocument *doc = SP_ACTIVE_DOCUMENT;
//     SPDesktop *desktop = SP_ACTIVE_DESKTOP;


//     g_message("drag-n-drop motion (%4d, %4d)  at %d", x, y, event_time);
}

static void sp_ui_drag_leave( GtkWidget */*widget*/,
                              GdkDragContext */*drag_context*/,
                              guint /*event_time*/,
                              gpointer /*user_data*/ )
{
//     g_message("drag-n-drop leave                at %d", event_time);
}

void
sp_ui_drag_setup(Gtk::Widget* win)
{
    if ( completeDropTargets == nullptr || completeDropTargetsCount == 0 )
    {
        std::vector<Glib::ustring> types;

        std::vector<Gdk::PixbufFormat> list = Gdk::Pixbuf::get_formats();
        for (auto one:list) {
            std::vector<Glib::ustring> typesXX = one.get_mime_types();
            for (auto i:typesXX) {
                types.push_back(i);
            }
        }
        completeDropTargetsCount = nui_drop_target_entries + types.size();
        completeDropTargets = new GtkTargetEntry[completeDropTargetsCount];
        for ( int i = 0; i < (int)nui_drop_target_entries; i++ ) {
            completeDropTargets[i] = ui_drop_target_entries[i];
        }
        int pos = nui_drop_target_entries;

        for (auto & type : types) {
            completeDropTargets[pos].target = g_strdup(type.c_str());
            completeDropTargets[pos].flags = 0;
            completeDropTargets[pos].info = IMAGE_DATA;
            pos++;
        }
    }

    gtk_drag_dest_set((GtkWidget*)win->gobj(),
                      GTK_DEST_DEFAULT_ALL,
                      completeDropTargets,
                      completeDropTargetsCount,
                      GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE));

    g_signal_connect(G_OBJECT(win->gobj()),
                     "drag_data_received",
                     G_CALLBACK(sp_ui_drag_data_received),
                     NULL);

    g_signal_connect(G_OBJECT(win->gobj()),
                     "drag_motion",
                     G_CALLBACK(sp_ui_drag_motion),
                     NULL);

    g_signal_connect(G_OBJECT(win->gobj()),
                     "drag_leave",
                     G_CALLBACK(sp_ui_drag_leave),
                     NULL);
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
