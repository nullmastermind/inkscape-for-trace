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
#include <glibmm/i18n.h>



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
    EMBRODERY_STITCH,
    POWERCLIP,
    POWERMASK,
    PTS2ELLIPSE,
    OFFSET,
    DASH_STROKE,
    DOEFFECTSTACK_TEST,
    ANGLE_BISECTOR,
    CIRCLE_WITH_RADIUS,
    CIRCLE_3PTS,
    DYNASTROKE,
    EXTRUDE,
    LATTICE,
    LINE_SEGMENT,
    PARALLEL,
    PATH_LENGTH,
    PERP_BISECTOR,
    RECURSIVE_SKELETON,
    TANGENT_TO_CURVE,
    TEXT_LABEL,
    INVALID_LPE // This must be last (I made it such that it is not needed anymore I think..., Don't trust on it being last. - johan)
};

<<<<<<< HEAD
extern const Util::EnumData<EffectType> LPETypeData[];  /// defined in effect.cpp
extern const Util::EnumDataConverter<EffectType> LPETypeConverter; /// defined in effect.cpp
=======
template <typename E>
struct EnumEffectData {
    E id;
    const Glib::ustring label;
    const Glib::ustring key;
    const Glib::ustring icon;
    const Glib::ustring description;
    const bool on_path;
    const bool on_shape;
    const bool on_group;
    const bool on_use;
    const bool on_image;
    const bool on_text;
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

    const Glib::ustring &get_description(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].description;
        }

        return empty_string;
    }

    const bool &get_on_path(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].path;
        }

        return false;
    }

    const bool &get_on_shape(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].shape;
        }

        return false;
    }

    const bool &get_on_group(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].group;
        }

        return false;
    }

    const bool &get_on_text(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].shape;
        }

        return false;
    }

    const bool &get_on_use(const E id) const
    {
        for (unsigned int i = 0; i < _length; ++i) {
            if (_data[i].id == id)
                return _data[i].use;
        }

        return false;
    }

    const EnumEffectData<E> &data(const unsigned int i) const { return _data[i]; }

    const unsigned int _length;

  private:
    const EnumEffectData<E> *_data;
};

const EnumEffectData<EffectType> LPETypeData[] = {
    // {constant defined in effect-enum.h, N_("name of your effect"), "name of your effect in SVG"}
/* 0.46 */
    {
        BEND_PATH
        , N_("Bend") //label
        , "bend_path" //key
        , "bend-path" //icon
        , N_("Curve a item based on skeleton path") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        GEARS
        , N_("Gears") //label
        , "gears" //key
        , "gears" //icon
        , N_("Create configurable gears") //description
        , true  //on_path
        , true  //on_shape
        , false  //on_group
        , false  //on_use
        , false //on_image
        , false //on_text
    },
    {
        PATTERN_ALONG_PATH
        , N_("Pattern Along Path") //label
        , "skeletal" //key
        , "skeletal" //icon
        , N_("transform a tiem along path with or without repeating") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    }, // for historic reasons, this effect is called skeletal(strokes) in Inkscape:SVG
    {
        CURVE_STITCH
        , N_("Stitch Sub-Paths") //label
        , "curvestitching" //key
        , "curvestitching" //icon
        , N_("curvestitching") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
/* 0.47 */
    {
        VONKOCH
        , N_("VonKoch") //label
        , "vonkoch" //key
        , "vonkoch" //icon
        , N_("vonkoch") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        KNOT
        , N_("Knot") //label
        , "knot" //key
        , "knot" //icon
        , N_("knot") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        CONSTRUCT_GRID
        , N_("Construct grid") //label
        , "construct_grid" //key
        , "construct-grid" //icon
        , N_("construct_grid") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        SPIRO
        , N_("Spiro spline") //label
        , "spiro" //key
        , "spiro" //icon
        , N_("spiro") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        ENVELOPE
        , N_("Envelope Deformation") //label
        , "envelope" //key
        , "envelope" //icon
        , N_("Envelope Deformation") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        INTERPOLATE
        , N_("Interpolate Sub-Paths") //label
        , "interpolate" //key
        , "interpolate" //icon
        , N_("Interpolate Sub-Paths") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        ROUGH_HATCHES
        , N_("Hatches (rough)") //label
        , "rough_hatches" //key
        , "rough-hatches" //icon
        , N_("Hatches (rough)") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        SKETCH
        , N_("Sketch") //label
        , "sketch" //key
        , "sketch" //icon
        , N_("Sketch") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        RULER
        , N_("Ruler") //label
        , "ruler" //key
        , "ruler" //icon
        , N_("Ruler") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
/* 0.91 */
    {
        POWERSTROKE
        , N_("Power stroke") //label
        , "powerstroke" //key
        , "powerstroke" //icon
        , N_("Power stroke") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        CLONE_ORIGINAL
        , N_("Clone original") //label
        , "clone_original" //key
        , "clone-original" //icon
        , N_("Clone original") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
/* 0.92 */
    {
        SIMPLIFY
        , N_("Simplify") //label
        , "simplify" //key
        , "simplify" //icon
        , N_("Simplify") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        LATTICE2
        , N_("Lattice Deformation 2") //label
        , "lattice2" //key
        , "lattice2" //icon
        , N_("Lattice Deformation 2") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        PERSPECTIVE_ENVELOPE
        , N_("Perspective/Envelope") //label
        , "perspective-envelope" //key wrong key with "-" retain because historic
        , "perspective-envelope" //icon
        , N_("Perspective/Envelope") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        INTERPOLATE_POINTS
        , N_("Interpolate points") //label
        , "interpolate_points" //key
        , "interpolate-points" //icon
        , N_("Interpolate points") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        TRANSFORM_2PTS
        , N_("Transform by 2 points") //label
        , "transform_2pts" //key
        , "transform-2pts" //icon
        , N_("Transform by 2 points") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        SHOW_HANDLES
        , N_("Show handles") //label
        , "show_handles" //key
        , "show-handles" //icon
        , N_("Show handles") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        ROUGHEN
        , N_("Roughen") //label
        , "roughen" //key
        , "roughen" //icon
        , N_("Roughen") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        BSPLINE
        , N_("BSpline") //label
        , "bspline" //key
        , "bspline" //icon
        , N_("BSpline") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        JOIN_TYPE
        , N_("Join type") //label
        , "join_type" //key
        , "join-type" //icon
        , N_("Join type") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        TAPER_STROKE
        , N_("Taper stroke") //label
        , "taper_stroke" //key
        , "taper-stroke" //icon
        , N_("Taper stroke") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        MIRROR_SYMMETRY
        , N_("Mirror symmetry") //label
        , "mirror_symmetry" //key
        , "mirror-symmetry" //icon
        , N_("Mirror symmetry") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        COPY_ROTATE
        , N_("Rotate copies") //label
        , "copy_rotate" //key
        , "copy-rotate" //icon
        , N_("Rotate copies") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
/* Ponyscape -> Inkscape 0.92*/
    {
        ATTACH_PATH
        , N_("Attach path") //label
        , "attach_path" //key
        , "attach-path" //icon
        , N_("Attach path") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        FILL_BETWEEN_STROKES
        , N_("Fill between strokes") //label
        , "fill_between_strokes" //key
        , "fill-between-strokes" //icon
        , N_("Fill between strokes") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        FILL_BETWEEN_MANY
        , N_("Fill between many") //label
        , "fill_between_many" //key
        , "fill-between-many" //icon
        , N_("Fill between many") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        ELLIPSE_5PTS
        , N_("Ellipse by 5 points") //label
        , "ellipse_5pts" //key
        , "ellipse-5pts" //icon
        , N_("Ellipse by 5 points") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        BOUNDING_BOX
        , N_("Bounding Box") //label
        , "bounding_box" //key
        , "bounding-box" //icon
        , N_("Bounding Box") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
/* 1.0 */
    {
        MEASURE_SEGMENTS
        , N_("Measure Segments") //label
        , "measure_segments" //key
        , "measure-segments" //icon
        , N_("Measure Segments") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        FILLET_CHAMFER
        , N_("Fillet/Chamfer") //label
        , "fillet_chamfer" //key
        , "fillet-chamfer" //icon
        , N_("Fillet/Chamfer") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        BOOL_OP
        , N_("Boolean operation") //label
        , "bool_op" //key
        , "bool-op" //icon
        , N_("Boolean operation") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        EMBRODERY_STITCH
        , N_("Embroidery stitch") //label
        , "embrodery_stitch" //key
        , "embrodery-stitch" //icon
        , N_("Embroidery stitch") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        POWERCLIP
        , N_("Power clip") //label
        , "powerclip" //key
        , "powerclip" //icon
        , N_("Power clip") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        POWERMASK
        , N_("Power mask") //label
        , "powermask" //key
        , "powermask" //icon
        , N_("Power mask") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        PTS2ELLIPSE
        , N_("Ellipse from points") //label
        , "pts2ellipse" //key
        , "pts2ellipse" //icon
        , N_("Ellipse from points") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        OFFSET
        , N_("Offset") //label
        , "offset" //key
        , "offset" //icon
        , N_("Offset") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        DASH_STROKE
        , N_("Dash Stroke") //label
        , "dash_stroke" //key
        , "dash-stroke" //icon
        , N_("Dash Stroke") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
#ifdef LPE_ENABLE_TEST_EFFECTS
    {
        DOEFFECTSTACK_TEST
        , N_("doEffect stack test") //label
        , "doeffectstacktest" //key
        , "experimental" //icon
        , N_("doEffect stack test") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        ANGLE_BISECTOR
        , N_("Angle bisector") //label
        , "angle_bisector" //key
        , "experimental" //icon
        , N_("Angle bisector") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        CIRCLE_WITH_RADIUS
        , N_("Circle (by center and radius)") //label
        , "circle_with_radius" //key
        , "experimental" //icon
        , N_("Circle (by center and radius)") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        CIRCLE_3PTS
        , N_("Circle by 3 points") //label
        , "circle_3pts" //key
        , "experimental" //icon
        , N_("Circle by 3 points") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        DYNASTROKE
        , N_("Dynamic stroke") //label
        , "dynastroke" //key
        , "experimental" //icon
        , N_("Dynamic stroke") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        EXTRUDE
        , N_("Extrude") //label
        , "extrude" //key
        , "experimental" //icon
        , N_("Extrude") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        LATTICE
        , N_("Lattice Deformation") //label
        , "lattice" //key
        , "experimental" //icon
        , N_("Lattice Deformation") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        LINE_SEGMENT
        , N_("Line Segment") //label
        , "line_segment" //key
        , "experimental" //icon
        , N_("Line Segment") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        PARALLEL
        , N_("Parallel") //label
        , "parallel" //key
        , "experimental" //icon
        , N_("Parallel") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        PATH_LENGTH
        , N_("Path length") //label
        , "path_length" //key
        , "experimental" //icon
        , N_("Path length") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        PERP_BISECTOR
        , N_("Perpendicular bisector") //label
        , "perp_bisector" //key
        , "experimental" //icon
        , N_("Perpendicular bisector") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        RECURSIVE_SKELETON
        , N_("Recursive skeleton") //label
        , "recursive_skeleton" //key
        , "experimental" //icon
        , N_("Recursive skeleton") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        TANGENT_TO_CURVE
        , N_("Tangent to curve") //label
        , "tangent_to_curve" //key
        , "experimental" //icon
        , N_("Tangent to curve") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
    {
        TEXT_LABEL
        , N_("Text label") //label
        , "text_label" //key
        , "experimental" //icon
        , N_("Text label") //description
        , true  //on_path
        , true  //on_shape
        , true  //on_group
        , true  //on_use
        , false //on_image
        , false //on_text
    },
#endif

};

extern const EnumEffectData<EffectType> LPETypeData[];  /// defined in effect.cpp
extern const EnumEffectDataConverter<EffectType> LPETypeConverter; /// defined in effect.cpp
>>>>>>> fixing coding style and translation and merge from master

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
