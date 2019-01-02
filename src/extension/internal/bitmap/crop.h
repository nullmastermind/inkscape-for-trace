// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2010 Authors:
 *   Christopher Brown <audiere@gmail.com>
 *   Ted Gould <ted@gould.cx>
 *   Nicolas Dufour <nicoduf@yahoo.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "imagemagick.h"

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {

class Crop : public ImageMagick
{
private:
    int _top;
    int _bottom;
    int _left;
    int _right;
public:
    void applyEffect(Magick::Image *image) override;
    void postEffect(Magick::Image *image, SPItem *item) override;
    void refreshParameters(Inkscape::Extension::Effect *module) override;
    static void init ();
};

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
