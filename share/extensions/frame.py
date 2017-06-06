#!/usr/bin/env python
"""
An Inkscape extension that creates a frame around a selected object.

Copyright (C) 2016 Richard White, rwhite8282@gmail.com

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
"""

# These two lines are only needed if you don't put the script directly into
# the installation directory
import sys
sys.path.append('/usr/share/inkscape/extensions')

import inkex
import simplestyle
from simpletransform import *
from simplestyle import *


def get_picker_data(value):
    """ Returns color data in style string format.
    value -- The value returned from the color picker.
    Returns an object with color and opacity properties.
    """
    v = '%08X' % (value & 0xFFFFFFFF)
    color = '#' + v[0:-2].rjust(6, '0')
    opacity = '%1.2f' % (float(int(v[6:].rjust(2, '0'), 16))/255)
    return type('', (object,), {'color':color, 'opacity':opacity})()


def size_box(box, delta):
    """ Returns a box with an altered size.
    delta -- The amount the box should grow.
    Returns a box with an altered size.
    """
    return ((box[0]-delta), (box[1]+delta), (box[2]-delta), (box[3]+delta))    


# Frame maker Inkscape effect extension
class Frame(inkex.Effect):
    """ An Inkscape extension that creates a frame around a selected object.
    """
    def __init__(self):
        inkex.Effect.__init__(self)
        self.defs = None
        
        # Parse the options.
        self.OptionParser.add_option('--clip',
            action='store', type='inkbool', 
            dest='clip', default=False)
        self.OptionParser.add_option('--corner_radius',
            action='store', type='int', 
            dest='corner_radius', default=0)
        self.OptionParser.add_option('--fill_color',
            action='store', type='int', 
            dest='fill_color', default='00000000')
        self.OptionParser.add_option('--group',
            action='store', type='inkbool', 
            dest='group', default=False)
        self.OptionParser.add_option('--position',
            action='store', type='string', 
            dest='position', default='outside')
        self.OptionParser.add_option('--stroke_color',
            action='store', type='int', 
            dest='stroke_color', default='00000000')
        self.OptionParser.add_option('--tab',
            action='store', type='string', 
            dest='tab', default='object')
        self.OptionParser.add_option('--width',
            action='store', type='float', 
            dest='width', default=2)
    
    
    def add_clip(self, node, clip_path):
        """ Adds a new clip path node to the defs and sets
                the clip-path on the node.
            node -- The node that will be clipped.
            clip_path -- The clip path object.
        """
        if self.defs is None:
            defs_nodes = self.document.getroot().xpath('//svg:defs', namespaces=inkex.NSS)
            if defs_nodes:
                self.defs = defs_nodes[0]
            else:
                inkex.errormsg('Could not locate defs node for clip.')
                return  
        clip = inkex.etree.SubElement(self.defs, inkex.addNS('clipPath','svg'))
        clip.append(copy.deepcopy(clip_path))
        clip_id = self.uniqueId('clipPath')
        clip.set('id', clip_id)
        node.set('clip-path', 'url(#%s)' % str(clip_id))
    
    
    def add_frame(self, parent, name, box, style, radius=0):
        """ Adds a new frame to the parent object.
            parent -- The parent that the frame will be added to.
            name -- The name of the new frame object.
            box -- The boundary box of the node.
            style -- The style used to draw the path.
            radius -- The corner radius of the frame.
            returns a new frame node.
        """
        r = min([radius, (abs(box[1]-box[0])/2), (abs(box[3]-box[2])/2)])
        if (radius > 0):
            d = ' '.join(str(x) for x in 
                            ['M', box[0], (box[2]+r)
                            ,'A', r, r, '0 0 1', (box[0]+r), box[2]
                            ,'L', (box[1]-r), box[2]
                            ,'A', r, r, '0 0 1', box[1], (box[2]+r)
                            ,'L', box[1], (box[3]-r)
                            ,'A', r, r, '0 0 1', (box[1]-r), box[3]
                            ,'L', (box[0]+r), box[3]
                            ,'A', r, r, '0 0 1', box[0], (box[3]-r), 'Z'])
        else:
            d = ' '.join(str(x) for x in 
                            ['M', box[0], box[2]
                            ,'L', box[1], box[2]
                            ,'L', box[1], box[3]
                            ,'L', box[0], box[3], 'Z'])
        
        attributes = {'style':style, inkex.addNS('label','inkscape'):name, 'd':d}
        return inkex.etree.SubElement(parent, inkex.addNS('path','svg'), attributes )


    def effect(self):
        """ Performs the effect.
        """
        # Get the style values.
        corner_radius = self.options.corner_radius
        stroke_data = get_picker_data(self.options.stroke_color)
        fill_data = get_picker_data(self.options.fill_color)
        
        # Determine common properties.
        parent = self.current_layer
        position = self.options.position
        width = self.options.width
        style = simplestyle.formatStyle({'stroke':stroke_data.color
            , 'stroke-opacity':stroke_data.opacity
            , 'stroke-width':str(width)
            , 'fill':(fill_data.color if (fill_data.opacity > 0) else 'none')
            , 'fill-opacity':fill_data.opacity})
        
        for id, node in self.selected.iteritems():
            box = computeBBox([node])
            if 'outside' == position:
                box = size_box(box, (width/2))
            else:
                box = size_box(box, -(width/2))
            name = 'Frame'
            frame = self.add_frame(parent, name, box, style, corner_radius)
            if self.options.clip:
                self.add_clip(node, frame)
            if self.options.group:
                group = inkex.etree.SubElement(node.getparent(),inkex.addNS('g','svg'))
                group.append(node)
                group.append(frame)


if __name__ == '__main__':   #pragma: no cover
    # Create effect instance and apply it.
    effect = Frame()
    effect.affect()