// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Filter Effects dialog.
 */
/* Authors:
 *   Nicholas Bishop <nicholasbishop@gmail.org>
 *   Rodrigo Kumpera <kumpera@gmail.com>
 *   Felipe C. da S. Sanches <juca@members.fsf.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   insaner
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/imagemenuitem.h>

#include <gdkmm/display.h>
#include <gdkmm/general.h>
#include <gdkmm/seat.h>

#include <gtkmm/checkbutton.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/sizegroup.h>

#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <glibmm/main.h>
#include <glibmm/convert.h>

#include <utility>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "filter-chemistry.h"
#include "filter-effects-dialog.h"
#include "filter-enums.h"
#include "inkscape.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "include/gtkmm_version.h"

#include "object/filters/blend.h"
#include "object/filters/colormatrix.h"
#include "object/filters/componenttransfer.h"
#include "object/filters/componenttransfer-funcnode.h"
#include "object/filters/convolvematrix.h"
#include "object/filters/distantlight.h"
#include "object/filters/merge.h"
#include "object/filters/mergenode.h"
#include "object/filters/pointlight.h"
#include "object/filters/spotlight.h"
#include "style.h"

#include "svg/svg-color.h"

#include "ui/dialog/filedialog.h"
#include "ui/widget/filter-effect-chooser.h"
#include "ui/widget/spinbutton.h"

#include "io/sys.h"


using namespace Inkscape::Filters;

namespace Inkscape {
namespace UI {
namespace Dialog {

using Inkscape::UI::Widget::AttrWidget;
using Inkscape::UI::Widget::ComboBoxEnum;
using Inkscape::UI::Widget::DualSpinScale;
using Inkscape::UI::Widget::SpinScale;


// Returns the number of inputs available for the filter primitive type
static int input_count(const SPFilterPrimitive* prim)
{
    if(!prim)
        return 0;
    else if(SP_IS_FEBLEND(prim) || SP_IS_FECOMPOSITE(prim) || SP_IS_FEDISPLACEMENTMAP(prim))
        return 2;
    else if(SP_IS_FEMERGE(prim)) {
        // Return the number of feMergeNode connections plus an extra
        return (int) (prim->children.size() + 1);
    }
    else
        return 1;
}

class CheckButtonAttr : public Gtk::CheckButton, public AttrWidget
{
public:
    CheckButtonAttr(bool def, const Glib::ustring& label,
                    Glib::ustring  tv, Glib::ustring  fv,
                    const SPAttr a, char* tip_text)
        : Gtk::CheckButton(label),
          AttrWidget(a, def),
          _true_val(std::move(tv)), _false_val(std::move(fv))
    {
        signal_toggled().connect(signal_attr_changed().make_slot());
        if (tip_text) {
            set_tooltip_text(tip_text);
        }
    }

    Glib::ustring get_as_attribute() const override
    {
        return get_active() ? _true_val : _false_val;
    }

    void set_from_attribute(SPObject* o) override
    {
        const gchar* val = attribute_value(o);
        if(val) {
            if(_true_val == val)
                set_active(true);
            else if(_false_val == val)
                set_active(false);
        } else {
            set_active(get_default()->as_bool());
        }
    }
private:
    const Glib::ustring _true_val, _false_val;
};

class SpinButtonAttr : public Inkscape::UI::Widget::SpinButton, public AttrWidget
{
public:
    SpinButtonAttr(double lower, double upper, double step_inc,
                   double climb_rate, int digits, const SPAttr a, double def, char* tip_text)
        : Inkscape::UI::Widget::SpinButton(climb_rate, digits),
          AttrWidget(a, def)
    {
        if (tip_text) {
            set_tooltip_text(tip_text);
        }
        set_range(lower, upper);
        set_increments(step_inc, 0);

        signal_value_changed().connect(signal_attr_changed().make_slot());
    }

    Glib::ustring get_as_attribute() const override
    {
        const double val = get_value();

        if(get_digits() == 0)
            return Glib::Ascii::dtostr((int)val);
        else
            return Glib::Ascii::dtostr(val);
    }

    void set_from_attribute(SPObject* o) override
    {
        const gchar* val = attribute_value(o);
        if(val){
            set_value(Glib::Ascii::strtod(val));
        } else {
            set_value(get_default()->as_double());
        }
    }
};

template< typename T> class ComboWithTooltip : public Gtk::EventBox
{
public:
    ComboWithTooltip<T>(T default_value, const Util::EnumDataConverter<T>& c, const SPAttr a = SPAttr::INVALID, char* tip_text = nullptr)
    {
        if (tip_text) {
            set_tooltip_text(tip_text);
        }
        combo = new ComboBoxEnum<T>(default_value, c, a, false);
        add(*combo);
        show_all();
    }

    ~ComboWithTooltip() override
    {
        delete combo;
    }

    ComboBoxEnum<T>* get_attrwidget()
    {
        return combo;
    }
private:
    ComboBoxEnum<T>* combo;
};

// Contains an arbitrary number of spin buttons that use separate attributes
class MultiSpinButton : public Gtk::Box
{
public:
    MultiSpinButton(double lower, double upper, double step_inc,
                    double climb_rate, int digits, std::vector<SPAttr> attrs, std::vector<double> default_values, std::vector<char*> tip_text)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    {
        g_assert(attrs.size()==default_values.size());
        g_assert(attrs.size()==tip_text.size());
        set_spacing(4);
        for(unsigned i = 0; i < attrs.size(); ++i) {
            unsigned index = attrs.size() - 1 - i;
            _spins.push_back(new SpinButtonAttr(lower, upper, step_inc, climb_rate, digits, attrs[index], default_values[index], tip_text[index]));
            pack_end(*_spins.back(), false, false);
        }
    }

    ~MultiSpinButton() override
    {
        for(auto & _spin : _spins)
            delete _spin;
    }

    std::vector<SpinButtonAttr*>& get_spinbuttons()
    {
        return _spins;
    }
private:
    std::vector<SpinButtonAttr*> _spins;
};

// Contains two spinbuttons that describe a NumberOptNumber
class DualSpinButton : public Gtk::Box, public AttrWidget
{
public:
    DualSpinButton(char* def, double lower, double upper, double step_inc,
                   double climb_rate, int digits, const SPAttr a, char* tt1, char* tt2)
        : AttrWidget(a, def), //TO-DO: receive default num-opt-num as parameter in the constructor
          Gtk::Box(Gtk::ORIENTATION_HORIZONTAL),
          _s1(climb_rate, digits), _s2(climb_rate, digits)
    {
        if (tt1) {
            _s1.set_tooltip_text(tt1);
        }
        if (tt2) {
            _s2.set_tooltip_text(tt2);
        }
        _s1.set_range(lower, upper);
        _s2.set_range(lower, upper);
        _s1.set_increments(step_inc, 0);
        _s2.set_increments(step_inc, 0);

        _s1.signal_value_changed().connect(signal_attr_changed().make_slot());
        _s2.signal_value_changed().connect(signal_attr_changed().make_slot());

        set_spacing(4);
        pack_end(_s2, false, false);
        pack_end(_s1, false, false);
    }

    Inkscape::UI::Widget::SpinButton& get_spinbutton1()
    {
        return _s1;
    }

    Inkscape::UI::Widget::SpinButton& get_spinbutton2()
    {
        return _s2;
    }

    Glib::ustring get_as_attribute() const override
    {
        double v1 = _s1.get_value();
        double v2 = _s2.get_value();

        if(_s1.get_digits() == 0) {
            v1 = (int)v1;
            v2 = (int)v2;
        }

        return Glib::Ascii::dtostr(v1) + " " + Glib::Ascii::dtostr(v2);
    }

    void set_from_attribute(SPObject* o) override
    {
        const gchar* val = attribute_value(o);
        NumberOptNumber n;
        if(val) {
            n.set(val);
        } else {
            n.set(get_default()->as_charptr());
        }
        _s1.set_value(n.getNumber());
        _s2.set_value(n.getOptNumber());

    }
private:
    Inkscape::UI::Widget::SpinButton _s1, _s2;
};

class ColorButton : public Gtk::ColorButton, public AttrWidget
{
public:
    ColorButton(unsigned int def, const SPAttr a, char* tip_text)
        : AttrWidget(a, def)
    {
        signal_color_set().connect(signal_attr_changed().make_slot());
        if (tip_text) {
            set_tooltip_text(tip_text);
        }

        Gdk::RGBA col;
        col.set_rgba_u(65535, 65535, 65535);
        set_rgba(col);
    }

    // Returns the color in 'rgb(r,g,b)' form.
    Glib::ustring get_as_attribute() const override
    {
        // no doubles here, so we can use the standard string stream.
        std::ostringstream os;

        const auto c = get_rgba();
        const int  r = c.get_red_u() / 257, g = c.get_green_u() / 257, b = c.get_blue_u() / 257;//TO-DO: verify this. This sounds a lot strange! shouldn't it be 256?
        os << "rgb(" << r << "," << g << "," << b << ")";
        return os.str();
    }


    void set_from_attribute(SPObject* o) override
    {
        const gchar* val = attribute_value(o);
        guint32 i = 0;
        if(val) {
            i = sp_svg_read_color(val, 0xFFFFFFFF);
        } else {
            i = (guint32) get_default()->as_uint();
        }
        const int r = SP_RGBA32_R_U(i), g = SP_RGBA32_G_U(i), b = SP_RGBA32_B_U(i);

        Gdk::RGBA col;
        col.set_rgba_u(r * 256, g * 256, b * 256);
        set_rgba(col);
    }
};

// Used for tableValue in feComponentTransfer
class EntryAttr : public Gtk::Entry, public AttrWidget
{
public:
    EntryAttr(const SPAttr a, char* tip_text)
        : AttrWidget(a)
    {
        signal_changed().connect(signal_attr_changed().make_slot());
        if (tip_text) {
            set_tooltip_text(tip_text);
        }
    }

    // No validity checking is done
    Glib::ustring get_as_attribute() const override
    {
        return get_text();
    }

    void set_from_attribute(SPObject* o) override
    {
        const gchar* val = attribute_value(o);
        if(val) {
            set_text( val );
        } else {
            set_text( "" );
        }
    }
};

/* Displays/Edits the matrix for feConvolveMatrix or feColorMatrix */
class FilterEffectsDialog::MatrixAttr : public Gtk::Frame, public AttrWidget
{
public:
    MatrixAttr(const SPAttr a, char* tip_text = nullptr)
        : AttrWidget(a), _locked(false)
    {
        _model = Gtk::ListStore::create(_columns);
        _tree.set_model(_model);
        _tree.set_headers_visible(false);
        _tree.show();
        add(_tree);
        set_shadow_type(Gtk::SHADOW_IN);
        if (tip_text) {
            _tree.set_tooltip_text(tip_text);
        }
    }

    std::vector<double> get_values() const
    {
        std::vector<double> vec;
        for(const auto & iter : _model->children()) {
            for(unsigned c = 0; c < _tree.get_columns().size(); ++c)
                vec.push_back(iter[_columns.cols[c]]);
        }
        return vec;
    }

    void set_values(const std::vector<double>& v)
    {
        unsigned i = 0;
        for(const auto & iter : _model->children()) {
            for(unsigned c = 0; c < _tree.get_columns().size(); ++c) {
                if(i >= v.size())
                    return;
                iter[_columns.cols[c]] = v[i];
                ++i;
            }
        }
    }

    Glib::ustring get_as_attribute() const override
    {
        // use SVGOStringStream to output SVG-compatible doubles
        Inkscape::SVGOStringStream os;

        for(const auto & iter : _model->children()) {
            for(unsigned c = 0; c < _tree.get_columns().size(); ++c) {
                os << iter[_columns.cols[c]] << " ";
            }
        }

        return os.str();
    }

    void set_from_attribute(SPObject* o) override
    {
        if(o) {
            if(SP_IS_FECONVOLVEMATRIX(o)) {
                SPFeConvolveMatrix* conv = SP_FECONVOLVEMATRIX(o);
                int cols, rows;
                cols = (int)conv->order.getNumber();
                if(cols > 5)
                    cols = 5;
                rows = conv->order.optNumber_set ? (int)conv->order.getOptNumber() : cols;
                update(o, rows, cols);
            }
            else if(SP_IS_FECOLORMATRIX(o))
                update(o, 4, 5);
        }
    }
private:
    class MatrixColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        MatrixColumns()
        {
            cols.resize(5);
            for(auto & col : cols)
                add(col);
        }
        std::vector<Gtk::TreeModelColumn<double> > cols;
    };

    void update(SPObject* o, const int rows, const int cols)
    {
        if(_locked)
            return;

        _model->clear();

        _tree.remove_all_columns();

        std::vector<gdouble>* values = nullptr;
        if(SP_IS_FECOLORMATRIX(o))
            values = &SP_FECOLORMATRIX(o)->values;
        else if(SP_IS_FECONVOLVEMATRIX(o))
            values = &SP_FECONVOLVEMATRIX(o)->kernelMatrix;
        else
            return;

        if(o) {
            int ndx = 0;

            for(int i = 0; i < cols; ++i) {
                _tree.append_column_numeric_editable("", _columns.cols[i], "%.2f");
                dynamic_cast<Gtk::CellRendererText*>(
                    _tree.get_column_cell_renderer(i))->signal_edited().connect(
                        sigc::mem_fun(*this, &MatrixAttr::rebind));
            }

            for(int r = 0; r < rows; ++r) {
                Gtk::TreeRow row = *(_model->append());
                // Default to identity matrix
                for(int c = 0; c < cols; ++c, ++ndx)
                    row[_columns.cols[c]] = ndx < (int)values->size() ? (*values)[ndx] : (r == c ? 1 : 0);
            }
        }
    }

    void rebind(const Glib::ustring&, const Glib::ustring&)
    {
        _locked = true;
        signal_attr_changed()();
        _locked = false;
    }

    bool _locked;
    Gtk::TreeView _tree;
    Glib::RefPtr<Gtk::ListStore> _model;
    MatrixColumns _columns;
};

// Displays a matrix or a slider for feColorMatrix
class FilterEffectsDialog::ColorMatrixValues : public Gtk::Frame, public AttrWidget
{
public:
    ColorMatrixValues()
        : AttrWidget(SPAttr::VALUES),
          // TRANSLATORS: this dialog is accessible via menu Filters - Filter editor
          _matrix(SPAttr::VALUES, _("This matrix determines a linear transform on color space. Each line affects one of the color components. Each column determines how much of each color component from the input is passed to the output. The last column does not depend on input colors, so can be used to adjust a constant component value.")),
          _saturation("", 0, 0, 1, 0.1, 0.01, 2, SPAttr::VALUES),
          _angle("", 0, 0, 360, 0.1, 0.01, 1, SPAttr::VALUES),
          _label(C_("Label", "None"), Gtk::ALIGN_START),
          _use_stored(false),
          _saturation_store(0),
          _angle_store(0)
    {
        _matrix.signal_attr_changed().connect(signal_attr_changed().make_slot());
        _saturation.signal_attr_changed().connect(signal_attr_changed().make_slot());
        _angle.signal_attr_changed().connect(signal_attr_changed().make_slot());
        signal_attr_changed().connect(sigc::mem_fun(*this, &ColorMatrixValues::update_store));

        _matrix.show();
        _saturation.show();
        _angle.show();
        _label.show();
        _label.set_sensitive(false);

        set_shadow_type(Gtk::SHADOW_NONE);
    }

    void set_from_attribute(SPObject* o) override
    {
        if(SP_IS_FECOLORMATRIX(o)) {
            SPFeColorMatrix* col = SP_FECOLORMATRIX(o);
            remove();
            switch(col->type) {
                case COLORMATRIX_SATURATE:
                    add(_saturation);
                    if(_use_stored)
                        _saturation.set_value(_saturation_store);
                    else
                        _saturation.set_from_attribute(o);
                    break;
                case COLORMATRIX_HUEROTATE:
                    add(_angle);
                    if(_use_stored)
                        _angle.set_value(_angle_store);
                    else
                        _angle.set_from_attribute(o);
                    break;
                case COLORMATRIX_LUMINANCETOALPHA:
                    add(_label);
                    break;
                case COLORMATRIX_MATRIX:
                default:
                    add(_matrix);
                    if(_use_stored)
                        _matrix.set_values(_matrix_store);
                    else
                        _matrix.set_from_attribute(o);
                    break;
            }
            _use_stored = true;
        }
    }

    Glib::ustring get_as_attribute() const override
    {
        const Widget* w = get_child();
        if(w == &_label)
            return "";
        else
            return dynamic_cast<const AttrWidget*>(w)->get_as_attribute();
    }

    void clear_store()
    {
        _use_stored = false;
    }
private:
    void update_store()
    {
        const Widget* w = get_child();
        if(w == &_matrix)
            _matrix_store = _matrix.get_values();
        else if(w == &_saturation)
            _saturation_store = _saturation.get_value();
        else if(w == &_angle)
            _angle_store = _angle.get_value();
    }

    MatrixAttr _matrix;
    SpinScale _saturation;
    SpinScale _angle;
    Gtk::Label _label;

    // Store separate values for the different color modes
    bool _use_stored;
    std::vector<double> _matrix_store;
    double _saturation_store;
    double _angle_store;
};

static Inkscape::UI::Dialog::FileOpenDialog * selectFeImageFileInstance = nullptr;

//Displays a chooser for feImage input
//It may be a filename or the id for an SVG Element
//described in xlink:href syntax
class FileOrElementChooser : public Gtk::Box, public AttrWidget
{
public:
    FileOrElementChooser(const SPAttr a)
        : AttrWidget(a)
        , Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    {
        pack_start(_entry, false, false);
        pack_start(_fromFile, false, false);
        pack_start(_fromSVGElement, false, false);

        _fromFile.set_label(_("Image File"));
        _fromFile.signal_clicked().connect(sigc::mem_fun(*this, &FileOrElementChooser::select_file));

        _fromSVGElement.set_label(_("Selected SVG Element"));
        _fromSVGElement.signal_clicked().connect(sigc::mem_fun(*this, &FileOrElementChooser::select_svg_element));

        _entry.signal_changed().connect(signal_attr_changed().make_slot());

        show_all();

    }

    // Returns the element in xlink:href form.
    Glib::ustring get_as_attribute() const override
    {
        return _entry.get_text();
    }


    void set_from_attribute(SPObject* o) override
    {
        const gchar* val = attribute_value(o);
        if(val) {
            _entry.set_text(val);
        } else {
            _entry.set_text("");
        }
    }

    void set_desktop(SPDesktop* d){
        _desktop = d;
    }

private:
    void select_svg_element(){
        Inkscape::Selection* sel = _desktop->getSelection();
        if (sel->isEmpty()) return;
        Inkscape::XML::Node* node = sel->xmlNodes().front();
        if (!node || !node->matchAttributeName("id")) return;

        std::ostringstream xlikhref;
        xlikhref << "#" << node->attribute("id");
        _entry.set_text(xlikhref.str());
    }

    void select_file(){

        //# Get the current directory for finding files
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        Glib::ustring open_path;
        Glib::ustring attr = prefs->getString("/dialogs/open/path");
        if (!attr.empty())
            open_path = attr;

        //# Test if the open_path directory exists
        if (!Inkscape::IO::file_test(open_path.c_str(),
                  (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
            open_path = "";

        //# If no open path, default to our home directory
        if (open_path.size() < 1)
            {
            open_path = g_get_home_dir();
            open_path.append(G_DIR_SEPARATOR_S);
            }

        //# Create a dialog if we don't already have one
        if (!selectFeImageFileInstance) {
            selectFeImageFileInstance =
                  Inkscape::UI::Dialog::FileOpenDialog::create(
                     *_desktop->getToplevel(),
                     open_path,
                     Inkscape::UI::Dialog::SVG_TYPES,/*TODO: any image, not just svg*/
                     (char const *)_("Select an image to be used as feImage input"));
        }

        //# Show the dialog
        bool const success = selectFeImageFileInstance->show();
        if (!success)
            return;

        //# User selected something.  Get name and type
        Glib::ustring fileName = selectFeImageFileInstance->getFilename();

        if (fileName.size() > 0) {

            Glib::ustring newFileName = Glib::filename_to_utf8(fileName);

            if ( newFileName.size() > 0)
                fileName = newFileName;
            else
                g_warning( "ERROR CONVERTING OPEN FILENAME TO UTF-8" );

            open_path = fileName;
            open_path.append(G_DIR_SEPARATOR_S);
            prefs->setString("/dialogs/open/path", open_path);

            _entry.set_text(fileName);
        }
        return;
    }

    Gtk::Entry _entry;
    Gtk::Button _fromFile;
    Gtk::Button _fromSVGElement;
    SPDesktop* _desktop;
};

class FilterEffectsDialog::Settings
{
public:
    typedef sigc::slot<void, const AttrWidget*> SetAttrSlot;

    Settings(FilterEffectsDialog& d, Gtk::Box& b, SetAttrSlot slot, const int maxtypes)
        : _dialog(d), _set_attr_slot(std::move(slot)), _current_type(-1), _max_types(maxtypes)
    {
        _groups.resize(_max_types);
        _attrwidgets.resize(_max_types);
        _size_group = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

        for(int i = 0; i < _max_types; ++i) {
            _groups[i] = new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 3);
            b.set_spacing(4);
            b.pack_start(*_groups[i], Gtk::PACK_SHRINK);
        }
        //_current_type = 0;  If set to 0 then update_and_show() fails to update properly.
    }

    ~Settings()
    {
        for(int i = 0; i < _max_types; ++i) {
            delete _groups[i];
            for(auto & j : _attrwidgets[i])
                delete j;
        }
    }

    // Show the active settings group and update all the AttrWidgets with new values
    void show_and_update(const int t, SPObject* ob)
    {
        if(t != _current_type) {
            type(t);
            for(auto & _group : _groups)
                _group->hide();
        }
        if(t >= 0) {
            _groups[t]->show(); // Do not use show_all(), it shows children than should be hidden
        }
        _dialog.set_attrs_locked(true);
        for(auto & i : _attrwidgets[_current_type])
            i->set_from_attribute(ob);
        _dialog.set_attrs_locked(false);
    }

    int get_current_type() const
    {
        return _current_type;
    }

    void type(const int t)
    {
        _current_type = t;
    }

    void add_no_params()
    {
        Gtk::Label* lbl = Gtk::manage(new Gtk::Label(_("This SVG filter effect does not require any parameters.")));
        add_widget(lbl, "");
    }

    void add_notimplemented()
    {
        Gtk::Label* lbl = Gtk::manage(new Gtk::Label(_("This SVG filter effect is not yet implemented in Inkscape.")));
        add_widget(lbl, "");
    }

    // LightSource
    LightSourceControl* add_lightsource();

    // Component Transfer Values
    ComponentTransferValues* add_componenttransfervalues(const Glib::ustring& label, SPFeFuncNode::Channel channel);

    // CheckButton
    CheckButtonAttr* add_checkbutton(bool def, const SPAttr attr, const Glib::ustring& label,
                                     const Glib::ustring& tv, const Glib::ustring& fv, char* tip_text = nullptr)
    {
        CheckButtonAttr* cb = new CheckButtonAttr(def, label, tv, fv, attr, tip_text);
        add_widget(cb, "");
        add_attr_widget(cb);
        return cb;
    }

    // ColorButton
    ColorButton* add_color(unsigned int def, const SPAttr attr, const Glib::ustring& label, char* tip_text = nullptr)
    {
        ColorButton* col = new ColorButton(def, attr, tip_text);
        add_widget(col, label);
        add_attr_widget(col);
        return col;
    }

    // Matrix
    MatrixAttr* add_matrix(const SPAttr attr, const Glib::ustring& label, char* tip_text)
    {
        MatrixAttr* conv = new MatrixAttr(attr, tip_text);
        add_widget(conv, label);
        add_attr_widget(conv);
        return conv;
    }

    // ColorMatrixValues
    ColorMatrixValues* add_colormatrixvalues(const Glib::ustring& label)
    {
        ColorMatrixValues* cmv = new ColorMatrixValues();
        add_widget(cmv, label);
        add_attr_widget(cmv);
        return cmv;
    }

    // SpinScale
    SpinScale* add_spinscale(double def, const SPAttr attr, const Glib::ustring& label,
                         const double lo, const double hi, const double step_inc, const double climb, const int digits, char* tip_text = nullptr)
    {
        Glib::ustring tip_text2;
        if (tip_text)
            tip_text2 = tip_text;
        SpinScale* spinslider = new SpinScale("", def, lo, hi, step_inc, climb, digits, attr, tip_text2);
        add_widget(spinslider, label);
        add_attr_widget(spinslider);
        return spinslider;
    }

    // DualSpinScale
    DualSpinScale* add_dualspinscale(const SPAttr attr, const Glib::ustring& label,
                                     const double lo, const double hi, const double step_inc,
                                     const double climb, const int digits,
                                     const Glib::ustring tip_text1 = "",
                                     const Glib::ustring tip_text2 = "")
    {
        DualSpinScale* dss = new DualSpinScale("", "", lo, lo, hi, step_inc, climb, digits, attr, tip_text1, tip_text2);
        add_widget(dss, label);
        add_attr_widget(dss);
        return dss;
    }

    // SpinButton
    SpinButtonAttr* add_spinbutton(double defalt_value, const SPAttr attr, const Glib::ustring& label,
                                       const double lo, const double hi, const double step_inc,
                                       const double climb, const int digits, char* tip = nullptr)
    {
        SpinButtonAttr* sb = new SpinButtonAttr(lo, hi, step_inc, climb, digits, attr, defalt_value, tip);
        add_widget(sb, label);
        add_attr_widget(sb);
        return sb;
    }

    // DualSpinButton
    DualSpinButton* add_dualspinbutton(char* defalt_value, const SPAttr attr, const Glib::ustring& label,
                                       const double lo, const double hi, const double step_inc,
                                       const double climb, const int digits, char* tip1 = nullptr, char* tip2 = nullptr)
    {
        DualSpinButton* dsb = new DualSpinButton(defalt_value, lo, hi, step_inc, climb, digits, attr, tip1, tip2);
        add_widget(dsb, label);
        add_attr_widget(dsb);
        return dsb;
    }

    // MultiSpinButton
    MultiSpinButton* add_multispinbutton(double def1, double def2, const SPAttr attr1, const SPAttr attr2,
                                         const Glib::ustring& label, const double lo, const double hi,
                                         const double step_inc, const double climb, const int digits, char* tip1 = nullptr, char* tip2 = nullptr)
    {
        std::vector<SPAttr> attrs;
        attrs.push_back(attr1);
        attrs.push_back(attr2);

        std::vector<double> default_values;
        default_values.push_back(def1);
        default_values.push_back(def2);

        std::vector<char*> tips;
        tips.push_back(tip1);
        tips.push_back(tip2);

        MultiSpinButton* msb = new MultiSpinButton(lo, hi, step_inc, climb, digits, attrs, default_values, tips);
        add_widget(msb, label);
        for(auto & i : msb->get_spinbuttons())
            add_attr_widget(i);
        return msb;
    }
    MultiSpinButton* add_multispinbutton(double def1, double def2, double def3, const SPAttr attr1, const SPAttr attr2,
                                         const SPAttr attr3, const Glib::ustring& label, const double lo,
                                         const double hi, const double step_inc, const double climb, const int digits, char* tip1 = nullptr, char* tip2 = nullptr, char* tip3 = nullptr)
    {
        std::vector<SPAttr> attrs;
        attrs.push_back(attr1);
        attrs.push_back(attr2);
        attrs.push_back(attr3);

        std::vector<double> default_values;
        default_values.push_back(def1);
        default_values.push_back(def2);
        default_values.push_back(def3);

        std::vector<char*> tips;
        tips.push_back(tip1);
        tips.push_back(tip2);
        tips.push_back(tip3);

        MultiSpinButton* msb = new MultiSpinButton(lo, hi, step_inc, climb, digits, attrs, default_values, tips);
        add_widget(msb, label);
        for(auto & i : msb->get_spinbuttons())
            add_attr_widget(i);
        return msb;
    }

    // FileOrElementChooser
    FileOrElementChooser* add_fileorelement(const SPAttr attr, const Glib::ustring& label)
    {
        FileOrElementChooser* foech = new FileOrElementChooser(attr);
        foech->set_desktop(_dialog.getDesktop());
        add_widget(foech, label);
        add_attr_widget(foech);
        return foech;
    }

    // ComboBoxEnum
    template<typename T> ComboBoxEnum<T>* add_combo(T default_value, const SPAttr attr,
                                  const Glib::ustring& label,
                                  const Util::EnumDataConverter<T>& conv, char* tip_text = nullptr)
    {
        ComboWithTooltip<T>* combo = new ComboWithTooltip<T>(default_value, conv, attr, tip_text);
        add_widget(combo, label);
        add_attr_widget(combo->get_attrwidget());
        return combo->get_attrwidget();
    }

    // Entry
    EntryAttr* add_entry(const SPAttr attr,
                         const Glib::ustring& label,
                         char* tip_text = nullptr)
    {
        EntryAttr* entry = new EntryAttr(attr, tip_text);
        add_widget(entry, label);
        add_attr_widget(entry);
        return entry;
    }

    Glib::RefPtr<Gtk::SizeGroup> _size_group;
private:
    void add_attr_widget(AttrWidget* a)
    {
        _attrwidgets[_current_type].push_back(a);
        a->signal_attr_changed().connect(sigc::bind(_set_attr_slot, a));
    }

    /* Adds a new settings widget using the specified label. The label will be formatted with a colon
       and all widgets within the setting group are aligned automatically. */
    void add_widget(Gtk::Widget* w, const Glib::ustring& label)
    {
        Gtk::Box *hb = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        hb->set_spacing(12);

        if (label != "") {
            Gtk::Label *lbl = Gtk::manage(new Gtk::Label(label));
            lbl->set_xalign(0.0);
            hb->pack_start(*lbl, Gtk::PACK_SHRINK);
            _size_group->add_widget(*lbl);
        }

        hb->pack_start(*w, Gtk::PACK_EXPAND_WIDGET);
        _groups[_current_type]->pack_start(*hb, Gtk::PACK_EXPAND_WIDGET);
        hb->show_all();
    }

    std::vector<Gtk::Box*> _groups;
    FilterEffectsDialog& _dialog;
    SetAttrSlot _set_attr_slot;
    std::vector<std::vector< AttrWidget*> > _attrwidgets;
    int _current_type, _max_types;
};

// Displays sliders and/or tables for feComponentTransfer
class FilterEffectsDialog::ComponentTransferValues : public Gtk::Frame, public AttrWidget
{
public:
    ComponentTransferValues(FilterEffectsDialog& d, SPFeFuncNode::Channel channel)
        : AttrWidget(SPAttr::INVALID),
          _dialog(d),
          _settings(d, _box, sigc::mem_fun(*this, &ComponentTransferValues::set_func_attr), COMPONENTTRANSFER_TYPE_ERROR),
          _type(ComponentTransferTypeConverter, SPAttr::TYPE, false),
          _channel(channel),
          _funcNode(nullptr),
          _box(Gtk::ORIENTATION_VERTICAL)
    {
        set_shadow_type(Gtk::SHADOW_IN);
        add(_box);
        _box.add(_type);
        _box.reorder_child(_type, 0);
        _type.signal_changed().connect(sigc::mem_fun(*this, &ComponentTransferValues::on_type_changed));

        _settings.type(COMPONENTTRANSFER_TYPE_LINEAR);
        _settings.add_spinscale(1, SPAttr::SLOPE,     _("Slope"),     -10, 10, 0.1, 0.01, 2);
        _settings.add_spinscale(0, SPAttr::INTERCEPT, _("Intercept"), -10, 10, 0.1, 0.01, 2);

        _settings.type(COMPONENTTRANSFER_TYPE_GAMMA);
        _settings.add_spinscale(1, SPAttr::AMPLITUDE, _("Amplitude"),   0, 10, 0.1, 0.01, 2);
        _settings.add_spinscale(1, SPAttr::EXPONENT,  _("Exponent"),    0, 10, 0.1, 0.01, 2);
        _settings.add_spinscale(0, SPAttr::OFFSET,    _("Offset"),    -10, 10, 0.1, 0.01, 2);

        _settings.type(COMPONENTTRANSFER_TYPE_TABLE);
        _settings.add_entry(SPAttr::TABLEVALUES,  _("Table"));

        _settings.type(COMPONENTTRANSFER_TYPE_DISCRETE);
        _settings.add_entry(SPAttr::TABLEVALUES,  _("Discrete"));

        //_settings.type(COMPONENTTRANSFER_TYPE_IDENTITY);
        _settings.type(-1); // Force update_and_show() to show/hide windows correctly
    }

    // FuncNode can be in any order so we must search to find correct one.
    SPFeFuncNode* find_node(SPFeComponentTransfer* ct)
    {
        SPFeFuncNode* funcNode = nullptr;
        bool found = false;
        for(auto& node: ct->children) {
            funcNode = SP_FEFUNCNODE(&node);
            if( funcNode->channel == _channel ) {
                found = true;
                break;
            }
        }
        if( !found )
            funcNode = nullptr;

        return funcNode;
    }

    void set_func_attr(const AttrWidget* input)
    {
        _dialog.set_attr( _funcNode, input->get_attribute(), input->get_as_attribute().c_str());
    }

    // Set new type and update widget visibility
    void set_from_attribute(SPObject* o) override
    {
        // See componenttransfer.cpp
        if(SP_IS_FECOMPONENTTRANSFER(o)) {
            SPFeComponentTransfer* ct = SP_FECOMPONENTTRANSFER(o);

            _funcNode = find_node(ct);
            if( _funcNode ) {
                _type.set_from_attribute( _funcNode );
            } else {
                // Create <funcNode>
                SPFilterPrimitive* prim = _dialog._primitive_list.get_selected();
                if(prim) {
                    Inkscape::XML::Document *xml_doc = prim->document->getReprDoc();
                    Inkscape::XML::Node *repr = nullptr;
                    switch(_channel) {
                        case SPFeFuncNode::R:
                            repr = xml_doc->createElement("svg:feFuncR");
                            break;
                        case SPFeFuncNode::G:
                            repr = xml_doc->createElement("svg:feFuncG");
                            break;
                        case SPFeFuncNode::B:
                            repr = xml_doc->createElement("svg:feFuncB");
                            break;
                        case SPFeFuncNode::A:
                            repr = xml_doc->createElement("svg:feFuncA");
                            break;
                    }

                    //XML Tree being used directly here while it shouldn't be.
                    prim->getRepr()->appendChild(repr);
                    Inkscape::GC::release(repr);

                    // Now we should find it!
                    _funcNode = find_node(ct);
                    if( _funcNode ) {
                        _funcNode->setAttribute( "type", "identity" );
                    } else {
                        //std::cout << "ERROR ERROR: feFuncX not found!" << std::endl;
                    }
                }
            }

            update();
        }
    }

private:
    void on_type_changed()
    {
        SPFilterPrimitive* prim = _dialog._primitive_list.get_selected();
        if(prim) {

            _funcNode->setAttributeOrRemoveIfEmpty("type", _type.get_as_attribute());

            SPFilter* filter = _dialog._filter_modifier.get_selected_filter();
            filter->requestModified(SP_OBJECT_MODIFIED_FLAG);

            DocumentUndo::done(prim->document, SP_VERB_DIALOG_FILTER_EFFECTS, _("New transfer function type"));
            update();
        }
    }

    void update()
    {
        SPFilterPrimitive* prim = _dialog._primitive_list.get_selected();
        if(prim && _funcNode) {
            _settings.show_and_update(_type.get_active_data()->id, _funcNode);
        }
    }

public:
    Glib::ustring get_as_attribute() const override
    {
        return "";
    }

    FilterEffectsDialog& _dialog;
    Gtk::Box _box;
    Settings _settings;
    ComboBoxEnum<FilterComponentTransferType> _type;
    SPFeFuncNode::Channel _channel; // RGBA
    SPFeFuncNode* _funcNode;
};

// Settings for the three light source objects
class FilterEffectsDialog::LightSourceControl : public AttrWidget
{
public:
    LightSourceControl(FilterEffectsDialog& d)
        : AttrWidget(SPAttr::INVALID),
          _dialog(d),
          _settings(d, _box, sigc::mem_fun(_dialog, &FilterEffectsDialog::set_child_attr_direct), LIGHT_ENDSOURCE),
          _light_label(_("Light Source:")),
          _light_source(LightSourceConverter),
          _locked(false),
          _box(Gtk::ORIENTATION_VERTICAL),
          _light_box(Gtk::ORIENTATION_HORIZONTAL)
    {
        _light_label.set_xalign(0.0);
        _settings._size_group->add_widget(_light_label);
        _light_box.pack_start(_light_label, Gtk::PACK_SHRINK);
        _light_box.pack_start(_light_source, Gtk::PACK_EXPAND_WIDGET);
        _light_box.show_all();
        _light_box.set_spacing(12);

        _box.add(_light_box);
        _box.reorder_child(_light_box, 0);
        _light_source.signal_changed().connect(sigc::mem_fun(*this, &LightSourceControl::on_source_changed));

        // FIXME: these range values are complete crap

        _settings.type(LIGHT_DISTANT);
        _settings.add_spinscale(0, SPAttr::AZIMUTH, _("Azimuth:"), 0, 360, 1, 1, 0, _("Direction angle for the light source on the XY plane, in degrees"));
        _settings.add_spinscale(0, SPAttr::ELEVATION, _("Elevation:"), 0, 360, 1, 1, 0, _("Direction angle for the light source on the YZ plane, in degrees"));

        _settings.type(LIGHT_POINT);
        _settings.add_multispinbutton(/*default x:*/ (double) 0, /*default y:*/ (double) 0, /*default z:*/ (double) 0, SPAttr::X, SPAttr::Y, SPAttr::Z, _("Location:"), -99999, 99999, 1, 100, 0, _("X coordinate"), _("Y coordinate"), _("Z coordinate"));

        _settings.type(LIGHT_SPOT);
        _settings.add_multispinbutton(/*default x:*/ (double) 0, /*default y:*/ (double) 0, /*default z:*/ (double) 0, SPAttr::X, SPAttr::Y, SPAttr::Z, _("Location:"), -99999, 99999, 1, 100, 0, _("X coordinate"), _("Y coordinate"), _("Z coordinate"));
        _settings.add_multispinbutton(/*default x:*/ (double) 0, /*default y:*/ (double) 0, /*default z:*/ (double) 0,
                                      SPAttr::POINTSATX, SPAttr::POINTSATY, SPAttr::POINTSATZ,
                                      _("Points at:"), -99999, 99999, 1, 100, 0, _("X coordinate"), _("Y coordinate"), _("Z coordinate"));
        _settings.add_spinscale(1, SPAttr::SPECULAREXPONENT, _("Specular Exponent:"), 1, 100, 1, 1, 0, _("Exponent value controlling the focus for the light source"));
        //TODO: here I have used 100 degrees as default value. But spec says that if not specified, no limiting cone is applied. So, there should be a way for the user to set a "no limiting cone" option.
        _settings.add_spinscale(100, SPAttr::LIMITINGCONEANGLE, _("Cone Angle:"), 1, 100, 1, 1, 0, _("This is the angle between the spot light axis (i.e. the axis between the light source and the point to which it is pointing at) and the spot light cone. No light is projected outside this cone."));

        _settings.type(-1); // Force update_and_show() to show/hide windows correctly

    }

    Gtk::Box& get_box()
    {
        return _box;
    }
protected:
    Glib::ustring get_as_attribute() const override
    {
        return "";
    }
    void set_from_attribute(SPObject* o) override
    {
        if(_locked)
            return;

        _locked = true;

        SPObject* child = o->firstChild();

        if(SP_IS_FEDISTANTLIGHT(child))
            _light_source.set_active(0);
        else if(SP_IS_FEPOINTLIGHT(child))
            _light_source.set_active(1);
        else if(SP_IS_FESPOTLIGHT(child))
            _light_source.set_active(2);
        else
            _light_source.set_active(-1);

        update();

        _locked = false;
    }
private:
    void on_source_changed()
    {
        if(_locked)
            return;

        SPFilterPrimitive* prim = _dialog._primitive_list.get_selected();
        if(prim) {
            _locked = true;

            SPObject* child = prim->firstChild();
            const int ls = _light_source.get_active_row_number();
            // Check if the light source type has changed
            if(!(ls == -1 && !child) &&
               !(ls == 0 && SP_IS_FEDISTANTLIGHT(child)) &&
               !(ls == 1 && SP_IS_FEPOINTLIGHT(child)) &&
               !(ls == 2 && SP_IS_FESPOTLIGHT(child))) {
                if(child)
                    //XML Tree being used directly here while it shouldn't be.
                    sp_repr_unparent(child->getRepr());

                if(ls != -1) {
                    Inkscape::XML::Document *xml_doc = prim->document->getReprDoc();
                    Inkscape::XML::Node *repr = xml_doc->createElement(_light_source.get_active_data()->key.c_str());
                    //XML Tree being used directly here while it shouldn't be.
                    prim->getRepr()->appendChild(repr);
                    Inkscape::GC::release(repr);
                }

                DocumentUndo::done(prim->document, SP_VERB_DIALOG_FILTER_EFFECTS, _("New light source"));
                update();
            }

            _locked = false;
        }
    }

    void update()
    {
        _box.hide();
        _box.show();
        _light_box.show_all();

        SPFilterPrimitive* prim = _dialog._primitive_list.get_selected();
        if(prim && prim->firstChild())
            _settings.show_and_update(_light_source.get_active_data()->id, prim->firstChild());
    }

    FilterEffectsDialog& _dialog;
    Gtk::Box _box;
    Settings _settings;
    Gtk::Box _light_box;
    Gtk::Label _light_label;
    ComboBoxEnum<LightSource> _light_source;
    bool _locked;
};

    // ComponentTransferValues
FilterEffectsDialog::ComponentTransferValues* FilterEffectsDialog::Settings::add_componenttransfervalues(const Glib::ustring& label, SPFeFuncNode::Channel channel)
    {
        ComponentTransferValues* ct = new ComponentTransferValues(_dialog, channel);
        add_widget(ct, label);
        add_attr_widget(ct);
        return ct;
    }


FilterEffectsDialog::LightSourceControl* FilterEffectsDialog::Settings::add_lightsource()
{
    LightSourceControl* ls = new LightSourceControl(_dialog);
    add_attr_widget(ls);
    add_widget(&ls->get_box(), "");
    return ls;
}

static Gtk::Menu * create_popup_menu(Gtk::Widget& parent,
                                     sigc::slot<void> dup,
                                     sigc::slot<void> rem)
{
    auto menu = Gtk::manage(new Gtk::Menu);

    Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(_("_Duplicate"),true));
    mi->signal_activate().connect(dup);
    mi->show();
    menu->append(*mi);

    mi = Gtk::manage(new Gtk::MenuItem(_("_Remove"), true));
    menu->append(*mi);
    mi->signal_activate().connect(rem);
    mi->show();
    menu->accelerate(parent);

    return menu;
}

/*** FilterModifier ***/
FilterEffectsDialog::FilterModifier::FilterModifier(FilterEffectsDialog& d)
    :    Gtk::Box(Gtk::ORIENTATION_VERTICAL),
         _desktop(nullptr),
         _dialog(d),
         _add(_("_New"), true),
         _observer(new Inkscape::XML::SignalObserver)
{
    Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow);
    pack_start(*sw);
    pack_start(_add, false, false);
    sw->add(_list);

    _model = Gtk::ListStore::create(_columns);
    _list.set_model(_model);
    _cell_toggle.set_active(true);
    const int selcol = _list.append_column("", _cell_toggle);
    Gtk::TreeViewColumn* col = _list.get_column(selcol - 1);
    if(col)
       col->add_attribute(_cell_toggle.property_active(), _columns.sel);
    _list.append_column_editable(_("_Filter"), _columns.label);
    ((Gtk::CellRendererText*)_list.get_column(1)->get_first_cell())->
        signal_edited().connect(sigc::mem_fun(*this, &FilterEffectsDialog::FilterModifier::on_name_edited));

    _list.append_column("#", _columns.count);
    _list.get_column(2)->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    _list.get_column(2)->set_expand(false);
    _list.get_column(2)->set_reorderable(true);

    sw->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _list.get_column(1)->set_resizable(true);
    _list.get_column(1)->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    _list.get_column(1)->set_expand(true);

    _list.set_reorderable(true);
    _list.enable_model_drag_dest (Gdk::ACTION_MOVE);

    _list.signal_drag_drop().connect( sigc::mem_fun(*this, &FilterModifier::on_filter_move), false );

    sw->set_shadow_type(Gtk::SHADOW_IN);
    show_all_children();
    _add.signal_clicked().connect(sigc::mem_fun(*this, &FilterModifier::add_filter));
    _cell_toggle.signal_toggled().connect(sigc::mem_fun(*this, &FilterModifier::on_selection_toggled));
    _list.signal_button_release_event().connect_notify(
        sigc::mem_fun(*this, &FilterModifier::filter_list_button_release));

    _menu = create_popup_menu(*this,
                              sigc::mem_fun(*this, &FilterModifier::duplicate_filter),
                              sigc::mem_fun(*this, &FilterModifier::remove_filter));

    Gtk::MenuItem *rename_item = Gtk::manage(new Gtk::MenuItem(_("R_ename"), true));
    rename_item->signal_activate().connect(sigc::mem_fun(*this, &FilterModifier::rename_filter));
    rename_item->show();
    _menu->append(*rename_item);
    _menu->accelerate(*this);

    Gtk::MenuItem *select_item = Gtk::manage(new Gtk::MenuItem(_("Select"), true));
    select_item->signal_activate().connect(sigc::mem_fun(*this, &FilterModifier::select_filter_elements));
    select_item->show();
    _menu->append(*select_item);
    _menu->accelerate(*this);

    _list.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &FilterModifier::on_filter_selection_changed));
    _observer->signal_changed().connect(signal_filter_changed().make_slot());
}

FilterEffectsDialog::FilterModifier::~FilterModifier()
{
   _selectChangedConn.disconnect();
   _selectModifiedConn.disconnect();
   _resource_changed.disconnect();
   _doc_replaced.disconnect();
}

void FilterEffectsDialog::FilterModifier::setTargetDesktop(SPDesktop *desktop)
{
    if (_desktop != desktop) {
        if (_desktop) {
            _selectChangedConn.disconnect();
            _selectModifiedConn.disconnect();
            _doc_replaced.disconnect();
            _resource_changed.disconnect();
        }
        _desktop = desktop;
        if (desktop) {
            if (desktop->selection) {
                _selectChangedConn = desktop->selection->connectChanged(sigc::hide(sigc::mem_fun(*this, &FilterModifier::on_change_selection)));
                _selectModifiedConn = desktop->selection->connectModified(sigc::hide<0>(sigc::mem_fun(*this, &FilterModifier::on_modified_selection)));
            }
            _doc_replaced = desktop->connectDocumentReplaced( sigc::mem_fun(*this, &FilterModifier::on_document_replaced));
            _resource_changed = desktop->getDocument()->connectResourcesChanged("filter",sigc::mem_fun(*this, &FilterModifier::update_filters));

            update_filters();
        }
    }
}

// When the document changes, update connection to resources
void FilterEffectsDialog::FilterModifier::on_document_replaced(SPDesktop * /*desktop*/, SPDocument *document)
{
    if (_resource_changed) {
        _resource_changed.disconnect();
    }
    if (document)
    {
        _resource_changed = document->connectResourcesChanged("filter",sigc::mem_fun(*this, &FilterModifier::update_filters));
    }

    update_filters();
}

// When the selection changes, show the active filter(s) in the dialog
void FilterEffectsDialog::FilterModifier::on_change_selection()
{
    if (!_desktop)
        return;

    Inkscape::Selection *selection = _desktop->getSelection();
    update_selection(selection);
}

void FilterEffectsDialog::FilterModifier::on_modified_selection( guint flags )
{
    if (flags & ( SP_OBJECT_MODIFIED_FLAG |
                   SP_OBJECT_PARENT_MODIFIED_FLAG |
                   SP_OBJECT_STYLE_MODIFIED_FLAG) ) {
        on_change_selection();
    }
}

// Update each filter's sel property based on the current object selection;
//  If the filter is not used by any selected object, sel = 0,
//  otherwise sel is set to the total number of filters in use by selected objects
//  If only one filter is in use, it is selected
void FilterEffectsDialog::FilterModifier::update_selection(Selection *sel)
{
    if (!sel) {
        return;
    }

    std::set<SPFilter*> used;
    auto itemlist= sel->items();
    for(auto i=itemlist.begin(); itemlist.end() != i; ++i) {
        SPObject *obj = *i;
        SPStyle *style = obj->style;
        if (!style || !SP_IS_ITEM(obj)) {
            continue;
        }

        if (style->filter.set && style->getFilter()) {
            SP_ITEM(obj)->bbox_valid = FALSE;
            used.insert(style->getFilter());
        } else {
            used.insert(nullptr);
        }
    }

    const int size = used.size();

    for (Gtk::TreeIter iter = _model->children().begin(); iter != _model->children().end(); ++iter) {
        if (used.find((*iter)[_columns.filter]) != used.end()) {
            // If only one filter is in use by the selection, select it
            if (size == 1) {
                _list.get_selection()->select(iter);
            }
            (*iter)[_columns.sel] = size;
        } else {
            (*iter)[_columns.sel] = 0;
        }
    }
    update_counts();
}

void FilterEffectsDialog::FilterModifier::on_filter_selection_changed()
{
    _observer->set(get_selected_filter());
    signal_filter_changed()();
}

void FilterEffectsDialog::FilterModifier::on_name_edited(const Glib::ustring& path, const Glib::ustring& text)
{
    Gtk::TreeModel::iterator iter = _model->get_iter(path);

    if(iter) {
        SPFilter* filter = (*iter)[_columns.filter];
        filter->setLabel(text.c_str());
        DocumentUndo::done(filter->document, SP_VERB_DIALOG_FILTER_EFFECTS, _("Rename filter"));
        if(iter)
            (*iter)[_columns.label] = text;
    }
}

bool FilterEffectsDialog::FilterModifier::on_filter_move(const Glib::RefPtr<Gdk::DragContext>& /*context*/, int /*x*/, int /*y*/, guint /*time*/) {

//const Gtk::TreeModel::Path& /*path*/) {
/* The code below is bugged. Use of "object->getRepr()->setPosition(0)" is dangerous!
   Writing back the reordered list to XML (reordering XML nodes) should be implemented differently.
   Note that the dialog does also not update its list of filters when the order is manually changed
   using the XML dialog
  for(Gtk::TreeModel::iterator i = _model->children().begin(); i != _model->children().end(); ++i) {
      SPObject* object = (*i)[_columns.filter];
      if(object && object->getRepr()) ;
        object->getRepr()->setPosition(0);
  }
*/
  return false;
}

void FilterEffectsDialog::FilterModifier::on_selection_toggled(const Glib::ustring& path)
{
    Gtk::TreeIter iter = _model->get_iter(path);

    if(iter) {
        SPDesktop *desktop = _dialog.getDesktop();
        SPDocument *doc = desktop->getDocument();
        SPFilter* filter = (*iter)[_columns.filter];
        Inkscape::Selection *sel = desktop->getSelection();

        /* If this filter is the only one used in the selection, unset it */
        if((*iter)[_columns.sel] == 1)
            filter = nullptr;

        auto itemlist= sel->items();
        for(auto i=itemlist.begin(); itemlist.end() != i; ++i) {
            SPItem * item = *i;
            SPStyle *style = item->style;
            g_assert(style != nullptr);

            if (filter) {
                sp_style_set_property_url(item, "filter", filter, false);
            } else {
                ::remove_filter(item, false);
            }

            item->requestDisplayUpdate((SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG ));
        }

        update_selection(sel);
        DocumentUndo::done(doc, SP_VERB_DIALOG_FILTER_EFFECTS,  _("Apply filter"));
    }
}


void FilterEffectsDialog::FilterModifier::update_counts()
{
    for(const auto & i : _model->children()) {
        SPFilter* f = SP_FILTER(i[_columns.filter]);
        i[_columns.count] = f->getRefCount();
        }
}

/* Add all filters in the document to the combobox.
   Keeps the same selection if possible, otherwise selects the first element */
void FilterEffectsDialog::FilterModifier::update_filters()
{
    SPDesktop* desktop = _dialog.getDesktop();
    SPDocument* document = desktop->getDocument();

    std::vector<SPObject *> filters = document->getResourceList( "filter" );

    _model->clear();

    for (auto filter : filters) {
        Gtk::TreeModel::Row row = *_model->append();
        SPFilter* f = SP_FILTER(filter);
        row[_columns.filter] = f;
        const gchar* lbl = f->label();
        const gchar* id = f->getId();
        row[_columns.label] = lbl ? lbl : (id ? id : "filter");
    }

    update_selection(desktop->selection);
    _dialog.update_filter_general_settings_view();
}

SPFilter* FilterEffectsDialog::FilterModifier::get_selected_filter()
{
    if(_list.get_selection()) {
        Gtk::TreeModel::iterator i = _list.get_selection()->get_selected();

        if(i)
            return (*i)[_columns.filter];
    }

    return nullptr;
}

void FilterEffectsDialog::FilterModifier::select_filter(const SPFilter* filter)
{
    if(filter) {
        for(Gtk::TreeModel::iterator i = _model->children().begin();
            i != _model->children().end(); ++i) {
            if((*i)[_columns.filter] == filter) {
                _list.get_selection()->select(i);
                break;
            }
        }
    }
}

void FilterEffectsDialog::FilterModifier::filter_list_button_release(GdkEventButton* event)
{
    if((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
        const bool sensitive = get_selected_filter() != nullptr;
	auto items = _menu->get_children();
	items[0]->set_sensitive(sensitive);
        items[1]->set_sensitive(sensitive);
        items[3]->set_sensitive(sensitive);

        _menu->popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
    }
}

void FilterEffectsDialog::FilterModifier::add_filter()
{
    SPDocument* doc = _dialog.getDesktop()->getDocument();
    SPFilter* filter = new_filter(doc);

    const int count = _model->children().size();
    std::ostringstream os;
    os << _("filter") << count;
    filter->setLabel(os.str().c_str());

    update_filters();

    select_filter(filter);

    DocumentUndo::done(doc, SP_VERB_DIALOG_FILTER_EFFECTS, _("Add filter"));
}

void FilterEffectsDialog::FilterModifier::remove_filter()
{
    SPFilter *filter = get_selected_filter();

    if(filter) {
        SPDocument* doc = filter->document;

        // Delete all references to this filter
        std::vector<SPItem*> x,y;
        std::vector<SPItem*> all = get_all_items(x, _desktop->currentRoot(), _desktop, false, false, true, y);
        for(std::vector<SPItem*>::const_iterator i=all.begin(); all.end() != i; ++i) {
            if (!SP_IS_ITEM(*i)) {
                continue;
            }
            SPItem *item = *i;
            if (!item->style) {
                continue;
            }

            const SPIFilter *ifilter = &(item->style->filter);
            if (ifilter && ifilter->href) {
                const SPObject *obj = ifilter->href->getObject();
                if (obj && obj == (SPObject *)filter) {
                    ::remove_filter(item, false);
                }
            }
        }

        //XML Tree being used directly here while it shouldn't be.
        sp_repr_unparent(filter->getRepr());

        DocumentUndo::done(doc, SP_VERB_DIALOG_FILTER_EFFECTS, _("Remove filter"));

        update_filters();
    }
}

void FilterEffectsDialog::FilterModifier::duplicate_filter()
{
    SPFilter* filter = get_selected_filter();

    if (filter) {
        Inkscape::XML::Node *repr = filter->getRepr();
        Inkscape::XML::Node *parent = repr->parent();
        repr = repr->duplicate(repr->document());
        parent->appendChild(repr);

        DocumentUndo::done(filter->document, SP_VERB_DIALOG_FILTER_EFFECTS, _("Duplicate filter"));

        update_filters();
    }
}

void FilterEffectsDialog::FilterModifier::rename_filter()
{
    _list.set_cursor(_model->get_path(_list.get_selection()->get_selected()), *_list.get_column(1), true);
}

void FilterEffectsDialog::FilterModifier::select_filter_elements()
{
    SPFilter *filter = get_selected_filter();

    if(!filter)
        return;

    std::vector<SPItem*> x,y;
    std::vector<SPItem*> items;
    std::vector<SPItem*> all = get_all_items(x, _desktop->currentRoot(), _desktop, false, false, true, y);
    for(SPItem *item: all) {
        if (!item->style) {
            continue;
        }

        SPIFilter const &ifilter = item->style->filter;
        if (ifilter.href) {
            const SPObject *obj = ifilter.href->getObject();
            if (obj && obj == (SPObject *)filter) {
                items.push_back(item);
            }
        }
    }
    Inkscape::Selection *selection = _desktop->getSelection();
    selection->setList(items);
}

FilterEffectsDialog::CellRendererConnection::CellRendererConnection()
    : Glib::ObjectBase(typeid(CellRendererConnection))
    , _primitive(*this, "primitive", nullptr)
{}

Glib::PropertyProxy<void*> FilterEffectsDialog::CellRendererConnection::property_primitive()
{
    return _primitive.get_proxy();
}

void FilterEffectsDialog::CellRendererConnection::get_preferred_width_vfunc(Gtk::Widget& widget,
                                                                            int& minimum_width,
                                                                            int& natural_width) const
{
    PrimitiveList& primlist = dynamic_cast<PrimitiveList&>(widget);
    minimum_width = natural_width = size * primlist.primitive_count() + primlist.get_input_type_width() * 6;
}

void FilterEffectsDialog::CellRendererConnection::get_preferred_width_for_height_vfunc(Gtk::Widget& widget,
                                                                                       int /* height */,
                                                                                       int& minimum_width,
                                                                                       int& natural_width) const
{
    get_preferred_width(widget, minimum_width, natural_width);
}

void FilterEffectsDialog::CellRendererConnection::get_preferred_height_vfunc(Gtk::Widget& widget,
                                                                             int& minimum_height,
                                                                             int& natural_height) const
{
    // Scale the height depending on the number of inputs, unless it's
    // the first primitive, in which case there are no connections
    SPFilterPrimitive* prim = SP_FILTER_PRIMITIVE(_primitive.get_value());
    minimum_height = natural_height = size * input_count(prim);
}

void FilterEffectsDialog::CellRendererConnection::get_preferred_height_for_width_vfunc(Gtk::Widget& widget,
                                                                                       int /* width */,
                                                                                       int& minimum_height,
                                                                                       int& natural_height) const
{
    get_preferred_height(widget, minimum_height, natural_height);
}

/*** PrimitiveList ***/
FilterEffectsDialog::PrimitiveList::PrimitiveList(FilterEffectsDialog& d)
    : _dialog(d),
      _in_drag(0),
      _observer(new Inkscape::XML::SignalObserver)
{
    signal_draw().connect(sigc::mem_fun(*this, &PrimitiveList::on_draw_signal));

    add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);

    _model = Gtk::ListStore::create(_columns);

    set_reorderable(true);

    set_model(_model);
    append_column(_("_Effect"), _columns.type);
    get_column(0)->set_resizable(true);
    set_headers_visible();

    _observer->signal_changed().connect(signal_primitive_changed().make_slot());
    get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PrimitiveList::on_primitive_selection_changed));
    signal_primitive_changed().connect(sigc::mem_fun(*this, &PrimitiveList::queue_draw));

    init_text();

    int cols_count = append_column(_("Connections"), _connection_cell);
    Gtk::TreeViewColumn* col = get_column(cols_count - 1);
    if(col)
       col->add_attribute(_connection_cell.property_primitive(), _columns.primitive);
}

// Sets up a vertical Pango context/layout, and returns the largest
// width needed to render the FilterPrimitiveInput labels.
void FilterEffectsDialog::PrimitiveList::init_text()
{
    // Set up a vertical context+layout
    Glib::RefPtr<Pango::Context> context = create_pango_context();
    const Pango::Matrix matrix = {0, -1, 1, 0, 0, 0};
    context->set_matrix(matrix);
    _vertical_layout = Pango::Layout::create(context);

    // Store the maximum height and width of the an input type label
    // for later use in drawing and measuring.
    _input_type_height = _input_type_width = 0;
    for(unsigned int i = 0; i < FPInputConverter._length; ++i) {
        _vertical_layout->set_text(_(FPInputConverter.get_label((FilterPrimitiveInput)i).c_str()));
        int fontw, fonth;
        _vertical_layout->get_pixel_size(fontw, fonth);
        if(fonth > _input_type_width)
            _input_type_width = fonth;
        if (fontw > _input_type_height)
            _input_type_height = fontw;
    }
}

sigc::signal<void>& FilterEffectsDialog::PrimitiveList::signal_primitive_changed()
{
    return _signal_primitive_changed;
}

void FilterEffectsDialog::PrimitiveList::on_primitive_selection_changed()
{
    _observer->set(get_selected());
    signal_primitive_changed()();
    _dialog._color_matrix_values->clear_store();
}

/* Add all filter primitives in the current to the list.
   Keeps the same selection if possible, otherwise selects the first element */
void FilterEffectsDialog::PrimitiveList::update()
{
    SPFilter* f = _dialog._filter_modifier.get_selected_filter();
    const SPFilterPrimitive* active_prim = get_selected();
    _model->clear();

    if(f) {
        bool active_found = false;
        _dialog._primitive_box->set_sensitive(true);
        _dialog.update_filter_general_settings_view();
        for(auto& prim_obj: f->children) {
            SPFilterPrimitive *prim = SP_FILTER_PRIMITIVE(&prim_obj);
            if(!prim) {
                break;
            }
            Gtk::TreeModel::Row row = *_model->append();
            row[_columns.primitive] = prim;

            //XML Tree being used directly here while it shouldn't be.
            row[_columns.type_id] = FPConverter.get_id_from_key(prim->getRepr()->name());
            row[_columns.type] = _(FPConverter.get_label(row[_columns.type_id]).c_str());

            if (prim->getId()) {
                row[_columns.id] =  Glib::ustring(prim->getId());
            }

            if(prim == active_prim) {
                get_selection()->select(row);
                active_found = true;
            }
        }

        if(!active_found && _model->children().begin())
            get_selection()->select(_model->children().begin());

        columns_autosize();

        int width, height;
        get_size_request(width, height);
        if (height == -1) {
               // Need to account for the height of the input type text (rotated text) as well as the
               // column headers.  Input type text height determined in init_text() by measuring longest
               // string. Column header height determined by mapping y coordinate of visible
               // rectangle to widget coordinates.
                       Gdk::Rectangle vis;
                       int vis_x, vis_y;
               get_visible_rect(vis);
               convert_tree_to_widget_coords(vis.get_x(), vis.get_y(), vis_x, vis_y);
                       set_size_request(width, _input_type_height + 2 + vis_y);
        }
    }
    else {
        _dialog._primitive_box->set_sensitive(false);
        set_size_request(-1, -1);
    }
}

void FilterEffectsDialog::PrimitiveList::set_menu(Gtk::Widget& parent,
                                                  sigc::slot<void> dup,
                                                  sigc::slot<void> rem)
{
    _primitive_menu = create_popup_menu(parent, dup, rem);
}

SPFilterPrimitive* FilterEffectsDialog::PrimitiveList::get_selected()
{
    if(_dialog._filter_modifier.get_selected_filter()) {
        Gtk::TreeModel::iterator i = get_selection()->get_selected();
        if(i)
            return (*i)[_columns.primitive];
    }

    return nullptr;
}

void FilterEffectsDialog::PrimitiveList::select(SPFilterPrimitive* prim)
{
    for(Gtk::TreeIter i = _model->children().begin();
        i != _model->children().end(); ++i) {
        if((*i)[_columns.primitive] == prim)
            get_selection()->select(i);
    }
}

void FilterEffectsDialog::PrimitiveList::remove_selected()
{
    SPFilterPrimitive* prim = get_selected();

    if(prim) {
        _observer->set(nullptr);
        _model->erase(get_selection()->get_selected());

        //XML Tree being used directly here while it shouldn't be.
        sp_repr_unparent(prim->getRepr());

        DocumentUndo::done(_dialog.getDesktop()->getDocument(), SP_VERB_DIALOG_FILTER_EFFECTS,
                           _("Remove filter primitive"));

        update();
    }
}

bool FilterEffectsDialog::PrimitiveList::on_draw_signal(const Cairo::RefPtr<Cairo::Context> & cr)
{
    cr->set_line_width(1.0);
    // In GTK+ 3, the draw function receives the widget window, not the
    // bin_window (i.e., just the area under the column headers).  We
    // therefore translate the origin of our coordinate system to account for this
    int x_origin, y_origin;
    convert_bin_window_to_widget_coords(0,0,x_origin,y_origin);
    cr->translate(x_origin, y_origin);

    auto sc = get_style_context();
    Gdk::RGBA bg_color("#f0f0f0"); // Fix bg as a light gray

    // TODO: In Gtk+ 4, the state is not used in get_color
    auto state = sc->get_state();
    auto fg_color = sc->get_color(state);

    Gdk::RGBA mid_color;
    mid_color.set_rgba((bg_color.get_red()   + fg_color.get_red()  )/2.0,
                       (bg_color.get_green() + fg_color.get_green())/2.0,
                       (bg_color.get_blue()  + fg_color.get_blue()) /2.0,
                       (bg_color.get_alpha() + fg_color.get_alpha())/2.0);

    SPFilterPrimitive* prim = get_selected();
    int row_count = get_model()->children().size();

    int fheight = CellRendererConnection::size;
    Gdk::Rectangle rct, vis;
    Gtk::TreeIter row = get_model()->children().begin();
    int text_start_x = 0;
    if(row) {
        get_cell_area(get_model()->get_path(row), *get_column(1), rct);
        get_visible_rect(vis);
        text_start_x = rct.get_x() + rct.get_width() - get_input_type_width() * FPInputConverter._length + 1;

	for(unsigned int i = 0; i < FPInputConverter._length; ++i) {
            _vertical_layout->set_text(_(FPInputConverter.get_label((FilterPrimitiveInput)i).c_str()));
            const int x = text_start_x + get_input_type_width() * i;
	    cr->save();
            Gdk::Cairo::set_source_rgba(cr, bg_color);
	    cr->rectangle(x, 0, get_input_type_width(), vis.get_height());
	    cr->fill_preserve();

            Gdk::Cairo::set_source_rgba(cr, fg_color);
	    cr->move_to(x + get_input_type_width(), 5);
	    cr->rotate_degrees(90);
	    _vertical_layout->show_in_cairo_context(cr);

            Gdk::Cairo::set_source_rgba(cr, mid_color);
	    cr->move_to(x, 0);
	    cr->line_to(x, vis.get_height());
	    cr->stroke();
	    cr->restore();
        }
        cr->rectangle(vis.get_x(), 0, vis.get_width(), vis.get_height());
        cr->clip();
    }

    int row_index = 0;
    for(; row != get_model()->children().end(); ++row, ++row_index) {
        get_cell_area(get_model()->get_path(row), *get_column(1), rct);
        const int x = rct.get_x(), y = rct.get_y(), h = rct.get_height();

        // Check mouse state
        int mx, my;
        Gdk::ModifierType mask;

        auto display = get_bin_window()->get_display();
        auto seat = display->get_default_seat();
        auto device = seat->get_pointer();
        cr->set_line_width(0.5);
        get_bin_window()->get_device_position(device, mx, my, mask);

        // Outline the bottom of the connection area
        const int outline_x = x + fheight * (row_count - row_index);
	cr->save();

        Gdk::Cairo::set_source_rgba(cr, mid_color);

	cr->move_to(vis.get_x(), y + h);
	cr->line_to(outline_x, y + h);
        // Side outline
	cr->line_to(outline_x, y - 1);

	cr->stroke();
	cr->restore();

        std::vector<Gdk::Point> con_poly;
        int con_drag_y = 0;
        int con_drag_x = 0;
        bool inside;
        const SPFilterPrimitive* row_prim = (*row)[_columns.primitive];
        const int inputs = input_count(row_prim);

        if(SP_IS_FEMERGE(row_prim)) {
            for(int i = 0; i < inputs; ++i) {
                inside = do_connection_node(row, i, con_poly, mx, my);

		cr->save();

                Gdk::Cairo::set_source_rgba(cr, mid_color);
		draw_connection_node(cr, con_poly, inside);

		cr->restore();

                if(_in_drag == (i + 1)) {
                    con_drag_y = con_poly[2].get_y();
                    con_drag_x = con_poly[2].get_x();
                }

                if(_in_drag != (i + 1) || row_prim != prim) {
                    draw_connection(cr, row, SPAttr::INVALID, text_start_x, outline_x, con_poly[2].get_y(), row_count, i);
                }
            }
        }
        else {
            // Draw "in" shape
            inside = do_connection_node(row, 0, con_poly, mx, my);
            con_drag_y = con_poly[2].get_y();
            con_drag_x = con_poly[2].get_x();

	    cr->save();

            Gdk::Cairo::set_source_rgba(cr, mid_color);
            draw_connection_node(cr, con_poly, inside);

	    cr->restore();

            // Draw "in" connection
            if(_in_drag != 1 || row_prim != prim) {
                draw_connection(cr, row, SPAttr::IN_, text_start_x, outline_x, con_poly[2].get_y(), row_count, -1);
            }

            if(inputs == 2) {
                // Draw "in2" shape
                inside = do_connection_node(row, 1, con_poly, mx, my);
                if(_in_drag == 2) {
                    con_drag_y = con_poly[2].get_y();
                    con_drag_x = con_poly[2].get_x();
                }

		cr->save();

                Gdk::Cairo::set_source_rgba(cr, mid_color);
                draw_connection_node(cr, con_poly, inside);

                cr->restore();

                // Draw "in2" connection
                if(_in_drag != 2 || row_prim != prim) {
                    draw_connection(cr, row, SPAttr::IN2, text_start_x, outline_x, con_poly[2].get_y(), row_count, -1);
                }
            }
        }

        // Draw drag connection
        if(row_prim == prim && _in_drag) {
		cr->save();
                cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->move_to(con_drag_x, con_drag_y);
		cr->line_to(mx, con_drag_y);
		cr->line_to(mx, my);
		cr->stroke();
		cr->restore();
  	}
    }

    return true;
}

void FilterEffectsDialog::PrimitiveList::draw_connection(const Cairo::RefPtr<Cairo::Context>& cr,
                                                         const Gtk::TreeIter& input, const SPAttr attr,
                                                         const int text_start_x, const int x1, const int y1,
                                                         const int row_count, const int pos)
{
    cr->save();

    auto sc = get_style_context();

    Gdk::RGBA bg_color("f0f0f0"); // Fix bg as a light gray

    // TODO: In Gtk+ 4, the state is not used in get_color
    auto state = sc->get_state();
    auto fg_color = sc->get_color(state);

    Gdk::RGBA mid_color;
    mid_color.set_rgba((bg_color.get_red()   + fg_color.get_red()  )/2.0,
                       (bg_color.get_green() + fg_color.get_green())/2.0,
                       (bg_color.get_blue()  + fg_color.get_blue() )/2.0,
                       (bg_color.get_alpha() + fg_color.get_alpha())/2.0);

    int src_id = 0;
    Gtk::TreeIter res = find_result(input, attr, src_id, pos);

    const bool is_first = input == get_model()->children().begin();
    const bool is_merge = SP_IS_FEMERGE((SPFilterPrimitive*)(*input)[_columns.primitive]);
    const bool use_default = !res && !is_merge;

    if(res == input || (use_default && is_first)) {
        // Draw straight connection to a standard input
        // Draw a lighter line for an implicit connection to a standard input
        const int tw = get_input_type_width();
        gint end_x = text_start_x + tw * src_id + (int)(tw * 0.5f) + 1;

	if(use_default && is_first) {
            Gdk::Cairo::set_source_rgba(cr, mid_color);
        } else {
            cr->set_source_rgb(0.0, 0.0, 0.0);
        }

	cr->rectangle(end_x-2, y1-2, 5, 5);
	cr->fill_preserve();
	cr->move_to(x1, y1);
	cr->line_to(end_x, y1);
	cr->stroke();
    }
    else {
        // Draw an 'L'-shaped connection to another filter primitive
        // If no connection is specified, draw a light connection to the previous primitive
	if(use_default) {
		res = input;
		--res;
	}

        if(res) {
            Gdk::Rectangle rct;

            get_cell_area(get_model()->get_path(_model->children().begin()), *get_column(1), rct);
            const int fheight = CellRendererConnection::size;

            get_cell_area(get_model()->get_path(res), *get_column(1), rct);
            const int row_index = find_index(res);
            const int x2 = rct.get_x() + fheight * (row_count - row_index) - fheight / 2;
            const int y2 = rct.get_y() + rct.get_height();

            // Draw a bevelled 'L'-shaped connection
	    cr->set_source_rgb(0.0, 0.0, 0.0);
	    cr->move_to(x1, y1);
	    cr->line_to(x2-fheight/4, y1);
	    cr->line_to(x2, y1-fheight/4);
	    cr->line_to(x2, y2);
	    cr->stroke();
        }
    }
    cr->restore();
}

// Draw the triangular outline of the connection node, and fill it
// if desired
void FilterEffectsDialog::PrimitiveList::draw_connection_node(const Cairo::RefPtr<Cairo::Context>& cr,
                                                              const std::vector<Gdk::Point>& points,
                                                              const bool fill)
{
    cr->save();
    cr->move_to(points[0].get_x()+0.5, points[0].get_y()+0.5);
    cr->line_to(points[1].get_x()+0.5, points[1].get_y()+0.5);
    cr->line_to(points[2].get_x()+0.5, points[2].get_y()+0.5);
    cr->line_to(points[0].get_x()+0.5, points[0].get_y()+0.5);

    if(fill) cr->fill();
    else cr->stroke();

    cr->restore();
}

// Creates a triangle outline of the connection node and returns true if (x,y) is inside the node
bool FilterEffectsDialog::PrimitiveList::do_connection_node(const Gtk::TreeIter& row, const int input,
                                                            std::vector<Gdk::Point>& points,
                                                            const int ix, const int iy)
{
    Gdk::Rectangle rct;
    const int icnt = input_count((*row)[_columns.primitive]);

    get_cell_area(get_model()->get_path(_model->children().begin()), *get_column(1), rct);
    const int fheight = CellRendererConnection::size;

    get_cell_area(_model->get_path(row), *get_column(1), rct);
    const float h = rct.get_height() / icnt;

    const int x = rct.get_x() + fheight * (_model->children().size() - find_index(row));
    const int con_w = (int)(fheight * 0.35f);
    const int con_y = (int)(rct.get_y() + (h / 2) - con_w + (input * h));
    points.clear();
    points.emplace_back(x, con_y);
    points.emplace_back(x, con_y + con_w * 2);
    points.emplace_back(x - con_w, con_y + con_w);

    return ix >= x - h && iy >= con_y && ix <= x && iy <= points[1].get_y();
}

const Gtk::TreeIter FilterEffectsDialog::PrimitiveList::find_result(const Gtk::TreeIter& start,
                                                                    const SPAttr attr, int& src_id,
                                                                    const int pos)
{
    SPFilterPrimitive* prim = (*start)[_columns.primitive];
    Gtk::TreeIter target = _model->children().end();
    int image = 0;

    if(SP_IS_FEMERGE(prim)) {
        int c = 0;
        bool found = false;
        for (auto& o: prim->children) {
            if(c == pos && SP_IS_FEMERGENODE(&o)) {
                image = SP_FEMERGENODE(&o)->input;
                found = true;
            }
            ++c;
        }
        if(!found)
            return target;
    }
    else {
        if(attr == SPAttr::IN_)
            image = prim->image_in;
        else if(attr == SPAttr::IN2) {
            if(SP_IS_FEBLEND(prim))
                image = SP_FEBLEND(prim)->in2;
            else if(SP_IS_FECOMPOSITE(prim))
                image = SP_FECOMPOSITE(prim)->in2;
            else if(SP_IS_FEDISPLACEMENTMAP(prim))
                image = SP_FEDISPLACEMENTMAP(prim)->in2;
            else
                return target;
        }
        else
            return target;
    }

    if(image >= 0) {
        for(Gtk::TreeIter i = _model->children().begin();
            i != start; ++i) {
            if(((SPFilterPrimitive*)(*i)[_columns.primitive])->image_out == image)
                target = i;
        }
        return target;
    }
    else if(image < -1) {
        src_id = -(image + 2);
        return start;
    }

    return target;
}

int FilterEffectsDialog::PrimitiveList::find_index(const Gtk::TreeIter& target)
{
    int i = 0;
    for(Gtk::TreeIter iter = _model->children().begin();
        iter != target; ++iter, ++i){};
    return i;
}

bool FilterEffectsDialog::PrimitiveList::on_button_press_event(GdkEventButton* e)
{
    Gtk::TreePath path;
    Gtk::TreeViewColumn* col;
    const int x = (int)e->x, y = (int)e->y;
    int cx, cy;

    _drag_prim = nullptr;

    if(get_path_at_pos(x, y, path, col, cx, cy)) {
        Gtk::TreeIter iter = _model->get_iter(path);
        std::vector<Gdk::Point> points;

        _drag_prim = (*iter)[_columns.primitive];
        const int icnt = input_count(_drag_prim);

        for(int i = 0; i < icnt; ++i) {
            if(do_connection_node(_model->get_iter(path), i, points, x, y)) {
                _in_drag = i + 1;
                break;
            }
        }

        queue_draw();
    }

    if(_in_drag) {
        _scroll_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &PrimitiveList::on_scroll_timeout), 150);
        _autoscroll_x = 0;
        _autoscroll_y = 0;
        get_selection()->select(path);
        return true;
    }
    else
        return Gtk::TreeView::on_button_press_event(e);
}

bool FilterEffectsDialog::PrimitiveList::on_motion_notify_event(GdkEventMotion* e)
{
    const int speed = 10;
    const int limit = 15;

    Gdk::Rectangle vis;
    get_visible_rect(vis);
    int vis_x, vis_y;

    int vis_x2, vis_y2;
    convert_widget_to_tree_coords(vis.get_x(), vis.get_y(), vis_x2, vis_y2);

    convert_tree_to_widget_coords(vis.get_x(), vis.get_y(), vis_x, vis_y);
    const int top = vis_y + vis.get_height();
    const int right_edge = vis_x + vis.get_width();

    // When autoscrolling during a connection drag, set the speed based on
    // where the mouse is in relation to the edges.
    if(e->y < vis_y)
        _autoscroll_y = -(int)(speed + (vis_y - e->y) / 5);
    else if(e->y < vis_y + limit)
        _autoscroll_y = -speed;
    else if(e->y > top)
        _autoscroll_y = (int)(speed + (e->y - top) / 5);
    else if(e->y > top - limit)
        _autoscroll_y = speed;
    else
        _autoscroll_y = 0;

    double e2 = ( e->x - vis_x2/2);
    // horizontal scrolling
    if(e2 < vis_x)
        _autoscroll_x = -(int)(speed + (vis_x - e2) / 5);
    else if(e2 < vis_x + limit)
        _autoscroll_x = -speed;
    else if(e2 > right_edge)
        _autoscroll_x = (int)(speed + (e2 - right_edge) / 5);
    else if(e2 > right_edge - limit)
        _autoscroll_x = speed;
    else
        _autoscroll_x = 0;



    queue_draw();

    return Gtk::TreeView::on_motion_notify_event(e);
}

bool FilterEffectsDialog::PrimitiveList::on_button_release_event(GdkEventButton* e)
{
    SPFilterPrimitive *prim = get_selected(), *target;

    _scroll_connection.disconnect();

    if(_in_drag && prim) {
        Gtk::TreePath path;
        Gtk::TreeViewColumn* col;
        int cx, cy;

        if(get_path_at_pos((int)e->x, (int)e->y, path, col, cx, cy)) {
            const gchar *in_val = nullptr;
            Glib::ustring result;
            Gtk::TreeIter target_iter = _model->get_iter(path);
            target = (*target_iter)[_columns.primitive];
            col = get_column(1);

            Gdk::Rectangle rct;
            get_cell_area(path, *col, rct);
            const int twidth = get_input_type_width();
            const int sources_x = rct.get_width() - twidth * FPInputConverter._length;
            if(cx > sources_x) {
                int src = (cx - sources_x) / twidth;
                if (src < 0) {
                    src = 0;
                } else if(src >= static_cast<int>(FPInputConverter._length)) {
                    src = FPInputConverter._length - 1;
                }
                result = FPInputConverter.get_key((FilterPrimitiveInput)src);
                in_val = result.c_str();
            }
            else {
                // Ensure that the target comes before the selected primitive
                for(Gtk::TreeIter iter = _model->children().begin();
                    iter != get_selection()->get_selected(); ++iter) {
                    if(iter == target_iter) {
                        Inkscape::XML::Node *repr = target->getRepr();
                        // Make sure the target has a result
                        const gchar *gres = repr->attribute("result");
                        if(!gres) {
                            result = SP_FILTER(prim->parent)->get_new_result_name();
                            repr->setAttributeOrRemoveIfEmpty("result", result);
                            in_val = result.c_str();
                        }
                        else
                            in_val = gres;
                        break;
                    }
                }
            }

            if(SP_IS_FEMERGE(prim)) {
                int c = 1;
                bool handled = false;
                for (auto& o: prim->children) {
                    if(c == _in_drag && SP_IS_FEMERGENODE(&o)) {
                        // If input is null, delete it
                        if(!in_val) {

                            //XML Tree being used directly here while it shouldn't be.
                            sp_repr_unparent(o.getRepr());
                            DocumentUndo::done(prim->document, SP_VERB_DIALOG_FILTER_EFFECTS,
                                               _("Remove merge node"));
                            (*get_selection()->get_selected())[_columns.primitive] = prim;
                        } else {
                            _dialog.set_attr(&o, SPAttr::IN_, in_val);
                        }
                        handled = true;
                        break;
                    }
                    ++c;
                }
                // Add new input?
                if(!handled && c == _in_drag && in_val) {
                    Inkscape::XML::Document *xml_doc = prim->document->getReprDoc();
                    Inkscape::XML::Node *repr = xml_doc->createElement("svg:feMergeNode");
                    repr->setAttribute("inkscape:collect", "always");

                    //XML Tree being used directly here while it shouldn't be.
                    prim->getRepr()->appendChild(repr);
                    SPFeMergeNode *node = SP_FEMERGENODE(prim->document->getObjectByRepr(repr));
                    Inkscape::GC::release(repr);
                    _dialog.set_attr(node, SPAttr::IN_, in_val);
                    (*get_selection()->get_selected())[_columns.primitive] = prim;
                }
            }
            else {
                if(_in_drag == 1)
                    _dialog.set_attr(prim, SPAttr::IN_, in_val);
                else if(_in_drag == 2)
                    _dialog.set_attr(prim, SPAttr::IN2, in_val);
            }
        }

        _in_drag = 0;
        queue_draw();

        _dialog.update_settings_view();
    }

    if((e->type == GDK_BUTTON_RELEASE) && (e->button == 3)) {
        const bool sensitive = get_selected() != nullptr;
	auto items = _primitive_menu->get_children();
        items[0]->set_sensitive(sensitive);
        items[1]->set_sensitive(sensitive);

        _primitive_menu->popup_at_pointer(reinterpret_cast<GdkEvent *>(e));

        return true;
    }
    else
        return Gtk::TreeView::on_button_release_event(e);
}

// Checks all of prim's inputs, removes any that use result
static void check_single_connection(SPFilterPrimitive* prim, const int result)
{
    if (prim && (result >= 0)) {
        if (prim->image_in == result) {
            prim->removeAttribute("in");
        }

        if (SP_IS_FEBLEND(prim)) {
            if (SP_FEBLEND(prim)->in2 == result) {
                prim->removeAttribute("in2");
            }
        } else if (SP_IS_FECOMPOSITE(prim)) {
            if (SP_FECOMPOSITE(prim)->in2 == result) {
                prim->removeAttribute("in2");
            }
        } else if (SP_IS_FEDISPLACEMENTMAP(prim)) {
            if (SP_FEDISPLACEMENTMAP(prim)->in2 == result) {
                prim->removeAttribute("in2");
            }
        }
    }
}

// Remove any connections going to/from prim_iter that forward-reference other primitives
void FilterEffectsDialog::PrimitiveList::sanitize_connections(const Gtk::TreeIter& prim_iter)
{
    SPFilterPrimitive *prim = (*prim_iter)[_columns.primitive];
    bool before = true;

    for(Gtk::TreeIter iter = _model->children().begin();
        iter != _model->children().end(); ++iter) {
        if(iter == prim_iter)
            before = false;
        else {
            SPFilterPrimitive* cur_prim = (*iter)[_columns.primitive];
            if(before)
                check_single_connection(cur_prim, prim->image_out);
            else
                check_single_connection(prim, cur_prim->image_out);
        }
    }
}

// Reorder the filter primitives to match the list order
void FilterEffectsDialog::PrimitiveList::on_drag_end(const Glib::RefPtr<Gdk::DragContext>& /*dc*/)
{
    SPFilter* filter = _dialog._filter_modifier.get_selected_filter();
    int ndx = 0;

    for (Gtk::TreeModel::iterator iter = _model->children().begin();
        iter != _model->children().end(); ++iter, ++ndx) {
        SPFilterPrimitive* prim = (*iter)[_columns.primitive];
        if (prim && prim == _drag_prim) {
            prim->getRepr()->setPosition(ndx);
            break;
        }
    }

    for (Gtk::TreeModel::iterator iter = _model->children().begin();
        iter != _model->children().end(); ++iter, ++ndx) {
        SPFilterPrimitive* prim = (*iter)[_columns.primitive];
        if (prim && prim == _drag_prim) {
            sanitize_connections(iter);
            get_selection()->select(iter);
            break;
        }
    }

    filter->requestModified(SP_OBJECT_MODIFIED_FLAG);

    DocumentUndo::done(filter->document, SP_VERB_DIALOG_FILTER_EFFECTS, _("Reorder filter primitive"));
}

// If a connection is dragged towards the top or bottom of the list, the list should scroll to follow.
bool FilterEffectsDialog::PrimitiveList::on_scroll_timeout()
{
    if(_autoscroll_y) {
        auto a = dynamic_cast<Gtk::ScrolledWindow*>(get_parent())->get_vadjustment();
        double v = a->get_value() + _autoscroll_y;

	if(v < 0)
            v = 0;
        if(v > a->get_upper() - a->get_page_size())
            v = a->get_upper() - a->get_page_size();

        a->set_value(v);

        queue_draw();
    }


    if(_autoscroll_x) {
        auto a_h = dynamic_cast<Gtk::ScrolledWindow*>(get_parent())->get_hadjustment();
        double h = a_h->get_value() + _autoscroll_x;

	if(h < 0)
            h = 0;
        if(h > a_h->get_upper() - a_h->get_page_size())
            h = a_h->get_upper() - a_h->get_page_size();

        a_h->set_value(h);

        queue_draw();
    }

    return true;
}

int FilterEffectsDialog::PrimitiveList::primitive_count() const
{
    return _model->children().size();
}

int FilterEffectsDialog::PrimitiveList::get_input_type_width() const
{
       // Maximum font height calculated in initText() and stored in _input_type_width.
       // Add 2 to font height to account for rectangle around text.
    return _input_type_width + 2;
}

/*** FilterEffectsDialog ***/

FilterEffectsDialog::FilterEffectsDialog()
    : DialogBase("/dialogs/filtereffects", SP_VERB_DIALOG_FILTER_EFFECTS)
    , _add_primitive_type(FPConverter)
    , _add_primitive(_("Add Effect:"))
    , _empty_settings(_("No effect selected"), Gtk::ALIGN_START)
    , _no_filter_selected(_("No filter selected"), Gtk::ALIGN_START)
    , _settings_initialized(false)
    , _locked(false)
    , _attr_lock(false)
    , _filter_modifier(*this)
    , _primitive_list(*this)
    , _settings_tab1(Gtk::ORIENTATION_VERTICAL)
    , _settings_tab2(Gtk::ORIENTATION_VERTICAL)
{
    _settings = new Settings(*this, _settings_tab1, sigc::mem_fun(*this, &FilterEffectsDialog::set_attr_direct),
                             NR_FILTER_ENDPRIMITIVETYPE);
    _filter_general_settings = new Settings(*this, _settings_tab2, sigc::mem_fun(*this, &FilterEffectsDialog::set_filternode_attr),
                             1);

    // Initialize widget hierarchy
    auto hpaned = Gtk::manage(new Gtk::Paned());
    _primitive_box = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_VERTICAL));

    _sw_infobox = Gtk::manage(new Gtk::ScrolledWindow);
    Gtk::ScrolledWindow* sw_prims = Gtk::manage(new Gtk::ScrolledWindow);
    Gtk::Box* infobox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, /*spacing:*/4));
    Gtk::Box* hb_prims = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    Gtk::Box* vb_prims = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    Gtk::Box* vb_desc = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    Gtk::Box* prim_vbox_p = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    Gtk::Box* prim_vbox_i = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    sw_prims->add(_primitive_list);

    prim_vbox_p->pack_start(*sw_prims, true, true);
    prim_vbox_i->pack_start(*vb_prims, true, true);

    _primitive_box->pack1(*prim_vbox_p);
    _primitive_box->pack2(*prim_vbox_i, false, false);

    hpaned->pack1(_filter_modifier);
    hpaned->pack2(*_primitive_box);
    pack_start(*hpaned, true, true);

    _infobox_icon.set_halign(Gtk::ALIGN_START);
    _infobox_icon.set_valign(Gtk::ALIGN_START);
    _infobox_desc.set_halign(Gtk::ALIGN_START);
    _infobox_desc.set_valign(Gtk::ALIGN_START);
    _infobox_desc.set_justify(Gtk::JUSTIFY_LEFT);
    _infobox_desc.set_line_wrap(true);

    vb_desc->pack_start(_infobox_desc, true, true);

    infobox->pack_start(_infobox_icon, false, false);
    infobox->pack_start(*vb_desc, true, true);

    _sw_infobox->add(*infobox);

    vb_prims->pack_start(*hb_prims, false, false);
    vb_prims->pack_start(*_sw_infobox, true, true);

    hb_prims->pack_start(_add_primitive, false, false);
    hb_prims->pack_start(_add_primitive_type, true, true);
    pack_start(_settings_tabs, false, false);
    _settings_tabs.append_page(_settings_tab1, _("Effect parameters"));
    _settings_tabs.append_page(_settings_tab2, _("Filter General Settings"));

    _primitive_list.signal_primitive_changed().connect(
        sigc::mem_fun(*this, &FilterEffectsDialog::update_settings_view));
    _filter_modifier.signal_filter_changed().connect(
        sigc::mem_fun(_primitive_list, &PrimitiveList::update));

    _add_primitive_type.signal_changed().connect(
        sigc::mem_fun(*this, &FilterEffectsDialog::update_primitive_infobox));

    sw_prims->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    sw_prims->set_shadow_type(Gtk::SHADOW_IN);
    _sw_infobox->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

//    al_settings->set_padding(0, 0, 12, 0);
//    fr_settings->set_shadow_type(Gtk::SHADOW_NONE);
//    ((Gtk::Label*)fr_settings->get_label_widget())->set_use_markup();
    _add_primitive.signal_clicked().connect(sigc::mem_fun(*this, &FilterEffectsDialog::add_primitive));
    _primitive_list.set_menu(*this, sigc::mem_fun(*this, &FilterEffectsDialog::duplicate_primitive),
                                    sigc::mem_fun(_primitive_list, &PrimitiveList::remove_selected));

    init_settings_widgets();
    _primitive_list.update();
    update_primitive_infobox();

    show();
    show_all_children();
    update();
}

FilterEffectsDialog::~FilterEffectsDialog()
{
    delete _settings;
    delete _filter_general_settings;
}

void FilterEffectsDialog::update()
{
    if (!_app) {
        std::cerr << "FilterEffectsDialog::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();

    _filter_modifier.setTargetDesktop(desktop);
}

void FilterEffectsDialog::set_attrs_locked(const bool l)
{
    _locked = l;
}

void FilterEffectsDialog::show_all_vfunc()
{
    update_settings_view();
}

void FilterEffectsDialog::init_settings_widgets()
{
    // TODO: Find better range/climb-rate/digits values for the SpinScales,
    //       most of the current values are complete guesses!

    _settings_tab1.set_border_width(4);
    _settings_tab2.set_border_width(4);

    _empty_settings.set_sensitive(false);
    _settings_tab1.pack_start(_empty_settings);

    _no_filter_selected.set_sensitive(false);
    _settings_tab2.pack_start(_no_filter_selected);
    _settings_initialized = true;

    _filter_general_settings->type(0);
    _filter_general_settings->add_checkbutton(true, SPAttr::AUTO_REGION, _("Automatic Region"), "true", "false", _("If unset the coordinates and dimensions won't be automatically updated."));
    _filter_general_settings->add_multispinbutton(/*default x:*/ (double) -0.1, /*default y:*/ (double) -0.1, SPAttr::X, SPAttr::Y, _("Coordinates:"), -100, 100, 0.01, 0.1, 2, _("X coordinate of the left corners of filter effects region"), _("Y coordinate of the upper corners of filter effects region"));
    _filter_general_settings->add_multispinbutton(/*default width:*/ (double) 1.2, /*default height:*/ (double) 1.2, SPAttr::WIDTH, SPAttr::HEIGHT, _("Dimensions:"), 0, 1000, 0.01, 0.1, 2, _("Width of filter effects region"), _("Height of filter effects region"));

    _settings->type(NR_FILTER_BLEND);
    _settings->add_combo(SP_CSS_BLEND_NORMAL, SPAttr::MODE, _("Mode:"), SPBlendModeConverter);

    _settings->type(NR_FILTER_COLORMATRIX);
    ComboBoxEnum<FilterColorMatrixType>* colmat = _settings->add_combo(COLORMATRIX_MATRIX, SPAttr::TYPE, _("Type:"), ColorMatrixTypeConverter, _("Indicates the type of matrix operation. The keyword 'matrix' indicates that a full 5x4 matrix of values will be provided. The other keywords represent convenience shortcuts to allow commonly used color operations to be performed without specifying a complete matrix."));
    _color_matrix_values = _settings->add_colormatrixvalues(_("Value(s):"));
    colmat->signal_attr_changed().connect(sigc::mem_fun(*this, &FilterEffectsDialog::update_color_matrix));

    _settings->type(NR_FILTER_COMPONENTTRANSFER);
    _settings->add_componenttransfervalues(_("R:"), SPFeFuncNode::R);
    _settings->add_componenttransfervalues(_("G:"), SPFeFuncNode::G);
    _settings->add_componenttransfervalues(_("B:"), SPFeFuncNode::B);
    _settings->add_componenttransfervalues(_("A:"), SPFeFuncNode::A);

    _settings->type(NR_FILTER_COMPOSITE);
    _settings->add_combo(COMPOSITE_OVER, SPAttr::OPERATOR, _("Operator:"), CompositeOperatorConverter);
    _k1 = _settings->add_spinscale(0, SPAttr::K1, _("K1:"), -10, 10, 0.1, 0.01, 2, _("If the arithmetic operation is chosen, each result pixel is computed using the formula k1*i1*i2 + k2*i1 + k3*i2 + k4 where i1 and i2 are the pixel values of the first and second inputs respectively."));
    _k2 = _settings->add_spinscale(0, SPAttr::K2, _("K2:"), -10, 10, 0.1, 0.01, 2, _("If the arithmetic operation is chosen, each result pixel is computed using the formula k1*i1*i2 + k2*i1 + k3*i2 + k4 where i1 and i2 are the pixel values of the first and second inputs respectively."));
    _k3 = _settings->add_spinscale(0, SPAttr::K3, _("K3:"), -10, 10, 0.1, 0.01, 2, _("If the arithmetic operation is chosen, each result pixel is computed using the formula k1*i1*i2 + k2*i1 + k3*i2 + k4 where i1 and i2 are the pixel values of the first and second inputs respectively."));
    _k4 = _settings->add_spinscale(0, SPAttr::K4, _("K4:"), -10, 10, 0.1, 0.01, 2, _("If the arithmetic operation is chosen, each result pixel is computed using the formula k1*i1*i2 + k2*i1 + k3*i2 + k4 where i1 and i2 are the pixel values of the first and second inputs respectively."));

    _settings->type(NR_FILTER_CONVOLVEMATRIX);
    _convolve_order = _settings->add_dualspinbutton((char*)"3", SPAttr::ORDER, _("Size:"), 1, 5, 1, 1, 0, _("width of the convolve matrix"), _("height of the convolve matrix"));
    _convolve_target = _settings->add_multispinbutton(/*default x:*/ (double) 0, /*default y:*/ (double) 0, SPAttr::TARGETX, SPAttr::TARGETY, _("Target:"), 0, 4, 1, 1, 0, _("X coordinate of the target point in the convolve matrix. The convolution is applied to pixels around this point."), _("Y coordinate of the target point in the convolve matrix. The convolution is applied to pixels around this point."));
    //TRANSLATORS: for info on "Kernel", see http://en.wikipedia.org/wiki/Kernel_(matrix)
    _convolve_matrix = _settings->add_matrix(SPAttr::KERNELMATRIX, _("Kernel:"), _("This matrix describes the convolve operation that is applied to the input image in order to calculate the pixel colors at the output. Different arrangements of values in this matrix result in various possible visual effects. An identity matrix would lead to a motion blur effect (parallel to the matrix diagonal) while a matrix filled with a constant non-zero value would lead to a common blur effect."));
    _convolve_order->signal_attr_changed().connect(sigc::mem_fun(*this, &FilterEffectsDialog::convolve_order_changed));
    _settings->add_spinscale(0, SPAttr::DIVISOR, _("Divisor:"), 0, 1000, 1, 0.1, 2, _("After applying the kernelMatrix to the input image to yield a number, that number is divided by divisor to yield the final destination color value. A divisor that is the sum of all the matrix values tends to have an evening effect on the overall color intensity of the result."));
    _settings->add_spinscale(0, SPAttr::BIAS, _("Bias:"), -10, 10, 1, 0.01, 1, _("This value is added to each component. This is useful to define a constant value as the zero response of the filter."));
    _settings->add_combo(CONVOLVEMATRIX_EDGEMODE_DUPLICATE, SPAttr::EDGEMODE, _("Edge Mode:"), ConvolveMatrixEdgeModeConverter, _("Determines how to extend the input image as necessary with color values so that the matrix operations can be applied when the kernel is positioned at or near the edge of the input image."));
    _settings->add_checkbutton(false, SPAttr::PRESERVEALPHA, _("Preserve Alpha"), "true", "false", _("If set, the alpha channel won't be altered by this filter primitive."));

    _settings->type(NR_FILTER_DIFFUSELIGHTING);
    _settings->add_color(/*default: white*/ 0xffffffff, SPAttr::LIGHTING_COLOR, _("Diffuse Color:"), _("Defines the color of the light source"));
    _settings->add_spinscale(1, SPAttr::SURFACESCALE, _("Surface Scale:"), -5, 5, 0.01, 0.001, 3, _("This value amplifies the heights of the bump map defined by the input alpha channel"));
    _settings->add_spinscale(1, SPAttr::DIFFUSECONSTANT, _("Constant:"), 0, 5, 0.1, 0.01, 2, _("This constant affects the Phong lighting model."));
    _settings->add_dualspinscale(SPAttr::KERNELUNITLENGTH, _("Kernel Unit Length:"), 0.01, 10, 1, 0.01, 1);
    _settings->add_lightsource();

    _settings->type(NR_FILTER_DISPLACEMENTMAP);
    _settings->add_spinscale(0, SPAttr::SCALE, _("Scale:"), 0, 100, 1, 0.01, 1, _("This defines the intensity of the displacement effect."));
    _settings->add_combo(DISPLACEMENTMAP_CHANNEL_ALPHA, SPAttr::XCHANNELSELECTOR, _("X displacement:"), DisplacementMapChannelConverter, _("Color component that controls the displacement in the X direction"));
    _settings->add_combo(DISPLACEMENTMAP_CHANNEL_ALPHA, SPAttr::YCHANNELSELECTOR, _("Y displacement:"), DisplacementMapChannelConverter, _("Color component that controls the displacement in the Y direction"));

    _settings->type(NR_FILTER_FLOOD);
    _settings->add_color(/*default: black*/ 0, SPAttr::FLOOD_COLOR, _("Flood Color:"), _("The whole filter region will be filled with this color."));
    _settings->add_spinscale(1, SPAttr::FLOOD_OPACITY, _("Opacity:"), 0, 1, 0.1, 0.01, 2);

    _settings->type(NR_FILTER_GAUSSIANBLUR);
    _settings->add_dualspinscale(SPAttr::STDDEVIATION, _("Standard Deviation:"), 0.01, 100, 1, 0.01, 2, _("The standard deviation for the blur operation."));

    _settings->type(NR_FILTER_MERGE);
    _settings->add_no_params();

    _settings->type(NR_FILTER_MORPHOLOGY);
    _settings->add_combo(MORPHOLOGY_OPERATOR_ERODE, SPAttr::OPERATOR, _("Operator:"), MorphologyOperatorConverter, _("Erode: performs \"thinning\" of input image.\nDilate: performs \"fattening\" of input image."));
    _settings->add_dualspinscale(SPAttr::RADIUS, _("Radius:"), 0, 100, 1, 0.01, 1);

    _settings->type(NR_FILTER_IMAGE);
    _settings->add_fileorelement(SPAttr::XLINK_HREF, _("Source of Image:"));
    _image_x = _settings->add_entry(SPAttr::X,_("X"),_("X"));
    _image_x->signal_attr_changed().connect(sigc::mem_fun(*this, &FilterEffectsDialog::image_x_changed));
    //This commented because we want the default empty value of X or Y and couldent get it from SpinButton
    //_image_y = _settings->add_spinbutton(0, SPAttr::Y, _("Y:"), -DBL_MAX, DBL_MAX, 1, 1, 5, _("Y"));
    _image_y = _settings->add_entry(SPAttr::Y,_("Y"),_("Y"));
    _image_y->signal_attr_changed().connect(sigc::mem_fun(*this, &FilterEffectsDialog::image_y_changed));
    _settings->type(NR_FILTER_OFFSET);
    _settings->add_checkbutton(false, SPAttr::PRESERVEALPHA, _("Preserve Alpha"), "true", "false", _("If set, the alpha channel won't be altered by this filter primitive."));
    _settings->add_spinscale(0, SPAttr::DX, _("Delta X:"), -100, 100, 1, 0.01, 2, _("This is how far the input image gets shifted to the right"));
    _settings->add_spinscale(0, SPAttr::DY, _("Delta Y:"), -100, 100, 1, 0.01, 2, _("This is how far the input image gets shifted downwards"));

    _settings->type(NR_FILTER_SPECULARLIGHTING);
    _settings->add_color(/*default: white*/ 0xffffffff, SPAttr::LIGHTING_COLOR, _("Specular Color:"), _("Defines the color of the light source"));
    _settings->add_spinscale(1, SPAttr::SURFACESCALE, _("Surface Scale:"), -5, 5, 0.1, 0.01, 2, _("This value amplifies the heights of the bump map defined by the input alpha channel"));
    _settings->add_spinscale(1, SPAttr::SPECULARCONSTANT, _("Constant:"), 0, 5, 0.1, 0.01, 2, _("This constant affects the Phong lighting model."));
    _settings->add_spinscale(1, SPAttr::SPECULAREXPONENT, _("Exponent:"), 1, 50, 1, 0.01, 1, _("Exponent for specular term, larger is more \"shiny\"."));
    _settings->add_dualspinscale(SPAttr::KERNELUNITLENGTH, _("Kernel Unit Length:"), 0.01, 10, 1, 0.01, 1);
    _settings->add_lightsource();

    _settings->type(NR_FILTER_TILE);
    _settings->add_no_params();

    _settings->type(NR_FILTER_TURBULENCE);
//    _settings->add_checkbutton(false, SPAttr::STITCHTILES, _("Stitch Tiles"), "stitch", "noStitch");
    _settings->add_combo(TURBULENCE_TURBULENCE, SPAttr::TYPE, _("Type:"), TurbulenceTypeConverter, _("Indicates whether the filter primitive should perform a noise or turbulence function."));
    _settings->add_dualspinscale(SPAttr::BASEFREQUENCY, _("Base Frequency:"), 0, 1, 0.001, 0.01, 3);
    _settings->add_spinscale(1, SPAttr::NUMOCTAVES, _("Octaves:"), 1, 10, 1, 1, 0);
    _settings->add_spinscale(0, SPAttr::SEED, _("Seed:"), 0, 1000, 1, 1, 0, _("The starting number for the pseudo random number generator."));
}

void FilterEffectsDialog::add_primitive()
{
    SPFilter* filter = _filter_modifier.get_selected_filter();

    if(filter) {
        SPFilterPrimitive* prim = filter_add_primitive(filter, _add_primitive_type.get_active_data()->id);

        _primitive_list.select(prim);

        DocumentUndo::done(filter->document, SP_VERB_DIALOG_FILTER_EFFECTS, _("Add filter primitive"));
    }
}

void FilterEffectsDialog::update_primitive_infobox()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/showfiltersinfobox/value", true)){
        _sw_infobox->show();
    } else {
        _sw_infobox->hide();
    }
    switch(_add_primitive_type.get_active_data()->id){
        case(NR_FILTER_BLEND):
            _infobox_icon.set_from_icon_name("feBlend-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feBlend</b> filter primitive provides different image blending modes, such as screen, multiply, darken and lighten."));
            break;
        case(NR_FILTER_COLORMATRIX):
            _infobox_icon.set_from_icon_name("feColorMatrix-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feColorMatrix</b> filter primitive applies a matrix transformation to color of each rendered pixel. This allows for effects like turning object to grayscale, modifying color saturation and changing color hue."));
            break;
        case(NR_FILTER_COMPONENTTRANSFER):
            _infobox_icon.set_from_icon_name("feComponentTransfer-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feComponentTransfer</b> filter primitive manipulates the input's color components (red, green, blue, and alpha) according to particular transfer functions, allowing operations like brightness and contrast adjustment, color balance, and thresholding."));
            break;
        case(NR_FILTER_COMPOSITE):
            _infobox_icon.set_from_icon_name("feComposite-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feComposite</b> filter primitive composites two images using one of the Porter-Duff blending modes or the arithmetic mode described in SVG standard. Porter-Duff blending modes are essentially logical operations between the corresponding pixel values of the images."));
            break;
        case(NR_FILTER_CONVOLVEMATRIX):
            _infobox_icon.set_from_icon_name("feConvolveMatrix-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feConvolveMatrix</b> lets you specify a Convolution to be applied on the image. Common effects created using convolution matrices are blur, sharpening, embossing and edge detection. Note that while gaussian blur can be created using this filter primitive, the special gaussian blur primitive is faster and resolution-independent."));
            break;
        case(NR_FILTER_DIFFUSELIGHTING):
            _infobox_icon.set_from_icon_name("feDiffuseLighting-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feDiffuseLighting</b> and feSpecularLighting filter primitives create \"embossed\" shadings.  The input's alpha channel is used to provide depth information: higher opacity areas are raised toward the viewer and lower opacity areas recede away from the viewer."));
            break;
        case(NR_FILTER_DISPLACEMENTMAP):
            _infobox_icon.set_from_icon_name("feDisplacementMap-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feDisplacementMap</b> filter primitive displaces the pixels in the first input using the second input as a displacement map, that shows from how far the pixel should come from. Classical examples are whirl and pinch effects."));
            break;
        case(NR_FILTER_FLOOD):
            _infobox_icon.set_from_icon_name("feFlood-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feFlood</b> filter primitive fills the region with a given color and opacity.  It is usually used as an input to other filters to apply color to a graphic."));
            break;
        case(NR_FILTER_GAUSSIANBLUR):
            _infobox_icon.set_from_icon_name("feGaussianBlur-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feGaussianBlur</b> filter primitive uniformly blurs its input.  It is commonly used together with feOffset to create a drop shadow effect."));
            break;
        case(NR_FILTER_IMAGE):
            _infobox_icon.set_from_icon_name("feImage-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feImage</b> filter primitive fills the region with an external image or another part of the document."));
            break;
        case(NR_FILTER_MERGE):
            _infobox_icon.set_from_icon_name("feMerge-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feMerge</b> filter primitive composites several temporary images inside the filter primitive to a single image. It uses normal alpha compositing for this. This is equivalent to using several feBlend primitives in 'normal' mode or several feComposite primitives in 'over' mode."));
            break;
        case(NR_FILTER_MORPHOLOGY):
            _infobox_icon.set_from_icon_name("feMorphology-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feMorphology</b> filter primitive provides erode and dilate effects. For single-color objects erode makes the object thinner and dilate makes it thicker."));
            break;
        case(NR_FILTER_OFFSET):
            _infobox_icon.set_from_icon_name("feOffset-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feOffset</b> filter primitive offsets the image by an user-defined amount. For example, this is useful for drop shadows, where the shadow is in a slightly different position than the actual object."));
            break;
        case(NR_FILTER_SPECULARLIGHTING):
            _infobox_icon.set_from_icon_name("feSpecularLighting-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feDiffuseLighting</b> and <b>feSpecularLighting</b> filter primitives create \"embossed\" shadings.  The input's alpha channel is used to provide depth information: higher opacity areas are raised toward the viewer and lower opacity areas recede away from the viewer."));
            break;
        case(NR_FILTER_TILE):
            _infobox_icon.set_from_icon_name("feTile-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feTile</b> filter primitive tiles a region with an input graphic. The source tile is defined by the filter primitive subregion of the input."));
            break;
        case(NR_FILTER_TURBULENCE):
            _infobox_icon.set_from_icon_name("feTurbulence-icon", Gtk::ICON_SIZE_DIALOG);
            _infobox_desc.set_markup(_("The <b>feTurbulence</b> filter primitive renders Perlin noise. This kind of noise is useful in simulating several nature phenomena like clouds, fire and smoke and in generating complex textures like marble or granite."));
            break;
        default:
            g_assert(false);
            break;
    }
    //_infobox_icon.set_pixel_size(96);
    _infobox_icon.set_pixel_size(64);
}

void FilterEffectsDialog::duplicate_primitive()
{
    SPFilter* filter = _filter_modifier.get_selected_filter();
    SPFilterPrimitive* origprim = _primitive_list.get_selected();

    if (filter && origprim) {
        Inkscape::XML::Node *repr;
        repr = origprim->getRepr()->duplicate(origprim->getRepr()->document());
        filter->getRepr()->appendChild(repr);

        DocumentUndo::done(filter->document, SP_VERB_DIALOG_FILTER_EFFECTS, _("Duplicate filter primitive"));

        _primitive_list.update();
    }
}

void FilterEffectsDialog::convolve_order_changed()
{
    _convolve_matrix->set_from_attribute(_primitive_list.get_selected());
    _convolve_target->get_spinbuttons()[0]->get_adjustment()->set_upper(_convolve_order->get_spinbutton1().get_value() - 1);
    _convolve_target->get_spinbuttons()[1]->get_adjustment()->set_upper(_convolve_order->get_spinbutton2().get_value() - 1);
}

bool number_or_empy(const Glib::ustring& text) {
    if (text.empty()) {
        return true;
    }
    double n = atof( text.c_str() );
    if (n == 0.0 && strcmp(text.c_str(), "0") != 0 && strcmp(text.c_str(), "0.0") != 0) {
        return false;
    }
    else {
        return true;
    }
}

void FilterEffectsDialog::image_x_changed()
{
    if (number_or_empy(_image_x->get_text())) {
        _image_x->set_from_attribute(_primitive_list.get_selected());
    }
}

void FilterEffectsDialog::image_y_changed()
{
    if (number_or_empy(_image_y->get_text())) {
        _image_y->set_from_attribute(_primitive_list.get_selected());
    }
}

void FilterEffectsDialog::set_attr_direct(const AttrWidget* input)
{
    set_attr(_primitive_list.get_selected(), input->get_attribute(), input->get_as_attribute().c_str());
}

void FilterEffectsDialog::set_filternode_attr(const AttrWidget* input)
{
    if(!_locked) {
        _attr_lock = true;
        SPFilter *filter = _filter_modifier.get_selected_filter();
        const gchar* name = (const gchar*)sp_attribute_name(input->get_attribute());
        if (filter && name && filter->getRepr()){
            filter->setAttributeOrRemoveIfEmpty(name, input->get_as_attribute());
            filter->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        _attr_lock = false;
    }
}

void FilterEffectsDialog::set_child_attr_direct(const AttrWidget* input)
{
    set_attr(_primitive_list.get_selected()->firstChild(), input->get_attribute(), input->get_as_attribute().c_str());
}

void FilterEffectsDialog::set_attr(SPObject* o, const SPAttr attr, const gchar* val)
{
    if(!_locked) {
        _attr_lock = true;

        SPFilter *filter = _filter_modifier.get_selected_filter();
        const gchar* name = (const gchar*)sp_attribute_name(attr);
        if(filter && name && o) {
            update_settings_sensitivity();

            o->setAttribute(name, val);
            filter->requestModified(SP_OBJECT_MODIFIED_FLAG);

            Glib::ustring undokey = "filtereffects:";
            undokey += name;
            DocumentUndo::maybeDone(filter->document, undokey.c_str(), SP_VERB_DIALOG_FILTER_EFFECTS,
                                    _("Set filter primitive attribute"));
        }

        _attr_lock = false;
    }
}

void FilterEffectsDialog::update_filter_general_settings_view()
{
    if(_settings_initialized != true) return;

    if(!_locked) {
        _attr_lock = true;

        SPFilter* filter = _filter_modifier.get_selected_filter();

        if(filter) {
            _filter_general_settings->show_and_update(0, filter);
            _no_filter_selected.hide();
        }
        else {
            std::vector<Gtk::Widget*> vect = _settings_tab2.get_children();
            vect[0]->hide();
            _no_filter_selected.show();
        }

        _attr_lock = false;
    }
}

void FilterEffectsDialog::update_settings_view()
{
    update_settings_sensitivity();

    if(_attr_lock)
        return;

//First Tab

    std::vector<Gtk::Widget*> vect1 = _settings_tab1.get_children();
    for(auto & i : vect1)
	    i->hide();
    _empty_settings.show();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/showfiltersinfobox/value", true)){
        _sw_infobox->show();
    } else {
        _sw_infobox->hide();
    }

    SPFilterPrimitive* prim = _primitive_list.get_selected();

    if(prim && prim->getRepr()) {

        //XML Tree being used directly here while it shouldn't be.
        _settings->show_and_update(FPConverter.get_id_from_key(prim->getRepr()->name()), prim);
        _empty_settings.hide();
    }

//Second Tab

    std::vector<Gtk::Widget*> vect2 = _settings_tab2.get_children();
    vect2[0]->hide();
    _no_filter_selected.show();

    SPFilter* filter = _filter_modifier.get_selected_filter();

    if(filter) {
        _filter_general_settings->show_and_update(0, filter);
        _no_filter_selected.hide();
    }

}

void FilterEffectsDialog::update_settings_sensitivity()
{
    SPFilterPrimitive* prim = _primitive_list.get_selected();
    const bool use_k = SP_IS_FECOMPOSITE(prim) && SP_FECOMPOSITE(prim)->composite_operator == COMPOSITE_ARITHMETIC;
    _k1->set_sensitive(use_k);
    _k2->set_sensitive(use_k);
    _k3->set_sensitive(use_k);
    _k4->set_sensitive(use_k);

}

void FilterEffectsDialog::update_color_matrix()
{
    _color_matrix_values->set_from_attribute(_primitive_list.get_selected());
}

} // namespace Dialog
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
