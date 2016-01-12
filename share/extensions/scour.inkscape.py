#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys, inkex
from scour.scour import scourString

class ScourInkscape (inkex.Effect):

    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option("--tab",                      type="string",  action="store", dest="tab")
        self.OptionParser.add_option("--simplify-colors",          type="inkbool", action="store", dest="simple_colors")
        self.OptionParser.add_option("--style-to-xml",             type="inkbool", action="store", dest="style_to_xml")
        self.OptionParser.add_option("--group-collapsing",         type="inkbool", action="store", dest="group_collapse")
        self.OptionParser.add_option("--create-groups",            type="inkbool", action="store", dest="group_create")
        self.OptionParser.add_option("--enable-id-stripping",      type="inkbool", action="store", dest="strip_ids")
        self.OptionParser.add_option("--shorten-ids",              type="inkbool", action="store", dest="shorten_ids")
        self.OptionParser.add_option("--shorten-ids-prefix",       type="string",  action="store", dest="shorten_ids_prefix", default="")
        self.OptionParser.add_option("--embed-rasters",            type="inkbool", action="store", dest="embed_rasters")
        self.OptionParser.add_option("--keep-unreferenced-defs",   type="inkbool", action="store", dest="keep_defs")
        self.OptionParser.add_option("--keep-editor-data",         type="inkbool", action="store", dest="keep_editor_data")
        self.OptionParser.add_option("--remove-metadata",          type="inkbool", action="store", dest="remove_metadata")
        self.OptionParser.add_option("--strip-xml-prolog",         type="inkbool", action="store", dest="strip_xml_prolog")
        self.OptionParser.add_option("--set-precision",            type=int,       action="store", dest="digits")
        self.OptionParser.add_option("--indent",                   type="string",  action="store", dest="indent_type")
        self.OptionParser.add_option("--nindent",                  type=int,       action="store", dest="indent_depth")
        self.OptionParser.add_option("--line-breaks",              type="inkbool", action="store", dest="newlines")
        self.OptionParser.add_option("--strip-xml-space",          type="inkbool", action="store", dest="strip_xml_space_attribute")
        self.OptionParser.add_option("--protect-ids-noninkscape",  type="inkbool", action="store", dest="protect_ids_noninkscape")
        self.OptionParser.add_option("--protect-ids-list",         type="string",  action="store", dest="protect_ids_list")
        self.OptionParser.add_option("--protect-ids-prefix",       type="string",  action="store", dest="protect_ids_prefix")
        self.OptionParser.add_option("--enable-viewboxing",        type="inkbool", action="store", dest="enable_viewboxing")
        self.OptionParser.add_option("--enable-comment-stripping", type="inkbool", action="store", dest="strip_comments")
        self.OptionParser.add_option("--renderer-workaround",      type="inkbool", action="store", dest="renderer_workaround")

    def effect(self):
        input = file(self.args[0], "r")
        self.options.infilename = self.args[0]
        sys.stdout.write(scourString(input.read(), self.options).encode("UTF-8"))
        input.close()
        sys.stdout.close()

if __name__ == '__main__':
    e = ScourInkscape()
    e.affect(output=False)
