// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 * Authors:
 *   Michael Forbes <miforbes-inkscape@mbhs.edu>
 *
 * Copyright (C) 2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_LATEX_OUT_H
#define EXTENSION_INTERNAL_LATEX_OUT_H

#include "extension/implementation/implementation.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

class LatexOutput : Inkscape::Extension::Implementation::Implementation { //This is a derived class
    
public:
    LatexOutput(); // Empty constructor

    ~LatexOutput() override;//Destructor

    bool check(Inkscape::Extension::Extension *module) override; //Can this module load (always yes for now)

    void save(Inkscape::Extension::Output *mod, // Save the given document to the given filename
              SPDocument *doc,
              gchar const *filename) override;

    static void init();//Initialize the class
};

} } }  /* namespace Inkscape, Extension, Implementation */

#endif /* EXTENSION_INTERNAL_LATEX_OUT_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
