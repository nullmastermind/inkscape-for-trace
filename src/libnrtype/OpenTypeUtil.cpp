
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

inline double FTFixedToDouble (FT_Fixed value) {
    return static_cast<FT_Int32>(value) / 65536.0;
}

inline FT_Fixed FTDoubleToFixed (double value) {
    return static_cast<FT_Fixed>(value * 65536);
}


// Make a list of all tables fount in the GSUB
// This list includes all tables regardless of script or language.
void readOpenTypeGsubTable (const FT_Face ft_face,
                            std::map<Glib::ustring, int>& tables,
                            std::map<Glib::ustring, Glib::ustring>& substitutions) {

    tables.clear();
    substitutions.clear();

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
        if (table.first == "salt" ||
            (table.first[0] == 's' && table.first[1] == 's' && !(table.first[2] == 't') ) ) {
            // std::cout << " Table: " << table.first << std::endl;

            Glib::ustring unicode_characters;

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
                    hb_set_t* glyphs_input  = hb_set_create();
                    hb_set_t* glyphs_after  = NULL; // hb_set_create();
                    hb_set_t* glyphs_output = NULL; // hb_set_create();

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

                    hb_codepoint_t codepoint = -1;
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
                    substitutions[table.first] = unicode_characters;

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
