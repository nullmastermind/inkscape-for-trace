#!/usr/bin/env python
# -*- coding: utf-8 -*-

import platform
import sys

from distutils.version import StrictVersion

import inkex

try:
    import scour
    try:
        from scour.scour import scourString
    except ImportError:  # compatibility for very old Scour (<= 0.26) - deprecated!
        try:
            from scour import scourString
            scour.__version__ = scour.VER
        except:
            raise
except Exception as e:
    inkex.errormsg("Failed to import Python module 'scour'.\nPlease make sure it is installed (e.g. using 'pip install scour' or 'sudo apt-get install python-scour') and try again.")
    inkex.errormsg("\nDetails:\n" + str(e))
    sys.exit()

try:
    import six
except Exception as e:
    inkex.errormsg("Failed to import Python module 'six'.\nPlease make sure it is installed (e.g. using 'pip install six' or 'sudo apt-get install python-six') and try again.")
    inkex.errormsg("\nDetails:\n" + str(e))
    sys.exit()


class ScourInkscape (inkex.Effect):

    def __init__(self):
        inkex.Effect.__init__(self)

        # Scour options
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

        # options for internal use of the extension
        self.OptionParser.add_option("--scour-version",            type="string",  action="store", dest="scour_version")
        self.OptionParser.add_option("--scour-version-warn-old",   type="inkbool", action="store", dest="scour_version_warn_old")

    def effect(self):
        # version check if enabled in options
        if (self.options.scour_version_warn_old):
            scour_version = scour.__version__
            scour_version_min = self.options.scour_version
            if (StrictVersion(scour_version) < StrictVersion(scour_version_min)):
                inkex.errormsg("The extension 'Optimized SVG Output' is designed for Scour " + scour_version_min + " and later "
                               "but you're using the older version Scour " + scour_version + ".")
                inkex.errormsg("This usually works just fine but not all options available in the UI might be supported "
                               "by the version of Scour installed on your system "
                               "(see https://github.com/scour-project/scour/blob/master/HISTORY.md for release notes of Scour).")
                inkex.errormsg("Note: You can permanently disable this message on the 'About' tab of the extension window.")
        del self.options.scour_version
        del self.options.scour_version_warn_old

        # do the scouring
        try:
            input = file(self.args[0], "r")
            self.options.infilename = self.args[0]
            sys.stdout.write(scourString(input.read(), self.options).encode("UTF-8"))
            input.close()
            sys.stdout.close()
        except Exception as e:
            inkex.errormsg("Error during optimization.")
            inkex.errormsg("\nDetails:\n" + str(e))
            inkex.errormsg("\nOS version: " + platform.platform())
            inkex.errormsg("Python version: " + sys.version)
            inkex.errormsg("Scour version: " + scour.__version__)
            sys.exit()


if __name__ == '__main__':
    e = ScourInkscape()
    e.affect(output=False)
