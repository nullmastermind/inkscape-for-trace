// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Generic paint selector widget
 *//*
 * Authors:
 *   Lauris
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_PAINT_SELECTOR_H
#define SEEN_SP_PAINT_SELECTOR_H

#include "color.h"
#include "fill-or-stroke.h"
#include <glib.h>
#include <gtkmm/box.h>

#include "object/sp-gradient-spread.h"
#include "object/sp-gradient-units.h"

#include "ui/selected-color.h"

class SPGradient;
#ifdef WITH_MESH
class SPMeshGradient;
#endif
class SPDesktop;
class SPPattern;
class SPStyle;

namespace Gtk {
class Label;
class RadioButton;
class ToggleButton;
} // namespace Gtk

namespace Inkscape {
namespace UI {
namespace Widget {

class FillRuleRadioButton;
class StyleToggleButton;
class GradientSelector;

/**
 * Generic paint selector widget.
 */
class PaintSelector : public Gtk::Box {
  public:
    enum Mode {
        MODE_EMPTY,
        MODE_MULTIPLE,
        MODE_NONE,
        MODE_SOLID_COLOR,
        MODE_GRADIENT_LINEAR,
        MODE_GRADIENT_RADIAL,
#ifdef WITH_MESH
        MODE_GRADIENT_MESH,
#endif
        MODE_PATTERN,
        MODE_HATCH,
        MODE_SWATCH,
        MODE_UNSET
    };

    enum FillRule { FILLRULE_NONZERO, FILLRULE_EVENODD };

  private:
    bool _update = false;

    Mode _mode;

    Gtk::Box *_style;
    StyleToggleButton *_none;
    StyleToggleButton *_solid;
    StyleToggleButton *_gradient;
    StyleToggleButton *_radial;
#ifdef WITH_MESH
    StyleToggleButton *_mesh;
#endif
    StyleToggleButton *_pattern;
    StyleToggleButton *_swatch;
    StyleToggleButton *_unset;

    Gtk::Box *_fillrulebox;
    FillRuleRadioButton *_evenodd;
    FillRuleRadioButton *_nonzero;

    Gtk::Box *_frame;
    Gtk::Box *_selector = nullptr;
    Gtk::Label *_label;
    GtkWidget *_patternmenu = nullptr;
    bool _patternmenu_update = false;
#ifdef WITH_MESH
    GtkWidget *_meshmenu = nullptr;
    bool _meshmenu_update = false;
#endif

    Inkscape::UI::SelectedColor *_selected_color;
    bool _updating_color;

    void getColorAlpha(SPColor &color, gfloat &alpha) const;

    static gboolean isSeparator(GtkTreeModel *model, GtkTreeIter *iter, gpointer data);

  private:
    sigc::signal<void, FillRule> _signal_fillrule_changed;
    sigc::signal<void> _signal_dragged;
    sigc::signal<void, Mode> _signal_mode_changed;
    sigc::signal<void> _signal_grabbed;
    sigc::signal<void> _signal_released;
    sigc::signal<void> _signal_changed;

    StyleToggleButton *style_button_add(gchar const *px, PaintSelector::Mode mode, gchar const *tip);
    void style_button_toggled(StyleToggleButton *tb);
    void fillrule_toggled(FillRuleRadioButton *tb);
    void onSelectedColorGrabbed();
    void onSelectedColorDragged();
    void onSelectedColorReleased();
    void onSelectedColorChanged();
    void set_mode_empty();
    void set_style_buttons(Gtk::ToggleButton *active);
    void set_mode_multiple();
    void set_mode_none();
    GradientSelector *getGradientFromData() const;
    void clear_frame();
    void set_mode_unset();
    void set_mode_color(PaintSelector::Mode mode);
    void set_mode_gradient(PaintSelector::Mode mode);
#ifdef WITH_MESH
    void set_mode_mesh(PaintSelector::Mode mode);
#endif
    void set_mode_pattern(PaintSelector::Mode mode);
    void set_mode_hatch(PaintSelector::Mode mode);
    void set_mode_swatch(PaintSelector::Mode mode);

    void gradient_grabbed();
    void gradient_dragged();
    void gradient_released();
    void gradient_changed(SPGradient *gr);

    static void mesh_change(GtkWidget *widget, PaintSelector *psel);
    static void mesh_destroy(GtkWidget *widget, PaintSelector *psel);

    static void pattern_change(GtkWidget *widget, PaintSelector *psel);
    static void pattern_destroy(GtkWidget *widget, PaintSelector *psel);

  public:
    PaintSelector(FillOrStroke kind);
    ~PaintSelector() override;

    inline decltype(_signal_fillrule_changed) signal_fillrule_changed() const { return _signal_fillrule_changed; }
    inline decltype(_signal_dragged) signal_dragged() const { return _signal_dragged; }
    inline decltype(_signal_mode_changed) signal_mode_changed() const { return _signal_mode_changed; }
    inline decltype(_signal_grabbed) signal_grabbed() const { return _signal_grabbed; }
    inline decltype(_signal_released) signal_released() const { return _signal_released; }
    inline decltype(_signal_changed) signal_changed() const { return _signal_changed; }

    void setMode(Mode mode);
    static Mode getModeForStyle(SPStyle const &style, FillOrStroke kind);
    void setFillrule(FillRule fillrule);
    void setColorAlpha(SPColor const &color, float alpha);
    void setSwatch(SPGradient *vector);
    void setGradientLinear(SPGradient *vector);
    void setGradientRadial(SPGradient *vector);
#ifdef WITH_MESH
    void setGradientMesh(SPMeshGradient *array);
#endif
    void setGradientProperties(SPGradientUnits units, SPGradientSpread spread);
    void getGradientProperties(SPGradientUnits &units, SPGradientSpread &spread) const;

#ifdef WITH_MESH
    SPMeshGradient *getMeshGradient();
    void updateMeshList(SPMeshGradient *pat);
#endif

    void updatePatternList(SPPattern *pat);
    inline decltype(_mode) get_mode() const { return _mode; }

    // TODO move this elsewhere:
    void setFlatColor(SPDesktop *desktop, const gchar *color_property, const gchar *opacity_property);

    SPGradient *getGradientVector();
    void pushAttrsToGradient(SPGradient *gr) const;
    SPPattern *getPattern();
};

enum {
    COMBO_COL_LABEL = 0,
    COMBO_COL_STOCK = 1,
    COMBO_COL_PATTERN = 2,
    COMBO_COL_MESH = COMBO_COL_PATTERN,
    COMBO_COL_SEP = 3,
    COMBO_N_COLS = 4
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape
#endif // SEEN_SP_PAINT_SELECTOR_H

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
