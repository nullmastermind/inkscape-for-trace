
// Command line export... should be using normal export.


class InkCommandLineExport {

public:
    enum {
        EXPORT_PLAIN_SVG,
        EXPORT_INKSCAPE_SVG,
        EXPORT_PNG,
        EXPORT_PS,
        EXPORT_EPS,
        EXPORT_PDF,
        EXPORT_LATEX,
        EXPORT_EMF,
        EXPORT_WMF,
        EXPORT_XAML,
        EXPORT_PRINT
    } ExportType;
  
    InkCommandLineExport(ExportType export_type,
                         Glib::ustring file_name);
    ~InkCommandLineExport() {};
    do_export();

    double        export_dpi;
    bool          export_area;
    bool          export_area_drawing;
    bool          export_area_page;
    double        export_margin;
    bool          export_snap;
    int           export_width;         // In pixels
    int           export_hight;         // In pixels
    Glib::ustring export_id;
    bool          export_id_only;
    bool          export_id_hints;
    Glib::ustring export_background;
    double        export_background_opacity;
    int           export_ps_level;
    double        export_pdf_level;
    bool          export_text_to_path;
    bool          export_ignore_filters;

private:
    ExportType export_type;
    Glib::ustring file_name;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
