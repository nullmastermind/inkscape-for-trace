
#ifndef SEEN_OPENTYPEUTIL_H
#define SEEN_OPENTYPEUTIL_H

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


void readOpenTypeGsubTable (const FT_Face ft_face,
                            std::map<Glib::ustring, int>& tables,
                            std::map<Glib::ustring, Glib::ustring>& substitutions);


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
