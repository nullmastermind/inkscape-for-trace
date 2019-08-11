// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "patheffect.h"

#include "db.h"

#include "object/sp-defs.h"

#include "xml/repr.h"


namespace Inkscape {
namespace Extension {

PathEffect::PathEffect (Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory)
    : Extension(in_repr, in_imp, base_directory)
{

}

PathEffect::~PathEffect (void)
= default;

void
PathEffect::processPath (SPDocument * /*doc*/, Inkscape::XML::Node * /*path*/, Inkscape::XML::Node * /*def*/)
{


}

void
PathEffect::processPathEffects (SPDocument * doc, Inkscape::XML::Node * path)
{
    gchar const * patheffectlist = path->attribute("inkscape:path-effects");
    if (patheffectlist == nullptr)
        return;

    gchar ** patheffects = g_strsplit(patheffectlist, ";", 128);
    Inkscape::XML::Node * defs = doc->getDefs()->getRepr();

    for (int i = 0; (i < 128) && (patheffects[i] != nullptr); i++) {
        gchar * patheffect = patheffects[i];

        // This is weird, they should all be references... but anyway
        if (patheffect[0] != '#') continue;

        Inkscape::XML::Node * prefs = sp_repr_lookup_child(defs, "id", &(patheffect[1]));
        if (prefs == nullptr) {

            continue;
        }

        gchar const * ext_id = prefs->attribute("extension");
        if (ext_id == nullptr) {

            continue;
        }

        Inkscape::Extension::PathEffect * peffect;
        peffect = dynamic_cast<Inkscape::Extension::PathEffect *>(Inkscape::Extension::db.get(ext_id));
        if (peffect != nullptr) {
            peffect->processPath(doc, path, prefs);
        }
    }

    g_strfreev(patheffects);
    return;
}


} }  /* namespace Inkscape, Extension */

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
