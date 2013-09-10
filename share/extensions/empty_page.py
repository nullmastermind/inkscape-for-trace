#!/usr/bin/env python

import inkex

class C(inkex.Effect):
  def __init__(self):
    inkex.Effect.__init__(self)
    self.OptionParser.add_option("-s", "--size", action="store", type="string", dest="page_size", default="a4", help="Page size")

  def effect(self): 
    root = self.document.getroot()
    root.set("width", "12in")
    root.set("height", "12in")
    if self.options.page_size == "a4":
        root.set("width", "210mm")
        root.set("height", "297mm")
    if self.options.page_size == "a4l":
        root.set("height", "210mm")
        root.set("width", "297mm")
        
    if self.options.page_size == "a5":
        root.set("width", "148mm")
        root.set("height", "210mm")
    if self.options.page_size == "a5l":
        root.set("width", "210mm")
        root.set("height", "148mm")
        
    if self.options.page_size == "a3":
        root.set("width", "297mm")
        root.set("height", "420mm")
    if self.options.page_size == "a3l":
        root.set("width", "420mm")
        root.set("height", "297mm")
        
    if self.options.page_size == "letter":
        root.set("width", "216mm")
        root.set("height", "279mm")
    if self.options.page_size == "letterl":
        root.set("width", "279mm")
        root.set("height", "216mm")

c = C()
c.affect()
