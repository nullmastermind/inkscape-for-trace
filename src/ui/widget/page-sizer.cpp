// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 *
 * Paper-size widget and helper functions
 */
/*
 * Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon Phillips <jon@rejon.org>
 *   Ralf Stephan <ralf@ark.in-berlin.de> (Gtkmm)
 *   Bob Jamison <ishmal@users.sf.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2000 - 2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "page-sizer.h"
#include "pages-skeleton.h"
#include <glib.h>
#include <glibmm/i18n.h>
#include "verbs.h"
#include "helper/action.h"
#include "object/sp-root.h"
#include "io/resource.h"

namespace Inkscape {
namespace UI {
namespace Widget {


//########################################################################
//# P A G E    S I Z E R
//########################################################################

/**
 * Constructor
 */
PageSizer::PageSizer(Registry & _wr)
    : Gtk::VBox(false,4),
      _dimensionUnits( _("U_nits:"), "units", _wr ),
      _dimensionWidth( _("_Width:"), _("Width of paper"), "width", _dimensionUnits, _wr ),
      _dimensionHeight( _("_Height:"), _("Height of paper"), "height", _dimensionUnits, _wr ),
      _marginLock( _("Loc_k margins"), _("Lock margins"), "lock-margins", _wr, false, nullptr, nullptr),
      _lock_icon(),
      _marginTop( _("T_op:"), _("Top margin"), "fit-margin-top", _wr ),
      _marginLeft( _("L_eft:"), _("Left margin"), "fit-margin-left", _wr),
      _marginRight( _("Ri_ght:"), _("Right margin"), "fit-margin-right", _wr),
      _marginBottom( _("Botto_m:"), _("Bottom margin"), "fit-margin-bottom", _wr),
      _lockMarginUpdate(false),
      _scaleX(_("Scale _x:"), _("Scale X"), "scale-x", _wr),
      _scaleY(_("Scale _y:"), _("While SVG allows non-uniform scaling it is recommended to use only uniform scaling in Inkscape. To set a non-uniform scaling, set the 'viewBox' directly."), "scale-y", _wr),
      _lockScaleUpdate(false),
      _viewboxX(_("X:"),      _("X"),      "viewbox-x", _wr),
      _viewboxY(_("Y:"),      _("Y"),      "viewbox-y", _wr),
      _viewboxW(_("Width:"),  _("Width"),  "viewbox-width", _wr),
      _viewboxH(_("Height:"), _("Height"), "viewbox-height", _wr),
      _lockViewboxUpdate(false),
      _widgetRegistry(&_wr)
{
    // set precision of scalar entry boxes
    _wr.setUpdating (true);
    _dimensionWidth.setDigits(5);
    _dimensionHeight.setDigits(5);
    _marginTop.setDigits(5);
    _marginLeft.setDigits(5);
    _marginRight.setDigits(5);
    _marginBottom.setDigits(5);
    _scaleX.setDigits(5);
    _scaleY.setDigits(5);
    _viewboxX.setDigits(5);
    _viewboxY.setDigits(5);
    _viewboxW.setDigits(5);
    _viewboxH.setDigits(5);
    _dimensionWidth.setRange( 0.00001, 10000000 );
    _dimensionHeight.setRange( 0.00001, 10000000 );
    _scaleX.setRange( 0.00001, 100000 );
    _scaleY.setRange( 0.00001, 100000 );
    _viewboxX.setRange( -10000000, 10000000 );
    _viewboxY.setRange( -10000000, 10000000 );
    _viewboxW.setRange( 0.00001, 10000000 );
    _viewboxH.setRange( 0.00001, 10000000 );

    _scaleY.set_sensitive (false); // We only want to display Y scale.

    _wr.setUpdating (false);

    //# Set up the Paper Size combo box
    _paperSizeListStore = Gtk::ListStore::create(_paperSizeListColumns);
    _paperSizeList.set_model(_paperSizeListStore);
    _paperSizeList.append_column(_("Name"),
                                 _paperSizeListColumns.nameColumn);
    _paperSizeList.append_column(_("Description"),
                                 _paperSizeListColumns.descColumn);
    _paperSizeList.set_headers_visible(false);
    _paperSizeListSelection = _paperSizeList.get_selection();
    _paper_size_list_connection =
        _paperSizeListSelection->signal_changed().connect (
            sigc::mem_fun (*this, &PageSizer::on_paper_size_list_changed));
    _paperSizeListScroller.add(_paperSizeList);
    _paperSizeListScroller.set_shadow_type(Gtk::SHADOW_IN);
    _paperSizeListScroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    _paperSizeListScroller.set_size_request(-1, 130);


    char *path = Inkscape::IO::Resource::profile_path("pages.csv");
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        if (!g_file_set_contents(path, pages_skeleton, -1, nullptr)) {
            g_warning("%s", _("Failed to create the page file."));
        }
    }

    gchar *content = nullptr;
    if (g_file_get_contents(path, &content, nullptr, nullptr)) {

        gchar **lines = g_strsplit_set(content, "\n", 0);

        for (int i = 0; lines && lines[i]; ++i) {
            gchar **line = g_strsplit_set(lines[i], ",", 5);
            if (!line[0] || !line[1] || !line[2] || !line[3] || line[0][0]=='#')
                continue;
            //name, width, height, unit
            double width = g_ascii_strtod(line[1], nullptr);
            double height = g_ascii_strtod(line[2], nullptr);
            g_strstrip(line[0]);
            g_strstrip(line[3]);
            Glib::ustring name = line[0];
            char formatBuf[80];
            snprintf(formatBuf, 79, "%0.1f x %0.1f", width, height);
            Glib::ustring desc = formatBuf;
            desc.append(" " + std::string(line[3]));
            PaperSize paper(name, width, height, Inkscape::Util::unit_table.getUnit(line[3]));
            _paperSizeTable[name] = paper;
            Gtk::TreeModel::Row row = *(_paperSizeListStore->append());
            row[_paperSizeListColumns.nameColumn] = name;
            row[_paperSizeListColumns.descColumn] = desc;
            g_strfreev(line);
        }
        g_strfreev(lines);
        g_free(content);
    }
    g_free(path);

    pack_start (_paperSizeListScroller, true, true, 0);

    //## Set up orientation radio buttons
    pack_start (_orientationBox, false, false, 0);
    _orientationLabel.set_label(_("Orientation:"));
    _orientationBox.pack_start(_orientationLabel, false, false, 0);
    _landscapeButton.set_use_underline();
    _landscapeButton.set_label(_("_Landscape"));
    _landscapeButton.set_active(true);
    Gtk::RadioButton::Group group = _landscapeButton.get_group();
    _orientationBox.pack_end (_landscapeButton, false, false, 5);
    _portraitButton.set_use_underline();
    _portraitButton.set_label(_("_Portrait"));
    _portraitButton.set_active(true);
    _orientationBox.pack_end (_portraitButton, false, false, 5);
    _portraitButton.set_group (group);
    _portraitButton.set_active (true);

    // Setting default custom unit to document unit
    SPDesktop *dt = SP_ACTIVE_DESKTOP;
    SPNamedView *nv = dt->getNamedView();
    _wr.setUpdating (true);
    if (nv->page_size_units) {
        _dimensionUnits.setUnit(nv->page_size_units->abbr);
    } else if (nv->display_units) {
        _dimensionUnits.setUnit(nv->display_units->abbr);
    }
    _wr.setUpdating (false);


    //## Set up custom size frame
    _customFrame.set_label(_("Custom size"));
    pack_start (_customFrame, false, false, 0);
    _customFrame.add(_customDimTable);

    _customDimTable.set_border_width(4);
    _customDimTable.set_row_spacing(4);
    _customDimTable.set_column_spacing(4);

    _dimensionHeight.set_halign(Gtk::ALIGN_CENTER);
    _dimensionUnits.set_halign(Gtk::ALIGN_END);
    _customDimTable.attach(_dimensionWidth,        0, 0, 1, 1);
    _customDimTable.attach(_dimensionHeight,       1, 0, 1, 1);
    _customDimTable.attach(_dimensionUnits,        2, 0, 1, 1);

    _customDimTable.attach(_fitPageMarginExpander, 0, 1, 3, 1);

    //## Set up fit page expander
    _fitPageMarginExpander.set_use_underline();
    _fitPageMarginExpander.set_label(_("Resi_ze page to content..."));
    _fitPageMarginExpander.add(_marginTable);

    _marginTable.set_border_width(4);
    _marginTable.set_row_spacing(4);
    _marginTable.set_column_spacing(4);

    //### margin label and lock button
    _marginLabel.set_markup(Glib::ustring("<b><i>") + _("Margins") + "</i></b>");
    _marginLabel.set_halign(Gtk::ALIGN_CENTER);

    _lock_icon.set_from_icon_name("object-unlocked", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    _lock_icon.show();
    _marginLock.set_active(false);
    _marginLock.add(_lock_icon);

    _marginBox.set_spacing(4);
    _marginBox.add(_marginLabel);
    _marginBox.add(_marginLock);
    _marginBox.set_halign(Gtk::ALIGN_CENTER);
    _marginTable.attach(_marginBox,      1, 1, 1, 1);

    //### margins
    _marginTop.set_halign(Gtk::ALIGN_CENTER);
    _marginLeft.set_halign(Gtk::ALIGN_START);
    _marginRight.set_halign(Gtk::ALIGN_END);
    _marginBottom.set_halign(Gtk::ALIGN_CENTER);

    _marginTable.attach(_marginTop,      0, 0, 3, 1);
    _marginTable.attach(_marginLeft,     0, 1, 1, 1);
    _marginTable.attach(_marginRight,    2, 1, 1, 1);
    _marginTable.attach(_marginBottom,   0, 2, 3, 1);

    //### fit page to drawing button
    _fitPageButton.set_use_underline();
    _fitPageButton.set_label(_("_Resize page to drawing or selection (Ctrl+Shift+R)"));
    _fitPageButton.set_tooltip_text(_("Resize the page to fit the current selection, or the entire drawing if there is no selection"));

    _fitPageButton.set_hexpand();
    _fitPageButton.set_halign(Gtk::ALIGN_CENTER);
    _marginTable.attach(_fitPageButton,  0, 3, 3, 1);


    //## Set up scale frame
    _scaleFrame.set_label(_("Scale"));
    pack_start (_scaleFrame, false, false, 0);
    _scaleFrame.add(_scaleTable);

    _scaleTable.set_border_width(4);
    _scaleTable.set_row_spacing(4);
    _scaleTable.set_column_spacing(4);

    _scaleTable.attach(_scaleX,          0, 0, 1, 1);
    _scaleTable.attach(_scaleY,          1, 0, 1, 1);
    _scaleTable.attach(_scaleLabel,      2, 0, 1, 1);

    _viewboxExpander.set_hexpand();
    _scaleTable.attach(_viewboxExpander, 0, 2, 3, 1);

    _viewboxExpander.set_use_underline();
    _viewboxExpander.set_label(_("_Viewbox..."));
    _viewboxExpander.add(_viewboxTable);

    _viewboxTable.set_border_width(4);
    _viewboxTable.set_row_spacing(4);
    _viewboxTable.set_column_spacing(4);

    _viewboxX.set_halign(Gtk::ALIGN_END);
    _viewboxY.set_halign(Gtk::ALIGN_END);
    _viewboxW.set_halign(Gtk::ALIGN_END);
    _viewboxH.set_halign(Gtk::ALIGN_END);
    _viewboxSpacer.set_hexpand();
    _viewboxTable.attach(_viewboxX,      0, 0, 1, 1);
    _viewboxTable.attach(_viewboxY,      1, 0, 1, 1);
    _viewboxTable.attach(_viewboxW,      0, 1, 1, 1);
    _viewboxTable.attach(_viewboxH,      1, 1, 1, 1);
    _viewboxTable.attach(_viewboxSpacer, 2, 0, 3, 1);

    _wr.setUpdating (true);
    updateScaleUI();
    _wr.setUpdating (false);
}


/**
 * Destructor
 */
PageSizer::~PageSizer()
= default;



/**
 * Initialize or reset this widget
 */
void
PageSizer::init ()
{
    _landscape_connection = _landscapeButton.signal_toggled().connect (sigc::mem_fun (*this, &PageSizer::on_landscape));
    _portrait_connection = _portraitButton.signal_toggled().connect (sigc::mem_fun (*this, &PageSizer::on_portrait));
    _changedw_connection = _dimensionWidth.signal_value_changed().connect (sigc::mem_fun (*this, &PageSizer::on_value_changed));
    _changedh_connection = _dimensionHeight.signal_value_changed().connect (sigc::mem_fun (*this, &PageSizer::on_value_changed));
    _changedu_connection = _dimensionUnits.getUnitMenu()->signal_changed().connect (sigc::mem_fun (*this, &PageSizer::on_units_changed));
    _fitPageButton.signal_clicked().connect(sigc::mem_fun(*this, &PageSizer::fire_fit_canvas_to_selection_or_drawing));
    _changeds_connection  = _scaleX.signal_value_changed().connect (sigc::mem_fun (*this, &PageSizer::on_scale_changed));
    _changedvx_connection = _viewboxX.signal_value_changed().connect (sigc::mem_fun (*this, &PageSizer::on_viewbox_changed));
    _changedvy_connection = _viewboxY.signal_value_changed().connect (sigc::mem_fun (*this, &PageSizer::on_viewbox_changed));
    _changedvw_connection = _viewboxW.signal_value_changed().connect (sigc::mem_fun (*this, &PageSizer::on_viewbox_changed));
    _changedvh_connection = _viewboxH.signal_value_changed().connect (sigc::mem_fun (*this, &PageSizer::on_viewbox_changed));
    _changedlk_connection = _marginLock.signal_toggled().connect (sigc::mem_fun (*this, &PageSizer::on_margin_lock_changed));
    _changedmt_connection = _marginTop.signal_value_changed().connect (sigc::bind<RegisteredScalar*>(sigc::mem_fun (*this, &PageSizer::on_margin_changed), &_marginTop));
    _changedmb_connection = _marginBottom.signal_value_changed().connect (sigc::bind<RegisteredScalar*>(sigc::mem_fun (*this, &PageSizer::on_margin_changed), &_marginBottom));
    _changedml_connection = _marginLeft.signal_value_changed().connect (sigc::bind<RegisteredScalar*>(sigc::mem_fun (*this, &PageSizer::on_margin_changed), &_marginLeft));
    _changedmr_connection = _marginRight.signal_value_changed().connect (sigc::bind<RegisteredScalar*>(sigc::mem_fun (*this, &PageSizer::on_margin_changed), &_marginRight));
    show_all_children();
}


/**
 * Set document dimensions (if not called by Doc prop's update()) and
 * set the PageSizer's widgets and text entries accordingly. If
 * 'changeList' is true, then adjust the paperSizeList to show the closest
 * standard page size.
 *
 * \param w, h
 * \param changeList whether to modify the paper size list
 */
void
PageSizer::setDim (Inkscape::Util::Quantity w, Inkscape::Util::Quantity h, bool changeList, bool changeSize)
{
    static bool _called = false;
    if (_called) {
        return;
    }

    _called = true;

    _paper_size_list_connection.block();
    _landscape_connection.block();
    _portrait_connection.block();
    _changedw_connection.block();
    _changedh_connection.block();

    _unit = w.unit->abbr;

    if (SP_ACTIVE_DESKTOP && !_widgetRegistry->isUpdating()) {
        SPDocument *doc = SP_ACTIVE_DESKTOP->getDocument();
        Inkscape::Util::Quantity const old_height = doc->getHeight();
        doc->setWidthAndHeight (w, h, changeSize);
        // The origin for the user is in the lower left corner; this point should remain stationary when
        // changing the page size. The SVG's origin however is in the upper left corner, so we must compensate for this
        if (changeSize && !doc->is_yaxisdown()) {
            Geom::Translate const vert_offset(Geom::Point(0, (old_height.value("px") - h.value("px"))));
            doc->getRoot()->translateChildItems(vert_offset);
        }
        DocumentUndo::done(doc, SP_VERB_NONE, _("Set page size"));
    }

    if ( w != h ) {
        _landscapeButton.set_sensitive(true);
        _portraitButton.set_sensitive (true);
        _landscape = ( w > h );
        _landscapeButton.set_active(_landscape ? true : false);
        _portraitButton.set_active (_landscape ? false : true);
    } else {
        _landscapeButton.set_sensitive(false);
        _portraitButton.set_sensitive (false);
    }

    if (changeList)
        {
        Gtk::TreeModel::Row row = (*find_paper_size(w, h));
        if (row)
            _paperSizeListSelection->select(row);
        }

    _dimensionWidth.setUnit(w.unit->abbr);
    _dimensionWidth.setValue (w.quantity);
    _dimensionHeight.setUnit(h.unit->abbr);
    _dimensionHeight.setValue (h.quantity);


    _paper_size_list_connection.unblock();
    _landscape_connection.unblock();
    _portrait_connection.unblock();
    _changedw_connection.unblock();
    _changedh_connection.unblock();

    _called = false;
}

/**
 * Updates the scalar widgets for the fit margins.  (Just changes the value
 * of the ui widgets to match the xml).
 */
void
PageSizer::updateFitMarginsUI(Inkscape::XML::Node *nv_repr)
{
    if (!_lockMarginUpdate) {
        double value = 0.0;
        if (sp_repr_get_double(nv_repr, "fit-margin-top", &value)) {
            _marginTop.setValue(value);
        }
        if (sp_repr_get_double(nv_repr, "fit-margin-left", &value)) {
            _marginLeft.setValue(value);
        }
        if (sp_repr_get_double(nv_repr, "fit-margin-right", &value)) {
            _marginRight.setValue(value);
        }
        if (sp_repr_get_double(nv_repr, "fit-margin-bottom", &value)) {
            _marginBottom.setValue(value);
        }
    }
}


/**
 * Returns an iterator pointing to a row in paperSizeListStore which
 * contains a paper of the specified size, or
 * paperSizeListStore->children().end() if no such paper exists.
 *
 * The code is not tested for the case where w and h have different units.
 */
Gtk::ListStore::iterator
PageSizer::find_paper_size (Inkscape::Util::Quantity w, Inkscape::Util::Quantity h) const
{
    // The code below assumes that w < h, so make sure that's the case:
    if ( h < w ) {
        std::swap(h,w);
    }

    std::map<Glib::ustring, PaperSize>::const_iterator iter;
    for (iter = _paperSizeTable.begin() ;
         iter != _paperSizeTable.end() ; ++iter) {
        PaperSize paper = iter->second;
        Inkscape::Util::Quantity smallX (paper.smaller, paper.unit);
        Inkscape::Util::Quantity largeX (paper.larger, paper.unit);

        // account for landscape formats (e.g. business cards)
        if (largeX < smallX) {
            std::swap(largeX, smallX);
        }

        if ( are_near(w, smallX, 0.1) && are_near(h, largeX, 0.1) ) {
            Gtk::ListStore::iterator p = _paperSizeListStore->children().begin();
            Gtk::ListStore::iterator pend = _paperSizeListStore->children().end();
            // We need to search paperSizeListStore explicitly for the
            // specified paper size because it is sorted in a different
            // way than paperSizeTable (which is sorted alphabetically)
            for ( ; p != pend; ++p) {
                if ((*p)[_paperSizeListColumns.nameColumn] == paper.name) {
                    return p;
                }
            }
        }
    }
    return _paperSizeListStore->children().end();
}



/**
 * Tell the desktop to fit the page size to the selection or drawing.
 */
void
PageSizer::fire_fit_canvas_to_selection_or_drawing()
{
    SPDesktop *dt = SP_ACTIVE_DESKTOP;
    if (!dt) {
        return;
    }
    SPDocument *doc;
    SPNamedView *nv;
    Inkscape::XML::Node *nv_repr;

    if ((doc = SP_ACTIVE_DESKTOP->getDocument())
        && (nv = sp_document_namedview(doc, nullptr))
        && (nv_repr = nv->getRepr())) {
        _lockMarginUpdate = true;
        sp_repr_set_svg_double(nv_repr, "fit-margin-top", _marginTop.getValue());
        sp_repr_set_svg_double(nv_repr, "fit-margin-left", _marginLeft.getValue());
        sp_repr_set_svg_double(nv_repr, "fit-margin-right", _marginRight.getValue());
        sp_repr_set_svg_double(nv_repr, "fit-margin-bottom", _marginBottom.getValue());
        _lockMarginUpdate = false;
    }

    Verb *verb = Verb::get( SP_VERB_FIT_CANVAS_TO_SELECTION_OR_DRAWING );
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(dt));
        if (action) {
            sp_action_perform(action, nullptr);
        }
    }
}



/**
 * Paper Size list callback for when a user changes the selection
 */
void
PageSizer::on_paper_size_list_changed()
{
    //Glib::ustring name = _paperSizeList.get_active_text();
    Gtk::TreeModel::iterator miter = _paperSizeListSelection->get_selected();
    if(!miter)
        {
        //error?
        return;
        }
    Gtk::TreeModel::Row row = *miter;
    Glib::ustring name = row[_paperSizeListColumns.nameColumn];
    std::map<Glib::ustring, PaperSize>::const_iterator piter =
                    _paperSizeTable.find(name);
    if (piter == _paperSizeTable.end()) {
        g_warning("paper size '%s' not found in table", name.c_str());
        return;
    }
    PaperSize paper = piter->second;
    Inkscape::Util::Quantity w = Inkscape::Util::Quantity(paper.smaller, paper.unit);
    Inkscape::Util::Quantity h = Inkscape::Util::Quantity(paper.larger, paper.unit);

    if ( w > h ) {
        // enforce landscape mode if this is desired for the given page format
        _landscape = true;
    } else {
        // otherwise we keep the current mode
        _landscape = _landscapeButton.get_active();
    }

    if ((_landscape && (w < h)) || (!_landscape && (w > h)))
        setDim (h, w, false);
    else
        setDim (w, h, false);

}


/**
 * Portrait button callback
 */
void
PageSizer::on_portrait()
{
    if (!_portraitButton.get_active())
        return;
    Inkscape::Util::Quantity w = Inkscape::Util::Quantity(_dimensionWidth.getValue(""), _dimensionWidth.getUnit());
    Inkscape::Util::Quantity h = Inkscape::Util::Quantity(_dimensionHeight.getValue(""), _dimensionHeight.getUnit());
    if (h < w) {
        setDim (h, w);
    }
}


/**
 * Landscape button callback
 */
void
PageSizer::on_landscape()
{
    if (!_landscapeButton.get_active())
        return;
    Inkscape::Util::Quantity w = Inkscape::Util::Quantity(_dimensionWidth.getValue(""), _dimensionWidth.getUnit());
    Inkscape::Util::Quantity h = Inkscape::Util::Quantity(_dimensionHeight.getValue(""), _dimensionHeight.getUnit());
    if (w < h) {
        setDim (h, w);
    }
}


/**
 * Update scale widgets
 */
void
PageSizer::updateScaleUI()
{

    static bool _called = false;
    if (_called) {
        return;
    }

    _called = true;

    _changeds_connection.block();
    _changedvx_connection.block();
    _changedvy_connection.block();
    _changedvw_connection.block();
    _changedvh_connection.block();

    SPDesktop *dt = SP_ACTIVE_DESKTOP;
    if (dt) {
        SPDocument *doc = dt->getDocument();

        // Update scale
        Geom::Scale scale = doc->getDocumentScale();
        SPNamedView *nv = dt->getNamedView();

        std::stringstream ss;
        ss << _("User units per ") << nv->display_units->abbr << "." ;
        _scaleLabel.set_text( ss.str() );

        if( !_lockScaleUpdate ) {

            double scaleX_inv =
                Inkscape::Util::Quantity::convert( scale[Geom::X], "px", nv->display_units );
            if( scaleX_inv > 0 ) {
                _scaleX.setValue(1.0/scaleX_inv);
            } else {
                // Should never happen
                std::cerr << "PageSizer::updateScaleUI(): Invalid scale value: " << scaleX_inv << std::endl;
                _scaleX.setValue(1.0);
            }
        }

        {  // Don't need to lock as scaleY widget not linked to callback.
            double scaleY_inv =
                Inkscape::Util::Quantity::convert( scale[Geom::Y], "px", nv->display_units );
            if( scaleY_inv > 0 ) {
                _scaleY.setValue(1.0/scaleY_inv);
            } else {
                // Should never happen
                std::cerr << "PageSizer::updateScaleUI(): Invalid scale value: " << scaleY_inv << std::endl;
                _scaleY.setValue(1.0);
            }
        }

        if( !_lockViewboxUpdate ) {
            Geom::Rect viewBox = doc->getViewBox();
            _viewboxX.setValue( viewBox.min()[Geom::X] );
            _viewboxY.setValue( viewBox.min()[Geom::Y] );
            _viewboxW.setValue( viewBox.width() );
            _viewboxH.setValue( viewBox.height() );
        }

    } else {
        // Should never happen
        std::cerr << "PageSizer::updateScaleUI(): No active desktop." << std::endl;
        _scaleLabel.set_text( "Unknown scale" );
    }

    _changeds_connection.unblock();
    _changedvx_connection.unblock();
    _changedvy_connection.unblock();
    _changedvw_connection.unblock();
    _changedvh_connection.unblock();

    _called = false;
}


/**
 * Callback for the dimension widgets
 */
void
PageSizer::on_value_changed()
{
    if (_widgetRegistry->isUpdating()) return;
    if (_unit != _dimensionUnits.getUnit()->abbr) return;
    setDim (Inkscape::Util::Quantity(_dimensionWidth.getValue(""), _dimensionUnits.getUnit()),
            Inkscape::Util::Quantity(_dimensionHeight.getValue(""), _dimensionUnits.getUnit()));
}

void
PageSizer::on_units_changed()
{
    if (_widgetRegistry->isUpdating()) return;
    _unit = _dimensionUnits.getUnit()->abbr;
    setDim (Inkscape::Util::Quantity(_dimensionWidth.getValue(""), _dimensionUnits.getUnit()),
            Inkscape::Util::Quantity(_dimensionHeight.getValue(""), _dimensionUnits.getUnit()),
            true, false);
}

/**
 * Callback for scale widgets
 */
void
PageSizer::on_scale_changed()
{
    if (_widgetRegistry->isUpdating()) return;

    double value = _scaleX.getValue();
    if( value > 0 ) {

        SPDesktop *dt = SP_ACTIVE_DESKTOP;
        if (dt) {
            SPDocument *doc = dt->getDocument();
            SPNamedView *nv = dt->getNamedView();

            double scaleX_inv = Inkscape::Util::Quantity(1.0/value, nv->display_units ).value("px");

            _lockScaleUpdate = true;
            doc->setDocumentScale( 1.0/scaleX_inv );
            updateScaleUI();
            _lockScaleUpdate = false;
            DocumentUndo::done(doc, SP_VERB_NONE, _("Set page scale"));
        }
    }
}

/**
 * Callback for viewbox widgets
 */
void
PageSizer::on_viewbox_changed()
{
    if (_widgetRegistry->isUpdating()) return;

    double viewboxX = _viewboxX.getValue();
    double viewboxY = _viewboxY.getValue();
    double viewboxW = _viewboxW.getValue();
    double viewboxH = _viewboxH.getValue();

    if( viewboxW > 0 && viewboxH > 0) {
        SPDesktop *dt = SP_ACTIVE_DESKTOP;
        if (dt) {
            SPDocument *doc = dt->getDocument();
            _lockViewboxUpdate = true;
            doc->setViewBox( Geom::Rect::from_xywh( viewboxX, viewboxY, viewboxW, viewboxH ) );
            updateScaleUI();
            _lockViewboxUpdate = false;
            DocumentUndo::done(doc, SP_VERB_NONE, _("Set 'viewBox'"));
        }
    } else {
        std::cerr
            << "PageSizer::on_viewbox_changed(): width and height must both be greater than zero."
            << std::endl;
    }
}

void
PageSizer::on_margin_lock_changed()
{
    if (_marginLock.get_active()) {
        _lock_icon.set_from_icon_name("object-locked", Gtk::ICON_SIZE_LARGE_TOOLBAR);
        double left   = _marginLeft.getValue();
        double right  = _marginRight.getValue();
        double top    = _marginTop.getValue();
        //double bottom = _marginBottom.getValue();
        if (Geom::are_near(left,right)) {
            if (Geom::are_near(left, top)) {
                on_margin_changed(&_marginBottom);
            } else {
                on_margin_changed(&_marginTop);
            }
        } else {
            if (Geom::are_near(left, top)) {
                on_margin_changed(&_marginRight);
            } else {
                on_margin_changed(&_marginLeft);
            }
        }
    } else {
        _lock_icon.set_from_icon_name("object-unlocked", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    }
}

void
PageSizer::on_margin_changed(RegisteredScalar* widg)
{
    double value = widg->getValue();
    if (_widgetRegistry->isUpdating()) return;
    if (_marginLock.get_active() && !_lockMarginUpdate) {
        _lockMarginUpdate = true;
        _marginLeft.setValue(value);
        _marginRight.setValue(value);
        _marginTop.setValue(value);
        _marginBottom.setValue(value);
        _lockMarginUpdate = false;
    }
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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
