// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2007 Authors:
 *   Christopher Brown <audiere@gmail.com>
 *   Ted Gould <ted@gould.cx>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extension/effect.h"
#include "extension/system.h"

#include "addNoise.h"
#include <Magick++.h>

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {
	
void
AddNoise::applyEffect(Magick::Image *image) {
	Magick::NoiseType noiseType = Magick::UniformNoise;
	if (!strcmp(_noiseTypeName,      "Uniform Noise"))		noiseType = Magick::UniformNoise;
	else if (!strcmp(_noiseTypeName, "Gaussian Noise"))		noiseType = Magick::GaussianNoise;
	else if (!strcmp(_noiseTypeName, "Multiplicative Gaussian Noise"))	noiseType = Magick::MultiplicativeGaussianNoise;
	else if (!strcmp(_noiseTypeName, "Impulse Noise"))		noiseType = Magick::ImpulseNoise;
	else if (!strcmp(_noiseTypeName, "Laplacian Noise"))	noiseType = Magick::LaplacianNoise;
	else if (!strcmp(_noiseTypeName, "Poisson Noise"))		noiseType = Magick::PoissonNoise;	
	
	image->addNoise(noiseType);
}

void
AddNoise::refreshParameters(Inkscape::Extension::Effect *module) {	
	_noiseTypeName = module->get_param_optiongroup("noiseType");
}

#include "../clear-n_.h"

void
AddNoise::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Add Noise") "</name>\n"
            "<id>org.inkscape.effect.bitmap.addNoise</id>\n"
            "<param name=\"noiseType\" gui-text=\"" N_("Type:") "\" type=\"optiongroup\" appearance=\"combo\" >\n"
                "<option value='Uniform Noise'>" N_("Uniform Noise") "</option>\n"
                "<option value='Gaussian Noise'>" N_("Gaussian Noise") "</option>\n"
                "<option value='Multiplicative Gaussian Noise'>" N_("Multiplicative Gaussian Noise") "</option>\n"
                "<option value='Impulse Noise'>" N_("Impulse Noise") "</option>\n"
                "<option value='Laplacian Noise'>" N_("Laplacian Noise") "</option>\n"
                "<option value='Poisson Noise'>" N_("Poisson Noise") "</option>\n"
            "</param>\n"
            "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                    "<submenu name=\"" N_("Raster") "\" />\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Add random noise to selected bitmap(s)") "</menu-tip>\n"
            "</effect>\n"
        "</inkscape-extension>\n", new AddNoise());
    // clang-format on
}

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
