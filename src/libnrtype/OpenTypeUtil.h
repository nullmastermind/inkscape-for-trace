
#ifndef SEEN_OPENTYPEUTIL_H
#define SEEN_OPENTYPEUTIL_H

#ifndef USE_PANGO_WIN32

#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glibmm/ustring.h>

/*
 * A set of utilities to extract data from OpenType fonts.
 *
 * Isolates dependencies on FreeType, Harfbuzz, and Pango.
 * All three provide variable amounts of access to data.
 */

// An OpenType fvar axis.
class OTVarAxis {
public:
    OTVarAxis()
        : minimum(0)
        , maximum(1000)
        , set_val(500)
        , index(-1) {};

    OTVarAxis(double _minimum, double _maximum, double _set_val, int _index)
        : minimum(_minimum)
        , maximum(_maximum)
        , set_val(_set_val)
        , index  (_index) {};

    double minimum;
    double maximum;
    double set_val;
    int    index;  // Index in OpenType file (since we use a map).
};

// A particular instance of a variable font.
// A map indexed by axis name with value.
class OTVarInstance {
  std::map<Glib::ustring, double> axes;
};

inline double FTFixedToDouble (FT_Fixed value) {
    return static_cast<FT_Int32>(value) / 65536.0;
}

inline FT_Fixed FTDoubleToFixed (double value) {
    return static_cast<FT_Fixed>(value * 65536);
}

void readOpenTypeGsubTable (const FT_Face ft_face,
                            std::map<Glib::ustring, int>& tables,
                            std::map<Glib::ustring, Glib::ustring>& stylistic,
                            std::map<Glib::ustring, Glib::ustring>& ligatures);

void readOpenTypeFvarAxes  (const FT_Face ft_face,
                            std::map<Glib::ustring, OTVarAxis>& axes);

void readOpenTypeFvarNamed (const FT_Face ft_face,
                            std::map<Glib::ustring, OTVarInstance>& named);

#endif /* !USE_PANGO_WIND32    */
#endif /* !SEEN_OPENTYPEUTIL_H */

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
