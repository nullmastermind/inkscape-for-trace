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

#include "channel.h"
#include <Magick++.h>

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {
	
void
Channel::applyEffect(Magick::Image *image) {	
	Magick::ChannelType layer = Magick::UndefinedChannel;
	if (!strcmp(_layerName,      "Red Channel"))		layer = Magick::RedChannel;
	else if (!strcmp(_layerName, "Green Channel"))		layer = Magick::GreenChannel;
	else if (!strcmp(_layerName, "Blue Channel"))		layer = Magick::BlueChannel;
	else if (!strcmp(_layerName, "Cyan Channel"))		layer = Magick::CyanChannel;
	else if (!strcmp(_layerName, "Magenta Channel"))	layer = Magick::MagentaChannel;
	else if (!strcmp(_layerName, "Yellow Channel"))		layer = Magick::YellowChannel;
	else if (!strcmp(_layerName, "Black Channel"))		layer = Magick::BlackChannel;
	else if (!strcmp(_layerName, "Opacity Channel"))	layer = Magick::OpacityChannel;
	else if (!strcmp(_layerName, "Matte Channel"))		layer = Magick::MatteChannel;		
	
	image->channel(layer);
}

void
Channel::refreshParameters(Inkscape::Extension::Effect *module) {	
	_layerName = module->get_param_optiongroup("layer");
}

#include "../clear-n_.h"

void
Channel::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Channel") "</name>\n"
            "<id>org.inkscape.effect.bitmap.channel</id>\n"
            "<param name=\"layer\" gui-text=\"" N_("Layer:") "\" type=\"optiongroup\" appearance=\"combo\" >\n"
                "<option value='Red Channel'>" N_("Red Channel") "</option>\n"
                "<option value='Green Channel'>" N_("Green Channel") "</option>\n"
                "<option value='Blue Channel'>" N_("Blue Channel") "</option>\n"
                "<option value='Cyan Channel'>" N_("Cyan Channel") "</option>\n"
                "<option value='Magenta Channel'>" N_("Magenta Channel") "</option>\n"
                "<option value='Yellow Channel'>" N_("Yellow Channel") "</option>\n"
                "<option value='Black Channel'>" N_("Black Channel") "</option>\n"
                "<option value='Opacity Channel'>" N_("Opacity Channel") "</option>\n"
                "<option value='Matte Channel'>" N_("Matte Channel") "</option>\n"
            "</param>\n"
            "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                    "<submenu name=\"" N_("Raster") "\" />\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Extract specific channel from image") "</menu-tip>\n"
            "</effect>\n"
        "</inkscape-extension>\n", new Channel());
    // clang-format on
}

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
