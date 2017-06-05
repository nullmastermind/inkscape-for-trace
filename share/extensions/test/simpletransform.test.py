#!/usr/bin/env python

import os
import sys
import unittest

sys.path.append('..') # this line allows to import the extension code

import inkex
from simpletransform import *

class ComputeBBoxTest(unittest.TestCase):
    def setUp(self):
        args = [ 'svg/simpletransform.test.svg' ]
        self.e = inkex.Effect()
        self.e.affect(args, False)

    def test_scaled_object(self):
        "Object in the defs with 50,50 scaled by 0.5 when used"
        bbox = computeBBox(self.e.document.xpath("//svg:g", namespaces=inkex.NSS))
        text_bbox = "{} {} {} {}".format(bbox[0], bbox[1], bbox[2], bbox[3])
        self.assertEqual(text_bbox, "0.0 25.0 0.0 25.0")



if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(ComputeBBoxTest)
    unittest.TextTestRunner(verbosity=2).run(suite)
