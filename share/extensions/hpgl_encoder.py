#!/usr/bin/env python 
# coding=utf-8
'''
Copyright (C) 2008 Aaron Spike, aaron@ekips.org
Overcut, Tool Offset, Rotation, Serial Com., Many Bugfixes and Improvements: Copyright (C) 2013 Sebastian Wüst, sebi@timewaster.de, http://www.timewasters-place.com/

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
            # path to HPGL commands
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
                # perform overcut
                if self.options.useOvercut and not self.dryRun:
                    # check if last and first points are the same, otherwise the path is not closed and no overcut can be performed
                    if int(oldPosX) == int(singlePath[0][1][0]) and int(oldPosY) == int(singlePath[0][1][1]):
                        overcutLength = 0
                        for singlePathPoint in singlePath:
                            posX, posY = singlePathPoint[1]
                            # check if point is repeating, if so, ignore
                            if posX != oldPosX or posY != oldPosY:
                                overcutLength += self.getLength(oldPosX, oldPosY, posX, posY)
                                if overcutLength >= self.options.overcut:
                                    newLength = self.changeLength(oldPosX, oldPosY, posX, posY, -(overcutLength - self.options.overcut));
                                    self.calcOffset(cmd, newLength[0], newLength[1])
                                    break
                                else:
                                    self.calcOffset(cmd, posX, posY)
                                oldPosX = posX
                                oldPosY = posY
    
    def getLength(self, x1, y1, x2, y2, absolute = True):
        # calc absoulute or relative length between two points
        if absolute: return math.fabs(math.sqrt((x2 - x1) ** 2.0 + (y2 - y1) ** 2.0))
        else: return math.sqrt((x2 - x1) ** 2.0 + (y2 - y1) ** 2.0)
    
    def changeLength(self, x1, y1, x2, y2, offset):
        # change length of line
        if offset < 0: offset = max(-self.getLength(x1, y1, x2, y2), offset)
        x = x2 + (x2 - x1) / self.getLength(x1, y1, x2, y2, False) * offset;
        y = y2 + (y2 - y1) / self.getLength(x1, y1, x2, y2, False) * offset;
        return [x, y]

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
                        if self.getAlpha(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.vData[3][1], self.vData[3][2]) > 3:
                            self.storeData(self.vData[2][0], self.vData[2][1], self.vData[2][2])
                            return
                    # perform tool offset correction (It's a *tad* complicated, if you want to understand it draw the data as lines on paper) 
                    if self.vData[2][0] == 'PD': # If the 3rd entry in the cache is a pen down command make the line longer by the tool offset
                        pointThree = self.changeLength(self.vData[1][1], self.vData[1][2], self.vData[2][1], self.vData[2][2], self.options.toolOffset)
                        self.storeData('PD', pointThree[0], pointThree[1])
                    elif self.vData[0][1] != -1.0: # Elif the 1st entry in the cache is filled with data and the 3rd entry is a pen up command shift the 3rd entry by the current tool offset position according to the 2nd command
                        pointThree = self.changeLength(self.vData[0][1], self.vData[0][2], self.vData[1][1], self.vData[1][2], self.options.toolOffset) 
                        pointThree[0] = self.vData[2][1] - (self.vData[1][1] - pointThree[0])
                        pointThree[1] = self.vData[2][2] - (self.vData[1][2] - pointThree[1])
                        self.storeData('PU', pointThree[0], pointThree[1])
                    else: # Else just write the 3rd entry
                        pointThree = [self.vData[2][1], self.vData[2][2]]
                        self.storeData('PU', pointThree[0], pointThree[1])
                    if self.vData[3][0] == 'PD': # If the 4th entry in the cache is a pen down command guide tool to next angle
                        # Create a circle between the prolonged 3rd and 4th entry to correctly guide the tool around the corner
                        if self.getLength(self.vData[2][1], self.vData[2][2], self.vData[3][1], self.vData[3][2]) >= self.options.toolOffset:
                            pointFour = self.changeLength(self.vData[3][1], self.vData[3][2], self.vData[2][1], self.vData[2][2], -self.options.toolOffset)
                        else:
                            pointFour = self.changeLength(self.vData[2][1], self.vData[2][2], self.vData[3][1], self.vData[3][2], (self.options.toolOffset - self.getLength(self.vData[2][1], self.vData[2][2], self.vData[3][1], self.vData[3][2])))
                        alpha1 = math.atan2(pointThree[1] - self.vData[2][2], pointThree[0] - self.vData[2][1])
                        alpha2 = math.atan2(pointFour[1] - self.vData[2][2], pointFour[0] - self.vData[2][1])
                        # TODO:2013-07-13:Sebastian Wüst:Fix that sucker! (number of points in the circle has to be calculated)
                        step = 10
                        #inkex.errormsg(str(alpha1) + ' | ' + str(alpha2))                        
                        for alpha in range(int(step), 101, int(step)):
                            alpha = alpha1 + alpha * (alpha2 - alpha1) / 100
                            self.storeData('PD', self.vData[2][1] + math.cos(alpha) * self.options.toolOffset, self.vData[2][2] + math.sin(alpha) * self.options.toolOffset)
                        self.storeData('PD', pointFour[0], pointFour[1])
    
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