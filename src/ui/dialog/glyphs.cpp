// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Jon A. Cruz
 *   Abhishek Sharma
 *   Tavmjong Bah
 *
 * Copyright (C) 2010 Jon A. Cruz
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>

#include "glyphs.h"

#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/grid.h>
#include <gtkmm/iconview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h" // for SPDocumentUndo::done()
#include "selection.h"
#include "text-editing.h"
#include "verbs.h"

#include "libnrtype/font-instance.h"
#include "libnrtype/font-lister.h"

#include "object/sp-flowtext.h"
#include "object/sp-text.h"

#include "ui/widget/font-selector.h"

namespace Inkscape {
namespace UI {
namespace Dialog {


GlyphsPanel &GlyphsPanel::getInstance()
{
    return *new GlyphsPanel();
}


static std::map<GUnicodeScript, Glib::ustring> & getScriptToName()
{
    static bool init = false;
    static std::map<GUnicodeScript, Glib::ustring> mappings;
    if (!init) {
        init = true;
        mappings[G_UNICODE_SCRIPT_INVALID_CODE]         = _("all");
        mappings[G_UNICODE_SCRIPT_COMMON]               = _("common");
        mappings[G_UNICODE_SCRIPT_INHERITED]            = _("inherited");
        mappings[G_UNICODE_SCRIPT_ARABIC]               = _("Arabic");
        mappings[G_UNICODE_SCRIPT_ARMENIAN]             = _("Armenian");
        mappings[G_UNICODE_SCRIPT_BENGALI]              = _("Bengali");
        mappings[G_UNICODE_SCRIPT_BOPOMOFO]             = _("Bopomofo");
        mappings[G_UNICODE_SCRIPT_CHEROKEE]             = _("Cherokee");
        mappings[G_UNICODE_SCRIPT_COPTIC]               = _("Coptic");
        mappings[G_UNICODE_SCRIPT_CYRILLIC]             = _("Cyrillic");
        mappings[G_UNICODE_SCRIPT_DESERET]              = _("Deseret");
        mappings[G_UNICODE_SCRIPT_DEVANAGARI]           = _("Devanagari");
        mappings[G_UNICODE_SCRIPT_ETHIOPIC]             = _("Ethiopic");
        mappings[G_UNICODE_SCRIPT_GEORGIAN]             = _("Georgian");
        mappings[G_UNICODE_SCRIPT_GOTHIC]               = _("Gothic");
        mappings[G_UNICODE_SCRIPT_GREEK]                = _("Greek");
        mappings[G_UNICODE_SCRIPT_GUJARATI]             = _("Gujarati");
        mappings[G_UNICODE_SCRIPT_GURMUKHI]             = _("Gurmukhi");
        mappings[G_UNICODE_SCRIPT_HAN]                  = _("Han");
        mappings[G_UNICODE_SCRIPT_HANGUL]               = _("Hangul");
        mappings[G_UNICODE_SCRIPT_HEBREW]               = _("Hebrew");
        mappings[G_UNICODE_SCRIPT_HIRAGANA]             = _("Hiragana");
        mappings[G_UNICODE_SCRIPT_KANNADA]              = _("Kannada");
        mappings[G_UNICODE_SCRIPT_KATAKANA]             = _("Katakana");
        mappings[G_UNICODE_SCRIPT_KHMER]                = _("Khmer");
        mappings[G_UNICODE_SCRIPT_LAO]                  = _("Lao");
        mappings[G_UNICODE_SCRIPT_LATIN]                = _("Latin");
        mappings[G_UNICODE_SCRIPT_MALAYALAM]            = _("Malayalam");
        mappings[G_UNICODE_SCRIPT_MONGOLIAN]            = _("Mongolian");
        mappings[G_UNICODE_SCRIPT_MYANMAR]              = _("Myanmar");
        mappings[G_UNICODE_SCRIPT_OGHAM]                = _("Ogham");
        mappings[G_UNICODE_SCRIPT_OLD_ITALIC]           = _("Old Italic");
        mappings[G_UNICODE_SCRIPT_ORIYA]                = _("Oriya");
        mappings[G_UNICODE_SCRIPT_RUNIC]                = _("Runic");
        mappings[G_UNICODE_SCRIPT_SINHALA]              = _("Sinhala");
        mappings[G_UNICODE_SCRIPT_SYRIAC]               = _("Syriac");
        mappings[G_UNICODE_SCRIPT_TAMIL]                = _("Tamil");
        mappings[G_UNICODE_SCRIPT_TELUGU]               = _("Telugu");
        mappings[G_UNICODE_SCRIPT_THAANA]               = _("Thaana");
        mappings[G_UNICODE_SCRIPT_THAI]                 = _("Thai");
        mappings[G_UNICODE_SCRIPT_TIBETAN]              = _("Tibetan");
        mappings[G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL]  = _("Canadian Aboriginal");
        mappings[G_UNICODE_SCRIPT_YI]                   = _("Yi");
        mappings[G_UNICODE_SCRIPT_TAGALOG]              = _("Tagalog");
        mappings[G_UNICODE_SCRIPT_HANUNOO]              = _("Hanunoo");
        mappings[G_UNICODE_SCRIPT_BUHID]                = _("Buhid");
        mappings[G_UNICODE_SCRIPT_TAGBANWA]             = _("Tagbanwa");
        mappings[G_UNICODE_SCRIPT_BRAILLE]              = _("Braille");
        mappings[G_UNICODE_SCRIPT_CYPRIOT]              = _("Cypriot");
        mappings[G_UNICODE_SCRIPT_LIMBU]                = _("Limbu");
        mappings[G_UNICODE_SCRIPT_OSMANYA]              = _("Osmanya");
        mappings[G_UNICODE_SCRIPT_SHAVIAN]              = _("Shavian");
        mappings[G_UNICODE_SCRIPT_LINEAR_B]             = _("Linear B");
        mappings[G_UNICODE_SCRIPT_TAI_LE]               = _("Tai Le");
        mappings[G_UNICODE_SCRIPT_UGARITIC]             = _("Ugaritic");
        mappings[G_UNICODE_SCRIPT_NEW_TAI_LUE]          = _("New Tai Lue");
        mappings[G_UNICODE_SCRIPT_BUGINESE]             = _("Buginese");
        mappings[G_UNICODE_SCRIPT_GLAGOLITIC]           = _("Glagolitic");
        mappings[G_UNICODE_SCRIPT_TIFINAGH]             = _("Tifinagh");
        mappings[G_UNICODE_SCRIPT_SYLOTI_NAGRI]         = _("Syloti Nagri");
        mappings[G_UNICODE_SCRIPT_OLD_PERSIAN]          = _("Old Persian");
        mappings[G_UNICODE_SCRIPT_KHAROSHTHI]           = _("Kharoshthi");
        mappings[G_UNICODE_SCRIPT_UNKNOWN]              = _("unassigned");
        mappings[G_UNICODE_SCRIPT_BALINESE]             = _("Balinese");
        mappings[G_UNICODE_SCRIPT_CUNEIFORM]            = _("Cuneiform");
        mappings[G_UNICODE_SCRIPT_PHOENICIAN]           = _("Phoenician");
        mappings[G_UNICODE_SCRIPT_PHAGS_PA]             = _("Phags-pa");
        mappings[G_UNICODE_SCRIPT_NKO]                  = _("N'Ko");
        mappings[G_UNICODE_SCRIPT_KAYAH_LI]             = _("Kayah Li");
        mappings[G_UNICODE_SCRIPT_LEPCHA]               = _("Lepcha");
        mappings[G_UNICODE_SCRIPT_REJANG]               = _("Rejang");
        mappings[G_UNICODE_SCRIPT_SUNDANESE]            = _("Sundanese");
        mappings[G_UNICODE_SCRIPT_SAURASHTRA]           = _("Saurashtra");
        mappings[G_UNICODE_SCRIPT_CHAM]                 = _("Cham");
        mappings[G_UNICODE_SCRIPT_OL_CHIKI]             = _("Ol Chiki");
        mappings[G_UNICODE_SCRIPT_VAI]                  = _("Vai");
        mappings[G_UNICODE_SCRIPT_CARIAN]               = _("Carian");
        mappings[G_UNICODE_SCRIPT_LYCIAN]               = _("Lycian");
        mappings[G_UNICODE_SCRIPT_LYDIAN]               = _("Lydian");
        mappings[G_UNICODE_SCRIPT_AVESTAN]              = _("Avestan");                 // Since: 2.26
        mappings[G_UNICODE_SCRIPT_BAMUM]                = _("Bamum");                 // Since: 2.26
        mappings[G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS] = _("Egyptian Hieroglpyhs");    // Since: 2.26
        mappings[G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC]     = _("Imperial Aramaic");        // Since: 2.26
        mappings[G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI]= _("Inscriptional Pahlavi");   // Since: 2.26
        mappings[G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN]= _("Inscriptional Parthian"); // Since: 2.26
        mappings[G_UNICODE_SCRIPT_JAVANESE]             = _("Javanese");                // Since: 2.26
        mappings[G_UNICODE_SCRIPT_KAITHI]               = _("Kaithi");                 // Since: 2.26
        mappings[G_UNICODE_SCRIPT_LISU]                 = _("Lisu");                    // Since: 2.26
        mappings[G_UNICODE_SCRIPT_MEETEI_MAYEK]         = _("Meetei Mayek");            // Since: 2.26
        mappings[G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN]    = _("Old South Arabian");       // Since: 2.26
        mappings[G_UNICODE_SCRIPT_OLD_TURKIC]           = _("Old Turkic");              // Since: 2.28
        mappings[G_UNICODE_SCRIPT_SAMARITAN]            = _("Samaritan");               // Since: 2.26
        mappings[G_UNICODE_SCRIPT_TAI_THAM]             = _("Tai Tham");                // Since: 2.26
        mappings[G_UNICODE_SCRIPT_TAI_VIET]             = _("Tai Viet");                // Since: 2.26
        mappings[G_UNICODE_SCRIPT_BATAK]                = _("Batak");                   // Since: 2.28
        mappings[G_UNICODE_SCRIPT_BRAHMI]               = _("Brahmi");                  // Since: 2.28
        mappings[G_UNICODE_SCRIPT_MANDAIC]              = _("Mandaic");                 // Since: 2.28
        mappings[G_UNICODE_SCRIPT_CHAKMA]               = _("Chakma");                  // Since: 2.32
        mappings[G_UNICODE_SCRIPT_MEROITIC_CURSIVE]     = _("Meroitic Cursive");        // Since: 2.32
        mappings[G_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS] = _("Meroitic Hieroglyphs");    // Since: 2.32
        mappings[G_UNICODE_SCRIPT_MIAO]                 = _("Miao");                    // Since: 2.32
        mappings[G_UNICODE_SCRIPT_SHARADA]              = _("Sharada");                 // Since: 2.32
        mappings[G_UNICODE_SCRIPT_SORA_SOMPENG]         = _("Sora Sompeng");            // Since: 2.32
        mappings[G_UNICODE_SCRIPT_TAKRI]                = _("Takri");                   // Since: 2.32
        mappings[G_UNICODE_SCRIPT_BASSA_VAH]            = _("Bassa");                   // Since: 2.42
        mappings[G_UNICODE_SCRIPT_CAUCASIAN_ALBANIAN]   = _("Caucasian Albanian");      // Since: 2.42
        mappings[G_UNICODE_SCRIPT_DUPLOYAN]             = _("Duployan");                // Since: 2.42
        mappings[G_UNICODE_SCRIPT_ELBASAN]              = _("Elbasan");                 // Since: 2.42
        mappings[G_UNICODE_SCRIPT_GRANTHA]              = _("Grantha");                 // Since: 2.42
        mappings[G_UNICODE_SCRIPT_KHOJKI]               = _("Khojki");                  // Since: 2.42
        mappings[G_UNICODE_SCRIPT_KHUDAWADI]            = _("Khudawadi, Sindhi");       // Since: 2.42
        mappings[G_UNICODE_SCRIPT_LINEAR_A]             = _("Linear A");                // Since: 2.42
        mappings[G_UNICODE_SCRIPT_MAHAJANI]             = _("Mahajani");                // Since: 2.42
        mappings[G_UNICODE_SCRIPT_MANICHAEAN]           = _("Manichaean");              // Since: 2.42
        mappings[G_UNICODE_SCRIPT_MENDE_KIKAKUI]        = _("Mende Kikakui");           // Since: 2.42
        mappings[G_UNICODE_SCRIPT_MODI]                 = _("Modi");                    // Since: 2.42
        mappings[G_UNICODE_SCRIPT_MRO]                  = _("Mro");                     // Since: 2.42
        mappings[G_UNICODE_SCRIPT_NABATAEAN]            = _("Nabataean");               // Since: 2.42
        mappings[G_UNICODE_SCRIPT_OLD_NORTH_ARABIAN]    = _("Old North Arabian");       // Since: 2.42
        mappings[G_UNICODE_SCRIPT_OLD_PERMIC]           = _("Old Permic");              // Since: 2.42
        mappings[G_UNICODE_SCRIPT_PAHAWH_HMONG]         = _("Pahawh Hmong");            // Since: 2.42
        mappings[G_UNICODE_SCRIPT_PALMYRENE]            = _("Palmyrene");               // Since: 2.42
        mappings[G_UNICODE_SCRIPT_PAU_CIN_HAU]          = _("Pau Cin Hau");             // Since: 2.42
        mappings[G_UNICODE_SCRIPT_PSALTER_PAHLAVI]      = _("Psalter Pahlavi");         // Since: 2.42
        mappings[G_UNICODE_SCRIPT_SIDDHAM]              = _("Siddham");                 // Since: 2.42
        mappings[G_UNICODE_SCRIPT_TIRHUTA]              = _("Tirhuta");                 // Since: 2.42
        mappings[G_UNICODE_SCRIPT_WARANG_CITI]          = _("Warang Citi");             // Since: 2.42
        mappings[G_UNICODE_SCRIPT_AHOM]                 = _("Ahom");                    // Since: 2.48
        mappings[G_UNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS]= _("Anatolian Hieroglyphs");   // Since: 2.48
        mappings[G_UNICODE_SCRIPT_HATRAN]               = _("Hatran");                  // Since: 2.48
        mappings[G_UNICODE_SCRIPT_MULTANI]              = _("Multani");                 // Since: 2.48
        mappings[G_UNICODE_SCRIPT_OLD_HUNGARIAN]        = _("Old Hungarian");           // Since: 2.48
        mappings[G_UNICODE_SCRIPT_SIGNWRITING]          = _("Signwriting");             // Since: 2.48
/*
        mappings[G_UNICODE_SCRIPT_ADLAM]                = _("Adlam");                   // Since: 2.50
        mappings[G_UNICODE_SCRIPT_BHAIKSUKI]            = _("Bhaiksuki");               // Since: 2.50
        mappings[G_UNICODE_SCRIPT_MARCHEN]              = _("Marchen");                 // Since: 2.50
        mappings[G_UNICODE_SCRIPT_NEWA]                 = _("Newa");                    // Since: 2.50
        mappings[G_UNICODE_SCRIPT_OSAGE]                = _("Osage");                   // Since: 2.50
        mappings[G_UNICODE_SCRIPT_TANGUT]               = _("Tangut");                  // Since: 2.50
        mappings[G_UNICODE_SCRIPT_MASARAM_GONDI]        = _("Masaram Gondi");           // Since: 2.54
        mappings[G_UNICODE_SCRIPT_NUSHU]                = _("Nushu");                   // Since: 2.54
        mappings[G_UNICODE_SCRIPT_SOYOMBO]              = _("Soyombo");                 // Since: 2.54
        mappings[G_UNICODE_SCRIPT_ZANABAZAR_SQUARE]     = _("Zanabazar Square");        // Since: 2.54
        mappings[G_UNICODE_SCRIPT_DOGRA]                = _("Dogra");                   // Since: 2.58
        mappings[G_UNICODE_SCRIPT_GUNJALA_GONDI]        = _("Gunjala Gondi");           // Since: 2.58
        mappings[G_UNICODE_SCRIPT_HANIFI_ROHINGYA]      = _("Hanifi Rohingya");         // Since: 2.58
        mappings[G_UNICODE_SCRIPT_MAKASAR]              = _("Makasar");                 // Since: 2.58
        mappings[G_UNICODE_SCRIPT_MEDEFAIDRIN]          = _("Medefaidrin");             // Since: 2.58
        mappings[G_UNICODE_SCRIPT_OLD_SOGDIAN]          = _("Old Sogdian");             // Since: 2.58
        mappings[G_UNICODE_SCRIPT_SOGDIAN]              = _("Sogdian");                 // Since: 2.58
        mappings[G_UNICODE_SCRIPT_ELYMAIC]              = _("Elym");                    // Since: 2.62
        mappings[G_UNICODE_SCRIPT_NANDINAGARI]          = _("Nand");                    // Since: 2.62
        mappings[G_UNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG]= _("Rohg");                   // Since: 2.62
        mappings[G_UNICODE_SCRIPT_WANCHO]               = _("Wcho");                    // Since: 2.62
*/
    }
    return mappings;
}

typedef std::pair<gunichar, gunichar> Range;
typedef std::pair<Range, Glib::ustring> NamedRange;

static std::vector<NamedRange> & getRanges()
{
    static bool init = false;
    static std::vector<NamedRange> ranges;
    if (!init) {
        init = true;
        ranges.emplace_back(std::make_pair(0x00000, 0x2FFFF), _("all"));
        ranges.emplace_back(std::make_pair(0x00000, 0x0FFFF), _("Basic Plane"));
        ranges.emplace_back(std::make_pair(0x10000, 0x1FFFF), _("Extended Multilingual Plane"));
        ranges.emplace_back(std::make_pair(0x20000, 0x2FFFF), _("Supplementary Ideographic Plane"));

        ranges.emplace_back(std::make_pair(0x0000, 0x007F), _("Basic Latin"));
        ranges.emplace_back(std::make_pair(0x0080, 0x00FF), _("Latin-1 Supplement"));
        ranges.emplace_back(std::make_pair(0x0100, 0x017F), _("Latin Extended-A"));
        ranges.emplace_back(std::make_pair(0x0180, 0x024F), _("Latin Extended-B"));
        ranges.emplace_back(std::make_pair(0x0250, 0x02AF), _("IPA Extensions"));
        ranges.emplace_back(std::make_pair(0x02B0, 0x02FF), _("Spacing Modifier Letters"));
        ranges.emplace_back(std::make_pair(0x0300, 0x036F), _("Combining Diacritical Marks"));
        ranges.emplace_back(std::make_pair(0x0370, 0x03FF), _("Greek and Coptic"));
        ranges.emplace_back(std::make_pair(0x0400, 0x04FF), _("Cyrillic"));
        ranges.emplace_back(std::make_pair(0x0500, 0x052F), _("Cyrillic Supplement"));
        ranges.emplace_back(std::make_pair(0x0530, 0x058F), _("Armenian"));
        ranges.emplace_back(std::make_pair(0x0590, 0x05FF), _("Hebrew"));
        ranges.emplace_back(std::make_pair(0x0600, 0x06FF), _("Arabic"));
        ranges.emplace_back(std::make_pair(0x0700, 0x074F), _("Syriac"));
        ranges.emplace_back(std::make_pair(0x0750, 0x077F), _("Arabic Supplement"));
        ranges.emplace_back(std::make_pair(0x0780, 0x07BF), _("Thaana"));
        ranges.emplace_back(std::make_pair(0x07C0, 0x07FF), _("NKo"));
        ranges.emplace_back(std::make_pair(0x0800, 0x083F), _("Samaritan"));
        ranges.emplace_back(std::make_pair(0x0900, 0x097F), _("Devanagari"));
        ranges.emplace_back(std::make_pair(0x0980, 0x09FF), _("Bengali"));
        ranges.emplace_back(std::make_pair(0x0A00, 0x0A7F), _("Gurmukhi"));
        ranges.emplace_back(std::make_pair(0x0A80, 0x0AFF), _("Gujarati"));
        ranges.emplace_back(std::make_pair(0x0B00, 0x0B7F), _("Oriya"));
        ranges.emplace_back(std::make_pair(0x0B80, 0x0BFF), _("Tamil"));
        ranges.emplace_back(std::make_pair(0x0C00, 0x0C7F), _("Telugu"));
        ranges.emplace_back(std::make_pair(0x0C80, 0x0CFF), _("Kannada"));
        ranges.emplace_back(std::make_pair(0x0D00, 0x0D7F), _("Malayalam"));
        ranges.emplace_back(std::make_pair(0x0D80, 0x0DFF), _("Sinhala"));
        ranges.emplace_back(std::make_pair(0x0E00, 0x0E7F), _("Thai"));
        ranges.emplace_back(std::make_pair(0x0E80, 0x0EFF), _("Lao"));
        ranges.emplace_back(std::make_pair(0x0F00, 0x0FFF), _("Tibetan"));
        ranges.emplace_back(std::make_pair(0x1000, 0x109F), _("Myanmar"));
        ranges.emplace_back(std::make_pair(0x10A0, 0x10FF), _("Georgian"));
        ranges.emplace_back(std::make_pair(0x1100, 0x11FF), _("Hangul Jamo"));
        ranges.emplace_back(std::make_pair(0x1200, 0x137F), _("Ethiopic"));
        ranges.emplace_back(std::make_pair(0x1380, 0x139F), _("Ethiopic Supplement"));
        ranges.emplace_back(std::make_pair(0x13A0, 0x13FF), _("Cherokee"));
        ranges.emplace_back(std::make_pair(0x1400, 0x167F), _("Unified Canadian Aboriginal Syllabics"));
        ranges.emplace_back(std::make_pair(0x1680, 0x169F), _("Ogham"));
        ranges.emplace_back(std::make_pair(0x16A0, 0x16FF), _("Runic"));
        ranges.emplace_back(std::make_pair(0x1700, 0x171F), _("Tagalog"));
        ranges.emplace_back(std::make_pair(0x1720, 0x173F), _("Hanunoo"));
        ranges.emplace_back(std::make_pair(0x1740, 0x175F), _("Buhid"));
        ranges.emplace_back(std::make_pair(0x1760, 0x177F), _("Tagbanwa"));
        ranges.emplace_back(std::make_pair(0x1780, 0x17FF), _("Khmer"));
        ranges.emplace_back(std::make_pair(0x1800, 0x18AF), _("Mongolian"));
        ranges.emplace_back(std::make_pair(0x18B0, 0x18FF), _("Unified Canadian Aboriginal Syllabics Extended"));
        ranges.emplace_back(std::make_pair(0x1900, 0x194F), _("Limbu"));
        ranges.emplace_back(std::make_pair(0x1950, 0x197F), _("Tai Le"));
        ranges.emplace_back(std::make_pair(0x1980, 0x19DF), _("New Tai Lue"));
        ranges.emplace_back(std::make_pair(0x19E0, 0x19FF), _("Khmer Symbols"));
        ranges.emplace_back(std::make_pair(0x1A00, 0x1A1F), _("Buginese"));
        ranges.emplace_back(std::make_pair(0x1A20, 0x1AAF), _("Tai Tham"));
        ranges.emplace_back(std::make_pair(0x1B00, 0x1B7F), _("Balinese"));
        ranges.emplace_back(std::make_pair(0x1B80, 0x1BBF), _("Sundanese"));
        ranges.emplace_back(std::make_pair(0x1C00, 0x1C4F), _("Lepcha"));
        ranges.emplace_back(std::make_pair(0x1C50, 0x1C7F), _("Ol Chiki"));
        ranges.emplace_back(std::make_pair(0x1CD0, 0x1CFF), _("Vedic Extensions"));
        ranges.emplace_back(std::make_pair(0x1D00, 0x1D7F), _("Phonetic Extensions"));
        ranges.emplace_back(std::make_pair(0x1D80, 0x1DBF), _("Phonetic Extensions Supplement"));
        ranges.emplace_back(std::make_pair(0x1DC0, 0x1DFF), _("Combining Diacritical Marks Supplement"));
        ranges.emplace_back(std::make_pair(0x1E00, 0x1EFF), _("Latin Extended Additional"));
        ranges.emplace_back(std::make_pair(0x1F00, 0x1FFF), _("Greek Extended"));
        ranges.emplace_back(std::make_pair(0x2000, 0x206F), _("General Punctuation"));
        ranges.emplace_back(std::make_pair(0x2070, 0x209F), _("Superscripts and Subscripts"));
        ranges.emplace_back(std::make_pair(0x20A0, 0x20CF), _("Currency Symbols"));
        ranges.emplace_back(std::make_pair(0x20D0, 0x20FF), _("Combining Diacritical Marks for Symbols"));
        ranges.emplace_back(std::make_pair(0x2100, 0x214F), _("Letterlike Symbols"));
        ranges.emplace_back(std::make_pair(0x2150, 0x218F), _("Number Forms"));
        ranges.emplace_back(std::make_pair(0x2190, 0x21FF), _("Arrows"));
        ranges.emplace_back(std::make_pair(0x2200, 0x22FF), _("Mathematical Operators"));
        ranges.emplace_back(std::make_pair(0x2300, 0x23FF), _("Miscellaneous Technical"));
        ranges.emplace_back(std::make_pair(0x2400, 0x243F), _("Control Pictures"));
        ranges.emplace_back(std::make_pair(0x2440, 0x245F), _("Optical Character Recognition"));
        ranges.emplace_back(std::make_pair(0x2460, 0x24FF), _("Enclosed Alphanumerics"));
        ranges.emplace_back(std::make_pair(0x2500, 0x257F), _("Box Drawing"));
        ranges.emplace_back(std::make_pair(0x2580, 0x259F), _("Block Elements"));
        ranges.emplace_back(std::make_pair(0x25A0, 0x25FF), _("Geometric Shapes"));
        ranges.emplace_back(std::make_pair(0x2600, 0x26FF), _("Miscellaneous Symbols"));
        ranges.emplace_back(std::make_pair(0x2700, 0x27BF), _("Dingbats"));
        ranges.emplace_back(std::make_pair(0x27C0, 0x27EF), _("Miscellaneous Mathematical Symbols-A"));
        ranges.emplace_back(std::make_pair(0x27F0, 0x27FF), _("Supplemental Arrows-A"));
        ranges.emplace_back(std::make_pair(0x2800, 0x28FF), _("Braille Patterns"));
        ranges.emplace_back(std::make_pair(0x2900, 0x297F), _("Supplemental Arrows-B"));
        ranges.emplace_back(std::make_pair(0x2980, 0x29FF), _("Miscellaneous Mathematical Symbols-B"));
        ranges.emplace_back(std::make_pair(0x2A00, 0x2AFF), _("Supplemental Mathematical Operators"));
        ranges.emplace_back(std::make_pair(0x2B00, 0x2BFF), _("Miscellaneous Symbols and Arrows"));
        ranges.emplace_back(std::make_pair(0x2C00, 0x2C5F), _("Glagolitic"));
        ranges.emplace_back(std::make_pair(0x2C60, 0x2C7F), _("Latin Extended-C"));
        ranges.emplace_back(std::make_pair(0x2C80, 0x2CFF), _("Coptic"));
        ranges.emplace_back(std::make_pair(0x2D00, 0x2D2F), _("Georgian Supplement"));
        ranges.emplace_back(std::make_pair(0x2D30, 0x2D7F), _("Tifinagh"));
        ranges.emplace_back(std::make_pair(0x2D80, 0x2DDF), _("Ethiopic Extended"));
        ranges.emplace_back(std::make_pair(0x2DE0, 0x2DFF), _("Cyrillic Extended-A"));
        ranges.emplace_back(std::make_pair(0x2E00, 0x2E7F), _("Supplemental Punctuation"));
        ranges.emplace_back(std::make_pair(0x2E80, 0x2EFF), _("CJK Radicals Supplement"));
        ranges.emplace_back(std::make_pair(0x2F00, 0x2FDF), _("Kangxi Radicals"));
        ranges.emplace_back(std::make_pair(0x2FF0, 0x2FFF), _("Ideographic Description Characters"));
        ranges.emplace_back(std::make_pair(0x3000, 0x303F), _("CJK Symbols and Punctuation"));
        ranges.emplace_back(std::make_pair(0x3040, 0x309F), _("Hiragana"));
        ranges.emplace_back(std::make_pair(0x30A0, 0x30FF), _("Katakana"));
        ranges.emplace_back(std::make_pair(0x3100, 0x312F), _("Bopomofo"));
        ranges.emplace_back(std::make_pair(0x3130, 0x318F), _("Hangul Compatibility Jamo"));
        ranges.emplace_back(std::make_pair(0x3190, 0x319F), _("Kanbun"));
        ranges.emplace_back(std::make_pair(0x31A0, 0x31BF), _("Bopomofo Extended"));
        ranges.emplace_back(std::make_pair(0x31C0, 0x31EF), _("CJK Strokes"));
        ranges.emplace_back(std::make_pair(0x31F0, 0x31FF), _("Katakana Phonetic Extensions"));
        ranges.emplace_back(std::make_pair(0x3200, 0x32FF), _("Enclosed CJK Letters and Months"));
        ranges.emplace_back(std::make_pair(0x3300, 0x33FF), _("CJK Compatibility"));
        ranges.emplace_back(std::make_pair(0x3400, 0x4DBF), _("CJK Unified Ideographs Extension A"));
        ranges.emplace_back(std::make_pair(0x4DC0, 0x4DFF), _("Yijing Hexagram Symbols"));
        ranges.emplace_back(std::make_pair(0x4E00, 0x9FFF), _("CJK Unified Ideographs"));
        ranges.emplace_back(std::make_pair(0xA000, 0xA48F), _("Yi Syllables"));
        ranges.emplace_back(std::make_pair(0xA490, 0xA4CF), _("Yi Radicals"));
        ranges.emplace_back(std::make_pair(0xA4D0, 0xA4FF), _("Lisu"));
        ranges.emplace_back(std::make_pair(0xA500, 0xA63F), _("Vai"));
        ranges.emplace_back(std::make_pair(0xA640, 0xA69F), _("Cyrillic Extended-B"));
        ranges.emplace_back(std::make_pair(0xA6A0, 0xA6FF), _("Bamum"));
        ranges.emplace_back(std::make_pair(0xA700, 0xA71F), _("Modifier Tone Letters"));
        ranges.emplace_back(std::make_pair(0xA720, 0xA7FF), _("Latin Extended-D"));
        ranges.emplace_back(std::make_pair(0xA800, 0xA82F), _("Syloti Nagri"));
        ranges.emplace_back(std::make_pair(0xA830, 0xA83F), _("Common Indic Number Forms"));
        ranges.emplace_back(std::make_pair(0xA840, 0xA87F), _("Phags-pa"));
        ranges.emplace_back(std::make_pair(0xA880, 0xA8DF), _("Saurashtra"));
        ranges.emplace_back(std::make_pair(0xA8E0, 0xA8FF), _("Devanagari Extended"));
        ranges.emplace_back(std::make_pair(0xA900, 0xA92F), _("Kayah Li"));
        ranges.emplace_back(std::make_pair(0xA930, 0xA95F), _("Rejang"));
        ranges.emplace_back(std::make_pair(0xA960, 0xA97F), _("Hangul Jamo Extended-A"));
        ranges.emplace_back(std::make_pair(0xA980, 0xA9DF), _("Javanese"));
        ranges.emplace_back(std::make_pair(0xAA00, 0xAA5F), _("Cham"));
        ranges.emplace_back(std::make_pair(0xAA60, 0xAA7F), _("Myanmar Extended-A"));
        ranges.emplace_back(std::make_pair(0xAA80, 0xAADF), _("Tai Viet"));
        ranges.emplace_back(std::make_pair(0xABC0, 0xABFF), _("Meetei Mayek"));
        ranges.emplace_back(std::make_pair(0xAC00, 0xD7AF), _("Hangul Syllables"));
        ranges.emplace_back(std::make_pair(0xD7B0, 0xD7FF), _("Hangul Jamo Extended-B"));
        ranges.emplace_back(std::make_pair(0xD800, 0xDB7F), _("High Surrogates"));
        ranges.emplace_back(std::make_pair(0xDB80, 0xDBFF), _("High Private Use Surrogates"));
        ranges.emplace_back(std::make_pair(0xDC00, 0xDFFF), _("Low Surrogates"));
        ranges.emplace_back(std::make_pair(0xE000, 0xF8FF), _("Private Use Area"));
        ranges.emplace_back(std::make_pair(0xF900, 0xFAFF), _("CJK Compatibility Ideographs"));
        ranges.emplace_back(std::make_pair(0xFB00, 0xFB4F), _("Alphabetic Presentation Forms"));
        ranges.emplace_back(std::make_pair(0xFB50, 0xFDFF), _("Arabic Presentation Forms-A"));
        ranges.emplace_back(std::make_pair(0xFE00, 0xFE0F), _("Variation Selectors"));
        ranges.emplace_back(std::make_pair(0xFE10, 0xFE1F), _("Vertical Forms"));
        ranges.emplace_back(std::make_pair(0xFE20, 0xFE2F), _("Combining Half Marks"));
        ranges.emplace_back(std::make_pair(0xFE30, 0xFE4F), _("CJK Compatibility Forms"));
        ranges.emplace_back(std::make_pair(0xFE50, 0xFE6F), _("Small Form Variants"));
        ranges.emplace_back(std::make_pair(0xFE70, 0xFEFF), _("Arabic Presentation Forms-B"));
        ranges.emplace_back(std::make_pair(0xFF00, 0xFFEF), _("Halfwidth and Fullwidth Forms"));
        ranges.emplace_back(std::make_pair(0xFFF0, 0xFFFF), _("Specials"));

        // Selected ranges in Extended Multilingual Plane
        ranges.emplace_back(std::make_pair(0x1F300, 0x1F5FF), _("Miscellaneous Symbols and Pictographs"));
        ranges.emplace_back(std::make_pair(0x1F600, 0x1F64F), _("Emoticons"));
        ranges.emplace_back(std::make_pair(0x1F650, 0x1F67F), _("Ornamental Dingbats"));
        ranges.emplace_back(std::make_pair(0x1F680, 0x1F6FF), _("Transport and Map Symbols"));
        ranges.emplace_back(std::make_pair(0x1F700, 0x1F77F), _("Alchemical Symbols"));
        ranges.emplace_back(std::make_pair(0x1F780, 0x1F7FF), _("Geometric Shapes Extended"));
        ranges.emplace_back(std::make_pair(0x1F800, 0x1F8FF), _("Supplemental Arrows-C"));
        ranges.emplace_back(std::make_pair(0x1F900, 0x1F9FF), _("Supplemental Symbols and Pictographs"));
        ranges.emplace_back(std::make_pair(0x1FA00, 0x1FA7F), _("Chess Symbols"));
        ranges.emplace_back(std::make_pair(0x1FA80, 0x1FAFF), _("Symbols and Pictographs Extended-A"));

    }

    return ranges;
}

class GlyphColumns : public Gtk::TreeModel::ColumnRecord
{
public:
    Gtk::TreeModelColumn<gunichar> code;
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> tooltip;

    GlyphColumns()
    {
        add(code);
        add(name);
        add(tooltip);
    }
};

GlyphColumns *GlyphsPanel::getColumns()
{
    static GlyphColumns *columns = new GlyphColumns();

    return columns;
}

/**
 * Constructor
 */
GlyphsPanel::GlyphsPanel() :
    Inkscape::UI::Widget::Panel("/dialogs/glyphs", SP_VERB_DIALOG_GLYPHS),
    store(Gtk::ListStore::create(*getColumns())),
    deskTrack(),
    instanceConns(),
    desktopConns()
{
    auto table = new Gtk::Grid();
    table->set_row_spacing(4);
    table->set_column_spacing(4);
    _getContents()->pack_start(*Gtk::manage(table), Gtk::PACK_EXPAND_WIDGET);
    guint row = 0;

// -------------------------------

    {
        fontSelector = new Inkscape::UI::Widget::FontSelector (false, false);
        fontSelector->set_name ("UnicodeCharacters");

        sigc::connection conn =
            fontSelector->connectChanged(sigc::hide(sigc::mem_fun(*this, &GlyphsPanel::rebuild)));
        instanceConns.push_back(conn);

        table->attach(*Gtk::manage(fontSelector), 0, row, 3, 1);
        row++;
    }

// -------------------------------

    {
        auto label = new Gtk::Label(_("Script: "));

        table->attach( *Gtk::manage(label), 0, row, 1, 1);

        scriptCombo = Gtk::manage(new Gtk::ComboBoxText());
        for (auto & it : getScriptToName())
        {
            scriptCombo->append(it.second);
        }

        scriptCombo->set_active_text(getScriptToName()[G_UNICODE_SCRIPT_INVALID_CODE]);
        sigc::connection conn = scriptCombo->signal_changed().connect(sigc::mem_fun(*this, &GlyphsPanel::rebuild));
        instanceConns.push_back(conn);

        scriptCombo->set_halign(Gtk::ALIGN_START);
        scriptCombo->set_valign(Gtk::ALIGN_START);
        scriptCombo->set_hexpand();
        table->attach(*scriptCombo, 1, row, 1, 1);
    }

    row++;

// -------------------------------

    {
        auto label = new Gtk::Label(_("Range: "));
        table->attach( *Gtk::manage(label), 0, row, 1, 1);

        rangeCombo = Gtk::manage(new Gtk::ComboBoxText());
        for (auto & it : getRanges()) {
            rangeCombo->append(it.second);
        }

        rangeCombo->set_active_text(getRanges()[4].second);
        sigc::connection conn = rangeCombo->signal_changed().connect(sigc::mem_fun(*this, &GlyphsPanel::rebuild));
        instanceConns.push_back(conn);

        rangeCombo->set_halign(Gtk::ALIGN_START);
        rangeCombo->set_valign(Gtk::ALIGN_START);
        rangeCombo->set_hexpand();
        table->attach(*rangeCombo, 1, row, 1, 1);
    }

    row++;

// -------------------------------

    GlyphColumns *columns = getColumns();

    iconView = new Gtk::IconView(static_cast<Glib::RefPtr<Gtk::TreeModel> >(store));
    iconView->set_name("UnicodeIconView");
    iconView->set_markup_column(columns->name);
    iconView->set_tooltip_column(2); // Uses Pango markup, must use column number.
    iconView->set_margin(0);
    iconView->set_item_padding(0);
    iconView->set_row_spacing(0);
    iconView->set_column_spacing(0);

    sigc::connection conn;
    conn = iconView->signal_item_activated().connect(sigc::mem_fun(*this, &GlyphsPanel::glyphActivated));
    instanceConns.push_back(conn);
    conn = iconView->signal_selection_changed().connect(sigc::mem_fun(*this, &GlyphsPanel::glyphSelectionChanged));
    instanceConns.push_back(conn);


    Gtk::ScrolledWindow *scroller = new Gtk::ScrolledWindow();
    scroller->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    scroller->add(*Gtk::manage(iconView));
    scroller->set_hexpand();
    scroller->set_vexpand();
    table->attach(*Gtk::manage(scroller), 0, row, 3, 1);

    row++;

// -------------------------------

    Gtk::HBox *box = new Gtk::HBox();

    entry = new Gtk::Entry();
    conn = entry->signal_changed().connect(sigc::mem_fun(*this, &GlyphsPanel::calcCanInsert));
    instanceConns.push_back(conn);
    entry->set_width_chars(18);
    box->pack_start(*Gtk::manage(entry), Gtk::PACK_SHRINK);

    Gtk::Label *pad = new Gtk::Label("    ");
    box->pack_start(*Gtk::manage(pad), Gtk::PACK_SHRINK);

    label = new Gtk::Label("      ");
    box->pack_start(*Gtk::manage(label), Gtk::PACK_SHRINK);

    pad = new Gtk::Label("");
    box->pack_start(*Gtk::manage(pad), Gtk::PACK_EXPAND_WIDGET);

    insertBtn = new Gtk::Button(_("Append"));
    conn = insertBtn->signal_clicked().connect(sigc::mem_fun(*this, &GlyphsPanel::insertText));
    instanceConns.push_back(conn);
    insertBtn->set_can_default();
    insertBtn->set_sensitive(false);

    box->pack_end(*Gtk::manage(insertBtn), Gtk::PACK_SHRINK);
    box->set_hexpand();
    table->attach( *Gtk::manage(box), 0, row, 3, 1);

    row++;

// -------------------------------


    show_all_children();

    // Connect this up last
    conn = deskTrack.connectDesktopChanged( sigc::mem_fun(*this, &GlyphsPanel::setTargetDesktop) );
    instanceConns.push_back(conn);
    deskTrack.connect(GTK_WIDGET(gobj()));
}

GlyphsPanel::~GlyphsPanel()
{
    for (auto & instanceConn : instanceConns) {
        instanceConn.disconnect();
    }
    instanceConns.clear();
    for (auto & desktopConn : desktopConns) {
        desktopConn.disconnect();
    }
    desktopConns.clear();
}


void GlyphsPanel::setDesktop(SPDesktop *desktop)
{
    Panel::setDesktop(desktop);
    deskTrack.setBase(desktop);
}

void GlyphsPanel::setTargetDesktop(SPDesktop *desktop)
{
    if (targetDesktop != desktop) {
        if (targetDesktop) {
            for (auto & desktopConn : desktopConns) {
                desktopConn.disconnect();
            }
            desktopConns.clear();
        }

        targetDesktop = desktop;

        if (targetDesktop && targetDesktop->selection) {
            sigc::connection conn = desktop->selection->connectChanged(sigc::hide(sigc::bind(sigc::mem_fun(*this, &GlyphsPanel::readSelection), true, true)));
            desktopConns.push_back(conn);

            // Text selection within selected items has changed:
            conn = desktop->connectToolSubselectionChanged(sigc::hide(sigc::bind(sigc::mem_fun(*this, &GlyphsPanel::readSelection), true, false)));
            desktopConns.push_back(conn);

            // Must check flags, so can't call performUpdate() directly.
            conn = desktop->selection->connectModified(sigc::hide<0>(sigc::mem_fun(*this, &GlyphsPanel::selectionModifiedCB)));
            desktopConns.push_back(conn);

            readSelection(true, true);
        }
    }
}

// Append selected glyphs to selected text
void GlyphsPanel::insertText()
{
    SPItem *textItem = nullptr;
    auto itemlist= targetDesktop->selection->items();
        for(auto i=itemlist.begin(); itemlist.end() != i; ++i) {
            if (SP_IS_TEXT(*i) || SP_IS_FLOWTEXT(*i)) {
            textItem = *i;
            break;
        }
    }

    if (textItem) {
        Glib::ustring glyphs;
        if (entry->get_text_length() > 0) {
            glyphs = entry->get_text();
        } else {
            auto itemArray = iconView->get_selected_items();

            if (!itemArray.empty()) {
                Gtk::TreeModel::Path const & path = *itemArray.begin();
                Gtk::ListStore::iterator row = store->get_iter(path);
                gunichar ch = (*row)[getColumns()->code];
                glyphs = ch;
            }
        }

        if (!glyphs.empty()) {
            Glib::ustring combined;
            gchar *str = sp_te_get_string_multiline(textItem);
            if (str) {
                combined = str;
                g_free(str);
                str = nullptr;
            }
            combined += glyphs;
            sp_te_set_repr_text_multiline(textItem, combined.c_str());
            DocumentUndo::done(targetDesktop->doc(), SP_VERB_CONTEXT_TEXT, _("Append text"));
        }
    }
}

void GlyphsPanel::glyphActivated(Gtk::TreeModel::Path const & path)
{
    Gtk::ListStore::iterator row = store->get_iter(path);
    gunichar ch = (*row)[getColumns()->code];
    Glib::ustring tmp;
    tmp += ch;

    int startPos = 0;
    int endPos = 0;
    if (entry->get_selection_bounds(startPos, endPos)) {
        // there was something selected.
        entry->delete_text(startPos, endPos);
    }
    startPos = entry->get_position();
    entry->insert_text(tmp, -1, startPos);
    entry->set_position(startPos);
}

void GlyphsPanel::glyphSelectionChanged()
{
    auto itemArray = iconView->get_selected_items();

    if (itemArray.empty()) {
        label->set_text("      ");
    } else {
        Gtk::TreeModel::Path const & path = *itemArray.begin();
        Gtk::ListStore::iterator row = store->get_iter(path);
        gunichar ch = (*row)[getColumns()->code];


        Glib::ustring scriptName;
        GUnicodeScript script = g_unichar_get_script(ch);
        std::map<GUnicodeScript, Glib::ustring> mappings = getScriptToName();
        if (mappings.find(script) != mappings.end()) {
            scriptName = mappings[script];
        }
        gchar * tmp = g_strdup_printf("U+%04X %s", ch, scriptName.c_str());
        label->set_text(tmp);
    }
    calcCanInsert();
}

void GlyphsPanel::selectionModifiedCB(guint flags)
{
    bool style = ((flags & ( SP_OBJECT_CHILD_MODIFIED_FLAG |
                             SP_OBJECT_STYLE_MODIFIED_FLAG  )) != 0 );

    bool content = ((flags & ( SP_OBJECT_CHILD_MODIFIED_FLAG |
                               SP_TEXT_CONTENT_MODIFIED_FLAG  )) != 0 );

    readSelection(style, content);
}

void GlyphsPanel::calcCanInsert()
{
    int items = 0;
    auto itemlist= targetDesktop->selection->items();
    for(auto i=itemlist.begin(); itemlist.end() != i; ++i) {
        if (SP_IS_TEXT(*i) || SP_IS_FLOWTEXT(*i)) {
            ++items;
        }
    }

    bool enable = (items == 1);
    if (enable) {
        enable &= (!iconView->get_selected_items().empty()
                   || (entry->get_text_length() > 0));
    }

    if (enable != insertBtn->is_sensitive()) {
        insertBtn->set_sensitive(enable);
    }
}

void GlyphsPanel::readSelection( bool updateStyle, bool updateContent )
{
    calcCanInsert();

    if (updateStyle) {
        Inkscape::FontLister* fontlister = Inkscape::FontLister::get_instance();

        // Update family/style based on selection.
        fontlister->selection_update();

        // Update GUI (based on fontlister values).
        fontSelector->update_font ();
    }
}


void GlyphsPanel::rebuild()
{
    Glib::ustring fontspec = fontSelector->get_fontspec();

    font_instance* font = nullptr;
    if( !fontspec.empty() ) {
        font = font_factory::Default()->FaceFromFontSpecification( fontspec.c_str() );
    }

    if (font) {

        GUnicodeScript script = G_UNICODE_SCRIPT_INVALID_CODE;
        Glib::ustring scriptName = scriptCombo->get_active_text();
        std::map<GUnicodeScript, Glib::ustring> items = getScriptToName();
        for (auto & item : items) {
            if (scriptName == item.second) {
                script = item.first;
                break;
            }
        }

        // Disconnect the model while we update it. Simple work-around for 5x+ performance boost.
        Glib::RefPtr<Gtk::ListStore> tmp = Gtk::ListStore::create(*getColumns());
        iconView->set_model(tmp);

        gunichar lower = 0x00001;
        gunichar upper = 0x2FFFF;
        int active = rangeCombo->get_active_row_number();
        if (active >= 0) {
            lower = getRanges()[active].first.first;
            upper = getRanges()[active].first.second;
        }
        std::vector<gunichar> present;
        for (gunichar ch = lower; ch <= upper; ch++) {
            int glyphId = font->MapUnicodeChar(ch);
            if (glyphId > 0) {
                if ((script == G_UNICODE_SCRIPT_INVALID_CODE) || (script == g_unichar_get_script(ch))) {
                    present.push_back(ch);
                }
            }
        }

        GlyphColumns *columns = getColumns();
        store->clear();
        for (unsigned int & it : present)
        {
            Gtk::ListStore::iterator row = store->append();
            Glib::ustring tmp;
            tmp += it;
            tmp = Glib::Markup::escape_text(tmp);  // Escape '&', '<', etc.
            (*row)[columns->code] = it;
            (*row)[columns->name]    = "<span font_desc=\"" + fontspec +                "\">" + tmp + "</span>";
            (*row)[columns->tooltip] = "<span font_desc=\"" + fontspec + "\" size=\"42000\">" + tmp + "</span>";
        }

        // Reconnect the model once it has been updated:
        iconView->set_model(store);
    }
}


} // namespace Dialogs
} // namespace UI
} // namespace Inkscape

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
