// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_SP_TEXTPATH_H
#define INKSCAPE_SP_TEXTPATH_H

#include "svg/svg-length.h"
#include "sp-item.h"
#include "sp-text.h"

class SPUsePath;
class Path;

#define SP_TEXTPATH(obj) (dynamic_cast<SPTextPath*>((SPObject*)obj))
#define SP_IS_TEXTPATH(obj) (dynamic_cast<const SPTextPath*>((SPObject*)obj) != NULL)

enum TextPathSide {
    SP_TEXT_PATH_SIDE_LEFT,
    SP_TEXT_PATH_SIDE_RIGHT
};

class SPTextPath : public SPItem {
public:
    SPTextPath();
    ~SPTextPath() override;

    TextTagAttributes attributes;
    SVGLength startOffset;
    TextPathSide side;

    Path *originalPath;
    bool isUpdating;
    SPUsePath *sourcePath;

    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void set(SPAttributeEnum key, const char* value) override;
    void update(SPCtx* ctx, unsigned int flags) override;
    void modified(unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

#define SP_IS_TEXT_TEXTPATH(obj) (SP_IS_TEXT(obj) && obj->firstChild() && SP_IS_TEXTPATH(obj->firstChild()))

SPItem *sp_textpath_get_path_item(SPTextPath *tp);
void sp_textpath_to_text(SPObject *tp);


#endif /* !INKSCAPE_SP_TEXTPATH_H */

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
