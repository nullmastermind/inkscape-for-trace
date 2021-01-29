// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef USE_PANGO_WIN32

#include "OpenTypeUtil.h"


#include <iostream>  // For debugging

// FreeType
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_SFNT_NAMES_H

// Harfbuzz
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>

// SVG in OpenType
#include "io/stream/gzipstream.h"
#include "io/stream/bufferstream.h"


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


#if HB_VERSION_ATLEAST(1,2,3)  // Released Feb 2016
void get_glyphs( hb_font_t* font, hb_set_t* set, Glib::ustring& characters) {

    // There is a unicode to glyph mapping function but not the inverse!
    hb_codepoint_t codepoint = -1;
    while (hb_set_next (set, &codepoint)) {
        for (hb_codepoint_t unicode_i = 0; unicode_i < 0xffff; ++unicode_i) {
            hb_codepoint_t glyph = 0;
            hb_font_get_nominal_glyph (font, unicode_i, &glyph);
            if (glyph == codepoint) {
                characters += (gunichar)unicode_i;
                break;
            }
        }
    }
}
#endif

// Make a list of all tables found in the GSUB
// This list includes all tables regardless of script or language.
// Use Harfbuzz, Pango's equivalent calls are deprecated.
void readOpenTypeGsubTable (hb_font_t* hb_font,
                            std::map<Glib::ustring, OTSubstitution>& tables)
{
    hb_face_t* hb_face = hb_font_get_face (hb_font);

    tables.clear();

    // First time to get size of array
    auto script_count = hb_ot_layout_table_get_script_tags(hb_face, HB_OT_TAG_GSUB, 0, nullptr, nullptr);
    auto const hb_scripts = g_new(hb_tag_t, script_count + 1);

    // Second time to fill array (this two step process was not necessary with Pango).
    hb_ot_layout_table_get_script_tags(hb_face, HB_OT_TAG_GSUB, 0, &script_count, hb_scripts);

    for(unsigned int i = 0; i < script_count; ++i) {
        // std::cout << " Script: " << extract_tag(&hb_scripts[i]) << std::endl;
        auto language_count = hb_ot_layout_script_get_language_tags(hb_face, HB_OT_TAG_GSUB, i, 0, nullptr, nullptr);

        if(language_count > 0) {
            auto const hb_languages = g_new(hb_tag_t, language_count + 1);
            hb_ot_layout_script_get_language_tags(hb_face, HB_OT_TAG_GSUB, i, 0, &language_count, hb_languages);

            for(unsigned int j = 0; j < language_count; ++j) {
                // std::cout << "  Language: " << extract_tag(&hb_languages[j]) << std::endl;
                auto feature_count = hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i, j, 0, nullptr, nullptr);
                auto const hb_features = g_new(hb_tag_t, feature_count + 1);
                hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i, j, 0, &feature_count, hb_features);

                for(unsigned int k = 0; k < feature_count; ++k) {
                    // std::cout << "   Feature: " << extract_tag(&hb_features[k]) << std::endl;
                    tables[ extract_tag(&hb_features[k])];
                }

                g_free(hb_features);
            }

            g_free(hb_languages);

        } else {

            // Even if no languages are present there is still the default.
            // std::cout << "  Language: " << " (dflt)" << std::endl;
            auto feature_count = hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i,
                                                                        HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                                        0, nullptr, nullptr);
            auto const hb_features = g_new(hb_tag_t, feature_count + 1); 
            hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i,
                                                   HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                   0, &feature_count, hb_features);

            for(unsigned int k = 0; k < feature_count; ++k) {
                // std::cout << "   Feature: " << extract_tag(&hb_features[k]) << std::endl;
                tables[ extract_tag(&hb_features[k])];
            }

            g_free(hb_features);
        }
    }

#if HB_VERSION_ATLEAST(1,2,3)  // Released Feb 2016
    // Find glyphs in OpenType substitution tables ('gsub').
    // Note that pango's functions are just dummies. Must use harfbuzz.

    // Loop over all tables
    for (auto table: tables) {

        // Only look at style substitution tables ('salt', 'ss01', etc. but not 'ssty').
        // Also look at character substitution tables ('cv01', etc.).
        bool style    =
            table.first == "case"  /* Case-Sensitive Forms   */                          ||
            table.first == "salt"  /* Stylistic Alternatives */                          ||
            table.first == "swsh"  /* Swash                  */                          ||
            table.first == "cwsh"  /* Contextual Swash       */                          ||
            table.first == "ornm"  /* Ornaments              */                          ||
            table.first == "nalt"  /* Alternative Annotation */                          ||
            table.first == "hist"  /* Historical Forms       */                          ||
            (table.first[0] == 's' && table.first[1] == 's' && !(table.first[2] == 't')) ||
            (table.first[0] == 'c' && table.first[1] == 'v');

        bool ligature = ( table.first == "liga" ||  // Standard ligatures
                          table.first == "clig" ||  // Common ligatures
                          table.first == "dlig" ||  // Discretionary ligatures
                          table.first == "hlig" ||  // Historical ligatures
                          table.first == "calt" );  // Contextual alternatives

        bool numeric  = ( table.first == "lnum" ||  // Lining numerals
                          table.first == "onum" ||  // Old style
                          table.first == "pnum" ||  // Proportional
                          table.first == "tnum" ||  // Tabular
                          table.first == "frac" ||  // Diagonal fractions
                          table.first == "afrc" ||  // Stacked fractions
                          table.first == "ordn" ||  // Ordinal fractions
                          table.first == "zero" );  // Slashed zero

        if (style || ligature || numeric) {

            unsigned int feature_index;
            if (  hb_ot_layout_language_find_feature (hb_face, HB_OT_TAG_GSUB,
                                                      0,  // Assume one script exists with index 0
                                                      HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                      HB_TAG(table.first[0],
                                                             table.first[1],
                                                             table.first[2],
                                                             table.first[3]),
                                                      &feature_index ) ) {

                // std::cout << "Table: " << table.first << std::endl;
                // std::cout << "  Found feature, number: " << feature_index << std::endl;
                unsigned int lookup_indexes[32]; 
                unsigned int lookup_count = 32;
                int count = hb_ot_layout_feature_get_lookups (hb_face, HB_OT_TAG_GSUB,
                                                              feature_index,
                                                              0, // Start
                                                              &lookup_count,
                                                              lookup_indexes );
                // std::cout << "  Lookup count: " << count << " total: " << lookup_count << std::endl;

                for (int i = 0; i < count; ++i) {
                    hb_set_t* glyphs_before = hb_set_create();
                    hb_set_t* glyphs_input  = hb_set_create();
                    hb_set_t* glyphs_after  = hb_set_create();
                    hb_set_t* glyphs_output = hb_set_create();

                    hb_ot_layout_lookup_collect_glyphs (hb_face, HB_OT_TAG_GSUB,
                                                        lookup_indexes[i],
                                                        glyphs_before,
                                                        glyphs_input,
                                                        glyphs_after,
                                                        glyphs_output );

                    // std::cout << "  Populations: "
                    //           << " " << hb_set_get_population (glyphs_before)
                    //           << " " << hb_set_get_population (glyphs_input)
                    //           << " " << hb_set_get_population (glyphs_after)
                    //           << " " << hb_set_get_population (glyphs_output)
                    //           << std::endl;

                    get_glyphs (hb_font, glyphs_before, tables[table.first].before);
                    get_glyphs (hb_font, glyphs_input,  tables[table.first].input );
                    get_glyphs (hb_font, glyphs_after,  tables[table.first].after );
                    get_glyphs (hb_font, glyphs_output, tables[table.first].output);

                    // std::cout << "  Before: " << tables[table.first].before.c_str() << std::endl;
                    // std::cout << "  Input:  " << tables[table.first].input.c_str() << std::endl;
                    // std::cout << "  After:  " << tables[table.first].after.c_str() << std::endl;
                    // std::cout << "  Output: " << tables[table.first].output.c_str() << std::endl;

                    hb_set_destroy (glyphs_before);
                    hb_set_destroy (glyphs_input);
                    hb_set_destroy (glyphs_after);
                    hb_set_destroy (glyphs_output);

                } // End count (lookups)

            } else {
                // std::cout << "  Did not find '" << table.first << "'!" << std::endl;
            }
        }

    }
#else
    std::cerr << "Requires Harfbuzz 1.2.3 for visualizing alternative glyph OpenType tables. "
              << "Compiled with: " << HB_VERSION_STRING << "." << std::endl;
#endif

    g_free(hb_scripts);
}

// Harfbuzz now as API for variations (Version 2.2, Nov 29 2018).
// Make a list of all Variation axes with ranges.
void readOpenTypeFvarAxes(const FT_Face ft_face,
                          std::map<Glib::ustring, OTVarAxis>& axes) {

#if FREETYPE_MAJOR *10000 + FREETYPE_MINOR*100 + FREETYPE_MICRO >= 20701
    FT_MM_Var* mmvar = nullptr;
    FT_Multi_Master mmtype;
    if (FT_HAS_MULTIPLE_MASTERS( ft_face )    &&    // Font has variables
        FT_Get_MM_Var( ft_face, &mmvar) == 0   &&    // We found the data
        FT_Get_Multi_Master( ft_face, &mmtype) !=0) {  // It's not an Adobe MM font

        FT_Fixed coords[mmvar->num_axis];
        FT_Get_Var_Design_Coordinates( ft_face, mmvar->num_axis, coords );

        for (size_t i = 0; i < mmvar->num_axis; ++i) {
            FT_Var_Axis* axis = &mmvar->axis[i];
            axes[axis->name] =  OTVarAxis(FTFixedToDouble(axis->minimum),
                                          FTFixedToDouble(axis->def),
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


// Harfbuzz now as API for named variations (Version 2.2, Nov 29 2018).
// Make a list of all Named instances with axis values.
void readOpenTypeFvarNamed(const FT_Face ft_face,
                           std::map<Glib::ustring, OTVarInstance>& named) {

#if FREETYPE_MAJOR *10000 + FREETYPE_MINOR*100 + FREETYPE_MICRO >= 20701
    FT_MM_Var* mmvar = nullptr;
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

#define HB_OT_TAG_SVG HB_TAG('S','V','G',' ')

// Get SVG glyphs out of an OpenType font.
void readOpenTypeSVGTable(hb_font_t* hb_font,
                          std::map<int, SVGTableEntry>& glyphs) {

    hb_face_t* hb_face = hb_font_get_face (hb_font);

    // Harfbuzz has some support for SVG fonts but it is not exposed until version 2.1 (Oct 30, 2018).
    // We do it the hard way!
    hb_blob_t *hb_blob = hb_face_reference_table (hb_face, HB_OT_TAG_SVG);

    if (!hb_blob) {
        // No SVG table in font!
        return;
    }

    unsigned int svg_length = hb_blob_get_length (hb_blob);
    if (svg_length == 0) {
        // No SVG glyphs in table!
        return;
    }

    const char* data = hb_blob_get_data(hb_blob, &svg_length);
    if (!data) {
        std::cerr << "readOpenTypeSVGTable: Failed to get data! " << std::endl;
        return;
    }

    // OpenType fonts use Big Endian
#if 0
    uint16_t version = ((data[0] & 0xff) <<  8) +  (data[1] & 0xff);
    // std::cout << "Version: " << version << std::endl;
#endif
    uint32_t offset  = ((data[2] & 0xff) << 24) + ((data[3] & 0xff) << 16) + ((data[4] & 0xff) << 8) + (data[5] & 0xff);

    // std::cout << "Offset: "  << offset << std::endl;
    // Bytes 6-9 are reserved.

    uint16_t entries = ((data[offset] & 0xff) << 8) + (data[offset+1] & 0xff);
    // std::cout << "Number of entries: " << entries << std::endl;

    for (int entry = 0; entry < entries; ++entry) {
        uint32_t base = offset + 2 + entry * 12;

        uint16_t startGlyphID = ((data[base  ] & 0xff) <<  8) + (data[base+1] & 0xff);
        uint16_t endGlyphID   = ((data[base+2] & 0xff) <<  8) + (data[base+3] & 0xff);
        uint32_t offsetGlyph  = ((data[base+4] & 0xff) << 24) + ((data[base+5] & 0xff) << 16) +((data[base+6]  & 0xff) << 8) + (data[base+7]  & 0xff);
        uint32_t lengthGlyph  = ((data[base+8] & 0xff) << 24) + ((data[base+9] & 0xff) << 16) +((data[base+10] & 0xff) << 8) + (data[base+11] & 0xff);

        // std::cout << "Entry " << entry << ": Start: " << startGlyphID << "  End: " << endGlyphID
        //           << "  Offset: " << offsetGlyph << " Length: " << lengthGlyph << std::endl;

        std::string svg;

        // static cast is needed as hb_blob_get_length returns char but we are comparing to a value greater than allowed by char.
        if (lengthGlyph > 1 && //
            static_cast<unsigned char>(data[offset + offsetGlyph + 0]) == 0x1f &&
            static_cast<unsigned char>(data[offset + offsetGlyph + 1]) == 0x8b) {
            // Glyph is gzipped

            std::vector<unsigned char> buffer;
            for (unsigned int c = offsetGlyph; c < offsetGlyph + lengthGlyph; ++c) {
                buffer.push_back(data[offset + c]);
            }

            Inkscape::IO::BufferInputStream zipped(buffer);
            Inkscape::IO::GzipInputStream gzin(zipped);
            for (int character = gzin.get(); character != -1; character = gzin.get()) {
               svg+= (char)character;
            }

        } else {
            // Glyph is not compressed

            for (unsigned int c = offsetGlyph; c < offsetGlyph + lengthGlyph; ++c) {
                svg += (unsigned char) data[offset + c];
            }
        }

        for (unsigned int i = startGlyphID; i < endGlyphID+1; ++i) {
            glyphs[i].svg = svg;
        }

        // for (auto glyph : glyphs) {
        //     std::cout << "Glyph: " << glyph.first << std::endl;
        //     std::cout << glyph.second.svg << std::endl;
        // }
    }
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
