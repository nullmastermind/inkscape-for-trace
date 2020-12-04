// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief SVG Gaussian blur filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_GAUSSIANBLUR_H_SEEN
#define SP_GAUSSIANBLUR_H_SEEN

#include "sp-filter-primitive.h"
#include "number-opt-number.h"

#define SP_GAUSSIANBLUR(obj) (dynamic_cast<SPGaussianBlur*>((SPObject*)obj))
#define SP_IS_GAUSSIANBLUR(obj) (dynamic_cast<const SPGaussianBlur*>((SPObject*)obj) != NULL)

class SPGaussianBlur : public SPFilterPrimitive {
public:
	SPGaussianBlur();
	~SPGaussianBlur() override;

        /** stdDeviation attribute */
        NumberOptNumber stdDeviation;

        Geom::Rect calculate_region(Geom::Rect region) override;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttr key, const gchar* value) override;

	void update(SPCtx* ctx, unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) override;

	void build_renderer(Inkscape::Filters::Filter* filter) override;
};

void  sp_gaussianBlur_setDeviation(SPGaussianBlur *blur, float num);
void  sp_gaussianBlur_setDeviation(SPGaussianBlur *blur, float num, float optnum);

#endif /* !SP_GAUSSIANBLUR_H_SEEN */

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
