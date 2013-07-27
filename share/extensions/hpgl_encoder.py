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
        self.divergenceX = 'False' # dirty hack: i need to know if this was set to a number before, but since False is evaluated to 0 it can not be determined, therefore the string.
        self.divergenceY = 'False'
        self.sizeX = 'False'
        self.sizeY = 'False'
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
        self.process_group(self.doc, self.groupmat)
        if self.divergenceX == 'False' or self.divergenceY == 'False' or self.sizeX == 'False' or self.sizeY == 'False':
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
            self.calcOffset('PD', 0, self.options.toolOffset * 8)
        # start conversion
        self.process_group(self.doc, self.groupmat)
        # shift an empty node in in order to process last node in cache
        self.calcOffset('PU', 0, 0)
        # add return to zero point
        self.hpgl += 'PU0,0;'
        return self.hpgl
    
    def process_group(self, group, groupmat):
        # process groups
        style = group.get('style')
        if style:
            style = simplestyle.parseStyle(style)
            if style.has_key('display'):
                if style['display'] == 'none':
                    return
        trans = group.get('transform')
        if trans:
            groupmat.append(simpletransform.composeTransform(groupmat[-1], simpletransform.parseTransform(trans)))
        for node in group:
            if node.tag == inkex.addNS('path', 'svg'):
                self.process_path(node, groupmat[-1])
            if node.tag == inkex.addNS('g', 'svg'):
                self.process_group(node, groupmat)
        if trans:
            groupmat.pop()
    
    def process_path(self, node, mat):
        # process path 
        drawing = node.get('d')
        if drawing:
            # transform path
            paths = cubicsuperpath.parsePath(drawing)
            trans = node.get('transform')
            if trans:
                mat = simpletransform.composeTransform(mat, simpletransform.parseTransform(trans))
            simpletransform.applyTransformToPath(mat, paths)
            cspsubdiv.cspsubdiv(paths, self.options.flat)
            # break path into HPGL commands
            oldPosX = ''
            oldPosY = ''
            for singlePath in paths:
                cmd = 'PU'
                for singlePathPoint in singlePath:
                    posX, posY = singlePathPoint[1]
                    # check if point is repeating, if so, ignore
                    if posX != oldPosX or posY != oldPosY:
                        self.calcOffset(cmd, posX, posY)
                        cmd = 'PD'
                        oldPosX = posX
                        oldPosY = posY
                endPosX = posX
                endPosY = posY
                # perform overcut
                if self.options.useOvercut and not self.dryRun:
                    # check if last and first points are the same, otherwise the path is not closed and no overcut can be performed
                    if int(endPosX) == int(singlePath[0][1][0]) and int(endPosY) == int(singlePath[0][1][1]):
                        for singlePathPoint in singlePath:
                            posX, posY = singlePathPoint[1]
                            # check if point is repeating, if so, ignore
                            if posX != oldPosX or posY != oldPosY:
                                self.calcOffset(cmd, posX, posY)
                                if self.options.overcut - self.getLength(endPosX, endPosY, posX, posY) <= 0:
                                    break                                      
                                oldPosX = posX
                                oldPosY = posY
    
    def getLength(self, x1, y1, x2, y2, absolute = True):
        # calc absoulute or relative length between two points
        if absolute: return math.fabs(math.sqrt((x2 - x1) ** 2.0 + (y2 - y1) ** 2.0))
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
    
    def calcOffset(self, cmd, posX, posY):
        # calculate offset correction (or dont)
        if not self.options.useToolOffset or self.dryRun:
            self.storeData(cmd, posX, posY)
        else:
            # insert data into cache
            self.vData.pop(0)
            self.vData.insert(3, [cmd, posX, posY])
            # decide if enough data is availabe
            if self.vData[2][1] != -1.0:
                if self.vData[1][1] == -1.0:
                    self.storeData(self.vData[2][0], self.vData[2][1], self.vData[2][2])
                else:
                    # check if tool offset correction is needed (if the angle is big enough)
                    if self.vData[2][0] == 'PD' and self.vData[3][0] == 'PD':
                        # TODO:2013-07-13:Sebastian Wüst:Is this necessary?
                        if self.getLength(self.vData[2][1], self.vData[2][2], self.vData[3][1], self.vData[3][2]) < self.options.toolOffset:
                            self.storeData(self.vData[2][0], self.vData[2][1], self.vData[2][2])
                            return
                        if self.getAlpha(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.vData[3][1], self.vData[3][2]) > 2.748893:
                            self.storeData(self.vData[2][0], self.vData[2][1], self.vData[2][2])
                            return
                    # perform tool offset correction (It's a *tad* complicated, if you want to understand it draw the data as lines on paper) 
                    if self.vData[2][0] == 'PD': # If the 3rd entry in the cache is a pen down command make the line longer by the tool offset
                        pointThreeX = self.changeLengthX(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.options.toolOffset)
                        pointThreeY = self.changeLengthY(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.options.toolOffset)
                        self.storeData('PD', pointThreeX, pointThreeY)
                    elif self.vData[0][1] != -1.0: # Elif the 1st entry in the cache is filled with data shift the 3rd entry by the current tool offset position according to the 2nd command 
                        pointThreeX = self.vData[2][1] - (self.vData[1][1] - self.changeLengthX(self.vData[0][1], self.vData[0][2], self.vData[1][1], self.vData[1][2], self.options.toolOffset))
                        pointThreeY = self.vData[2][2] - (self.vData[1][2] - self.changeLengthY(self.vData[0][1], self.vData[0][2], self.vData[1][1], self.vData[1][2], self.options.toolOffset))
                        self.storeData('PU', pointThreeX, pointThreeY)
                    else: # Else just write the 3rd entry to HPGL
                        pointThreeX = self.vData[2][1]
                        pointThreeY = self.vData[2][2]
                        self.storeData('PU', pointThreeX, pointThreeY)
                    if self.vData[3][0] == 'PD': # If the 4th entry in the cache is a pen down command
                        # TODO:2013-07-13:Sebastian Wüst:Either remove old method or make it selectable by parameter.
                        if 1 == 2:
                            pointFourX = self.changeLengthX(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -(self.options.toolOffset * self.options.toolOffsetReturn))
                            pointFourY = self.changeLengthY(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -(self.options.toolOffset * self.options.toolOffsetReturn))
                            self.storeData('PD', pointFourX, pointFourY)
                        else:
                            # Create a circle between 3rd and 4th entry to correctly guide the tool around the corner
                            pointFourX = self.changeLengthX(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -self.options.toolOffset)
                            pointFourY = self.changeLengthY(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -self.options.toolOffset)
                            # TODO:2013-07-13:Sebastian Wüst:Fix that sucker! (number of points in the circle has to be calculated)
                            alpha1 = math.atan2(pointThreeY - self.vData[2][2], pointThreeX - self.vData[2][1])
                            alpha2 = math.atan2(pointFourY - self.vData[2][2], pointFourX - self.vData[2][1])
                            inkex.errormsg(str(math.fabs(alpha2 - alpha1)))
                            step = (2 * math.pi - math.fabs(alpha2 - alpha1)) * 6 + 1
                            #inkex.errormsg(str(alpha1) + ' | ' + str(alpha2))                        
                            for alpha in range(int(step), 101, int(step)):
                                alpha = alpha1 + alpha * (alpha2 - alpha1) / 100
                                self.storeData('PD', self.vData[2][1] + math.cos(alpha) * self.options.toolOffset, self.vData[2][2] + math.sin(alpha) * self.options.toolOffset)
                            self.storeData('PD', pointFourX, pointFourY)
    
    def storeData(self, command, x, y):
        # store point
        if self.dryRun:
            if self.divergenceX == 'False' or x < self.divergenceX: self.divergenceX = x 
            if self.divergenceY == 'False' or y < self.divergenceY: self.divergenceY = y
            if self.sizeX == 'False' or x > self.sizeX: self.sizeX = x
            if self.sizeY == 'False' or y > self.sizeY: self.sizeY = y
        else:
            if not self.options.center:
                if x < 0: x = 0 # only positive values are allowed (usually)
                if y < 0: y = 0
            self.hpgl += '%s%d,%d;' % (command, x, y)

# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99