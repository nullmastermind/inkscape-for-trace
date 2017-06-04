#!/usr/bin/env python

"""
An Inkscape frame extension test class.

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

import sys
sys.path.append('/usr/share/inkscape/extensions')
sys.path.append('..') # this line allows to import the extension code

import unittest
import inkex
from frame import *


class Frame_Test(unittest.TestCase):


    def get_frame(self, document):
        return document.xpath('//svg:g[@id="layer1"]//svg:path[@inkscape:label="Frame"]'
            , namespaces=inkex.NSS)[0] 
  
    
    def test_empty_no_parameters(self):
        args = [ 'svg/empty-SVG.svg' ]
        uut = Frame()
        uut.affect( args, False )


    def test_single_frame(self):
        args = [
            '--corner_radius=20'
            , '--fill_color=-16777124'
            , '--id=rect3006'
            , '--position=inside'
            , '--stroke_color=255'
            , '--tab="stroke"'
            , '--width=10'
            , 'svg/single_box.svg']
        uut = Frame()
        uut.affect( args, False )
        new_frame = self.get_frame(uut.document)
        self.assertIsNotNone(new_frame)
        self.assertEqual('{http://www.w3.org/2000/svg}path', new_frame.tag)
        new_frame_style = new_frame.attrib['style'].lower()
        self.assertTrue('fill-opacity:0.36' in new_frame_style
            , 'Invalid fill-opacity in "' + new_frame_style + '".')
        self.assertTrue('stroke:#000000' in new_frame_style
            , 'Invalid stroke in "' + new_frame_style + '".')
        self.assertTrue('stroke-width:10.0' in new_frame_style
            , 'Invalid stroke-width in "' + new_frame_style + '".')
        self.assertTrue('stroke-opacity:1.00' in new_frame_style
            , 'Invalid stroke-opacity in "' + new_frame_style + '".')
        self.assertTrue('fill:#ff0000' in new_frame_style
            , 'Invalid fill in "' + new_frame_style + '".')

    
    def test_single_frame_grouped(self):
        args = [
            '--corner_radius=20'
            , '--fill_color=-16777124'
            , '--group=True'
            , '--id=rect3006'
            , '--position=inside'
            , '--stroke_color=255'
            , '--tab="stroke"'
            , '--width=10'
            , 'svg/single_box.svg']
        uut = Frame()
        uut.affect( args, False )
        new_frame = self.get_frame(uut.document)
        self.assertIsNotNone(new_frame)
        self.assertEqual('{http://www.w3.org/2000/svg}path', new_frame.tag)
        group = new_frame.getparent()
        self.assertEqual('{http://www.w3.org/2000/svg}g', group.tag)
        self.assertEqual('{http://www.w3.org/2000/svg}rect', group[0].tag)
        self.assertEqual('{http://www.w3.org/2000/svg}path', group[1].tag)
        self.assertEqual("Frame", group[1].xpath('@inkscape:label', namespaces=inkex.NSS)[0])
    
    
    def test_single_frame_clipped(self):
        args = [
            '--clip=True'
            , '--corner_radius=20'
            , '--fill_color=-16777124'
            , '--id=rect3006'
            , '--position=inside'
            , '--stroke_color=255'
            , '--tab="stroke"'
            , '--width=10'
            , 'svg/single_box.svg']
        uut = Frame()
        uut.affect( args, False )
        new_frame = self.get_frame(uut.document)
        self.assertIsNotNone(new_frame)
        self.assertEqual('{http://www.w3.org/2000/svg}path', new_frame.tag)
        group = new_frame.getparent()
        self.assertEqual('url(#clipPath)', group[0].get('clip-path'))
        clip_path = uut.document.xpath('//svg:defs/svg:clipPath', namespaces=inkex.NSS)[0]
        self.assertEqual('{http://www.w3.org/2000/svg}clipPath', clip_path.tag)


if __name__ == '__main__':
    unittest.main()