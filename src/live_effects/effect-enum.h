// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_ENUM_H
#define INKSCAPE_LIVEPATHEFFECT_ENUM_H

/*
 * Inkscape::LivePathEffect::EffectType
 *
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "util/enums.h"

namespace Inkscape {
namespace LivePathEffect {

//Please fill in the same order than in effect.cpp:98
enum EffectType {
    BEND_PATH = 0,
    GEARS,
    PATTERN_ALONG_PATH,
    CURVE_STITCH,
    VONKOCH,
    KNOT,
    CONSTRUCT_GRID,
    SPIRO,
    ENVELOPE,
    INTERPOLATE,
    ROUGH_HATCHES,
    SKETCH,
    RULER,
    POWERSTROKE,
    CLONE_ORIGINAL,
    SIMPLIFY,
    LATTICE2,
    PERSPECTIVE_ENVELOPE,
    INTERPOLATE_POINTS,
    TRANSFORM_2PTS,
    SHOW_HANDLES,
    ROUGHEN,
    BSPLINE,
    JOIN_TYPE,
    TAPER_STROKE,
    MIRROR_SYMMETRY,
    COPY_ROTATE,
    ATTACH_PATH,
    FILL_BETWEEN_STROKES,
    FILL_BETWEEN_MANY,
    ELLIPSE_5PTS,
    BOUNDING_BOX,
    MEASURE_SEGMENTS,
    FILLET_CHAMFER,
    BOOL_OP,
    POWERCLIP,
    POWERMASK,
    PTS2ELLIPSE,
    OFFSET,
    DASHED_STROKE,
    ANGLE_BISECTOR,
    CIRCLE_WITH_RADIUS,
    CIRCLE_3PTS,
    EXTRUDE,
    LINE_SEGMENT,
    PARALLEL,
    PERP_BISECTOR,
    TANGENT_TO_CURVE,
    DOEFFECTSTACK_TEST,
    DYNASTROKE,
    LATTICE,
    PATH_LENGTH,
    RECURSIVE_SKELETON,
    TEXT_LABEL,
    EMBRODERY_STITCH,
    INVALID_LPE // This must be last (I made it such that it is not needed anymore I think..., Don't trust on it being
                // last. - johan)
};

template <typename E>
struct EnumEffectData {
    E id;
    const Glib::ustring label;
    const Glib::ustring key;
    const Glib::ustring icon;
    const Glib::ustring untranslated_label;
    const Glib::ustring description;
    const bool on_path;
    const bool on_shape;
    const bool on_group;
    const bool on_image;
    const bool on_text;
    const bool experimental;
};

const Glib::ustring empty_string("");

/**
 * Simplified management of enumerations of LPE items with UI labels.
 *
 * @note that get_id_from_key and get_id_from_label return 0 if it cannot find an entry for that key string.
 * @note that get_label and get_key return an empty string when the requested id is not in the list.
 */
template <typename E>
class EnumEffectDataConverter {
  public:
    typedef EnumEffectData<E> Data;

    EnumEffectDataConverter(const EnumEffectData<E> *cd, const unsigned int length)
        : _length(length)
        , _data(cd)
    {
    }

    E get_id_from_label(const Glib::ustring &label) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].label == label)
                return _data[i].id;
        }

        return (E)0;
    }

    E get_id_from_key(const Glib::ustring &key) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].key == key)
                return _data[i].id;
        }

        return (E)0;
    }

    bool is_valid_key(const Glib::ustring &key) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].key == key)
                return true;
        }

        return false;
    }

    bool is_valid_id(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return true;
        }
        return false;
    }

    const Glib::ustring &get_label(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].label;
        }

        return empty_string;
    }

    const Glib::ustring &get_key(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].key;
        }

        return empty_string;
    }

    const Glib::ustring &get_icon(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].icon;
        }

        return empty_string;
    }

    const Glib::ustring &get_untranslated_label(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].untranslated_label;
        }

        return empty_string;
    }

    const Glib::ustring &get_description(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].description;
        }

        return empty_string;
    }

    const bool get_on_path(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].on_path;
        }

        return false;
    }

    const bool get_on_shape(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].on_shape;
        }

        return false;
    }

    const bool get_on_group(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].on_group;
        }

        return false;
    }

    const bool get_on_image(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].on_image;
        }

        return false;
    }

    const bool get_on_text(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].on_text;
        }

        return false;
    }

    const bool get_experimental(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].experimental;
        }

        return false;
    }

    const EnumEffectData<E> &data(const unsigned int i) const { return _data[i]; }

    const unsigned int _length;

  private:
    const EnumEffectData<E> *_data;
};

extern const EnumEffectData<EffectType> LPETypeData[];             /// defined in effect.cpp
extern const EnumEffectDataConverter<EffectType> LPETypeConverter; /// defined in effect.cpp

} //namespace LivePathEffect
} //namespace Inkscape

#endif

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
