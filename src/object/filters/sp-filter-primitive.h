// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_FILTER_PRIMITIVE_H
#define SEEN_SP_FILTER_PRIMITIVE_H

/** \file
 * Document level base class for all SVG filter primitives.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "../sp-object.h"
#include "../sp-dimensions.h"

#define SP_FILTER_PRIMITIVE(obj) (dynamic_cast<SPFilterPrimitive*>((SPObject*)obj))
#define SP_IS_FILTER_PRIMITIVE(obj) (dynamic_cast<const SPFilterPrimitive*>((SPObject*)obj) != NULL)

namespace Inkscape {
namespace Filters {
class Filter;
class FilterPrimitive;
} }

class SPFilterPrimitive : public SPObject, public SPDimensions {
public:
	SPFilterPrimitive();
	~SPFilterPrimitive() override;

    int image_in, image_out;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttributeEnum key, char const* value) override;

	void update(SPCtx* ctx, unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;

public:
	virtual void build_renderer(Inkscape::Filters::Filter* filter) = 0;

	/* Common initialization for filter primitives */
	void renderer_common(Inkscape::Filters::FilterPrimitive *nr_prim);

	int name_previous_out();
	int read_in(char const *name);
	int read_result(char const *name);
};

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
