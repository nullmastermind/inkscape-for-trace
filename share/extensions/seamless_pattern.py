#!/usr/bin/env python

# Written by Jabiertxof
# V.03

import inkex
import re
import os
from lxml import etree

class C(inkex.Effect):
  def __init__(self):
    inkex.Effect.__init__(self)
    self.OptionParser.add_option("-w", "--width",  action="store", type="int",    dest="desktop_width",  default="100", help="Custom width")
    self.OptionParser.add_option("-z", "--height", action="store", type="int",    dest="desktop_height", default="100", help="Custom height")

  def effect(self):

    width  = self.options.desktop_width
    height = self.options.desktop_height
    path = os.path.dirname(os.path.realpath(__file__))
    self.document = etree.parse(os.path.join(path, "seamless_pattern.svg"))
    root = self.document.getroot()
    root.set("id", "SVGRoot")
    root.set("width",  str(width) + 'px')
    root.set("height", str(height) + 'px')
    root.set("viewBox", "0 0 " + str(width) + " " + str(height) )

    xpathStr = '//svg:use[@id="sampleTile"]'
    sampleTile = root.xpath(xpathStr, namespaces=inkex.NSS)
    if sampleTile != []:
        sampleTile[0].set("transform", "translate(" + str(width * 6) + ",-" + str(height) + ")")
        
        
    xpathStr = '//svg:use[@id="clipPathRect"]'
    clipPathRect = root.xpath(xpathStr, namespaces=inkex.NSS)
    if clipPathRect != []:
        clipPathRect[0].set("transform", "translate(" + str((width*3)) + "," + str(height) +")")

    xpathStr = '//svg:tspan[@id="infoText"]'
    infoText = root.xpath(xpathStr, namespaces=inkex.NSS)
    if infoText != []:
        infoText[0].text = infoText[0].text.replace("100x100", str(width) + "x" + str(height))

    xpathStr = '//svg:use[@id="backgroundPattern"]'
    backgroundPattern = root.xpath(xpathStr, namespaces=inkex.NSS)
    if backgroundPattern != []:
        backgroundPattern[0].set("transform", "translate(-" + str(width * 2) + ",0)")

    xpathStr = '//svg:rect[@id="cuadroPattern"]'
    cuadroPattern = root.xpath(xpathStr, namespaces=inkex.NSS)
    if cuadroPattern != []:
        cuadroPattern[0].set("width", str(width))
        cuadroPattern[0].set("height", str(height))
        cuadroPattern[0].set("y", "-" + str(height*2))

    xpathStr = '//svg:use[@id="cuadroPatternDisenador"]'
    cuadroPatternDisenador = root.xpath(xpathStr, namespaces=inkex.NSS)
    if cuadroPatternDisenador != []:
        cuadroPatternDisenador[0].set("transform", "translate(" + str(width * 3) + "," + str(height) + ")")

    xpathStr = '//svg:g[@id="designZone"]'
    designZone = root.xpath(xpathStr, namespaces=inkex.NSS)
    if designZone != []:
        designZone[0].set("transform", "translate(" + str((width*5)) + ",0)scale(" + str(((width+height)/2)/100.) + ")")

    xpathStr = '//svg:g[@id="patternResult"]'
    patternResult = root.xpath(xpathStr, namespaces=inkex.NSS)
    if patternResult != []:
        patternResult[0].set("transform", "translate(-" + str((width * 3)) + "," + str(height) + ")")

    xpathStr = '//svg:use[@id="patternTileBackground"]'
    patternTileBackground = root.xpath(xpathStr, namespaces=inkex.NSS)
    if patternTileBackground != []:
        patternTileBackground[0].set("transform", "matrix(3,0,0,3," + str((width * 2)) + "," + str(height * 4) + ")")
        patternTileBackground[0].set("width", str(width*3))
        patternTileBackground[0].set("height", str(height*3))

    xpathStr = '//svg:use[@id="patternGenerated"]'
    patternGenerated = root.xpath(xpathStr, namespaces=inkex.NSS)
    if patternGenerated != []:
        patternGenerated[0].set("width", str(width))
        patternGenerated[0].set("height", str(height))

    xpathStr = '//svg:use[@id="pattern1"]'
    pattern1 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern1 != []:
        pattern1[0].set("transform", "translate(" + str(-width) + "," + str(-height) + ")")

    xpathStr = '//svg:use[@id="pattern2"]'
    pattern2 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern2 != []:
        pattern2[0].set("transform", "translate(0," + str(-height) +")" )

    xpathStr = '//svg:use[@id="pattern3"]'
    pattern3 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern3 != []:
        pattern3[0].set("transform", "translate(" + str(width) + "," + str(-height) + ")" )

    xpathStr = '//svg:use[@id="pattern4"]'
    pattern4 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern4 != []:
        pattern4[0].set("transform", "translate(" + str(-width) + ",0)" )

    xpathStr = '//svg:use[@id="pattern5"]'
    pattern5 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern5 != []:
        pattern5[0].set("transform", "translate(0,0)" )

    xpathStr = '//svg:use[@id="pattern6"]'
    pattern6 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern6 != []:
        pattern6[0].set("transform", "translate(" + str(width) + ",0)" )

    xpathStr = '//svg:use[@id="pattern7"]'
    pattern7 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern7 != []:
        pattern7[0].set("transform", "translate(" + str(-width) + "," + str(height) + ")" )

    xpathStr = '//svg:use[@id="pattern8"]'
    pattern8 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern8 != []:
        pattern8[0].set("transform", "translate(0," + str(height) + ")" )

    xpathStr = '//svg:use[@id="pattern9"]'
    pattern9 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if pattern9 != []:
        pattern9[0].set("transform", "translate(" + str(width) + "," + str(height) + ")" )

    xpathStr = '//svg:use[@id="samplePattern1"]'
    samplePattern1 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern1 != []:
        samplePattern1[0].set("transform", "translate(" + str(-width) + "," + str(-height) + ")")

    xpathStr = '//svg:use[@id="samplePattern2"]'
    samplePattern2 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern2 != []:
        samplePattern2[0].set("transform", "translate(0," + str(-height) +")" )

    xpathStr = '//svg:use[@id="samplePattern3"]'
    samplePattern3 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern3 != []:
        samplePattern3[0].set("transform", "translate(" + str(width) + "," + str(-height) + ")" )

    xpathStr = '//svg:use[@id="samplePattern4"]'
    samplePattern4 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern4 != []:
        samplePattern4[0].set("transform", "translate(" + str(-width) + ",0)" )

    xpathStr = '//svg:use[@id="samplePattern5"]'
    samplePattern5 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern5 != []:
        samplePattern5[0].set("transform", "translate(0,0)" )

    xpathStr = '//svg:use[@id="samplePattern6"]'
    samplePattern6 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern6 != []:
        samplePattern6[0].set("transform", "translate(" + str(width) + ",0)" )

    xpathStr = '//svg:use[@id="samplePattern7"]'
    samplePattern7 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern7 != []:
        samplePattern7[0].set("transform", "translate(" + str(-width) + "," + str(height) + ")" )

    xpathStr = '//svg:use[@id="samplePattern8"]'
    samplePattern8 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern8 != []:
        samplePattern8[0].set("transform", "translate(0," + str(height) + ")" )

    xpathStr = '//svg:use[@id="samplePattern9"]'
    samplePattern9 = root.xpath(xpathStr, namespaces=inkex.NSS)
    if samplePattern9 != []:
        samplePattern9[0].set("transform", "translate(" + str(width) + "," + str(height) + ")" )

    xpathStr = '//svg:g[@id="patternPreview"]'
    patternPreview = root.xpath(xpathStr, namespaces=inkex.NSS)
    if patternPreview != []:
        patternPreview[0].set("transform", "translate(0," + str(height*2) + ")" )

    xpathStr = '//svg:g[@id="helperLayer"]'
    helperLayer = root.xpath(xpathStr, namespaces=inkex.NSS)
    if helperLayer != []:
        helperLayer[0].set("transform", "scale(" + str(width/100.) + "," + str(height/100.) + ")" )

    xpathStr = '//svg:rect[@id="fondoGris"]'
    fondoGris = root.xpath(xpathStr, namespaces=inkex.NSS)
    if fondoGris != []:
        fondoGris[0].set("transform", "scale(" + str(width/100.) + "," + str(height/100.) + ")" )

    xpathStr = '//svg:use[@id="patternGenerator"  or id="patternPreviewFast"]'
    patternGenerator = root.xpath(xpathStr, namespaces=inkex.NSS)
    if patternGenerator != []:
        patternGenerator[0].set("{http://www.inkscape.org/namespaces/inkscape}tile-cx", str(width/2))
        patternGenerator[0].set("{http://www.inkscape.org/namespaces/inkscape}tile-cy", str(height/2))
        patternGenerator[0].set("{http://www.inkscape.org/namespaces/inkscape}tile-w", str(width))
        patternGenerator[0].set("{http://www.inkscape.org/namespaces/inkscape}tile-h", str(height))
        patternGenerator[0].set("{http://www.inkscape.org/namespaces/inkscape}tile-x0", str(width))
        patternGenerator[0].set("{http://www.inkscape.org/namespaces/inkscape}tile-y0", str(height))
        patternGenerator[0].set("width", str(width))
        patternGenerator[0].set("height", str(height))

    namedview = root.find(inkex.addNS('namedview', 'sodipodi'))
    if namedview is None:
        namedview = inkex.etree.SubElement( root, inkex.addNS('namedview', 'sodipodi') );
     
    namedview.set(inkex.addNS('document-units', 'inkscape'), 'px')

    namedview.set(inkex.addNS('cx',        'inkscape'), str((width*8)/2.0) )
    namedview.set(inkex.addNS('cy',        'inkscape'), "0" )
    namedview.set(inkex.addNS('zoom',        'inkscape'), str(0.5 / (((width+height)/2)/100.)) )

c = C()
c.affect()
