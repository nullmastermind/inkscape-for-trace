#!/usr/bin/env python
# coding=utf-8 
'''
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
import sys
# local library
import gettext, hpgl_decoder, hpgl_encoder, inkex
inkex.localize()


class MyEffect(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option('--tab',              action='store', type='string',  dest='tab')
        self.OptionParser.add_option('--resolutionX',      action='store', type='float',   dest='resolutionX',      default=1016.0,  help='Resolution X (dpi)')
        self.OptionParser.add_option('--resolutionY',      action='store', type='float',   dest='resolutionY',      default=1016.0,  help='Resolution Y (dpi)')
        self.OptionParser.add_option('--pen',              action='store', type='int',     dest='pen',              default=1,       help='Pen number')
        self.OptionParser.add_option('--orientation',      action='store', type='string',  dest='orientation',      default='90',    help='orientation')
        self.OptionParser.add_option('--mirrorX',          action='store', type='inkbool', dest='mirrorX',          default='FALSE', help='Mirror X-axis')
        self.OptionParser.add_option('--mirrorY',          action='store', type='inkbool', dest='mirrorY',          default='FALSE', help='Mirror Y-axis')
        self.OptionParser.add_option('--center',           action='store', type='inkbool', dest='center',           default='FALSE', help='Center zero point')
        self.OptionParser.add_option('--flat',             action='store', type='float',   dest='flat',             default=1.2,     help='Curve flatness')
        self.OptionParser.add_option('--useOvercut',       action='store', type='inkbool', dest='useOvercut',       default='TRUE',  help='Use overcut')
        self.OptionParser.add_option('--overcut',          action='store', type='float',   dest='overcut',          default=1.0,     help='Overcut (mm)')
        self.OptionParser.add_option('--useToolOffset',    action='store', type='inkbool', dest='useToolOffset',    default='TRUE',  help='Correct tool offset')
        self.OptionParser.add_option('--toolOffset',       action='store', type='float',   dest='toolOffset',       default=0.25,    help='Tool offset (mm)')
        self.OptionParser.add_option('--toolOffsetReturn', action='store', type='float',   dest='toolOffsetReturn', default=2.5,     help='Return factor')
        self.OptionParser.add_option('--precut',           action='store', type='inkbool', dest='precut',           default='TRUE',  help='Use precut')
        self.OptionParser.add_option('--offsetX',          action='store', type='float',   dest='offsetX',          default=0.0,     help='X offset (mm)')
        self.OptionParser.add_option('--offsetY',          action='store', type='float',   dest='offsetY',          default=0.0,     help='Y offset (mm)')
        self.OptionParser.add_option('--serialPort',       action='store', type='string',  dest='serialPort',       default='COM1',  help='Serial port')
        self.OptionParser.add_option('--serialBaudRate',   action='store', type='string',  dest='serialBaudRate',   default='9600',  help='Serial Baud rate')

    def effect(self):
        # gracefully exit script when pySerial is missing
        try:
            import serial
        except ImportError, e:
            inkex.errormsg(_("pySerial is not installed."
                + "\n\n1. Download pySerial here: http://pypi.python.org/pypi/pyserial"
                + "\n2. Extract the \"serial\" subfolder from the zip to the following folder: \"C:\\Program Files (x86)\\inkscape\\python\\Lib\\\" (Or wherever your Inkscape is installed to)"
                + "\n3. Restart Inkscape."))
            return
        # get hpgl data 
        myHpglEncoder = hpgl_encoder.hpglEncoder(self.document.getroot(), self.options)
        try:
            self.hpgl = myHpglEncoder.getHpgl()
        except Exception as inst:
            if inst.args[0] == 'NO_PATHS':
                # issue error if no paths found
                inkex.errormsg(_("No paths where found. Please convert all objects you want to plot into paths."))
                return 1
            else:
                raise Exception(inst)
        # TODO:2013-07-13:Sebastian Wüst:Get preview to work. This requires some work on the C++ side to be able to determine if it is a preview or a final run.
        '''
            # reparse data for preview
            self.options.showMovements = True
            self.options.docWidth = float(inkex.unittouu(self.document.getroot().get('width')))
            self.options.docHeight = float(inkex.unittouu(self.document.getroot().get('height')))
            myHpglDecoder = hpgl_decoder.hpglDecoder(self.hpgl, self.options)
            try:
                doc, warnings = myHpglDecoder.getSvg()
                # deliver document to inkscape
                self.document = doc 
            except Exception as inst:
                if inst.args[0] == 'NO_HPGL_DATA':
                    # do nothing
                else:
                    raise Exception(inst)
        '''
        # send data to plotter
        # TODO:2013-07-13:Sebastian Wüst:Somehow slow down sending to avoid buffer overruns in the plotter on very large drawings.
        mySerial = serial.Serial(port=self.options.serialPort, baudrate=self.options.serialBaudRate, timeout=0.1, writeTimeout=None)
        mySerial.write(self.hpgl)
        # Read back 2 chars to avoid plotter not plotting last command (I have no idea why this is necessary)
        mySerial.read(2)
        mySerial.close()

if __name__ == '__main__':
    # Raise recursion limit to avoid exceptions on big documents
    sys.setrecursionlimit(20000);
    # start extension
    e = MyEffect()
    e.affect()

# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99