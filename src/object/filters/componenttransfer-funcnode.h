// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SP_FECOMPONENTTRANSFER_FUNCNODE_H_SEEN
#define SP_FECOMPONENTTRANSFER_FUNCNODE_H_SEEN

/** \file
 * SVG <filter> implementation, see sp-filter.cpp.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "../sp-object.h"
#include "display/nr-filter-component-transfer.h"

#define SP_FEFUNCNODE(obj) (dynamic_cast<SPFeFuncNode*>((SPObject*)obj))

class SPFeFuncNode : public SPObject {
public:
    enum Channel {
        R, G, B, A
    };

	SPFeFuncNode(Channel channel);
	~SPFeFuncNode() override;

    Inkscape::Filters::FilterComponentTransferType type;
    std::vector<double> tableValues;
    double slope;
    double intercept;
    double amplitude;
    double exponent;
    double offset;
    Channel channel;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttributeEnum key, const gchar* value) override;

	void update(SPCtx* ctx, unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) override;
};

#endif /* !SP_FECOMPONENTTRANSFER_FUNCNODE_H_SEEN */

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
