// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Bitmap tracing settings dialog - second implementation.
 */
/* Authors:
 *   Marc Jeanmougin <marc.jeanmougin@telecom-paristech.fr>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "tracedialog.h"

#include <gtkmm.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/notebook.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/stack.h>


#include <glibmm/i18n.h>

#include "desktop-tracker.h"
#include "desktop.h"
#include "selection.h"

#include "inkscape.h"
#include "io/resource.h"
#include "io/sys.h"
#include "trace/autotrace/inkscape-autotrace.h"
#include "trace/potrace/inkscape-potrace.h"
#include "trace/depixelize/inkscape-depixelize.h"



namespace Inkscape {
namespace UI {
namespace Dialog {

class TraceDialogImpl2 : public TraceDialog {
  public:
    TraceDialogImpl2();
    ~TraceDialogImpl2() override;

  private:
    Inkscape::Trace::Tracer tracer;
    void traceProcess(bool do_i_trace);
    void abort();

    void previewCallback();
    bool previewResize(const Cairo::RefPtr<Cairo::Context>&);
    void traceCallback();
    void onSelectionModified(guint flags);
    void onSetDefaults();

    void setDesktop(SPDesktop *desktop) override;
    void setTargetDesktop(SPDesktop *desktop);

    SPDesktop *desktop;
    DesktopTracker deskTrack;
    sigc::connection desktopChangeConn;
    sigc::connection selectChangedConn;
    sigc::connection selectModifiedConn;

    Glib::RefPtr<Gtk::Builder> builder;

    Glib::RefPtr<Gtk::Adjustment> MS_scans, PA_curves, PA_islands, PA_sparse1, PA_sparse2, SS_AT_ET_T, SS_AT_FI_T, SS_BC_T, SS_CQ_T,
        SS_ED_T, optimize, smooth, speckles;
    Gtk::ComboBoxText *CBT_SS, *CBT_MS;
    Gtk::CheckButton *CB_invert, *CB_MS_smooth, *CB_MS_stack, *CB_MS_rb, *CB_speckles, *CB_smooth, *CB_optimize,
        /* *CB_live,*/ *CB_SIOX;
    Gtk::RadioButton *RB_PA_voronoi;
    Gtk::Button *B_RESET, *B_STOP, *B_OK, *B_Update;
    Gtk::Box *mainBox;
    Gtk::Stack *choice_scan;
    Gtk::Notebook *choice_tab;
    Glib::RefPtr<Gdk::Pixbuf> scaledPreview;
    Gtk::DrawingArea *previewArea;
};

void TraceDialogImpl2::setDesktop(SPDesktop *desktop)
{
    Panel::setDesktop(desktop);
    deskTrack.setBase(desktop);
}

void TraceDialogImpl2::setTargetDesktop(SPDesktop *desktop)
{
    if (this->desktop != desktop) {
        if (this->desktop) {
            selectChangedConn.disconnect();
            selectModifiedConn.disconnect();
        }
        this->desktop = desktop;
        if (desktop && desktop->selection) {
            selectModifiedConn = desktop->selection->connectModified(
                sigc::hide<0>(sigc::mem_fun(*this, &TraceDialogImpl2::onSelectionModified)));
        }
    }
}

void TraceDialogImpl2::traceProcess(bool do_i_trace)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop)
        desktop->setWaitingCursor();

    if (CB_SIOX->get_active())
        tracer.enableSiox(true);
    else
        tracer.enableSiox(false);

    Glib::ustring type =
        choice_scan->get_visible_child_name() == "SingleScan" ? CBT_SS->get_active_text() : CBT_MS->get_active_text();

    Inkscape::Trace::Potrace::TraceType potraceType;
    // Inkscape::Trace::Autotrace::TraceType autotraceType;

    bool use_autotrace = false;
    Inkscape::Trace::Autotrace::AutotraceTracingEngine ate; // TODO

    if (type == _("Brightness cutoff"))
      potraceType = Inkscape::Trace::Potrace::TRACE_BRIGHTNESS;
    else if (type == _("Edge detection"))
      potraceType = Inkscape::Trace::Potrace::TRACE_CANNY;
    else if (type == _("Color quantization"))
      potraceType = Inkscape::Trace::Potrace::TRACE_QUANT;
    else if (type == _("Autotrace"))
    {
      // autotraceType = Inkscape::Trace::Autotrace::TRACE_CENTERLINE
      use_autotrace = true;
      ate.opts->color_count = 2;
    }
    else if (type == _("Centerline tracing (autotrace)"))
    {
      // autotraceType = Inkscape::Trace::Autotrace::TRACE_CENTERLINE
      use_autotrace = true;
      ate.opts->color_count = 2;
      ate.opts->centerline = true;
      ate.opts->preserve_width = true;
    }
    else if (type == _("Brightness steps"))
      potraceType = Inkscape::Trace::Potrace::TRACE_BRIGHTNESS_MULTI;
    else if (type == _("Colors"))
      potraceType = Inkscape::Trace::Potrace::TRACE_QUANT_COLOR;
    else if (type == _("Grays"))
      potraceType = Inkscape::Trace::Potrace::TRACE_QUANT_MONO;
    else if (type == _("Autotrace (slower)"))
    {
      // autotraceType = Inkscape::Trace::Autotrace::TRACE_CENTERLINE
      use_autotrace = true;
      ate.opts->color_count = (int)MS_scans->get_value() + 1;
    }
    else 
    {
      g_warning("Should not happen!");
    }
    ate.opts->filter_iterations = (int) SS_AT_FI_T->get_value();
    ate.opts->error_threshold = SS_AT_ET_T->get_value();

    Inkscape::Trace::Potrace::PotraceTracingEngine pte(
        potraceType, CB_invert->get_active(), (int)SS_CQ_T->get_value(), SS_BC_T->get_value(),
        0., // Brightness floor
        SS_ED_T->get_value(), (int)MS_scans->get_value(), CB_MS_stack->get_active(), CB_MS_smooth->get_active(),
        CB_MS_rb->get_active());
    pte.potraceParams->opticurve = CB_optimize->get_active();
    pte.potraceParams->opttolerance = optimize->get_value();
    pte.potraceParams->alphamax = CB_smooth->get_active() ? smooth->get_value() : 0;
    pte.potraceParams->turdsize = CB_speckles->get_active() ? (int)speckles->get_value() : 0;



    //Inkscape::Trace::Autotrace::AutotraceTracingEngine ate; // TODO
    Inkscape::Trace::Depixelize::DepixelizeTracingEngine dte(RB_PA_voronoi->get_active() ? Inkscape::Trace::Depixelize::TraceType::TRACE_VORONOI : Inkscape::Trace::Depixelize::TraceType::TRACE_BSPLINES, PA_curves->get_value(), (int) PA_islands->get_value(), (int) PA_sparse1->get_value(), PA_sparse2->get_value() );


    Glib::RefPtr<Gdk::Pixbuf> pixbuf = tracer.getSelectedImage();
    if (pixbuf) {
        Glib::RefPtr<Gdk::Pixbuf> preview = use_autotrace ? ate.preview(pixbuf) : pte.preview(pixbuf);
        if (preview) {
            int width = preview->get_width();
            int height = preview->get_height();
            const Gtk::Allocation &vboxAlloc = previewArea->get_allocation();
            double scaleFX = vboxAlloc.get_width() / (double)width;
            double scaleFY = vboxAlloc.get_height() / (double)height;
            double scaleFactor = scaleFX > scaleFY ? scaleFY : scaleFX;
            int newWidth = (int)(((double)width) * scaleFactor);
            int newHeight = (int)(((double)height) * scaleFactor);
            scaledPreview = preview->scale_simple(newWidth, newHeight, Gdk::INTERP_NEAREST);
            previewArea->queue_draw();
        }
    }
    if (do_i_trace){
        if (choice_tab->get_current_page() == 1){
            tracer.trace(&dte);
            printf("dt\n");
        } else if (use_autotrace) {
	          tracer.trace(&ate);
            printf("at\n");
        } else if (choice_tab->get_current_page() == 0) {
            tracer.trace(&pte);
            printf("pt\n");
        }
    }

    if (desktop)
        desktop->clearWaitingCursor();
}

bool TraceDialogImpl2::previewResize(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (!scaledPreview) return false; // return early
    int width = scaledPreview->get_width();
    int height = scaledPreview->get_height();
    const Gtk::Allocation &vboxAlloc = previewArea->get_allocation();
    double scaleFX = vboxAlloc.get_width() / (double)width;
    double scaleFY = vboxAlloc.get_height() / (double)height;
    double scaleFactor = scaleFX > scaleFY ? scaleFY : scaleFX;
    int newWidth = (int)(((double)width) * scaleFactor);
    int newHeight = (int)(((double)height) * scaleFactor);
    int offsetX = (vboxAlloc.get_width() - newWidth)/2;
    int offsetY = (vboxAlloc.get_height() - newHeight)/2;

    Glib::RefPtr<Gdk::Pixbuf> temp = scaledPreview->scale_simple(newWidth, newHeight, Gdk::INTERP_NEAREST);
    Gdk::Cairo::set_source_pixbuf(cr, temp, offsetX, offsetY);
    cr->paint();
    return false;
}

void TraceDialogImpl2::abort()
{ 
     SPDesktop *desktop = SP_ACTIVE_DESKTOP;
     if (desktop)
         desktop->clearWaitingCursor();
     tracer.abort();
}

void TraceDialogImpl2::onSelectionModified(guint flags)
{
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG)) {
        previewCallback();
    }
}

void TraceDialogImpl2::onSetDefaults()
{
    MS_scans->set_value(8);
    PA_curves->set_value(1);
    PA_islands->set_value(5);
    PA_sparse1->set_value(4);
    PA_sparse2->set_value(1);
    SS_AT_FI_T->set_value(4);
    SS_AT_ET_T->set_value(2);
    SS_BC_T->set_value(0.45);
    SS_CQ_T->set_value(64);
    SS_ED_T->set_value(.65);
    optimize->set_value(0.2);
    smooth->set_value(1);
    speckles->set_value(2);
    CB_invert->set_active(false);
    CB_MS_smooth->set_active(true);
    CB_MS_stack->set_active(true);
    CB_MS_rb->set_active(false);
    CB_speckles->set_active(true);
    CB_smooth->set_active(true);
    CB_optimize->set_active(true);
    //CB_live->set_active(false);
    CB_SIOX->set_active(false);
}

void TraceDialogImpl2::previewCallback() { traceProcess(false); }
void TraceDialogImpl2::traceCallback() { traceProcess(true); }


TraceDialogImpl2::TraceDialogImpl2()
    : TraceDialog()
{
    const std::string req_widgets[] = { "MS_scans",    "PA_curves", "PA_islands",  "PA_sparse1", "PA_sparse2",
                                        "SS_AT_FI_T", "SS_AT_ET_T",     "SS_BC_T",   "SS_CQ_T",     "SS_ED_T",
                                        "optimize",    "smooth",    "speckles",    "CB_invert",  "CB_MS_smooth",
                                        "CB_MS_stack", "CB_MS_rb",  "CB_speckles", "CB_smooth",  "CB_optimize",
                                        /*"CB_live",*/ "CB_SIOX",   "CBT_SS",      "CBT_MS",     "B_RESET",
                                        "B_STOP",      "B_OK",      "mainBox",     "choice_tab", "choice_scan",
                                        "previewArea" };
    Glib::ustring gladefile = get_filename(Inkscape::IO::Resource::UIS, "dialog-trace.glade");
    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("Glade file loading failed for filter effect dialog");
        return;
    }

    Glib::RefPtr<Glib::Object> test;
    for (std::string w : req_widgets) {
        test = builder->get_object(w);
        if (!test) {
            g_warning("Required widget %s does not exist", w.c_str());
            return;
        }
    }

#define GET_O(name)                                                                                                    \
    tmp = builder->get_object(#name);                                                                                  \
    name = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(tmp);

    Glib::RefPtr<Glib::Object> tmp;

#define GET_W(name) builder->get_widget(#name, name);
    GET_O(MS_scans)
    GET_O(PA_curves)
    GET_O(PA_islands)
    GET_O(PA_sparse1)
    GET_O(PA_sparse2)
    GET_O(SS_AT_FI_T)
    GET_O(SS_AT_ET_T)
    GET_O(SS_BC_T)
    GET_O(SS_CQ_T)
    GET_O(SS_ED_T)
    GET_O(optimize)
    GET_O(smooth)
    GET_O(speckles)

    GET_W(CB_invert)
    GET_W(CB_MS_smooth)
    GET_W(CB_MS_stack)
    GET_W(CB_MS_rb)
    GET_W(CB_speckles)
    GET_W(CB_smooth)
    GET_W(CB_optimize)
    //GET_W(CB_live)
    GET_W(CB_SIOX)
    GET_W(RB_PA_voronoi)
    GET_W(CBT_SS)
    GET_W(CBT_MS)
    GET_W(B_RESET)
    GET_W(B_STOP)
    GET_W(B_OK)
    GET_W(B_Update)
    GET_W(mainBox)
    GET_W(choice_tab)
    GET_W(choice_scan)
    GET_W(previewArea)
#undef GET_W
#undef GET_O
    _getContents()->add(*mainBox);
    // show_all_children();
    desktopChangeConn = deskTrack.connectDesktopChanged(sigc::mem_fun(*this, &TraceDialogImpl2::setTargetDesktop));
    deskTrack.connect(GTK_WIDGET(gobj()));

    B_Update->signal_clicked().connect(sigc::mem_fun(*this, &TraceDialogImpl2::previewCallback));
    B_OK->signal_clicked().connect(sigc::mem_fun(*this, &TraceDialogImpl2::traceCallback));
    B_STOP->signal_clicked().connect(sigc::mem_fun(*this, &TraceDialogImpl2::abort));
    B_RESET->signal_clicked().connect(sigc::mem_fun(*this, &TraceDialogImpl2::onSetDefaults));
    previewArea->signal_draw().connect(sigc::mem_fun(*this, &TraceDialogImpl2::previewResize));
}


TraceDialogImpl2::~TraceDialogImpl2()
{
    selectChangedConn.disconnect();
    selectModifiedConn.disconnect();
    desktopChangeConn.disconnect();
}



TraceDialog &TraceDialog::getInstance()
{
    TraceDialog *dialog = new TraceDialogImpl2();
    return *dialog;
}



} // namespace Dialog
} // namespace UI
} // namespace Inkscape
