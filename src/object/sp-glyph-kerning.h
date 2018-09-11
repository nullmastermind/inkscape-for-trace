// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <hkern> and <vkern> elements implementation
 *
 * Authors:
 *    Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Felipe C. da S. Sanches
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_GLYPH_KERNING_H
#define SEEN_SP_GLYPH_KERNING_H

#include "sp-object.h"
#include "unicoderange.h"

#define SP_HKERN(obj) (dynamic_cast<SPHkern*>(obj))
#define SP_IS_HKERN(obj) (dynamic_cast<const SPHkern*>(obj) != NULL)

#define SP_VKERN(obj) (dynamic_cast<SPVkern*>(obj))
#define SP_IS_VKERN(obj) (dynamic_cast<const SPVkern*>(obj) != NULL)

// CPPIFY: These casting macros are buggy, as Vkern and Hkern aren't "real" classes.

class GlyphNames {
public: 
    GlyphNames(char const* value);
    ~GlyphNames();
    bool contains(char const* name);
private:
    char* names;
};

class SPGlyphKerning : public SPObject {
public:
    SPGlyphKerning();
    ~SPGlyphKerning() override = default;

    // FIXME encapsulation
    UnicodeRange* u1;
    GlyphNames* g1;
    UnicodeRange* u2;
    GlyphNames* g2;
    double k;

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void set(SPAttributeEnum key, char const* value) override;
    void update(SPCtx* ctx, unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

class SPHkern : public SPGlyphKerning {
    ~SPHkern() override = default;
};

class SPVkern : public SPGlyphKerning {
    ~SPVkern() override = default;
};

#endif // !SEEN_SP_GLYPH_KERNING_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
