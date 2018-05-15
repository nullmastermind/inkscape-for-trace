#!/usr/bin/env python 
'''
Copyright (C) 2005 Aaron Spike, aaron@ekips.org

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
import base64
import os
# local library
import inkex

class MyEffect(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option("--desc")
        self.OptionParser.add_option("-s", "--selectedonly",
            action="store", type="inkbool", 
            dest="selectedonly", default=True,
            help="extract only selected images")
        self.OptionParser.add_option("--filepath",
                        action="store", type="string", 
                        dest="filepath", default=None,
                        help="")

    def effect(self):
        # if slectedonly is enabled and there is a selection only extractselected
        # images. otherwise extract all images
        if (self.options.selectedonly):
            self.extractSelected(self.document, self.selected)
        else:
            self.extractAll(self.document)

    def extractSelected(self, document, selected):
        self.document=document
        self.selected=selected
        if (self.options.ids):
            for id, node in selected.iteritems():
                if node.tag == inkex.addNS('image','svg'):
                    self.extractImage(node)

    def extractAll(self, document):
        self.document=document #not that nice... oh well
        path = '//svg:image'
        for node in self.document.getroot().xpath(path, namespaces=inkex.NSS):
            self.extractImage(node)
    
    def extractImage(self, node):
        mimesubext={
            'png'     : '.png',
            'svg+xml' : '.svg',
            'bmp'     : '.bmp',
            'jpeg'    : '.jpg',
            'jpg'     : '.jpg', #bogus mime
            'icon'    : '.ico',
            'gif'     : '.gif'
        }
        
        # exbed the first embedded image
        path = self.options.filepath
        if (path != ''):
            if node.tag == inkex.addNS('image','svg'):
                xlink = node.get(inkex.addNS('href','xlink'))
                if (xlink[:4]=='data'):
                    comma = xlink.find(',')
                    if comma>0:
                        #get extension
                        fileext=''
                        semicolon = xlink.find(';')
                        if semicolon>0:
                            for sub in mimesubext.keys():
                                if sub in xlink[5:semicolon].lower():
                                    fileext=mimesubext[sub]
                                    pathwext=path+fileext
                                    if os.path.isfile(pathwext):
                                        pathwext=path + "_" + node.get("id") +fileext
                                    if (not os.path.isabs(pathwext)):
                                        if os.name == 'nt':
                                            pathwext = os.path.join(os.environ['USERPROFILE'],pathwext)
                                        else:
                                            pathwext = os.path.join(os.path.expanduser("~"),pathwext)
                                    inkex.errormsg(_('Image extracted to: %s') % pathwext)
                                    break 
                        #save
                        data = base64.decodestring(xlink[comma:])
                        open(pathwext,'wb').write(data)
                        node.set(inkex.addNS('href','xlink'),os.path.realpath(pathwext)) #absolute for making in-mem cycles work
                    else:
                        inkex.errormsg(_('Unable to find image data.'))

if __name__ == '__main__':
    e = MyEffect()
    e.affect()


# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99
