/** @file
 * @brief Enhanced Metafile Input/Output
 */
/* Authors:
 *   Ulf Erikson <ulferikson@users.sf.net>
 *
 * Copyright (C) 2006-2008 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SEEN_EXTENSION_INTERNAL_EMF_H
#define SEEN_EXTENSION_INTERNAL_EMF_H

#include "extension/implementation/implementation.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

class Emf : Inkscape::Extension::Implementation::Implementation { //This is a derived class

public:
    Emf(); // Empty constructor

    virtual ~Emf();//Destructor

    bool check(Inkscape::Extension::Extension *module); //Can this module load (always yes for now)

    void save(Inkscape::Extension::Output *mod, // Save the given document to the given filename
              SPDocument *doc,
              gchar const *filename);

    virtual SPDocument *open( Inkscape::Extension::Input *mod,
                                const gchar *uri );

    static void init(void);//Initialize the class

private:
};

} } }  /* namespace Inkscape, Extension, Implementation */


#endif /* EXTENSION_INTERNAL_EMF_H */

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
