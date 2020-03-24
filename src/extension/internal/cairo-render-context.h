// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef EXTENSION_INTERNAL_CAIRO_RENDER_CONTEXT_H_SEEN
#define EXTENSION_INTERNAL_CAIRO_RENDER_CONTEXT_H_SEEN

/** \file
 * Declaration of CairoRenderContext, a class used for rendering with Cairo.
 */
/*
 * Authors:
 *     Miklos Erdelyi <erdelyim@gmail.com>
 *
 * Copyright (C) 2006 Miklos Erdelyi
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extension/extension.h"
#include <set>
#include <string>

#include <2geom/forward.h>
#include <2geom/affine.h>

#include "style-internal.h" // SPIEnum

#include <cairo.h>

class SPClipPath;
class SPMask;

typedef struct _PangoFont PangoFont;
typedef struct _PangoLayout PangoLayout;

namespace Inkscape {
class Pixbuf;

namespace Extension {
namespace Internal {

class CairoRenderer;
class CairoRenderContext;
struct CairoRenderState;
struct CairoGlyphInfo;

// Holds info for rendering a glyph
struct CairoGlyphInfo {
    unsigned long index;
    double x;
    double y;
};

struct CairoRenderState {
    unsigned int merge_opacity : 1;     // whether fill/stroke opacity can be mul'd with item opacity
    unsigned int need_layer : 1;        // whether object is masked, clipped, and/or has a non-zero opacity
    unsigned int has_overflow : 1;
    unsigned int parent_has_userspace : 1;  // whether the parent's ctm should be applied
    float opacity;
    bool has_filtereffect;
    Geom::Affine item_transform;     // this item's item->transform, for correct clipping

    SPClipPath *clip_path;
    SPMask* mask;

    Geom::Affine transform;     // the CTM
};

// Metadata to set on the cairo surface (if the surface supports it)
struct CairoRenderContextMetadata {
    Glib::ustring title = "";
    Glib::ustring author = "";
    Glib::ustring subject = "";
    Glib::ustring keywords = "";
    Glib::ustring copyright = "";
    Glib::ustring creator = "";
    Glib::ustring cdate = ""; // currently unused
    Glib::ustring mdate = ""; // currently unused
};

class CairoRenderContext {
    friend class CairoRenderer;
public:
    CairoRenderContext *cloneMe() const;
    CairoRenderContext *cloneMe(double width, double height) const;
    bool finish(bool finish_surface = true);

    CairoRenderer *getRenderer() const;
    cairo_t *getCairoContext() const;

    enum CairoRenderMode {
        RENDER_MODE_NORMAL,
        RENDER_MODE_CLIP
    };

    enum CairoClipMode {
        CLIP_MODE_PATH,
        CLIP_MODE_MASK
    };

    bool setImageTarget(cairo_format_t format);
    bool setPdfTarget(gchar const *utf8_fn);
    bool setPsTarget(gchar const *utf8_fn);
    /** Set the cairo_surface_t from an external source */
    bool setSurfaceTarget(cairo_surface_t *surface, bool is_vector, cairo_matrix_t *ctm=nullptr);

    void setPSLevel(unsigned int level);
    void setEPS(bool eps);
    unsigned int getPSLevel();
    void setPDFLevel(unsigned int level);
    void setTextToPath(bool texttopath);
    bool getTextToPath();
    void setOmitText(bool omittext);
    bool getOmitText();
    void setFilterToBitmap(bool filtertobitmap);
    bool getFilterToBitmap();
    void setBitmapResolution(int resolution);
    int getBitmapResolution();

    /** Creates the cairo_surface_t for the context with the
    given width, height and with the currently set target
    surface type. Also sets supported metadata on the surface. */
    bool setupSurface(double width, double height);

    cairo_surface_t *getSurface();

    /** Saves the contents of the context to a PNG file. */
    bool saveAsPng(const char *file_name);

    /** On targets supporting multiple pages, sends subsequent rendering to a new page*/
    void newPage();

    /* Render/clip mode setting/query */
    void setRenderMode(CairoRenderMode mode);
    CairoRenderMode getRenderMode() const;
    void setClipMode(CairoClipMode mode);
    CairoClipMode getClipMode() const;

    void addPathVector(Geom::PathVector const &pv);
    void setPathVector(Geom::PathVector const &pv);

    void pushLayer();
    void popLayer(cairo_operator_t composite = CAIRO_OPERATOR_CLEAR);

    void tagBegin(const char* link);
    void tagEnd();

    /* Graphics state manipulation */
    void pushState();
    void popState();
    CairoRenderState *getCurrentState() const;
    CairoRenderState *getParentState() const;
    void setStateForStyle(SPStyle const *style);

    void transform(Geom::Affine const &transform);
    void setTransform(Geom::Affine const &transform);
    Geom::Affine getTransform() const;
    Geom::Affine getParentTransform() const;

    /* Clipping methods */
    void addClipPath(Geom::PathVector const &pv, SPIEnum<SPWindRule> const *fill_rule);
    void addClippingRect(double x, double y, double width, double height);

    /* Rendering methods */
    enum CairoPaintOrder {
        STROKE_OVER_FILL,
        FILL_OVER_STROKE,
        FILL_ONLY,
        STROKE_ONLY
    };

    bool renderPathVector(Geom::PathVector const &pathv, SPStyle const *style, Geom::OptRect const &pbox, CairoPaintOrder order = STROKE_OVER_FILL);
    bool renderImage(Inkscape::Pixbuf *pb,
                     Geom::Affine const &image_transform, SPStyle const *style);
    bool renderGlyphtext(PangoFont *font, Geom::Affine const &font_matrix,
                         std::vector<CairoGlyphInfo> const &glyphtext, SPStyle const *style);

    /* More general rendering methods will have to be added (like fill, stroke) */

protected:
    CairoRenderContext(CairoRenderer *renderer);
    virtual ~CairoRenderContext();

    enum CairoOmitTextPageState {
        EMPTY,
        GRAPHIC_ON_TOP,
        NEW_PAGE_ON_GRAPHIC
    };

    float _width;
    float _height;
    unsigned short _dpi;
    unsigned int _pdf_level;
    unsigned int _ps_level;
    bool _eps;
    bool _is_texttopath;
    bool _is_omittext;
    bool _is_filtertobitmap;
    int _bitmapresolution;

    FILE *_stream;

    unsigned int _is_valid : 1;
    unsigned int _vector_based_target : 1;

    cairo_t *_cr; // Cairo context
    cairo_surface_t *_surface;
    cairo_surface_type_t _target;
    cairo_format_t _target_format;
    PangoLayout *_layout;

    unsigned int _clip_rule : 8;
    unsigned int _clip_winding_failed : 1;

    std::vector<CairoRenderState *> _state_stack;
    CairoRenderState *_state;    // the current state

    CairoRenderer *_renderer;

    CairoRenderMode _render_mode;
    CairoClipMode _clip_mode;

    CairoOmitTextPageState _omittext_state;

    CairoRenderContextMetadata _metadata;

    cairo_pattern_t *_createPatternForPaintServer(SPPaintServer const *const paintserver,
                                                  Geom::OptRect const &pbox, float alpha);
    cairo_pattern_t *_createPatternPainter(SPPaintServer const *const paintserver, Geom::OptRect const &pbox);
    cairo_pattern_t *_createHatchPainter(SPPaintServer const *const paintserver, Geom::OptRect const &pbox);

    unsigned int _showGlyphs(cairo_t *cr, PangoFont *font, std::vector<CairoGlyphInfo> const &glyphtext, bool is_stroke);

    bool _finishSurfaceSetup(cairo_surface_t *surface, cairo_matrix_t *ctm = nullptr);
    void _setSurfaceMetadata(cairo_surface_t *surface);

    void _setFillStyle(SPStyle const *style, Geom::OptRect const &pbox);
    void _setStrokeStyle(SPStyle const *style, Geom::OptRect const &pbox);

    void _initCairoMatrix(cairo_matrix_t *matrix, Geom::Affine const &transform);
    void _concatTransform(cairo_t *cr, double xx, double yx, double xy, double yy, double x0, double y0);
    void _concatTransform(cairo_t *cr, Geom::Affine const &transform);

    void _prepareRenderGraphic();
    void _prepareRenderText();

    std::map<gpointer, cairo_font_face_t *> font_table;
    static void font_data_free(gpointer data);

    CairoRenderState *_createState();
};

}  /* namespace Internal */
}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* !EXTENSION_INTERNAL_CAIRO_RENDER_CONTEXT_H_SEEN */

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
