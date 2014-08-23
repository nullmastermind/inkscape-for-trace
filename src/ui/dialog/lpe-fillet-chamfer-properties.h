/**
 *
 * From the code of Liam P.White from his Power Stroke Knot dialog
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information
 */

#ifndef INKSCAPE_DIALOG_FILLET_CHAMFER_PROPERTIES_H
#define INKSCAPE_DIALOG_FILLET_CHAMFER_PROPERTIES_H

#if HAVE_CONFIG_H
 #include "config.h"
#endif

#include <2geom/point.h>
#include "live_effects/parameter/filletchamferpointarray.h"

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/radiobutton.h>

#if WITH_GTKMM_3_0
 #include <gtkmm/grid.h>
#else
 #include <gtkmm/table.h>
#endif

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Dialogs {

class FilletChamferPropertiesDialog : public Gtk::Dialog {
public:
    FilletChamferPropertiesDialog();
    virtual ~FilletChamferPropertiesDialog();

    Glib::ustring getName() const {
        return "LayerPropertiesDialog";
    }

    static void showDialog(SPDesktop *desktop, Geom::Point knotpoint,
                           const Inkscape::LivePathEffect::
                           FilletChamferPointArrayParamKnotHolderEntity *pt,
                           const gchar *unit);

protected:

    SPDesktop *_desktop;
    Inkscape::LivePathEffect::FilletChamferPointArrayParamKnotHolderEntity *
    _knotpoint;

    Gtk::Label _fillet_chamfer_position_label;
    Gtk::Entry _fillet_chamfer_position_entry;
    Gtk::RadioButton::Group _fillet_chamfer_type_group;
    Gtk::RadioButton _fillet_chamfer_type_fillet;
    Gtk::RadioButton _fillet_chamfer_type_inverse_fillet;
    Gtk::RadioButton _fillet_chamfer_type_chamfer;
    Gtk::RadioButton _fillet_chamfer_type_double_chamfer;

#if WITH_GTKMM_3_0
    Gtk::Grid _layout_table;
#else
    Gtk::Table _layout_table;
#endif

    bool _position_visible;
    double _index;

    Gtk::Button _close_button;
    Gtk::Button _apply_button;

    sigc::connection _destroy_connection;

    static FilletChamferPropertiesDialog &_instance() {
        static FilletChamferPropertiesDialog instance;
        return instance;
    }

    void _setDesktop(SPDesktop *desktop);
    void _setPt(const Inkscape::LivePathEffect::
                FilletChamferPointArrayParamKnotHolderEntity *pt);
    void _setUnit(const gchar *abbr);
    void _apply();
    void _close();
    bool _flexible;
    const gchar *unit;
    void _setKnotPoint(Geom::Point knotpoint);
    void _prepareLabelRenderer(Gtk::TreeModel::const_iterator const &row);

    bool _handleKeyEvent(GdkEventKey *event);
    void _handleButtonEvent(GdkEventButton *event);

    friend class Inkscape::LivePathEffect::
            FilletChamferPointArrayParamKnotHolderEntity;

private:
    FilletChamferPropertiesDialog(
        FilletChamferPropertiesDialog const &); // no copy
    FilletChamferPropertiesDialog &operator=(
        FilletChamferPropertiesDialog const &); // no assign
};

} // namespace
} // namespace
} // namespace

#endif //INKSCAPE_DIALOG_LAYER_PROPERTIES_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:
// filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99
// :
