// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2015, 2018 Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include <libnrtype/font-instance.h>

#include "font-variants.h"

// For updating from selection
#include "desktop.h"
#include "object/sp-text.h"

namespace Inkscape {
namespace UI {
namespace Widget {

  // A simple class to handle UI for one feature. We could of derived this from Gtk::HBox but by
  // attaching widgets directly to Gtk::Grid, we keep columns lined up (which may or may not be a
  // good thing).
  class Feature
  {
  public:
      Feature( const Glib::ustring& name, OTSubstitution& glyphs, int options, Glib::ustring family, Gtk::Grid& grid, int &row, FontVariants* parent)
          : _name (name)
          , _options (options)
      {
          Gtk::Label* table_name = Gtk::manage (new Gtk::Label());
          table_name->set_markup ("\"" + name + "\" ");

          grid.attach (*table_name, 0, row, 1, 1);

          Gtk::FlowBox* flow_box = nullptr;
          Gtk::ScrolledWindow* scrolled_window = nullptr;
          if (options > 2) {
              // If there are more than 2 option, pack them into a flowbox instead of directly putting them in the grid.
              // Some fonts might have a table with many options (Bungee Hairline table 'ornm' has 113 entries).
              flow_box = Gtk::manage (new Gtk::FlowBox());
              flow_box->set_selection_mode(); // Turn off selection
              flow_box->set_homogeneous();
              flow_box->set_max_children_per_line (100); // Override default value
              flow_box->set_min_children_per_line (10);  // Override default value

              // We pack this into a scrollbar... otherwise the minimum height is set to what is required to fit all
              // flow box children into the flow box when the flow box has minimum width. (Crazy if you ask me!)
              scrolled_window = Gtk::manage (new Gtk::ScrolledWindow());
              scrolled_window->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
              scrolled_window->add(*flow_box);
          }

          Gtk::RadioButton::Group group;
          for (int i = 0; i < options; ++i) {

              // Create radio button and create or add to button group.
              Gtk::RadioButton* button = Gtk::manage (new Gtk::RadioButton());
              if (i == 0) {
                  group = button->get_group();
              } else {
                  button->set_group (group);
              }
              button->signal_clicked().connect ( sigc::mem_fun(*parent, &FontVariants::feature_callback) );
              buttons.push_back (button);

              // Create label.
              Gtk::Label* label = Gtk::manage (new Gtk::Label());

              // Restrict label width (some fonts have lots of alternatives).
              label->set_line_wrap( true );
              label->set_line_wrap_mode( Pango::WRAP_WORD_CHAR );
              label->set_ellipsize( Pango::ELLIPSIZE_END );
              label->set_lines(3);
              label->set_hexpand();

              Glib::ustring markup;
              markup += "<span font_family='";
              markup += family;
              markup += "' font_features='";
              markup += name;
              markup += " ";
              markup += std::to_string (i);
              markup += "'>";
              markup += Glib::Markup::escape_text (glyphs.input);
              markup += "</span>";
              label->set_markup (markup);

              // Add button and label to widget
              if (!flow_box) {
                  // Attach directly to grid (keeps things aligned row-to-row).
                  grid.attach (*button, 2*i+1, row, 1, 1);
                  grid.attach (*label,  2*i+2, row, 1, 1);
              } else {
                  // Pack into FlowBox

                  // Pack button and label into a box so they stay together.
                  Gtk::Box* box = Gtk::manage (new Gtk::Box());
                  box->add(*button);
                  box->add(*label);

                  flow_box->add(*box);
              }
          }

          if (scrolled_window) {
              grid.attach (*scrolled_window, 1, row, 4, 1);
          }
      }

      Glib::ustring
      get_css()
      {
          int i = 0;
          for (auto b: buttons) {
              if (b->get_active()) {
                  if (i == 0) {
                      // Features are always off by default (for those handled here).
                      return "";
                  } else if (i == 1) {
                      // Feature without value has implied value of 1.
                      return ("\"" + _name + "\", ");
                  } else {
                      // Feature with value greater than 1 must be explicitly set.
                      return ("\"" + _name + "\" " + std::to_string (i) + ", ");
                  }
              }
              ++i;
          }
          return "";
      }

      void
      set_active(int i)
      {
          if (i < buttons.size()) {
              buttons[i]->set_active();
          }
      }

  private:
      Glib::ustring _name;
      int _options;
      std::vector <Gtk::RadioButton*> buttons;
  };

  FontVariants::FontVariants () :
    Gtk::VBox (),
    _ligatures_frame          ( Glib::ustring(C_("Font feature", "Ligatures"    )) ),
    _ligatures_common         ( Glib::ustring(C_("Font feature", "Common"       )) ),
    _ligatures_discretionary  ( Glib::ustring(C_("Font feature", "Discretionary")) ),
    _ligatures_historical     ( Glib::ustring(C_("Font feature", "Historical"   )) ),
    _ligatures_contextual     ( Glib::ustring(C_("Font feature", "Contextual"   )) ),

    _position_frame           ( Glib::ustring(C_("Font feature", "Position"     )) ),
    _position_normal          ( Glib::ustring(C_("Font feature", "Normal"       )) ),
    _position_sub             ( Glib::ustring(C_("Font feature", "Subscript"    )) ),
    _position_super           ( Glib::ustring(C_("Font feature", "Superscript"  )) ),

    _caps_frame               ( Glib::ustring(C_("Font feature", "Capitals"     )) ),
    _caps_normal              ( Glib::ustring(C_("Font feature", "Normal"       )) ),
    _caps_small               ( Glib::ustring(C_("Font feature", "Small"        )) ),
    _caps_all_small           ( Glib::ustring(C_("Font feature", "All small"    )) ),
    _caps_petite              ( Glib::ustring(C_("Font feature", "Petite"       )) ),
    _caps_all_petite          ( Glib::ustring(C_("Font feature", "All petite"   )) ),
    _caps_unicase             ( Glib::ustring(C_("Font feature", "Unicase"      )) ),
    _caps_titling             ( Glib::ustring(C_("Font feature", "Titling"      )) ),

    _numeric_frame            ( Glib::ustring(C_("Font feature", "Numeric"      )) ),
    _numeric_lining           ( Glib::ustring(C_("Font feature", "Lining"       )) ),
    _numeric_old_style        ( Glib::ustring(C_("Font feature", "Old Style"    )) ),
    _numeric_default_style    ( Glib::ustring(C_("Font feature", "Default Style")) ),
    _numeric_proportional     ( Glib::ustring(C_("Font feature", "Proportional" )) ),
    _numeric_tabular          ( Glib::ustring(C_("Font feature", "Tabular"      )) ),
    _numeric_default_width    ( Glib::ustring(C_("Font feature", "Default Width")) ),
    _numeric_diagonal         ( Glib::ustring(C_("Font feature", "Diagonal"     )) ),
    _numeric_stacked          ( Glib::ustring(C_("Font feature", "Stacked"      )) ),
    _numeric_default_fractions( Glib::ustring(C_("Font feature", "Default Fractions")) ),
    _numeric_ordinal          ( Glib::ustring(C_("Font feature", "Ordinal"      )) ),
    _numeric_slashed_zero     ( Glib::ustring(C_("Font feature", "Slashed Zero" )) ),

    _asian_frame              ( Glib::ustring(C_("Font feature", "East Asian"   )) ),
    _asian_default_variant    ( Glib::ustring(C_("Font feature", "Default"      )) ),
    _asian_jis78              ( Glib::ustring(C_("Font feature", "JIS78"        )) ),
    _asian_jis83              ( Glib::ustring(C_("Font feature", "JIS83"        )) ),
    _asian_jis90              ( Glib::ustring(C_("Font feature", "JIS90"        )) ),
    _asian_jis04              ( Glib::ustring(C_("Font feature", "JIS04"        )) ),
    _asian_simplified         ( Glib::ustring(C_("Font feature", "Simplified"   )) ),
    _asian_traditional        ( Glib::ustring(C_("Font feature", "Traditional"  )) ),
    _asian_default_width      ( Glib::ustring(C_("Font feature", "Default"      )) ),
    _asian_full_width         ( Glib::ustring(C_("Font feature", "Full Width"   )) ),
    _asian_proportional_width ( Glib::ustring(C_("Font feature", "Proportional" )) ),
    _asian_ruby               ( Glib::ustring(C_("Font feature", "Ruby"         )) ),

    _feature_frame            ( Glib::ustring(C_("Font feature", "Feature Settings")) ),
    _feature_label            ( Glib::ustring(C_("Font feature", "Selection has different Feature Settings!")) ),
    
    _ligatures_changed( false ),
    _position_changed( false ),
    _caps_changed( false ),
    _numeric_changed( false ),
    _asian_changed( false )

  {

    set_name ( "FontVariants" );

    // Ligatures --------------------------

    // Add tooltips
    _ligatures_common.set_tooltip_text(
      _("Common ligatures. On by default. OpenType tables: 'liga', 'clig'"));
    _ligatures_discretionary.set_tooltip_text(
      _("Discretionary ligatures. Off by default. OpenType table: 'dlig'"));
    _ligatures_historical.set_tooltip_text(
      _("Historical ligatures. Off by default. OpenType table: 'hlig'"));
    _ligatures_contextual.set_tooltip_text(
      _("Contextual forms. On by default. OpenType table: 'calt'"));

    // Add signals
    _ligatures_common.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::ligatures_callback) );
    _ligatures_discretionary.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::ligatures_callback) );
    _ligatures_historical.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::ligatures_callback) );
    _ligatures_contextual.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::ligatures_callback) );

    // Restrict label widths (some fonts have lots of ligatures). Must also set ellipsize mode.
    _ligatures_label_common.set_max_width_chars(        60 );
    _ligatures_label_discretionary.set_max_width_chars( 60 );
    _ligatures_label_historical.set_max_width_chars(    60 );
    _ligatures_label_contextual.set_max_width_chars(    60 );

    _ligatures_label_common.set_ellipsize(        Pango::ELLIPSIZE_END );
    _ligatures_label_discretionary.set_ellipsize( Pango::ELLIPSIZE_END );
    _ligatures_label_historical.set_ellipsize(    Pango::ELLIPSIZE_END );
    _ligatures_label_contextual.set_ellipsize(    Pango::ELLIPSIZE_END );

    _ligatures_label_common.set_lines(        5 );
    _ligatures_label_discretionary.set_lines( 5 );
    _ligatures_label_historical.set_lines(    5 );
    _ligatures_label_contextual.set_lines(    5 );

    // Allow user to select characters. Not useful as this selects the ligatures.
    // _ligatures_label_common.set_selectable(        true );
    // _ligatures_label_discretionary.set_selectable( true );
    // _ligatures_label_historical.set_selectable(    true );
    // _ligatures_label_contextual.set_selectable(    true );

    // Add to frame
    _ligatures_grid.attach( _ligatures_common,              0, 0, 1, 1);
    _ligatures_grid.attach( _ligatures_discretionary,       0, 1, 1, 1);
    _ligatures_grid.attach( _ligatures_historical,          0, 2, 1, 1);
    _ligatures_grid.attach( _ligatures_contextual,          0, 3, 1, 1);
    _ligatures_grid.attach( _ligatures_label_common,        1, 0, 1, 1);
    _ligatures_grid.attach( _ligatures_label_discretionary, 1, 1, 1, 1);
    _ligatures_grid.attach( _ligatures_label_historical,    1, 2, 1, 1);
    _ligatures_grid.attach( _ligatures_label_contextual,    1, 3, 1, 1);

    _ligatures_grid.set_margin_start(15);
    _ligatures_grid.set_margin_end(15);

    _ligatures_frame.add( _ligatures_grid );
    pack_start( _ligatures_frame, Gtk::PACK_SHRINK );

    ligatures_init();
    
    // Position ----------------------------------

    // Add tooltips
    _position_normal.set_tooltip_text( _("Normal position."));
    _position_sub.set_tooltip_text( _("Subscript. OpenType table: 'subs'") );
    _position_super.set_tooltip_text( _("Superscript. OpenType table: 'sups'") );

    // Group buttons
    Gtk::RadioButton::Group position_group = _position_normal.get_group();
    _position_sub.set_group(position_group);
    _position_super.set_group(position_group);

    // Add signals
    _position_normal.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::position_callback) );
    _position_sub.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::position_callback) );
    _position_super.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::position_callback) );

    // Add to frame
    _position_grid.attach( _position_normal, 0, 0, 1, 1);
    _position_grid.attach( _position_sub,    1, 0, 1, 1);
    _position_grid.attach( _position_super,  2, 0, 1, 1);

    _position_grid.set_margin_start(15);
    _position_grid.set_margin_end(15);

    _position_frame.add( _position_grid );
    pack_start( _position_frame, Gtk::PACK_SHRINK );

    position_init();

    // Caps ----------------------------------

    // Add tooltips
    _caps_normal.set_tooltip_text( _("Normal capitalization."));
    _caps_small.set_tooltip_text( _("Small-caps (lowercase). OpenType table: 'smcp'"));
    _caps_all_small.set_tooltip_text( _("All small-caps (uppercase and lowercase). OpenType tables: 'c2sc' and 'smcp'"));
    _caps_petite.set_tooltip_text( _("Petite-caps (lowercase). OpenType table: 'pcap'"));
    _caps_all_petite.set_tooltip_text( _("All petite-caps (uppercase and lowercase). OpenType tables: 'c2sc' and 'pcap'"));
    _caps_unicase.set_tooltip_text( _("Unicase (small caps for uppercase, normal for lowercase). OpenType table: 'unic'"));
    _caps_titling.set_tooltip_text( _("Titling caps (lighter-weight uppercase for use in titles). OpenType table: 'titl'"));

    // Group buttons
    Gtk::RadioButton::Group caps_group = _caps_normal.get_group();
    _caps_small.set_group(caps_group);
    _caps_all_small.set_group(caps_group);
    _caps_petite.set_group(caps_group);
    _caps_all_petite.set_group(caps_group);
    _caps_unicase.set_group(caps_group);
    _caps_titling.set_group(caps_group);

    // Add signals
    _caps_normal.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::caps_callback) );
    _caps_small.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::caps_callback) );
    _caps_all_small.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::caps_callback) );
    _caps_petite.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::caps_callback) );
    _caps_all_petite.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::caps_callback) );
    _caps_unicase.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::caps_callback) );
    _caps_titling.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::caps_callback) );

    // Add to frame
    _caps_grid.attach( _caps_normal,     0, 0, 1, 1);
    _caps_grid.attach( _caps_unicase,    1, 0, 1, 1);
    _caps_grid.attach( _caps_titling,    2, 0, 1, 1);
    _caps_grid.attach( _caps_small,      0, 1, 1, 1);
    _caps_grid.attach( _caps_all_small,  1, 1, 1, 1);
    _caps_grid.attach( _caps_petite,     2, 1, 1, 1);
    _caps_grid.attach( _caps_all_petite, 3, 1, 1, 1);

    _caps_grid.set_margin_start(15);
    _caps_grid.set_margin_end(15);

    _caps_frame.add( _caps_grid );
    pack_start( _caps_frame, Gtk::PACK_SHRINK );

    caps_init();

    // Numeric ------------------------------

    // Add tooltips
    _numeric_default_style.set_tooltip_text( _("Normal style."));
    _numeric_lining.set_tooltip_text( _("Lining numerals. OpenType table: 'lnum'"));
    _numeric_old_style.set_tooltip_text( _("Old style numerals. OpenType table: 'onum'"));
    _numeric_default_width.set_tooltip_text( _("Normal widths."));
    _numeric_proportional.set_tooltip_text( _("Proportional width numerals. OpenType table: 'pnum'"));
    _numeric_tabular.set_tooltip_text( _("Same width numerals. OpenType table: 'tnum'"));
    _numeric_default_fractions.set_tooltip_text( _("Normal fractions."));
    _numeric_diagonal.set_tooltip_text( _("Diagonal fractions. OpenType table: 'frac'"));
    _numeric_stacked.set_tooltip_text( _("Stacked fractions. OpenType table: 'afrc'"));
    _numeric_ordinal.set_tooltip_text( _("Ordinals (raised 'th', etc.). OpenType table: 'ordn'"));
    _numeric_slashed_zero.set_tooltip_text( _("Slashed zeros. OpenType table: 'zero'"));

    // Group buttons
    Gtk::RadioButton::Group style_group = _numeric_default_style.get_group();
    _numeric_lining.set_group(style_group);
    _numeric_old_style.set_group(style_group);

    Gtk::RadioButton::Group width_group = _numeric_default_width.get_group();
    _numeric_proportional.set_group(width_group);
    _numeric_tabular.set_group(width_group);

    Gtk::RadioButton::Group fraction_group = _numeric_default_fractions.get_group();
    _numeric_diagonal.set_group(fraction_group);
    _numeric_stacked.set_group(fraction_group);

    // Add signals
    _numeric_default_style.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_lining.signal_clicked().connect (        sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_old_style.signal_clicked().connect (     sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_default_width.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_proportional.signal_clicked().connect (  sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_tabular.signal_clicked().connect (       sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_default_fractions.signal_clicked().connect ( sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_diagonal.signal_clicked().connect (      sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_stacked.signal_clicked().connect (       sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_ordinal.signal_clicked().connect (       sigc::mem_fun(*this, &FontVariants::numeric_callback) );
    _numeric_slashed_zero.signal_clicked().connect (  sigc::mem_fun(*this, &FontVariants::numeric_callback) );

    // Add to frame
    _numeric_grid.attach (_numeric_default_style,        0, 0, 1, 1);
    _numeric_grid.attach (_numeric_lining,               1, 0, 1, 1);
    _numeric_grid.attach (_numeric_lining_label,         2, 0, 1, 1);
    _numeric_grid.attach (_numeric_old_style,            3, 0, 1, 1);
    _numeric_grid.attach (_numeric_old_style_label,      4, 0, 1, 1);

    _numeric_grid.attach (_numeric_default_width,        0, 1, 1, 1);
    _numeric_grid.attach (_numeric_proportional,         1, 1, 1, 1);
    _numeric_grid.attach (_numeric_proportional_label,   2, 1, 1, 1);
    _numeric_grid.attach (_numeric_tabular,              3, 1, 1, 1);
    _numeric_grid.attach (_numeric_tabular_label,        4, 1, 1, 1);

    _numeric_grid.attach (_numeric_default_fractions,    0, 2, 1, 1);
    _numeric_grid.attach (_numeric_diagonal,             1, 2, 1, 1);
    _numeric_grid.attach (_numeric_diagonal_label,       2, 2, 1, 1);
    _numeric_grid.attach (_numeric_stacked,              3, 2, 1, 1);
    _numeric_grid.attach (_numeric_stacked_label,        4, 2, 1, 1);

    _numeric_grid.attach (_numeric_ordinal,              0, 3, 1, 1);
    _numeric_grid.attach (_numeric_ordinal_label,        1, 3, 4, 1);

    _numeric_grid.attach (_numeric_slashed_zero,         0, 4, 1, 1);
    _numeric_grid.attach (_numeric_slashed_zero_label,   1, 4, 1, 1);

    _numeric_grid.set_margin_start(15);
    _numeric_grid.set_margin_end(15);

    _numeric_frame.add( _numeric_grid );
    pack_start( _numeric_frame, Gtk::PACK_SHRINK );
    
    // East Asian

    // Add tooltips
    _asian_default_variant.set_tooltip_text (  _("Default variant."));
    _asian_jis78.set_tooltip_text(             _("JIS78 forms. OpenType table: 'jp78'."));
    _asian_jis83.set_tooltip_text(             _("JIS83 forms. OpenType table: 'jp83'."));
    _asian_jis90.set_tooltip_text(             _("JIS90 forms. OpenType table: 'jp90'."));
    _asian_jis04.set_tooltip_text(             _("JIS2004 forms. OpenType table: 'jp04'."));
    _asian_simplified.set_tooltip_text(        _("Simplified forms. OpenType table: 'smpl'."));
    _asian_traditional.set_tooltip_text(       _("Traditional forms. OpenType table: 'trad'."));
    _asian_default_width.set_tooltip_text (    _("Default width."));
    _asian_full_width.set_tooltip_text(        _("Full width variants. OpenType table: 'fwid'."));
    _asian_proportional_width.set_tooltip_text(_("Proportional width variants. OpenType table: 'pwid'."));
    _asian_ruby.set_tooltip_text(              _("Ruby variants. OpenType table: 'ruby'."));

    // Add signals
    _asian_default_variant.signal_clicked().connect (   sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_jis78.signal_clicked().connect (             sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_jis83.signal_clicked().connect (             sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_jis90.signal_clicked().connect (             sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_jis04.signal_clicked().connect (             sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_simplified.signal_clicked().connect (        sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_traditional.signal_clicked().connect (       sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_default_width.signal_clicked().connect (     sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_full_width.signal_clicked().connect (        sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_proportional_width.signal_clicked().connect (sigc::mem_fun(*this, &FontVariants::asian_callback) );
    _asian_ruby.signal_clicked().connect(               sigc::mem_fun(*this, &FontVariants::asian_callback) );

    // Add to frame
    _asian_grid.attach (_asian_default_variant,         0, 0, 1, 1);
    _asian_grid.attach (_asian_jis78,                   1, 0, 1, 1);
    _asian_grid.attach (_asian_jis83,                   2, 0, 1, 1);
    _asian_grid.attach (_asian_jis90,                   1, 1, 1, 1);
    _asian_grid.attach (_asian_jis04,                   2, 1, 1, 1);
    _asian_grid.attach (_asian_simplified,              1, 2, 1, 1);
    _asian_grid.attach (_asian_traditional,             2, 2, 1, 1);
    _asian_grid.attach (_asian_default_width,           0, 3, 1, 1);
    _asian_grid.attach (_asian_full_width,              1, 3, 1, 1);
    _asian_grid.attach (_asian_proportional_width,      2, 3, 1, 1);
    _asian_grid.attach (_asian_ruby,                    0, 4, 1, 1);

    _asian_grid.set_margin_start(15);
    _asian_grid.set_margin_end(15);

    _asian_frame.add( _asian_grid );
    pack_start( _asian_frame, Gtk::PACK_SHRINK );

    // Group Buttons
    Gtk::RadioButton::Group asian_variant_group = _asian_default_variant.get_group();
    _asian_jis78.set_group(asian_variant_group);
    _asian_jis83.set_group(asian_variant_group);
    _asian_jis90.set_group(asian_variant_group);
    _asian_jis04.set_group(asian_variant_group);
    _asian_simplified.set_group(asian_variant_group);
    _asian_traditional.set_group(asian_variant_group);

    Gtk::RadioButton::Group asian_width_group = _asian_default_width.get_group();
    _asian_full_width.set_group (asian_width_group);
    _asian_proportional_width.set_group (asian_width_group);

    // Feature settings ---------------------

    // Add tooltips
    _feature_entry.set_tooltip_text( _("Feature settings in CSS form (e.g. \"wxyz\" or \"wxyz\" 3)."));

    _feature_substitutions.set_justify( Gtk::JUSTIFY_LEFT );
    _feature_substitutions.set_line_wrap( true );
    _feature_substitutions.set_line_wrap_mode( Pango::WRAP_WORD_CHAR );

    _feature_list.set_justify( Gtk::JUSTIFY_LEFT );
    _feature_list.set_line_wrap( true );

    // Add to frame
    _feature_vbox.pack_start( _feature_grid,          Gtk::PACK_SHRINK );
    _feature_vbox.pack_start( _feature_entry,         Gtk::PACK_SHRINK );
    _feature_vbox.pack_start( _feature_label,         Gtk::PACK_SHRINK );
    _feature_vbox.pack_start( _feature_substitutions, Gtk::PACK_SHRINK );
    _feature_vbox.pack_start( _feature_list,          Gtk::PACK_SHRINK );

    _feature_vbox.set_margin_start(15);
    _feature_vbox.set_margin_end(15);

    _feature_frame.add( _feature_vbox );
    pack_start( _feature_frame, Gtk::PACK_SHRINK );

    // Add signals
    //_feature_entry.signal_key_press_event().connect ( sigc::mem_fun(*this, &FontVariants::feature_callback) );
    _feature_entry.signal_changed().connect( sigc::mem_fun(*this, &FontVariants::feature_callback) );
    
    show_all_children();

  }

  void
  FontVariants::ligatures_init() {
      // std::cout << "FontVariants::ligatures_init()" << std::endl;
  }
  
  void
  FontVariants::ligatures_callback() {
      // std::cout << "FontVariants::ligatures_callback()" << std::endl;
      _ligatures_changed = true;
      _changed_signal.emit();
  }

  void
  FontVariants::position_init() {
      // std::cout << "FontVariants::position_init()" << std::endl;
  }
  
  void
  FontVariants::position_callback() {
      // std::cout << "FontVariants::position_callback()" << std::endl;
      _position_changed = true;
      _changed_signal.emit();
  }

  void
  FontVariants::caps_init() {
      // std::cout << "FontVariants::caps_init()" << std::endl;
  }
  
  void
  FontVariants::caps_callback() {
      // std::cout << "FontVariants::caps_callback()" << std::endl;
      _caps_changed = true;
      _changed_signal.emit();
  }

  void
  FontVariants::numeric_init() {
      // std::cout << "FontVariants::numeric_init()" << std::endl;
  }
  
  void
  FontVariants::numeric_callback() {
      // std::cout << "FontVariants::numeric_callback()" << std::endl;
      _numeric_changed = true;
      _changed_signal.emit();
  }

  void
  FontVariants::asian_init() {
      // std::cout << "FontVariants::asian_init()" << std::endl;
  }

  void
  FontVariants::asian_callback() {
      // std::cout << "FontVariants::asian_callback()" << std::endl;
      _asian_changed = true;
      _changed_signal.emit();
  }

  void
  FontVariants::feature_init() {
      // std::cout << "FontVariants::feature_init()" << std::endl;
  }
  
  void
  FontVariants::feature_callback() {
      // std::cout << "FontVariants::feature_callback()" << std::endl;
      _feature_changed = true;
      _changed_signal.emit();
  }

  // Update GUI based on query.
  void
  FontVariants::update( SPStyle const *query, bool different_features, Glib::ustring& font_spec ) {

      update_opentype( font_spec );

      _ligatures_all = query->font_variant_ligatures.computed;
      _ligatures_mix = query->font_variant_ligatures.value;

      _ligatures_common.set_active(       _ligatures_all & SP_CSS_FONT_VARIANT_LIGATURES_COMMON );
      _ligatures_discretionary.set_active(_ligatures_all & SP_CSS_FONT_VARIANT_LIGATURES_DISCRETIONARY );
      _ligatures_historical.set_active(   _ligatures_all & SP_CSS_FONT_VARIANT_LIGATURES_HISTORICAL );
      _ligatures_contextual.set_active(   _ligatures_all & SP_CSS_FONT_VARIANT_LIGATURES_CONTEXTUAL );
      
      _ligatures_common.set_inconsistent(        _ligatures_mix & SP_CSS_FONT_VARIANT_LIGATURES_COMMON );
      _ligatures_discretionary.set_inconsistent( _ligatures_mix & SP_CSS_FONT_VARIANT_LIGATURES_DISCRETIONARY );
      _ligatures_historical.set_inconsistent(    _ligatures_mix & SP_CSS_FONT_VARIANT_LIGATURES_HISTORICAL );
      _ligatures_contextual.set_inconsistent(    _ligatures_mix & SP_CSS_FONT_VARIANT_LIGATURES_CONTEXTUAL );

      _position_all = query->font_variant_position.computed;
      _position_mix = query->font_variant_position.value;
      
      _position_normal.set_active( _position_all & SP_CSS_FONT_VARIANT_POSITION_NORMAL );
      _position_sub.set_active(    _position_all & SP_CSS_FONT_VARIANT_POSITION_SUB );
      _position_super.set_active(  _position_all & SP_CSS_FONT_VARIANT_POSITION_SUPER );

      _position_normal.set_inconsistent( _position_mix & SP_CSS_FONT_VARIANT_POSITION_NORMAL );
      _position_sub.set_inconsistent(    _position_mix & SP_CSS_FONT_VARIANT_POSITION_SUB );
      _position_super.set_inconsistent(  _position_mix & SP_CSS_FONT_VARIANT_POSITION_SUPER );

      _caps_all = query->font_variant_caps.computed;
      _caps_mix = query->font_variant_caps.value;

      _caps_normal.set_active(     _caps_all & SP_CSS_FONT_VARIANT_CAPS_NORMAL );
      _caps_small.set_active(      _caps_all & SP_CSS_FONT_VARIANT_CAPS_SMALL );
      _caps_all_small.set_active(  _caps_all & SP_CSS_FONT_VARIANT_CAPS_ALL_SMALL );
      _caps_petite.set_active(     _caps_all & SP_CSS_FONT_VARIANT_CAPS_PETITE );
      _caps_all_petite.set_active( _caps_all & SP_CSS_FONT_VARIANT_CAPS_ALL_PETITE );
      _caps_unicase.set_active(    _caps_all & SP_CSS_FONT_VARIANT_CAPS_UNICASE );
      _caps_titling.set_active(    _caps_all & SP_CSS_FONT_VARIANT_CAPS_TITLING );

      _caps_normal.set_inconsistent(     _caps_mix & SP_CSS_FONT_VARIANT_CAPS_NORMAL );
      _caps_small.set_inconsistent(      _caps_mix & SP_CSS_FONT_VARIANT_CAPS_SMALL );
      _caps_all_small.set_inconsistent(  _caps_mix & SP_CSS_FONT_VARIANT_CAPS_ALL_SMALL );
      _caps_petite.set_inconsistent(     _caps_mix & SP_CSS_FONT_VARIANT_CAPS_PETITE );
      _caps_all_petite.set_inconsistent( _caps_mix & SP_CSS_FONT_VARIANT_CAPS_ALL_PETITE );
      _caps_unicase.set_inconsistent(    _caps_mix & SP_CSS_FONT_VARIANT_CAPS_UNICASE );
      _caps_titling.set_inconsistent(    _caps_mix & SP_CSS_FONT_VARIANT_CAPS_TITLING );

      _numeric_all = query->font_variant_numeric.computed;
      _numeric_mix = query->font_variant_numeric.value;

      if (_numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS) {
          _numeric_lining.set_active();
      } else if (_numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS) {
          _numeric_old_style.set_active();
      } else {
          _numeric_default_style.set_active();
      }

      if (_numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS) {
          _numeric_proportional.set_active();
      } else if (_numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS) {
          _numeric_tabular.set_active();
      } else {
          _numeric_default_width.set_active();
      }

      if (_numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS) {
          _numeric_diagonal.set_active();
      } else if (_numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS) {
          _numeric_stacked.set_active();
      } else {
          _numeric_default_fractions.set_active();
      }

      _numeric_ordinal.set_active(      _numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_ORDINAL );
      _numeric_slashed_zero.set_active( _numeric_all & SP_CSS_FONT_VARIANT_NUMERIC_SLASHED_ZERO );


      _numeric_lining.set_inconsistent(       _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS );
      _numeric_old_style.set_inconsistent(    _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS );
      _numeric_proportional.set_inconsistent( _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS );
      _numeric_tabular.set_inconsistent(      _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS );
      _numeric_diagonal.set_inconsistent(     _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS );
      _numeric_stacked.set_inconsistent(      _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS );
      _numeric_ordinal.set_inconsistent(      _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_ORDINAL );
      _numeric_slashed_zero.set_inconsistent( _numeric_mix & SP_CSS_FONT_VARIANT_NUMERIC_SLASHED_ZERO );

      _asian_all = query->font_variant_east_asian.computed;
      _asian_mix = query->font_variant_east_asian.value;

      if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS78) {
          _asian_jis78.set_active();
      } else if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS83) {
          _asian_jis83.set_active();
      } else if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS90) {
          _asian_jis90.set_active();
      } else if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS04) {
          _asian_jis04.set_active();
      } else if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED) {
          _asian_simplified.set_active();
      } else if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL) {
          _asian_traditional.set_active();
      } else {
          _asian_default_variant.set_active();
      }

      if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH) {
          _asian_full_width.set_active();
      } else if (_asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL_WIDTH) {
          _asian_proportional_width.set_active();
      } else {
          _asian_default_width.set_active();
      }

      _asian_ruby.set_active ( _asian_all & SP_CSS_FONT_VARIANT_EAST_ASIAN_RUBY );

      _asian_jis78.set_inconsistent(             _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS78);
      _asian_jis83.set_inconsistent(             _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS83);
      _asian_jis90.set_inconsistent(             _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS90);
      _asian_jis04.set_inconsistent(             _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_JIS04);
      _asian_simplified.set_inconsistent(        _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED);
      _asian_traditional.set_inconsistent(       _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL);
      _asian_full_width.set_inconsistent(        _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH);
      _asian_proportional_width.set_inconsistent(_asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL_WIDTH);
      _asian_ruby.set_inconsistent(              _asian_mix & SP_CSS_FONT_VARIANT_EAST_ASIAN_RUBY);

      // Fix me: Should match a space if second part matches.            ---,
      //       : Add boundary to 'on' and 'off'.                            v
      Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("\"(\\w{4})\"\\s*([0-9]+|on|off|)");
      Glib::MatchInfo matchInfo;
      std::string setting;

      // Set feature radiobutton (if it exists) or add to _feature_entry string.
      char const *val = query->font_feature_settings.value();
      if (val) {

          std::vector<Glib::ustring> tokens =
              Glib::Regex::split_simple("\\s*,\\s*", val);

          for (auto token: tokens) {
              regex->match(token, matchInfo);
              if (matchInfo.matches()) {
                  Glib::ustring table = matchInfo.fetch(1);
                  Glib::ustring value = matchInfo.fetch(2);

                  if (_features.find(table) != _features.end()) {
                      int v = 0;
                      if (value == "0" || value == "off") v = 0;
                      else if (value == "1" || value == "on" || value.empty() ) v = 1;
                      else v = std::stoi(value);
                      _features[table]->set_active(v);
                  } else {
                      setting += token + ", ";
                  }
              }
          }
      }

      // Remove final ", "
      if (setting.length() > 1) {
          setting.pop_back();
          setting.pop_back();
      }

      // Tables without radiobuttons.
      _feature_entry.set_text( setting );

      if( different_features ) {
          _feature_label.show();
      } else {
          _feature_label.hide();
      }
  }

  // Update GUI based on OpenType tables of selected font (which may be changed in font selector tab).
  void
  FontVariants::update_opentype (Glib::ustring& font_spec) {

      // Disable/Enable based on available OpenType tables.
      font_instance* res = font_factory::Default()->FaceFromFontSpecification( font_spec.c_str() );
      if( res ) {

          std::map<Glib::ustring, OTSubstitution>::iterator it;

          if((it = res->openTypeTables.find("liga"))!= res->openTypeTables.end() ||
             (it = res->openTypeTables.find("clig"))!= res->openTypeTables.end()) {
              _ligatures_common.set_sensitive();
          } else {
              _ligatures_common.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("dlig"))!= res->openTypeTables.end()) {
              _ligatures_discretionary.set_sensitive();
          } else {
              _ligatures_discretionary.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("hlig"))!= res->openTypeTables.end()) {
              _ligatures_historical.set_sensitive();
          } else {
              _ligatures_historical.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("calt"))!= res->openTypeTables.end()) {
              _ligatures_contextual.set_sensitive();
          } else {
              _ligatures_contextual.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("subs"))!= res->openTypeTables.end()) {
              _position_sub.set_sensitive();
          } else {
              _position_sub.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("sups"))!= res->openTypeTables.end()) {
              _position_super.set_sensitive();
          } else {
              _position_super.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("smcp"))!= res->openTypeTables.end()) {
              _caps_small.set_sensitive();
          } else {
              _caps_small.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("c2sc"))!= res->openTypeTables.end() &&
             (it = res->openTypeTables.find("smcp"))!= res->openTypeTables.end()) {
              _caps_all_small.set_sensitive();
          } else {
              _caps_all_small.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("pcap"))!= res->openTypeTables.end()) {
              _caps_petite.set_sensitive();
          } else {
              _caps_petite.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("c2sc"))!= res->openTypeTables.end() &&
             (it = res->openTypeTables.find("pcap"))!= res->openTypeTables.end()) {
              _caps_all_petite.set_sensitive();
          } else {
              _caps_all_petite.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("unic"))!= res->openTypeTables.end()) {
              _caps_unicase.set_sensitive();
          } else {
              _caps_unicase.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("titl"))!= res->openTypeTables.end()) {
              _caps_titling.set_sensitive();
          } else {
              _caps_titling.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("lnum"))!= res->openTypeTables.end()) {
              _numeric_lining.set_sensitive();
          } else {
              _numeric_lining.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("onum"))!= res->openTypeTables.end()) {
              _numeric_old_style.set_sensitive();
          } else {
              _numeric_old_style.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("pnum"))!= res->openTypeTables.end()) {
              _numeric_proportional.set_sensitive();
          } else {
              _numeric_proportional.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("tnum"))!= res->openTypeTables.end()) {
              _numeric_tabular.set_sensitive();
          } else {
              _numeric_tabular.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("frac"))!= res->openTypeTables.end()) {
              _numeric_diagonal.set_sensitive();
          } else {
              _numeric_diagonal.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("afrac"))!= res->openTypeTables.end()) {
              _numeric_stacked.set_sensitive();
          } else {
              _numeric_stacked.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("ordn"))!= res->openTypeTables.end()) {
              _numeric_ordinal.set_sensitive();
          } else {
              _numeric_ordinal.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("zero"))!= res->openTypeTables.end()) {
              _numeric_slashed_zero.set_sensitive();
          } else {
              _numeric_slashed_zero.set_sensitive( false );
          }

          // East-Asian
          if((it = res->openTypeTables.find("jp78"))!= res->openTypeTables.end()) {
              _asian_jis78.set_sensitive();
          } else {
              _asian_jis78.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("jp83"))!= res->openTypeTables.end()) {
              _asian_jis83.set_sensitive();
          } else {
              _asian_jis83.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("jp90"))!= res->openTypeTables.end()) {
              _asian_jis90.set_sensitive();
          } else {
              _asian_jis90.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("jp04"))!= res->openTypeTables.end()) {
              _asian_jis04.set_sensitive();
          } else {
              _asian_jis04.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("smpl"))!= res->openTypeTables.end()) {
              _asian_simplified.set_sensitive();
          } else {
              _asian_simplified.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("trad"))!= res->openTypeTables.end()) {
              _asian_traditional.set_sensitive();
          } else {
              _asian_traditional.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("fwid"))!= res->openTypeTables.end()) {
              _asian_full_width.set_sensitive();
          } else {
              _asian_full_width.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("pwid"))!= res->openTypeTables.end()) {
              _asian_proportional_width.set_sensitive();
          } else {
              _asian_proportional_width.set_sensitive( false );
          }

          if((it = res->openTypeTables.find("ruby"))!= res->openTypeTables.end()) {
              _asian_ruby.set_sensitive();
          } else {
              _asian_ruby.set_sensitive( false );
          }

          // List available ligatures
          Glib::ustring markup_liga;
          Glib::ustring markup_dlig;
          Glib::ustring markup_hlig;
          Glib::ustring markup_calt;

          for (auto table: res->openTypeTables) {

              if (table.first == "liga" ||
                  table.first == "clig" ||
                  table.first == "dlig" ||
                  table.first == "hgli" ||
                  table.first == "calt") {

                  Glib::ustring markup;
                  markup += "<span font_family='";
                  markup += sp_font_description_get_family(res->descr);
                  markup += "'>";
                  markup += Glib::Markup::escape_text(table.second.output);
                  markup += "</span>";

                  if (table.first == "liga") markup_liga += markup;
                  if (table.first == "clig") markup_liga += markup;
                  if (table.first == "dlig") markup_dlig += markup;
                  if (table.first == "hlig") markup_hlig += markup;
                  if (table.first == "calt") markup_calt += markup;
              }
          }

          _ligatures_label_common.set_markup        ( markup_liga.c_str() );
          _ligatures_label_discretionary.set_markup ( markup_dlig.c_str() );
          _ligatures_label_historical.set_markup    ( markup_hlig.c_str() );
          _ligatures_label_contextual.set_markup    ( markup_calt.c_str() );

          // List available numeric variants
          Glib::ustring markup_lnum;
          Glib::ustring markup_onum;
          Glib::ustring markup_pnum;
          Glib::ustring markup_tnum;
          Glib::ustring markup_frac;
          Glib::ustring markup_afrc;
          Glib::ustring markup_ordn;
          Glib::ustring markup_zero;

          for (auto table: res->openTypeTables) {

              Glib::ustring markup;
              markup += "<span font_family='";
              markup += sp_font_description_get_family(res->descr);
              markup += "' font_features='";
              markup += table.first;
              markup += "'>";
              if (table.first == "lnum" ||
                  table.first == "onum" ||
                  table.first == "pnum" ||
                  table.first == "tnum") markup += "0123456789";
              if (table.first == "zero") markup += "0";
              if (table.first == "ordn") markup += "[" + table.second.before + "]" + table.second.output;
              if (table.first == "frac" ||
                  table.first == "afrc" ) markup += "1/2 2/3 3/4 4/5 5/6"; // Can we do better?
              markup += "</span>";

              if (table.first == "lnum") markup_lnum += markup;
              if (table.first == "onum") markup_onum += markup;
              if (table.first == "pnum") markup_pnum += markup;
              if (table.first == "tnum") markup_tnum += markup;
              if (table.first == "frac") markup_frac += markup;
              if (table.first == "afrc") markup_afrc += markup;
              if (table.first == "ordn") markup_ordn += markup;
              if (table.first == "zero") markup_zero += markup;
          }

          _numeric_lining_label.set_markup       ( markup_lnum.c_str() );
          _numeric_old_style_label.set_markup    ( markup_onum.c_str() );
          _numeric_proportional_label.set_markup ( markup_pnum.c_str() );
          _numeric_tabular_label.set_markup      ( markup_tnum.c_str() );
          _numeric_diagonal_label.set_markup     ( markup_frac.c_str() );
          _numeric_stacked_label.set_markup      ( markup_afrc.c_str() );
          _numeric_ordinal_label.set_markup      ( markup_ordn.c_str() );
          _numeric_slashed_zero_label.set_markup ( markup_zero.c_str() );

          // Make list of tables not handled above.
          std::map<Glib::ustring, OTSubstitution> table_copy = res->openTypeTables;
          if( (it = table_copy.find("liga")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("clig")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("dlig")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("hlig")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("calt")) != table_copy.end() ) table_copy.erase( it );

          if( (it = table_copy.find("subs")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("sups")) != table_copy.end() ) table_copy.erase( it );

          if( (it = table_copy.find("smcp")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("c2sc")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("pcap")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("c2pc")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("unic")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("titl")) != table_copy.end() ) table_copy.erase( it );

          if( (it = table_copy.find("lnum")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("onum")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("pnum")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("tnum")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("frac")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("afrc")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("ordn")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("zero")) != table_copy.end() ) table_copy.erase( it );

          if( (it = table_copy.find("jp78")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("jp83")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("jp90")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("jp04")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("smpl")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("trad")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("fwid")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("pwid")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("ruby")) != table_copy.end() ) table_copy.erase( it );

          // An incomplete list of tables that should not be exposed to the user:
          if( (it = table_copy.find("abvf")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("abvs")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("akhn")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("blwf")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("blws")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("ccmp")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("cjct")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("dnom")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("dtls")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("fina")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("half")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("haln")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("init")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("isol")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("locl")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("medi")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("nukt")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("numr")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("pref")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("pres")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("pstf")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("psts")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("rlig")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("rkrf")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("rphf")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("rtlm")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("ssty")) != table_copy.end() ) table_copy.erase( it );
          if( (it = table_copy.find("vatu")) != table_copy.end() ) table_copy.erase( it );

          // Clear out old features
          auto children = _feature_grid.get_children();
          for (auto child: children) {
              _feature_grid.remove (*child);
          }
          _features.clear();

          std::string markup;
          int grid_row = 0;

          // GSUB lookup type 1 (1 to 1 mapping).
          for (auto table: res->openTypeTables) {
              if (table.first == "case" ||
                  table.first == "hist" ||
                  (table.first[0] == 's' && table.first[1] == 's' && !(table.first[2] == 't'))) {

                  if( (it = table_copy.find(table.first)) != table_copy.end() ) table_copy.erase( it );

                  _features[table.first] = new Feature (table.first, table.second, 2,
                                                        sp_font_description_get_family(res->descr),
                                                        _feature_grid, grid_row, this);
                  grid_row++;
              }
          }

          // GSUB lookup type 3 (1 to many mapping). Optionally type 1.
          for (auto table: res->openTypeTables) {
              if (table.first == "salt" ||
                  table.first == "swsh" ||
                  table.first == "cwsh" ||
                  table.first == "ornm" ||
                  table.first == "nalt" ||
                  (table.first[0] == 'c' && table.first[1] == 'v')) {

                  if (table.second.input.length() == 0) {
                      // This can happen if a table is not in the 'DFLT' script and 'dflt' language.
                      // We should be using the 'lang' attribute to find the correct tables.
                      // std::cerr << "FontVariants::open_type_update: "
                      //           << table.first << " has no entries!" << std::endl;
                      continue;
                  }

                  if( (it = table_copy.find(table.first)) != table_copy.end() ) table_copy.erase( it );

                  // Our lame attempt at determining number of alternative glyphs for one glyph:
                  int number = table.second.output.length() / table.second.input.length();
                  if (number < 1) {
                      number = 1; // Must have at least on/off, see comment above about 'lang' attribute.
                      // std::cout << table.first << " "
                      //           << table.second.output.length() << "/"
                      //           << table.second.input.length() << "="
                      //           << number << std::endl;
                  }

                  _features[table.first] = new Feature (table.first, table.second, number+1,
                                                        sp_font_description_get_family(res->descr),
                                                        _feature_grid, grid_row, this);
                  grid_row++;
              }
          }

          _feature_grid.show_all();

          _feature_substitutions.set_markup ( markup.c_str() );

          std::string ott_list = "OpenType tables not included above: ";
          for(it = table_copy.begin(); it != table_copy.end(); ++it) {
              ott_list += it->first;
              ott_list += ", ";
          }

          if (table_copy.size() > 0) {
              ott_list.pop_back();
              ott_list.pop_back();
              _feature_list.set_text( ott_list.c_str() );
          } else {
              _feature_list.set_text( "" );
          }

      } else {
          std::cerr << "FontVariants::update(): Couldn't find font_instance for: "
                    << font_spec << std::endl;
      }

      _ligatures_changed = false;
      _position_changed  = false;
      _caps_changed      = false;
      _numeric_changed   = false;
      _feature_changed   = false;
  }

  void
  FontVariants::fill_css( SPCSSAttr *css ) {

      // Ligatures
      bool common        = _ligatures_common.get_active();
      bool discretionary = _ligatures_discretionary.get_active();
      bool historical    = _ligatures_historical.get_active();
      bool contextual    = _ligatures_contextual.get_active();

      if( !common && !discretionary && !historical && !contextual ) {
          sp_repr_css_set_property(css, "font-variant-ligatures", "none" );
      } else if ( common && !discretionary && !historical && contextual ) {
          sp_repr_css_set_property(css, "font-variant-ligatures", "normal" );
      } else {
          Glib::ustring css_string;
          if ( !common )
              css_string += "no-common-ligatures ";
          if ( discretionary )
              css_string += "discretionary-ligatures ";
          if ( historical )
              css_string += "historical-ligatures ";
          if ( !contextual )
              css_string += "no-contextual ";
          sp_repr_css_set_property(css, "font-variant-ligatures", css_string.c_str() );
      }

      // Position
      {
          unsigned position_new = SP_CSS_FONT_VARIANT_POSITION_NORMAL;
          Glib::ustring css_string;
          if( _position_normal.get_active() ) {
              css_string = "normal";
          } else if( _position_sub.get_active() ) {
              css_string = "sub";
              position_new = SP_CSS_FONT_VARIANT_POSITION_SUB;
          } else if( _position_super.get_active() ) {
              css_string = "super";
              position_new = SP_CSS_FONT_VARIANT_POSITION_SUPER;
          }

          // 'if' may not be necessary... need to test.
          if( (_position_all != position_new) || ((_position_mix != 0) && _position_changed) ) {
              sp_repr_css_set_property(css, "font-variant-position", css_string.c_str() );
          }
      }
      
      // Caps
      {
          //unsigned caps_new;
          Glib::ustring css_string;
          if( _caps_normal.get_active() ) {
              css_string = "normal";
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_NORMAL;
          } else if( _caps_small.get_active() ) {
              css_string = "small-caps";
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_SMALL;
          } else if( _caps_all_small.get_active() ) {
              css_string = "all-small-caps";
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_ALL_SMALL;
          } else if( _caps_petite.get_active() ) {
              css_string = "petite";
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_PETITE;
          } else if( _caps_all_petite.get_active() ) {
              css_string = "all-petite";
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_ALL_PETITE;
          } else if( _caps_unicase.get_active() ) {
              css_string = "unicase";
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_UNICASE;
          } else if( _caps_titling.get_active() ) {
              css_string = "titling";
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_TITLING;
          //} else {
          //    caps_new = SP_CSS_FONT_VARIANT_CAPS_NORMAL;
          }

          // May not be necessary... need to test.
          //if( (_caps_all != caps_new) || ((_caps_mix != 0) && _caps_changed) ) {
          sp_repr_css_set_property(css, "font-variant-caps", css_string.c_str() );
          //}
      }

      // Numeric
      bool default_style = _numeric_default_style.get_active();
      bool lining        = _numeric_lining.get_active();
      bool old_style     = _numeric_old_style.get_active();

      bool default_width = _numeric_default_width.get_active();
      bool proportional  = _numeric_proportional.get_active();
      bool tabular       = _numeric_tabular.get_active();

      bool default_fractions = _numeric_default_fractions.get_active();
      bool diagonal          = _numeric_diagonal.get_active();
      bool stacked           = _numeric_stacked.get_active();

      bool ordinal           = _numeric_ordinal.get_active();
      bool slashed_zero      = _numeric_slashed_zero.get_active();

      if (default_style & default_width & default_fractions & !ordinal & !slashed_zero) {
          sp_repr_css_set_property(css, "font-variant-numeric", "normal");
      } else {
          Glib::ustring css_string;
          if ( lining )
              css_string += "lining-nums ";
          if ( old_style )
              css_string += "oldstyle-nums ";
          if ( proportional )
              css_string += "proportional-nums ";
          if ( tabular )
              css_string += "tabular-nums ";
          if ( diagonal )
              css_string += "diagonal-fractions ";
          if ( stacked )
              css_string += "stacked-fractions ";
          if ( ordinal )
              css_string += "ordinal ";
          if ( slashed_zero )
              css_string += "slashed-zero ";
          sp_repr_css_set_property(css, "font-variant-numeric", css_string.c_str() );
      }

      // East Asian
      bool default_variant = _asian_default_variant.get_active();
      bool jis78           = _asian_jis78.get_active();
      bool jis83           = _asian_jis83.get_active();
      bool jis90           = _asian_jis90.get_active();
      bool jis04           = _asian_jis04.get_active();
      bool simplified      = _asian_simplified.get_active();
      bool traditional     = _asian_traditional.get_active();
      bool asian_width     = _asian_default_width.get_active();
      bool fwid            = _asian_full_width.get_active();
      bool pwid            = _asian_proportional_width.get_active();
      bool ruby            = _asian_ruby.get_active();

      if (default_style & asian_width & !ruby) {
          sp_repr_css_set_property(css, "font-variant-east-asian", "normal");
      } else {
          Glib::ustring css_string;
          if (jis78)          css_string += "jis78 ";
          if (jis83)          css_string += "jis83 ";
          if (jis90)          css_string += "jis90 ";
          if (jis04)          css_string += "jis04 ";
          if (simplified)     css_string += "simplfied ";
          if (traditional)    css_string += "traditional ";

          if (fwid)           css_string += "fwid ";
          if (pwid)           css_string += "pwid ";

          if (ruby)           css_string += "ruby ";

          sp_repr_css_set_property(css, "font-variant-east-asian", css_string.c_str() );
      }

      // Feature settings
      Glib::ustring feature_string;
      for (auto i: _features) {
          feature_string += i.second->get_css();
      }

      feature_string += _feature_entry.get_text();
      // std::cout << "feature_string: " << feature_string << std::endl;

      if (!feature_string.empty()) {
          sp_repr_css_set_property(css, "font-feature-settings", feature_string.c_str());
      } else {
          sp_repr_css_unset_property(css, "font-feature-settings");
      }
  }
   
  Glib::ustring
  FontVariants::get_markup() {

      Glib::ustring markup;

      // Ligatures
      bool common        = _ligatures_common.get_active();
      bool discretionary = _ligatures_discretionary.get_active();
      bool historical    = _ligatures_historical.get_active();
      bool contextual    = _ligatures_contextual.get_active();

      if (!common)         markup += "liga=0,clig=0,"; // On by default.
      if (discretionary)   markup += "dlig=1,";
      if (historical)      markup += "hlig=1,";
      if (contextual)      markup += "calt=1,";

      // Position
      if (      _position_sub.get_active()    ) markup += "subs=1,";
      else if ( _position_super.get_active()  ) markup += "sups=1,";

      // Caps
      if (      _caps_small.get_active()      ) markup += "smcp=1,";
      else if ( _caps_all_small.get_active()  ) markup += "c2sc=1,smcp=1,";
      else if ( _caps_petite.get_active()     ) markup += "pcap=1,";
      else if ( _caps_all_petite.get_active() ) markup += "c2pc=1,pcap=1,";
      else if ( _caps_unicase.get_active()    ) markup += "unic=1,";
      else if ( _caps_titling.get_active()    ) markup += "titl=1,";

      // Numeric
      bool default_style = _numeric_default_style.get_active();
      bool lining        = _numeric_lining.get_active();
      bool old_style     = _numeric_old_style.get_active();

      bool default_width = _numeric_default_width.get_active();
      bool proportional  = _numeric_proportional.get_active();
      bool tabular       = _numeric_tabular.get_active();

      bool default_fractions = _numeric_default_fractions.get_active();
      bool diagonal          = _numeric_diagonal.get_active();
      bool stacked           = _numeric_stacked.get_active();

      bool ordinal           = _numeric_ordinal.get_active();
      bool slashed_zero      = _numeric_slashed_zero.get_active();

      if (lining)          markup += "lnum=1,";
      if (old_style)       markup += "onum=1,";
      if (proportional)    markup += "pnum=1,";
      if (tabular)         markup += "tnum=1,";
      if (diagonal)        markup += "frac=1,";
      if (stacked)         markup += "afrc=1,";
      if (ordinal)         markup += "ordn=1,";
      if (slashed_zero)    markup += "zero=1,";

      // East Asian
      bool default_variant = _asian_default_variant.get_active();
      bool jis78           = _asian_jis78.get_active();
      bool jis83           = _asian_jis83.get_active();
      bool jis90           = _asian_jis90.get_active();
      bool jis04           = _asian_jis04.get_active();
      bool simplified      = _asian_simplified.get_active();
      bool traditional     = _asian_traditional.get_active();
      bool asian_width     = _asian_default_width.get_active();
      bool fwid            = _asian_full_width.get_active();
      bool pwid            = _asian_proportional_width.get_active();
      bool ruby            = _asian_ruby.get_active();

      if (jis78          )   markup += "jp78=1,";
      if (jis83          )   markup += "jp83=1,";
      if (jis90          )   markup += "jp90=1,";
      if (jis04          )   markup += "jp04=1,";
      if (simplified     )   markup += "smpl=1,";
      if (traditional    )   markup += "trad=1,";

      if (fwid           )   markup += "fwid=1,";
      if (pwid           )   markup += "pwid=1,";

      if (ruby           )   markup += "ruby=1,";

      // Feature settings
      Glib::ustring feature_string;
      for (auto i: _features) {
          feature_string += i.second->get_css();
      }

      feature_string += _feature_entry.get_text();
      if (!feature_string.empty()) {
          markup += feature_string;
      }

      // std::cout << "|" << markup << "|" << std::endl;
      return markup;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
