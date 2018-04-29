
#ifndef USE_PANGO_WIN32

#include "OpenTypeUtil.h"


#include <iostream>  // For debugging

// FreeType
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_SFNT_NAMES_H

// Harfbuzz
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>


// Utilities used in this file

void dump_tag( guint32 *tag, Glib::ustring prefix = "", bool lf=true ) {
    std::cout << prefix
              << ((char)((*tag & 0xff000000)>>24))
              << ((char)((*tag & 0x00ff0000)>>16))
              << ((char)((*tag & 0x0000ff00)>> 8))
              << ((char)((*tag & 0x000000ff)    ));
    if( lf ) {
        std::cout << std::endl;
    }
}

Glib::ustring extract_tag( guint32 *tag ) {
    Glib::ustring tag_name;
    tag_name += ((char)((*tag & 0xff000000)>>24));
    tag_name += ((char)((*tag & 0x00ff0000)>>16));
    tag_name += ((char)((*tag & 0x0000ff00)>> 8));
    tag_name += ((char)((*tag & 0x000000ff)    ));
    return tag_name;
}


// Make a list of all tables found in the GSUB
// This list includes all tables regardless of script or language.
void readOpenTypeGsubTable (const FT_Face ft_face,
                            std::map<Glib::ustring, int>& tables,
                            std::map<Glib::ustring, Glib::ustring>& stylistic,
                            std::map<Glib::ustring, Glib::ustring>& ligatures
    ) {

    tables.clear();
    stylistic.clear();
    ligatures.clear();

    // Use Harfbuzz, Pango's equivalent calls are deprecated.
    auto const hb_face = hb_ft_face_create(ft_face, NULL);

    // First time to get size of array
    auto script_count = hb_ot_layout_table_get_script_tags(hb_face, HB_OT_TAG_GSUB, 0, NULL, NULL);
    auto const hb_scripts = g_new(hb_tag_t, script_count + 1);

    // Second time to fill array (this two step process was not necessary with Pango).
    hb_ot_layout_table_get_script_tags(hb_face, HB_OT_TAG_GSUB, 0, &script_count, hb_scripts);

    for(unsigned int i = 0; i < script_count; ++i) {
        auto language_count = hb_ot_layout_script_get_language_tags(hb_face, HB_OT_TAG_GSUB, i, 0, NULL, NULL);

        if(language_count > 0) {
            auto const hb_languages = g_new(hb_tag_t, language_count + 1);
            hb_ot_layout_script_get_language_tags(hb_face, HB_OT_TAG_GSUB, i, 0, &language_count, hb_languages);

            for(unsigned int j = 0; j < language_count; ++j) {
                auto feature_count = hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i, j, 0, NULL, NULL);
                auto const hb_features = g_new(hb_tag_t, feature_count + 1);
                hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i, j, 0, &feature_count, hb_features);

                for(unsigned int k = 0; k < feature_count; ++k) {
                    ++(tables[ extract_tag(&hb_features[k])]);
                }

                g_free(hb_features);
            }

            g_free(hb_languages);

        } else {

            // Even if no languages are present there is still the default.
            auto feature_count = hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i,
                                                                        HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                                        0, NULL, NULL);
            auto const hb_features = g_new(hb_tag_t, feature_count + 1); 
            hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i,
                                                   HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                   0, &feature_count, hb_features);

            for(unsigned int k = 0; k < feature_count; ++k) {
                ++(tables[ extract_tag(&hb_features[k])]);
            }

            g_free(hb_features);
        }
    }

// TODO: Ideally, we should use the HB_VERSION_ATLEAST macro here,
// but this was only released in harfbuzz >= 0.9.30
// #if HB_VERSION_ATLEAST(1,2,3)
#if HB_VERSION_MAJOR*10000 + HB_VERSION_MINOR*100 + HB_VERSION_MICRO >= 10203
    // Find glyphs in OpenType substitution tables ('gsub').
    // Note that pango's functions are just dummies. Must use harfbuzz.

    // Loop over all tables
    for (auto table: tables) {

        // Only look at style substitution tables ('salt', 'ss01', etc. but not 'ssty').
        bool style    = table.first == "salt" ||
            (table.first[0] == 's' && table.first[1] == 's' && !(table.first[2] == 't'));

        bool ligature = ( table.first == "liga" ||  // Standard ligatures
                          table.first == "clig" ||  // Common ligatures
                          table.first == "dlig" ||  // Discretionary ligatures
                          table.first == "hlig" ||  // Historical ligatures
                          table.first == "calt" );  // Contextual alternatives

        if (style || ligature ) {

            unsigned int feature_index;
            if (  hb_ot_layout_language_find_feature (hb_face, HB_OT_TAG_GSUB,
                                                      0,  // Assume one script exists with index 0
                                                      HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                      HB_TAG(table.first[0],
                                                             table.first[1],
                                                             table.first[2],
                                                             table.first[3]),
                                                      &feature_index ) ) {

                // std::cout << "  Found feature, number: " << feature_index << std::endl;
                unsigned int lookup_indexes[32]; 
                unsigned int lookup_count = 32;
                int count = hb_ot_layout_feature_get_lookups (hb_face, HB_OT_TAG_GSUB,
                                                              feature_index,
                                                              0, // Start
                                                              &lookup_count,
                                                              lookup_indexes );
                // std::cout << "  Lookup count: " << count << " total: " << lookup_count << std::endl;

                if (count > 0) {
                    hb_set_t* glyphs_before = NULL; // hb_set_create();
                    hb_set_t* glyphs_input  = hb_set_create();  // For stylistic
                    hb_set_t* glyphs_after  = NULL; // hb_set_create();
                    hb_set_t* glyphs_output = hb_set_create();  // For ligatures

                    // For now, just look at first index
                    hb_ot_layout_lookup_collect_glyphs (hb_face, HB_OT_TAG_GSUB,
                                                        lookup_indexes[0],
                                                        glyphs_before,
                                                        glyphs_input,
                                                        glyphs_after,
                                                        glyphs_output );

                    hb_font_t *hb_font = hb_font_create (hb_face);

                    // Without this, all functions return 0, etc.
                    hb_ft_font_set_funcs (hb_font);

                    Glib::ustring unicode_characters;

                    hb_codepoint_t codepoint = -1;

                    if (style) {
                        while (hb_set_next (glyphs_input, &codepoint)) {

                            // There is a unicode to glyph mapping function but not the inverse!
                            for (hb_codepoint_t unicode_i = 0; unicode_i < 0xffff; ++unicode_i) {
                                hb_codepoint_t glyph = 0;
                                hb_font_get_nominal_glyph (hb_font, unicode_i, &glyph);
                                if ( glyph == codepoint) {
                                    unicode_characters += (gunichar)unicode_i;
                                    continue;
                                }
                            }
                        }
                        stylistic[table.first] = unicode_characters;
                    }

                    // Don't know how to extract all input glyphs...
                    // glyphs_input contains last input glyph, so just use output.
                    if (ligature) {
                        while (hb_set_next (glyphs_output, &codepoint)) {

                            // There is a unicode to glyph mapping function but not the inverse!
                            for (hb_codepoint_t unicode_i = 0; unicode_i < 0xffff; ++unicode_i) {
                                hb_codepoint_t glyph = 0;
                                hb_font_get_nominal_glyph (hb_font, unicode_i, &glyph);
                                if ( glyph == codepoint) {
                                    unicode_characters += (gunichar)unicode_i;
                                    unicode_characters += " "; // Add space
                                    continue;
                                }
                            }
                        }
                        ligatures[table.first] = unicode_characters;
                    }

                    hb_set_destroy (glyphs_input);
                    hb_font_destroy (hb_font);
                }
            } else {
                // std::cout << "  Did not find '" << table.first << "'!" << std::endl;
            }
        }

    }
    // for (auto table: res->openTypeSubstitutions) {
    //     std::cout << table.first << ": " << table.second << std::endl;
    // }
#else
    std::cerr << "Requires Harfbuzz 1.2.3 for visualizing alternative glyph OpenType tables. "
              << "Compiled with: " << HB_VERSION_STRING << "." << std::endl;
#endif

    g_free(hb_scripts);
    hb_face_destroy (hb_face);
}

// Make a list of all Variation axes with ranges.
void readOpenTypeFvarAxes(const FT_Face ft_face,
                          std::map<Glib::ustring, OTVarAxis>& axes) {

#if FREETYPE_MAJOR *10000 + FREETYPE_MINOR*100 + FREETYPE_MICRO >= 20701
    FT_MM_Var* mmvar = NULL;
    FT_Multi_Master mmtype;
    if (FT_HAS_MULTIPLE_MASTERS( ft_face )    &&    // Font has variables
        FT_Get_MM_Var( ft_face, &mmvar) == 0   &&    // We found the data
        FT_Get_Multi_Master( ft_face, &mmtype) !=0) {  // It's not an Adobe MM font

        FT_Fixed coords[mmvar->num_axis];
        FT_Get_Var_Design_Coordinates( ft_face, mmvar->num_axis, coords );

        for (size_t i = 0; i < mmvar->num_axis; ++i) {
            FT_Var_Axis* axis = &mmvar->axis[i];
            axes[axis->name] =  OTVarAxis(FTFixedToDouble(axis->minimum),
                                          FTFixedToDouble(axis->maximum),
                                          FTFixedToDouble(coords[i]),
                                          i);
        }

        // for (auto a: axes) {
        //     std::cout << " " << a.first
        //               << " min: " << a.second.minimum
        //               << " max: " << a.second.maximum
        //               << " set: " << a.second.set_val << std::endl;
        // }

    }

#endif /* FREETYPE Version */
}


// Make a list of all Named instances with axis values.
void readOpenTypeFvarNamed(const FT_Face ft_face,
                           std::map<Glib::ustring, OTVarInstance>& named) {

#if FREETYPE_MAJOR *10000 + FREETYPE_MINOR*100 + FREETYPE_MICRO >= 20701
    FT_MM_Var* mmvar = NULL;
    FT_Multi_Master mmtype;
    if (FT_HAS_MULTIPLE_MASTERS( ft_face )    &&    // Font has variables
        FT_Get_MM_Var( ft_face, &mmvar) == 0   &&    // We found the data
        FT_Get_Multi_Master( ft_face, &mmtype) !=0) {  // It's not an Adobe MM font

        std::cout << "  Multiple Masters: variables: " << mmvar->num_axis
                  << "  named styles: " << mmvar->num_namedstyles << std::endl;

    //     const FT_UInt numNames = FT_Get_Sfnt_Name_Count(ft_face);
    //     std::cout << "  number of names: " << numNames << std::endl;
    //     FT_SfntName ft_name;
    //     for (FT_UInt i = 0; i < numNames; ++i) {

    //         if (FT_Get_Sfnt_Name(ft_face, i, &ft_name) != 0) {
    //             continue;
    //         }

    //         Glib::ustring name;
    //         for (size_t j = 0; j < ft_name.string_len; ++j) {
    //             name += (char)ft_name.string[j];
    //         }
    //         std::cout << " " << i << ": " << name << std::endl;
    //     }

    }

#endif /* FREETYPE Version */
}

#endif /* !USE_PANGO_WIND32    */

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
