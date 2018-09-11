// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * @brief SVG morphology filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_FEMORPHOLOGY_H_SEEN
#define SP_FEMORPHOLOGY_H_SEEN

#include "sp-filter-primitive.h"
#include "number-opt-number.h"
#include "display/nr-filter-morphology.h"

#define SP_FEMORPHOLOGY(obj) (dynamic_cast<SPFeMorphology*>((SPObject*)obj))
#define SP_IS_FEMORPHOLOGY(obj) (dynamic_cast<const SPFeMorphology*>((SPObject*)obj) != NULL)

class SPFeMorphology : public SPFilterPrimitive {
public:
	SPFeMorphology();
	~SPFeMorphology() override;

    Inkscape::Filters::FilterMorphologyOperator Operator;
    NumberOptNumber radius;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttributeEnum key, const gchar* value) override;

	void update(SPCtx* ctx, unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) override;

	void build_renderer(Inkscape::Filters::Filter* filter) override;
};

#endif /* !SP_FEMORPHOLOGY_H_SEEN */

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
