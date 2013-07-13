#!/usr/bin/env python 
# coding=utf-8
'''
Copyright (C) 2013 Sebastian Wüst, sebi@timewaster.de, http://www.timewasters-place.com/
This importer supports the "HP-GL/2 kernel" set of commands only (More should not be necessary).

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
import inkex


class hpglDecoder:
    def __init__(self, options):
        ''' options:
                "resolutionX":float
                "resolutionY":float
                "showMovements":bool
        '''
        self.options = options
        self.scaleX = options.resolutionX / 90.0 # dots/inch to dots/pixels
        self.scaleY = options.resolutionY / 90.0 # dots/inch to dots/pixels
        self.hasUnknownCommands = False

    def getSvg(self, data):
        # parse hpgl data
        data = data.split(';')
        if data[-1].strip() == '':
            data.pop()
        if len(data) < 3:
            return (False, True, '')
        # prepare document
        doc = inkex.etree.parse(StringIO('<svg xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" width="%s" height="%s"></svg>' % (self.options.docWidth, self.options.docHeight)))
        layerDrawing = inkex.etree.SubElement(doc.getroot(), 'g', {inkex.addNS('groupmode','inkscape'):'layer', inkex.addNS('label','inkscape'):'Drawing'})
        if self.options.showMovements:
        	layerMovements = inkex.etree.SubElement(doc.getroot(), 'g', {inkex.addNS('groupmode','inkscape'):'layer', inkex.addNS('label','inkscape'):'Movements'})
        # parse paths
        oldCoordinates = (0.0, self.options.docHeight) 
        path = ''
        for i, command in enumerate(data):
            if command.strip() != '':
                # TODO:2013-07-13:Sebastian Wüst:Implement the "HP-GL/2 kernel" set of commands.
                if command[:2] == 'PU': # if Pen Up command
                    if " L" in path:
                        # TODO:2013-07-13:Sebastian Wüst:Make a method for adding a SubElement.
                        inkex.etree.SubElement(layerDrawing, 'path', {'d':path, 'style':'stroke:#000000; stroke-width:0.3; fill:none;'})
                    if self.options.showMovements and i != len(data) - 1:
                        path = 'M %f,%f' % oldCoordinates
                        path += ' L %f,%f' % self.getCoordinates(command[2:])
                        inkex.etree.SubElement(layerMovements, 'path', {'d':path, 'style':'stroke:#ff0000; stroke-width:0.3; fill:none;'})
                    path = 'M %f,%f' % self.getCoordinates(command[2:])
                elif command[:2] == 'PD': # if Pen Down command
                    path += ' L %f,%f' % self.getCoordinates(command[2:])
                    oldCoordinates = self.getCoordinates(command[2:])
                elif command[:2] == 'IN': # if Initialize command
                    pass
                elif command[:2] == 'SP': # if Select Pen command
                    # TODO:2013-07-13:Sebastian Wüst:Every pen number should go to a different layer.
                    pass
                else:
                    self.hasUnknownCommands = True
        if " L" in path:
            inkex.etree.SubElement(layerDrawing, 'path', {'d':path, 'style':'stroke:#000000; stroke-width:0.3; fill:none;'})
        return (self.hasUnknownCommands, False, doc)
    
    def getCoordinates(self, coord):
        # process coordinates
        (x, y) = coord.split(',')
        x = float(x) / self.scaleX; # convert to pixels coordinate system
        y = self.options.docHeight - float(y) / self.scaleY; # convert to pixels coordinate system, flip vertically for inkscape coordinate system
        return (x, y)  

# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99