#!/usr/bin/env python
# coding=utf-8
'''
hpgl_input.py - input a HP Graphics Language file

Copyright (C) 2013 Sebastian Wüst, sebi@timewaster.de, http://www.timewasters-place.com/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
'''

# standard library
from StringIO import StringIO
# local library
import hpgl_decoder, inkex
inkex.localize()


# parse options
parser = inkex.optparse.OptionParser(usage="usage: %prog [options] HPGLfile", option_class=inkex.InkOption)
parser.add_option("--resolutionX",   action="store", type="float",   dest="resolutionX",   default=1016.0,  help="Resolution X (dpi)")
parser.add_option("--resolutionY",   action="store", type="float",   dest="resolutionY",   default=1016.0,  help="Resolution Y (dpi)")
parser.add_option("--showMovements", action="store", type="inkbool", dest="showMovements", default="FALSE", help="Show Movements between paths")
(options, args) = parser.parse_args(inkex.sys.argv[1:])
# needed to initialize the document
options.docWidth = 210.0 * 3.5433070866 # 210mm to pixels (DIN A4)
options.docHeight = 297.0 * 3.5433070866 # 297mm to pixels (DIN A4)

# TODO:2013-07-13:Sebastian Wüst:Load the whole file and try to parse all the different HPGL formats correctly.
# read file (read only one line, there should not be more than one line)
stream = open(args[0], 'r')
data = stream.readline().strip()

# interpret data
myHpglDecoder = hpgl_decoder.hpglDecoder(options)
# TODO:2013-07-13:Sebastian Wüst:Find a better way to pass errors correctly, think about how data is passed.
(hasUnknownCommands, hasNoHpglData, doc) = myHpglDecoder.getSvg(data)

# issue warning if no hpgl data found
if hasNoHpglData:
    inkex.errormsg(_("No HPGL data found."))
    doc = inkex.etree.parse(StringIO('<svg xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" width="%s" height="%s"></svg>' % (options.docWidth, options.docHeight)))

# issue warning if unknown commands where found
if hasUnknownCommands:
    inkex.errormsg(_("The HPGL data contained unknown (unsupported) commands, there is a possibility that the drawing is missing some content."))

# deliver document to inkscape 
doc.write(inkex.sys.stdout)

# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99