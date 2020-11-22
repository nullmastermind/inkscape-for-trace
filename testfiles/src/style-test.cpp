// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Unit test for style properties.
 *
 * Author:
 *   Tavmjong Bah <tavjong@free.fr>
 *
 * Copyright (C) 2017 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "style.h"

namespace {

class StyleRead {

public:
  StyleRead(std::string src, std::string dst, std::string uri) :
    src(std::move(src)), dst(std::move(dst)), uri(std::move(uri))
  {
  }

  StyleRead(std::string src, std::string dst) :
    src(std::move(src)), dst(std::move(dst)), uri("")
  {
  }

  StyleRead(std::string const &src) :
    src(src), dst(src), uri("")
  {
  }

  std::string src;
  std::string dst;
  std::string uri;
  
};

std::vector<StyleRead> getStyleData()
{
    StyleRead all_style_data[] = {

        // Paint -----------------------------------------------
        StyleRead("fill:none"), StyleRead("fill:currentColor"), StyleRead("fill:#ff00ff"),
        StyleRead("fill:rgb(100%, 0%, 100%)", "fill:#ff00ff"), StyleRead("fill:rgb(255, 0, 255)", "fill:#ff00ff"),

        // TODO - fix this to preserve the string
        //    StyleRead("fill:url(#painter) rgb(100%, 0%, 100%)",
        //	            "fill:url(#painter) #ff00ff", "#painter"     ),

        // TODO - fix this to preserve the string
        // StyleRead("fill:url(#painter) rgb(255, 0, 255)",
        //	        "fill:url(#painter) #ff00ff", "#painter"),


        StyleRead("fill:#ff00ff icc-color(colorChange, 0.1, 0.5, 0.1)"),

        //  StyleRead("fill:url(#painter)",                     "", "#painter"),
        //  StyleRead("fill:url(#painter) none",                "", "#painter"),
        //  StyleRead("fill:url(#painter) currentColor",        "", "#painter"),
        //  StyleRead("fill:url(#painter) #ff00ff",             "", "#painter"),
        //  StyleRead("fill:url(#painter) rgb(100%, 0%, 100%)", "", "#painter"),
        //  StyleRead("fill:url(#painter) rgb(255, 0, 255)",    "", "#painter"),

        //  StyleRead("fill:url(#painter) #ff00ff icc-color(colorChange, 0.1, 0.5, 0.1)", "", "#painter"),

        //  StyleRead("fill:url(#painter) inherit",             "", "#painter"),

        StyleRead("fill:inherit"),


        // General tests (in general order of appearance in sp_style_read), SPIPaint tested above
        StyleRead("visibility:hidden"), // SPIEnum
        StyleRead("visibility:collapse"), StyleRead("visibility:visible"),
        StyleRead("display:none"),     // SPIEnum
        StyleRead("overflow:visible"), // SPIEnum
        StyleRead("overflow:auto"),    // SPIEnum

        StyleRead("color:#ff0000"), StyleRead("color:blue", "color:#0000ff"),
        // StyleRead("color:currentColor"),  SVG 1.1 does not allow color value 'currentColor'

        // Font shorthand
        StyleRead("font:bold 12px Arial", "font-style:normal;font-variant:normal;font-weight:bold;font-stretch:normal;"
                                          "font-size:12px;line-height:normal;font-family:Arial"),
        StyleRead("font:bold 12px/24px 'Times New Roman'",
                  "font-style:normal;font-variant:normal;font-weight:bold;font-stretch:normal;font-size:12px;line-"
                  "height:24px;font-family:\'Times New Roman\'"),

        // From CSS 3 Fonts (examples):
        StyleRead("font: 12pt/15pt sans-serif", "font-style:normal;font-variant:normal;font-weight:normal;font-stretch:"
                                                "normal;font-size:16px;line-height:15pt;font-family:sans-serif"),
        // StyleRead("font: 80% sans-serif",
        //	        "font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:80%;line-height:normal;font-family:sans-serif"),
        // StyleRead("font: x-large/110% 'new century schoolbook', serif",
        //	      "font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:x-large;line-height:110%;font-family:\'new
        //century schoolbook\', serif"),
        StyleRead("font: bold italic large Palatino, serif",
                  "font-style:italic;font-variant:normal;font-weight:bold;font-stretch:normal;font-size:large;line-"
                  "height:normal;font-family:Palatino, serif"),
        // StyleRead("font: normal small-caps 120%/120% fantasy",
        //	      "font-style:normal;font-variant:small-caps;font-weight:normal;font-stretch:normal;font-size:120%;line-height:120%;font-family:fantasy"),
        StyleRead("font: condensed oblique 12pt 'Helvetica Neue', serif;",
                  "font-style:oblique;font-variant:normal;font-weight:normal;font-stretch:condensed;font-size:16px;"
                  "line-height:normal;font-family:\'Helvetica Neue\', serif"),

        StyleRead("font-family:sans-serif"), // SPIString, text_private
        StyleRead("font-family:Arial"),
        // StyleRead("font-variant:normal;font-stretch:normal;-inkscape-font-specification:Nimbus Roman No9 L Bold
        // Italic"),

        // Needs to be fixed (quotes should be around each font-family):
        StyleRead("font-family:Georgia, 'Minion Web'", "font-family:Georgia, \'Minion Web\'"),
        StyleRead("font-size:12", "font-size:12px"), // SPIFontSize
        StyleRead("font-size:12px"), StyleRead("font-size:12pt", "font-size:16px"), StyleRead("font-size:medium"),
        StyleRead("font-size:smaller"),
        StyleRead("font-style:italic"),       // SPIEnum
        StyleRead("font-variant:small-caps"), // SPIEnum
        StyleRead("font-weight:100"),         // SPIEnum
        StyleRead("font-weight:normal"), StyleRead("font-weight:bolder"),
        StyleRead("font-stretch:condensed"), // SPIEnum

        StyleRead("font-variant-ligatures:none"), // SPILigatures
        StyleRead("font-variant-ligatures:normal"), StyleRead("font-variant-ligatures:no-common-ligatures"),
        StyleRead("font-variant-ligatures:discretionary-ligatures"),
        StyleRead("font-variant-ligatures:historical-ligatures"), StyleRead("font-variant-ligatures:no-contextual"),
        StyleRead("font-variant-ligatures:common-ligatures", "font-variant-ligatures:normal"),
        StyleRead("font-variant-ligatures:contextual", "font-variant-ligatures:normal"),
        StyleRead("font-variant-ligatures:no-common-ligatures historical-ligatures"),
        StyleRead("font-variant-ligatures:historical-ligatures no-contextual"),
        StyleRead("font-variant-position:normal"), StyleRead("font-variant-position:sub"),
        StyleRead("font-variant-position:super"), StyleRead("font-variant-caps:normal"),
        StyleRead("font-variant-caps:small-caps"), StyleRead("font-variant-caps:all-small-caps"),
        StyleRead("font-variant-numeric:normal"), StyleRead("font-variant-numeric:lining-nums"),
        StyleRead("font-variant-numeric:oldstyle-nums"), StyleRead("font-variant-numeric:proportional-nums"),
        StyleRead("font-variant-numeric:tabular-nums"), StyleRead("font-variant-numeric:diagonal-fractions"),
        StyleRead("font-variant-numeric:stacked-fractions"), StyleRead("font-variant-numeric:ordinal"),
        StyleRead("font-variant-numeric:slashed-zero"), StyleRead("font-variant-numeric:tabular-nums slashed-zero"),
        StyleRead("font-variant-numeric:tabular-nums proportional-nums", "font-variant-numeric:proportional-nums"),

        StyleRead("font-variation-settings:'wght' 400"),
        StyleRead("font-variation-settings:'wght'  400", "font-variation-settings:'wght' 400"),
        StyleRead("font-variation-settings:'wght' 400, 'slnt' 0.5", "font-variation-settings:'slnt' 0.5, 'wght' 400"),
        StyleRead("font-variation-settings:\"wght\" 400", "font-variation-settings:'wght' 400"),

        // Should be moved down
        StyleRead("text-indent:12em"),  // SPILength?
        StyleRead("text-align:center"), // SPIEnum

        // SPITextDecoration
        // The default value for 'text-decoration-color' is 'currentColor', but
        // we cannot set the default to that value yet. (We need to switch
        // SPIPaint to SPIColor and then add the ability to set default.)
        // StyleRead("text-decoration: underline",
        //          "text-decoration: underline;text-decoration-line: underline;text-decoration-color:currentColor"),
        // StyleRead("text-decoration: overline underline",
        //          "text-decoration: underline overline;text-decoration-line: underline
        //          overline;text-decoration-color:currentColor"),

        StyleRead("text-decoration: underline wavy #0000ff",
                  "text-decoration:underline;text-decoration-line:"
                  "underline;text-decoration-style:wavy;text-decoration-color:#0000ff"),
        StyleRead("text-decoration: double overline underline #ff0000",
                  "text-decoration:underline overline;text-decoration-line:underline "
                  "overline;text-decoration-style:double;text-decoration-color:#ff0000"),

        // SPITextDecorationLine
        // If only "text-decoration-line" is set but not "text-decoration", don't write "text-decoration" (changed in 1.1)
        StyleRead("text-decoration-line:underline", "text-decoration-line:underline"),
        // "text-decoration" overwrites "text-decoration-line" and vice versa, last one counts
        StyleRead("text-decoration-line:overline;text-decoration:underline",
                  "text-decoration:underline;text-decoration-line:underline"),
        StyleRead("text-decoration:underline;text-decoration-line:overline",
                  "text-decoration:overline;text-decoration-line:overline"),

        // SPITextDecorationStyle
        StyleRead("text-decoration-style:solid"), StyleRead("text-decoration-style:dotted"),

        // SPITextDecorationColor
        StyleRead("text-decoration-color:#ff00ff"),

        // Should be moved up
        StyleRead("line-height:24px"), // SPILengthOrNormal
        StyleRead("line-height:1.5"),
        StyleRead("letter-spacing:2px"), // SPILengthOrNormal
        StyleRead("word-spacing:2px"),   // SPILengthOrNormal
        StyleRead("word-spacing:normal"),
        StyleRead("text-transform:lowercase"), // SPIEnum
        // ...
        StyleRead("baseline-shift:baseline"), // SPIBaselineShift
        StyleRead("baseline-shift:sub"), StyleRead("baseline-shift:12.5%"), StyleRead("baseline-shift:2px"),

        StyleRead("opacity:0.1"), // SPIScale24
        // ...
        StyleRead("stroke-width:2px"),      // SPILength
        StyleRead("stroke-linecap:round"),  // SPIEnum
        StyleRead("stroke-linejoin:round"), // SPIEnum
        StyleRead("stroke-miterlimit:4"),   // SPIFloat
        StyleRead("marker:url(#Arrow)"),    // SPIString
        StyleRead("marker-start:url(#Arrow)"), StyleRead("marker-mid:url(#Arrow)"), StyleRead("marker-end:url(#Arrow)"),
        StyleRead("stroke-opacity:0.5"), // SPIScale24
        // Currently inkscape handle unit conversion in dasharray but need
        // a active document to do it, so we can't include in any test
        StyleRead("stroke-dasharray:0, 1, 0, 1"), // SPIDashArray
        StyleRead("stroke-dasharray:0 1 0 1", "stroke-dasharray:0, 1, 0, 1"),
        StyleRead("stroke-dasharray:0  1  2  3", "stroke-dasharray:0, 1, 2, 3"),
        StyleRead("stroke-dashoffset:13"), // SPILength
        StyleRead("stroke-dashoffset:10px"),
        // ...
        // StyleRead("filter:url(#myfilter)"),                   // SPIFilter segfault in read
        StyleRead("filter:inherit"),

        StyleRead("opacity:0.1;fill:#ff0000;stroke:#0000ff;stroke-width:2px"),
        StyleRead("opacity:0.1;fill:#ff0000;stroke:#0000ff;stroke-width:2px;stroke-dasharray:1, 2, 3, "
                  "4;stroke-dashoffset:15"),

        StyleRead("paint-order:stroke"), // SPIPaintOrder
        StyleRead("paint-order:normal"),
        StyleRead("paint-order: markers stroke   fill", "paint-order:markers stroke fill"),

        // !important  (in order of appearance in style-internal.h)
        StyleRead("stroke-miterlimit:4 !important"), // SPIFloat
        StyleRead("stroke-opacity:0.5 !important"),  // SPIScale24
        StyleRead("stroke-width:2px !important"),    // SPILength
        StyleRead("line-height:24px !important"),    // SPILengthOrNormal
        StyleRead("line-height:normal !important"),
        StyleRead("font-stretch:condensed !important"), // SPIEnum
        StyleRead("marker:url(#Arrow) !important"),     // SPIString
        StyleRead("color:#0000ff !important"),          // SPIColor
        StyleRead("fill:none !important"),              // SPIPaint
        StyleRead("fill:currentColor !important"), StyleRead("fill:#ff00ff !important"),
        StyleRead("paint-order:stroke !important"), // SPIPaintOrder
        StyleRead("paint-order:normal !important"),
        StyleRead("stroke-dasharray:0, 1, 0, 1 !important"), // SPIDashArray
        StyleRead("font-size:12px !important"),              // SPIFontSize
        StyleRead("baseline-shift:baseline !important"),     // SPIBaselineShift
        StyleRead("baseline-shift:sub !important"),
        // StyleRead("text-decoration-line: underline !important"),         // SPITextDecorationLine

    };

    size_t count = sizeof(all_style_data) / sizeof(all_style_data[0]);
    std::vector<StyleRead> vect(all_style_data, all_style_data + count);
    return vect;
}

TEST(StyleTest, Read) {
  std::vector<StyleRead> all_style = getStyleData();
  EXPECT_GT(all_style.size(), 0);
  for (auto i : all_style) {

    SPStyle style;
    style.mergeString (i.src.c_str());

    if (!i.uri.empty()) {
      //EXPECT_EQ (style.fill.value.href->getURI()->toString(), i.uri);
    }

    std::string out = style.write();
    if (i.dst.empty()) {
      // std::cout << "out:   " << out   << std::endl;
      // std::cout << "i.src: " << i.src << std::endl;
      EXPECT_EQ (out, i.src);
    } else {
      // std::cout << "out:   " << out   << std::endl;
      // std::cout << "i.dst: " << i.dst << std::endl;
      EXPECT_EQ (out, i.dst);
    }
  }
}


// ------------------------------------------------------------------------------------

class StyleMatch {

public:
  StyleMatch(std::string src, std::string dst, bool const &match) :
    src(std::move(src)), dst(std::move(dst)), match(match)
  {
  }

  std::string src;
  std::string dst;
  bool        match;
  
};

std::vector<StyleMatch> getStyleMatchData()
{
  StyleMatch all_style_data[] = {

    // SPIFloat
    StyleMatch("stroke-miterlimit:4",     "stroke-miterlimit:4",      true ),
    StyleMatch("stroke-miterlimit:4",     "stroke-miterlimit:2",      false),
    StyleMatch("stroke-miterlimit:4",     "",                         true ), // Default

    // SPIScale24
    StyleMatch("opacity:0.3",             "opacity:0.3",              true ),
    StyleMatch("opacity:0.3",             "opacity:0.6",              false),
    StyleMatch("opacity:1.0",             "",                         true ), // Default

    // SPILength
    StyleMatch("text-indent:3",           "text-indent:3",            true ),
    StyleMatch("text-indent:6",           "text-indent:3",            false),
    StyleMatch("text-indent:6px",         "text-indent:3",            false),
    StyleMatch("text-indent:1px",         "text-indent:12pc",         false),
    StyleMatch("text-indent:2ex",         "text-indent:2ex",          false),

    // SPILengthOrNormal
    StyleMatch("letter-spacing:normal",   "letter-spacing:normal",    true ),
    StyleMatch("letter-spacing:2",        "letter-spacing:normal",    false),
    StyleMatch("letter-spacing:normal",   "letter-spacing:2",         false),
    StyleMatch("letter-spacing:5px",      "letter-spacing:5px",       true ),
    StyleMatch("letter-spacing:10px",     "letter-spacing:5px",       false),
    StyleMatch("letter-spacing:10em",     "letter-spacing:10em",      false),

    // SPIEnum
    StyleMatch("text-anchor:start",       "text-anchor:start",        true ),
    StyleMatch("text-anchor:start",       "text-anchor:middle",       false),
    StyleMatch("text-anchor:start",       "",                         true ), // Default
    StyleMatch("text-anchor:start",       "text-anchor:junk",         true ), // Bad value

    StyleMatch("font-weight:normal",      "font-weight:400",          true ),
    StyleMatch("font-weight:bold",        "font-weight:700",          true ),


    // SPIString and SPIFontString
    StyleMatch("font-family:Arial",       "font-family:Arial",        true ),
    StyleMatch("font-family:A B",         "font-family:A B",          true ),
    StyleMatch("font-family:A B",         "font-family:A C",          false),
    // Default is not set by class... value is NULL which cannot be compared
    // StyleMatch("font-family:sans-serif",  "",                       true ), // Default

    // SPIColor
    StyleMatch("color:blue",              "color:blue",               true ),
    StyleMatch("color:blue",              "color:red",                false),
    StyleMatch("color:red",               "color:#ff0000",            true ),

    // SPIPaint
    StyleMatch("fill:blue",               "fill:blue",               true ),
    StyleMatch("fill:blue",               "fill:red",                false),
    StyleMatch("fill:currentColor",       "fill:currentColor",       true ),
    StyleMatch("fill:url(#xxx)",          "fill:url(#xxx)",          true ),
    // Needs URL defined as in test 1
    //StyleMatch("fill:url(#xxx)",          "fill:url(#yyy)",          false),

    // SPIPaintOrder
    StyleMatch("paint-order:markers",     "paint-order:markers",     true ),
    StyleMatch("paint-order:markers",     "paint-order:stroke",      false),
    //StyleMatch("paint-order:fill stroke markers", "",                true ), // Default
    StyleMatch("paint-order:normal",      "paint-order:normal",      true ),
    //StyleMatch("paint-order:fill stroke markers", "paint-order:normal", true ),

    // SPIDashArray
    StyleMatch("stroke-dasharray:0 1 2 3","stroke-dasharray:0 1 2 3",true ),
    StyleMatch("stroke-dasharray:0 1",    "stroke-dasharray:0 2",    false),

    // SPIFilter

    // SPIFontSize
    StyleMatch("font-size:12px",          "font-size:12px",          true ),
    StyleMatch("font-size:12px",          "font-size:24px",          false),
    StyleMatch("font-size:12ex",          "font-size:24ex",          false),
    StyleMatch("font-size:medium",        "font-size:medium",        true ),
    StyleMatch("font-size:medium",        "font-size:large",         false),

    // SPIBaselineShift
    StyleMatch("baseline-shift:baseline", "baseline-shift:baseline", true ),
    StyleMatch("baseline-shift:sub",      "baseline-shift:sub",      true ),
    StyleMatch("baseline-shift:sub",      "baseline-shift:super",    false),
    StyleMatch("baseline-shift:baseline", "baseline-shift:sub",      false),
    StyleMatch("baseline-shift:10px",     "baseline-shift:10px",     true ),
    StyleMatch("baseline-shift:10px",     "baseline-shift:12px",     false),


    // SPITextDecorationLine
    StyleMatch("text-decoration-line:underline", "text-decoration-line:underline", true ),
    StyleMatch("text-decoration-line:underline", "text-decoration-line:overline",  false),
    StyleMatch("text-decoration-line:underline overline", "text-decoration-line:underline overline", true ),
    StyleMatch("text-decoration-line:none",      "", true ), // Default


    // SPITextDecorationStyle
    StyleMatch("text-decoration-style:solid",    "text-decoration-style:solid", true ),
    StyleMatch("text-decoration-style:dotted",   "text-decoration-style:solid", false),
    StyleMatch("text-decoration-style:solid",    "",   true ), // Default

    // SPITextDecoration
    StyleMatch("text-decoration:underline",          "text-decoration:underline", true ),
    StyleMatch("text-decoration:underline",          "text-decoration:overline",  false),
    StyleMatch("text-decoration:underline overline","text-decoration:underline overline",true ),
    StyleMatch("text-decoration:overline underline","text-decoration:underline overline",true ),
    // StyleMatch("text-decoration:none",               "text-decoration-color:currentColor", true ), // Default

  };

  size_t count = sizeof(all_style_data) / sizeof(all_style_data[0]);
  std::vector<StyleMatch> vect(all_style_data, all_style_data + count);
  return vect;
}

TEST(StyleTest, Match) {
  std::vector<StyleMatch> all_style = getStyleMatchData();
  EXPECT_GT(all_style.size(), 0);
  for (auto i : all_style) {

    SPStyle style_src;
    SPStyle style_dst;

    style_src.mergeString( i.src.c_str() );
    style_dst.mergeString( i.dst.c_str() );

    // std::cout << "Test:" << std::endl;
    // std::cout << "  C: |" << i.src
    // 	         << "|    |" << i.dst << "|" << std::endl;
    // std::cout << "  S: |" << style_src.write( SP_STYLE_FLAG_IFSET )
    // 	         << "|    |" << style_dst.write( SP_STYLE_FLAG_IFSET ) << "|" <<std::endl;

    EXPECT_TRUE( (style_src == style_dst) == i.match );
  }
}

// ------------------------------------------------------------------------------------

class StyleCascade {

public:
  StyleCascade(std::string parent, std::string child, std::string result, char const *d = nullptr) :
    parent(std::move(parent)), child(std::move(child)), result(std::move(result))
  {
      diff = d ? d : (this->result == this->parent ? "" : this->result);
  }

  std::string parent;
  std::string child;
  std::string result;
  std::string diff;
  
};

std::vector<StyleCascade> getStyleCascadeData()
{

  StyleCascade all_style_data[] = {

    // SPIFloat
    StyleCascade("stroke-miterlimit:6",   "stroke-miterlimit:2",   "stroke-miterlimit:2"    ),
    StyleCascade("stroke-miterlimit:6",   "",                      "stroke-miterlimit:6"    ),
    StyleCascade("",                      "stroke-miterlimit:2",   "stroke-miterlimit:2"    ),

    // SPIScale24
    StyleCascade("opacity:0.3",           "opacity:0.3",            "opacity:0.3", "opacity:0.3" ),
    StyleCascade("opacity:0.3",           "opacity:0.6",            "opacity:0.6"           ),
    // 'opacity' does not inherit
    StyleCascade("opacity:0.3",           "",                       "opacity:1"             ),
    StyleCascade("",                      "opacity:0.3",            "opacity:0.3"           ),
    StyleCascade("opacity:0.5",           "opacity:inherit",        "opacity:0.5", "opacity:0.5" ),
    StyleCascade("",                      "",                       "opacity:1"             ),

    // SPILength
    StyleCascade("text-indent:3",         "text-indent:3",          "text-indent:3"         ),
    StyleCascade("text-indent:6",         "text-indent:3",          "text-indent:3"         ),
    StyleCascade("text-indent:6px",       "text-indent:3",          "text-indent:3"         ),
    StyleCascade("text-indent:1px",       "text-indent:12pc",       "text-indent:12pc"      ),
    // ex, em cannot be equal
    //StyleCascade("text-indent:2ex",       "text-indent:2ex",        "text-indent:2ex"       ),
    StyleCascade("text-indent:3",         "",                       "text-indent:3"         ),
    StyleCascade("text-indent:3",         "text-indent:inherit",    "text-indent:3"         ),

    // SPILengthOrNormal
    StyleCascade("letter-spacing:normal", "letter-spacing:normal",  "letter-spacing:normal" ), 
    StyleCascade("letter-spacing:2",      "letter-spacing:normal",  "letter-spacing:normal" ), 
    StyleCascade("letter-spacing:normal", "letter-spacing:2",       "letter-spacing:2"      ),
    StyleCascade("letter-spacing:5px",    "letter-spacing:5px",     "letter-spacing:5px"    ),
    StyleCascade("letter-spacing:10px",   "letter-spacing:5px",     "letter-spacing:5px"    ),
    // ex, em cannot be equal
    // StyleCascade("letter-spacing:10em",   "letter-spacing:10em",    "letter-spacing:10em"   ),    

    // SPIEnum
    StyleCascade("text-anchor:start",     "text-anchor:start",      "text-anchor:start"     ),
    StyleCascade("text-anchor:start",     "text-anchor:middle",     "text-anchor:middle"    ),
    StyleCascade("text-anchor:start",     "",                       "text-anchor:start"     ),
    StyleCascade("text-anchor:start",     "text-anchor:junk",       "text-anchor:start"     ),
    StyleCascade("text-anchor:end",       "text-anchor:inherit",    "text-anchor:end"       ),

    StyleCascade("font-weight:400",       "font-weight:400",        "font-weight:400"       ),
    StyleCascade("font-weight:400",       "font-weight:700",        "font-weight:700"       ),
    StyleCascade("font-weight:400",       "font-weight:bolder",     "font-weight:700"       ),
    StyleCascade("font-weight:700",       "font-weight:bolder",     "font-weight:900"       ),
    StyleCascade("font-weight:400",       "font-weight:lighter",    "font-weight:100"       ),
    StyleCascade("font-weight:200",       "font-weight:lighter",    "font-weight:100"       ),

    StyleCascade("font-stretch:condensed","font-stretch:expanded",  "font-stretch:expanded" ),
    StyleCascade("font-stretch:condensed","font-stretch:wider",     "font-stretch:semi-condensed" ),

    // SPIString and SPIFontString

    StyleCascade("font-variation-settings:'wght' 400", "",          "font-variation-settings:'wght' 400"),
    StyleCascade("font-variation-settings:'wght' 100",
                 "font-variation-settings:'wght' 400",
                 "font-variation-settings:'wght' 400"),

    StyleCascade("font-variant-ligatures:no-common-ligatures", "",                               "font-variant-ligatures:no-common-ligatures"),
    StyleCascade("font-variant-ligatures:no-common-ligatures", "inherit",                        "font-variant-ligatures:no-common-ligatures"),
    StyleCascade("font-variant-ligatures:normal", "font-variant-ligatures:no-common-ligatures",  "font-variant-ligatures:no-common-ligatures"),
    StyleCascade("",                              "font-variant-ligatures:no-common-ligatures",  "font-variant-ligatures:no-common-ligatures"),

    // SPIPaint

    // SPIPaintOrder

    // SPIDashArray

    // SPIFilter

    // SPIFontSize

    // SPIBaselineShift


    // SPITextDecorationLine
    StyleCascade("text-decoration-line:overline", "text-decoration-line:underline",
    	         "text-decoration-line:underline"          ),
    StyleCascade("text-decoration:overline",
                 "text-decoration:underline",
                 "text-decoration:underline;text-decoration-line:underline"),
    StyleCascade("text-decoration:underline",
                 "text-decoration:underline",
                 "text-decoration:underline;text-decoration-line:underline",
                 ""),
    StyleCascade("text-decoration:overline;text-decoration-line:underline",
                 "text-decoration:overline",
                 "text-decoration:overline;text-decoration-line:overline"),
    StyleCascade("text-decoration:overline;text-decoration-line:underline",
                 "text-decoration:underline",
                 "text-decoration:underline;text-decoration-line:underline",
                 ""),

    // SPITextDecorationStyle

    // SPITextDecoration
  };

  size_t count = sizeof(all_style_data) / sizeof(all_style_data[0]);
  std::vector<StyleCascade> vect(all_style_data, all_style_data + count);
  return vect;

}

TEST(StyleTest, Cascade) {
   std::vector<StyleCascade> all_style = getStyleCascadeData();
  EXPECT_GT(all_style.size(), 0);
  for (auto i : all_style) {
    
    SPStyle style_parent;
    SPStyle style_child;
    SPStyle style_result;

    style_parent.mergeString( i.parent.c_str() );
    style_child.mergeString(  i.child.c_str()  );
    style_result.mergeString( i.result.c_str() );

    // std::cout << "Test:" << std::endl;
    // std::cout << " Input: ";
    // std::cout << "  Parent: " << i.parent
    //           << "  Child: "  << i.child
    //           << "  Result: " << i.result << std::endl;
    // std::cout << " Write: ";
    // std::cout << "  Parent: " << style_parent.write( SP_STYLE_FLAG_IFSET )
    //           << "  Child: "  << style_child.write( SP_STYLE_FLAG_IFSET )
    //           << "  Result: " << style_result.write( SP_STYLE_FLAG_IFSET ) << std::endl;

    style_child.cascade( &style_parent );

    EXPECT_TRUE(style_child == style_result );

    // if diff
    EXPECT_STREQ(style_result.writeIfDiff(nullptr).c_str(), i.result.c_str());
    EXPECT_STREQ(style_result.writeIfDiff(&style_parent).c_str(), i.diff.c_str());
  }
}


} // namespace

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
