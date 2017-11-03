#!/usr/bin/env python
'''
Copyright (C) 2009 Aurelio A. Heckert, aurium (a) gmail dot com

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
'''
# standard library
import sys
import math
import re
import string
# local library
import inkex
import simplestyle
from pathmodifier import zSort


class InterpAttG(inkex.Effect):

    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option("-a", "--att",
                        action="store", type="string",
                        dest="att", default="fill",
                        help="Attribute to be interpolated.")
        self.OptionParser.add_option("-o", "--att-other",
                        action="store", type="string",
                        dest="att_other",
                        help="Other attribute (for a limited UI).")
        self.OptionParser.add_option("-t", "--att-other-type",
                        action="store", type="string",
                        dest="att_other_type",
                        help="The other attribute type.")
        self.OptionParser.add_option("-w", "--att-other-where",
                        action="store", type="string",
                        dest="att_other_where",
                        help="That is a tag attribute or a style attribute?")
        self.OptionParser.add_option("-s", "--start-val",
                        action="store", type="string",
                        dest="start_val", default="#F00",
                        help="Initial interpolation value.")
        self.OptionParser.add_option("-e", "--end-val",
                        action="store", type="string",
                        dest="end_val", default="#00F",
                        help="End interpolation value.")
        self.OptionParser.add_option("-u", "--unit",
                        action="store", type="string",
                        dest="unit", default="color",
                        help="Values unit.")
        self.OptionParser.add_option("--zsort",
                        action="store", type="inkbool",
                        dest="zsort", default=True,
                        help="use z-order instead of selection order")
        self.OptionParser.add_option("--tab",
                        action="store", type="string",
                        dest="tab",
                        help="The selected UI-tab when OK was pressed")

    def getColorValues(self):
        sv = string.replace( self.options.start_val, '#', '' )
        ev = string.replace( self.options.end_val, '#', '' )
        
        # index 0: start color, index 1: end color
        self.R, self.G, self.B = [0,0],[0,0],[0,0]
        raw_colors = [sv, ev]
        
        for i in [0,1]:
            if re.search('\s|,', raw_colors[i]):
                # There are separators. That must be an integer RGB color definition.
                raw_colors[i] = re.split( '[\s,]+', raw_colors[i])
                self.R[i] = int(raw_colors[i][0])
                self.G[i] = int(raw_colors[i][1])
                self.B[i] = int(raw_colors[i][2])
            else:
                # There is no separator. That must be a Hex RGB color definition.
                if len(raw_colors[i]) == 3:
                    self.R[i] = int(raw_colors[i][0] + raw_colors[i][0], 16)
                    self.G[i] = int(raw_colors[i][1] + raw_colors[i][1], 16)
                    self.B[i] = int(raw_colors[i][2] + raw_colors[i][2], 16)
                else: # the len must be 6
                    self.R[i] = int( raw_colors[i][0] + raw_colors[i][1], 16)
                    self.G[i] = int( raw_colors[i][2] + raw_colors[i][3], 16)
                    self.B[i] = int( raw_colors[i][4] + raw_colors[i][5], 16)

        self.R_inc = (self.R[1] - self.R[0]) / float(self.tot_el - 1)
        self.G_inc = (self.G[1] - self.G[0]) / float(self.tot_el - 1)
        self.B_inc = (self.B[1] - self.B[0]) / float(self.tot_el - 1)
        self.R_cur = self.R[0]
        self.G_cur = self.G[0]
        self.B_cur = self.B[0]

    def getNumberValues(self):
        sv = self.options.start_val.replace(",", ".")
        ev = self.options.end_val.replace(",", ".")
        unit = self.options.unit

        if unit != 'none':
            sv = self.unittouu(sv + unit)
            ev = self.unittouu(ev + unit)
        else:
            sv = float(sv)
            ev = float(ev)
        self.val_cur = self.val_ini = sv
        self.val_end = ev
        self.val_inc = (ev - sv)/float(self.tot_el - 1)

    def getTotElements(self):
        self.tot_el = 0
        self.collection = None
        if len( self.selected ) == 0:
            return False
        if len( self.selected ) > 1:
            # multiple selection
            if self.options.zsort:
                sorted_ids = zSort(self.document.getroot(),self.selected.keys())
            else:
                sorted_ids = self.options.ids
            self.collection = list(sorted_ids)
            for i in sorted_ids:
                path = '//*[@id="%s"]' % i
                self.collection[self.tot_el] = self.document.xpath(path, namespaces=inkex.NSS)[0]
                self.tot_el += 1
        else:
            # must be a group
            self.collection = self.selected[ self.options.ids[0] ]
            for i in self.collection:
                self.tot_el += 1

    def effect(self):
        if self.options.att == 'other':
            if self.options.att_other is not None:
                self.inte_att = self.options.att_other
            else:
                inkex.errormsg(_("You selected 'Other'. Please enter an attribute to interpolate."))
                sys.exit(0)
            self.inte_att_type = self.options.att_other_type
            self.where = self.options.att_other_where
        else:
            self.inte_att = self.options.att
            if self.inte_att == 'width':
                self.inte_att_type = 'float'
                self.where = 'tag'
            elif self.inte_att == 'height':
                self.inte_att_type = 'float'
                self.where = 'tag'
            elif self.inte_att == 'scale':
                self.inte_att_type = 'float'
                self.where = 'transform'
            elif self.inte_att == 'trans-x':
                self.inte_att_type = 'float'
                self.where = 'transform'
            elif self.inte_att == 'trans-y':
                self.inte_att_type = 'float'
                self.where = 'transform'
            elif self.inte_att == 'fill':
                self.inte_att_type = 'color'
                self.where = 'style'
            elif self.inte_att == 'opacity':
                self.inte_att_type = 'float'
                self.where = 'style'

        self.getTotElements()

        if self.inte_att_type == 'color':
            self.getColorValues()
        else:
            self.getNumberValues()

        if self.collection is None:
            inkex.errormsg( _('There is no selection to interpolate' ))
            return False

        for node in self.collection:
            if self.inte_att_type == 'color':
                val = 'rgb('+ \
                    str(int(round(self.R_cur))) +','+ \
                    str(int(round(self.G_cur))) +','+ \
                    str(int(round(self.B_cur))) +')'
            else:
                if self.inte_att_type == 'float':
                    val = self.val_cur
                else: # inte_att_type == 'int'
                    val = int(round(self.val_cur))

            if self.where == 'style':
                s = node.get('style')
                re_find = '(^|;)'+ self.inte_att +':[^;]*(;|$)'
                if re.search( re_find, s ):
                    s = re.sub( re_find, '\\1'+ self.inte_att +':'+ str(val) +'\\2', s )
                else:
                    s += ';'+ self.inte_att +':'+ str(val)
                node.set( 'style', s )
            elif self.where == 'transform':
                t = node.get('transform')
                if t == None: t = ""
                if self.inte_att == 'trans-x':
                    val = "translate("+ str(val) +",0)"
                elif self.inte_att == 'trans-y':
                    val = "translate(0,"+ str(val) +")"
                else:
                    val = self.inte_att + "("+ str(val) +")"
                node.set( 'transform', t +" "+ val )
            else: # self.where == 'tag':
                if ":" in self.inte_att:
                    ns, attrib = self.inte_att.split(":")
                    node.set(inkex.addNS(attrib, ns), str(val))
                else:
                    node.set( self.inte_att, str(val) )

            if self.inte_att_type == 'color':
                self.R_cur += self.R_inc
                self.G_cur += self.G_inc
                self.B_cur += self.B_inc
            else:
                self.val_cur += self.val_inc

        return True

if __name__ == '__main__':
    e = InterpAttG()
    if e.affect():
        exit(0)
    else:
        exit(1)
