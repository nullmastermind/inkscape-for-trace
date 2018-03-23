
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

// An OpenType fvar axis
class OTVarAxis {
 public:
  OTVarAxis()
    : minimum(0)
    , maximum(1000)
    , set_val(500) {};

  OTVarAxis(double _minimum, double _maximum, double _set_val)
    : minimum(_minimum)
    , maximum(_maximum)
    , set_val(_set_val) {};

  double minimum;
  double maximum;
  double set_val;
};

class OTVarNamed {
  std::map<Glib::ustring, double> axes;
};


void readOpenTypeGsubTable (const FT_Face ft_face,
                            std::map<Glib::ustring, int>& tables,
                            std::map<Glib::ustring, Glib::ustring>& substitutions);

void readOpenTypeFvarTable (const FT_Face ft_face,
                            std::map<Glib::ustring, OTVarAxis>& axes,
                            std::map<Glib::ustring, OTVarNamed>& named);

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
