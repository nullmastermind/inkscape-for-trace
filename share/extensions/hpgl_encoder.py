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
import math, string
# local library
import bezmisc, cspsubdiv, cubicsuperpath, inkex, simplestyle, simpletransform

class hpglEncoder:
    def __init__(self, doc, options):
        ''' options:
                "resolutionX":float
                "resolutionY":float
                "pen":int
                "orientation":string // "0", "90", "-90", "180"
                "mirrorX":bool
                "mirrorY":bool
                "center":bool
                "flat":float
                "useOvercut":bool
                "overcut":float
                "useToolOffset":bool
                "toolOffset":float
                "toolOffsetReturn":float
                "precut":bool
                "offsetX":float
                "offsetY":float
        '''
        self.doc = doc
        self.options = options
        # TODO:2013-07-13:Sebastian Wüst:Find a way to avoid this crap, maybe make it a string so one can check if it was set before.
        self.divergenceX = 9999999999999.0
        self.divergenceY = 9999999999999.0
        self.sizeX = -9999999999999.0
        self.sizeY = -9999999999999.0
        self.dryRun = True
        self.scaleX = self.options.resolutionX / 90 # inch to pixels
        self.scaleY = self.options.resolutionY / 90 # inch to pixels
        self.options.offsetX = self.options.offsetX * 3.5433070866 * self.scaleX # mm to dots (plotter coordinate system)
        self.options.offsetY = self.options.offsetY * 3.5433070866 * self.scaleY # mm to dots
        self.options.overcut = self.options.overcut * 3.5433070866 * ((self.scaleX + self.scaleY) / 2) # mm to dots
        self.options.toolOffset = self.options.toolOffset * 3.5433070866 * ((self.scaleX + self.scaleY) / 2) # mm to dots
        self.options.flat = ((self.options.resolutionX + self.options.resolutionY) / 2) * self.options.flat / 1000 # scale flatness to resolution
        self.mirrorX = 1.0
        if self.options.mirrorX:
            self.mirrorX = -1.0
        self.mirrorY = -1.0
        if self.options.mirrorY:
            self.mirrorY = 1.0
        # process viewBox parameter to correct scaling
        viewBox = doc.get('viewBox')
        self.viewBoxTransformX = 1
        self.viewBoxTransformY = 1
        if viewBox:
            viewBox = string.split(viewBox, ' ')
            if viewBox[2] and viewBox[3]:
                self.viewBoxTransformX = float(inkex.unittouu(doc.get('width'))) / float(viewBox[2])
                self.viewBoxTransformY = float(inkex.unittouu(doc.get('height'))) / float(viewBox[3])

    def getHpgl(self):
        # dryRun to find edges
        self.groupmat = [[[self.mirrorX * self.scaleX * self.viewBoxTransformX, 0.0, 0.0], [0.0, self.mirrorY * self.scaleY * self.viewBoxTransformY, 0.0]]]
        self.groupmat[0] = simpletransform.composeTransform(self.groupmat[0], simpletransform.parseTransform('rotate(' + self.options.orientation + ')'))
        self.vData = [['', -1.0, -1.0], ['', -1.0, -1.0], ['', -1.0, -1.0], ['', -1.0, -1.0]]
        self.process_group(self.doc)
        if self.divergenceX == 9999999999999.0 or self.divergenceY == 9999999999999.0 or self.sizeX == -9999999999999.0 or self.sizeY == -9999999999999.0:
            raise Exception('NO_PATHS')
        # live run
        self.dryRun = False
        if self.options.center:
            self.divergenceX += (self.sizeX - self.divergenceX) / 2
            self.divergenceY += (self.sizeY - self.divergenceY) / 2
        elif self.options.useToolOffset:
            self.options.offsetX += self.options.toolOffset
            self.options.offsetY += self.options.toolOffset
        self.groupmat = [[[self.mirrorX * self.scaleX * self.viewBoxTransformX, 0.0, -self.divergenceX + self.options.offsetX], [0.0, self.mirrorY * self.scaleY * self.viewBoxTransformY, -self.divergenceY + self.options.offsetY]]]
        self.groupmat[0] = simpletransform.composeTransform(self.groupmat[0], simpletransform.parseTransform('rotate(' + self.options.orientation + ')'))
        self.vData = [['', -1.0, -1.0], ['', -1.0, -1.0], ['', -1.0, -1.0], ['', -1.0, -1.0]]
        # store first hpgl commands
        self.hpgl = 'IN;SP%d;' % self.options.pen
        # add precut
        if self.options.useToolOffset and self.options.precut:
            self.calcOffset('PU', 0, 0)
            self.calcOffset('PD', 0, self.options.toolOffset * self.options.toolOffsetReturn * 2)
        # start conversion
        self.process_group(self.doc)
        # shift an empty node in in order to process last node in cache
        self.calcOffset('PU', 0, 0)
        # add return to zero point
        self.hpgl += 'PU0,0;'
        return self.hpgl
    
    def process_group(self, group):
        # process groups
        style = group.get('style')
        if style:
            style = simplestyle.parseStyle(style)
            if style.has_key('display'):
                if style['display'] == 'none':
                    return
        # TODO:2013-07-13:Sebastian Wüst:Pass groupmat in recursion instead of manipulating it.
        trans = group.get('transform')
        if trans:
            self.groupmat.append(simpletransform.composeTransform(self.groupmat[-1], simpletransform.parseTransform(trans)))
        for node in group:
            if node.tag == inkex.addNS('path', 'svg'):
                self.process_path(node, self.groupmat[-1])
            if node.tag == inkex.addNS('g', 'svg'):
                self.process_group(node)
        if trans:
            self.groupmat.pop()
    
    def process_path(self, node, mat):
        # process path 
        # TODO:2013-07-13:Sebastian Wüst:Find better variable names.
        d = node.get('d')
        if d:
            # transform path
            p = cubicsuperpath.parsePath(d)
            trans = node.get('transform')
            if trans:
                mat = simpletransform.composeTransform(mat, simpletransform.parseTransform(trans))
            simpletransform.applyTransformToPath(mat, p)
            cspsubdiv.cspsubdiv(p, self.options.flat)
            # break path into HPGL commands
            # TODO:2013-07-13:Sebastian Wüst:Somehow make this for more readable.
            xPosOld = -1
            yPosOld = -1
            for sp in p:
                cmd = 'PU'
                for csp in sp:
                    xPos = csp[1][0]
                    yPos = csp[1][1]
                    if int(xPos) != int(xPosOld) or int(yPos) != int(yPosOld): 
                        self.calcOffset(cmd, xPos, yPos)
                        cmd = 'PD'
                        xPosOld = xPos
                        yPosOld = yPos
                # perform overcut
                if self.options.useOvercut and not self.dryRun:
                    if int(xPos) == int(sp[0][1][0]) and int(yPos) == int(sp[0][1][1]):
                        for csp in sp:
                            xPos2 = csp[1][0]
                            yPos2 = csp[1][1]
                            if int(xPos) != int(xPos2) or int(yPos) != int(yPos2):
                                self.calcOffset(cmd, xPos2, yPos2)
                                if self.options.overcut - self.getLength(xPosOld, yPosOld, xPos2, yPos2) <= 0:
                                    break                                      
                                xPos = xPos2
                                yPos = yPos2
    
    # TODO:2013-07-13:Sebastian Wüst:Find methods from the existing classes to replace the next 4 methods.
    def getLength(self, x1, y1, x2, y2, abs = True):
        # calc absoulute or relative length between two points
        if abs: return math.fabs(math.sqrt((x2 - x1) ** 2.0 + (y2 - y1) ** 2.0))
        else: return math.sqrt((x2 - x1) ** 2.0 + (y2 - y1) ** 2.0)
    
    def changeLengthX(self, x1, y1, x2, y2, offset):
        # change length of line - x axis
        if offset < 0: offset = max(-self.getLength(x1, y1, x2, y2), offset)
        return x2 + (x2 - x1) / self.getLength(x1, y1, x2, y2, False) * offset;
    
    def changeLengthY(self, x1, y1, x2, y2, offset):
        # change length of line - y axis
        if offset < 0: offset = max(-self.getLength(x1, y1, x2, y2), offset)
        return y2 + (y2 - y1) / self.getLength(x1, y1, x2, y2, False) * offset;
    
    def getAlpha(self, x1, y1, x2, y2, x3, y3):
        # get alpha of point 2
        temp1 = (x1-x2)**2 + (y1-y2)**2 + (x3-x2)**2 + (y3-y2)**2 - (x1-x3)**2 - (y1-y3)**2
        temp2 = 2 * math.sqrt((x1-x2)**2 + (y1-y2)**2) * math.sqrt((x3-x2)**2 + (y3-y2)**2)
        temp3 = temp1 / temp2
        if temp3 < -1.0:
            temp3 = -1.0
        if temp3 > 1.0:
            temp3 = 1.0
        return math.acos(temp3)
    
    def calcOffset(self, cmd, xPos, yPos):
        # calculate offset correction (or dont)
        if not self.options.useToolOffset or self.dryRun:
            self.storeData(cmd, xPos, yPos)
        else:
            # insert data into cache
            self.vData.pop(0)
            self.vData.insert(3, [cmd, xPos, yPos])
            # decide if enough data is availabe
            if self.vData[2][1] != -1.0:
                if self.vData[1][1] == -1.0:
                    self.storeData(self.vData[2][0], self.vData[2][1], self.vData[2][2])
                else:
                    # check if tool offset correction is needed (if the angle is big enough)
                    if self.vData[2][0] == 'PD' and self.vData[3][0] == 'PD':
                        if self.getAlpha(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.vData[3][1], self.vData[3][2]) > 2.748893:
                            self.storeData(self.vData[2][0], self.vData[2][1], self.vData[2][2])
                            return
                    # perform tool offset correction (It's a *tad* complicated, if you want to understand it draw the data as lines on paper) 
                    if self.vData[2][0] == 'PD': # If the 3rd entry in the cache is a pen down command make the line longer by the tool offset
                        # TODO:2013-07-13:Sebastian Wüst:Find a better name for the variables.
                        pointTwoX = self.changeLengthX(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.options.toolOffset)
                        pointTwoY = self.changeLengthY(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.options.toolOffset)
                        self.storeData('PD', pointTwoX, pointTwoY)
                    elif self.vData[0][1] != -1.0: # Elif the 1st entry in the cache is filled with data shift the 3rd entry by the current tool offset position according to the 2nd command 
                        pointTwoX = self.vData[2][1] - (self.vData[1][1] - self.changeLengthX(self.vData[0][1], self.vData[0][2], self.vData[1][1], self.vData[1][2], self.options.toolOffset))
                        pointTwoY = self.vData[2][2] - (self.vData[1][2] - self.changeLengthY(self.vData[0][1], self.vData[0][2], self.vData[1][1], self.vData[1][2], self.options.toolOffset))
                        self.storeData('PU', pointTwoX, pointTwoY)
                    else: # Else just write the 3rd entry to HPGL
                        pointTwoX = self.vData[2][1]
                        pointTwoY = self.vData[2][2]
                        self.storeData('PU', pointTwoX, pointTwoY)
                    if self.vData[3][0] == 'PD': # If the 4th entry in the cache is a pen down command
                        # TODO:2013-07-13:Sebastian Wüst:Either remove old method or make it selectable by parameter.
                        if 1 == 1:
                            pointThreeX = self.changeLengthX(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -(self.options.toolOffset * self.options.toolOffsetReturn))
                            pointThreeY = self.changeLengthY(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -(self.options.toolOffset * self.options.toolOffsetReturn))
                            self.storeData('PD', pointThreeX, pointThreeY)
                        else:
                            # Create a circle between 3rd and 4th entry to correctly guide the tool around the corner
                            pointThreeX = self.changeLengthX(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -self.options.toolOffset)
                            pointThreeY = self.changeLengthY(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -self.options.toolOffset)
                            # TODO:2013-07-13:Sebastian Wüst:Fix that sucker! (number of points in the circle has to be calculated)
                            alpha1 = math.atan2(pointTwoY - self.vData[2][2], pointTwoX - self.vData[2][1])
                            alpha2 = math.atan2(pointThreeY - self.vData[2][2], pointThreeX - self.vData[2][1])
                            step = (2 * math.pi - math.fabs(alpha2 - alpha1)) * 6 + 1
                            #inkex.errormsg(str(alpha1) + ' | ' + str(alpha2))                        
                            for alpha in range(int(step), 101, int(step)):
                                alpha = alpha1 + alpha * (alpha2 - alpha1) / 100
                                self.storeData('PD', self.vData[2][1] + math.cos(alpha) * self.options.toolOffset, self.vData[2][2] + math.sin(alpha) * self.options.toolOffset)
                            self.storeData('PD', pointThreeX, pointThreeY)
    
    def storeData(self, command, x, y):
        # store point
        if self.dryRun:
            if x < self.divergenceX: self.divergenceX = x 
            if y < self.divergenceY: self.divergenceY = y
            if x > self.sizeX: self.sizeX = x
            if y > self.sizeY: self.sizeY = y
        else:
            if not self.options.center:
                if x < 0: x = 0 # only positive values are allowed
                if y < 0: y = 0
            self.hpgl += '%s%d,%d;' % (command, x, y)

# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99