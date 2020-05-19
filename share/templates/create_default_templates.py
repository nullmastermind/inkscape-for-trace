#!/usr/bin/env python3
#
# Creates localized default templates
# (uses default.svg as base and reads localized strings directly from .po/.gmo files)
#

from __future__ import print_function
from __future__ import unicode_literals  # make all literals unicode strings by default (even in Python 2)

import gettext
import glob
import os
import shutil
import sys
from io import open  # needed for support of encoding parameter in Python 2


LAYER_STRING = 'Layer'


if len(sys.argv) != 3:
    sys.exit("Usage:\n  %s ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}" % sys.argv[0])

source_dir = sys.argv[1]
binary_dir = sys.argv[2]


# get available languages (should match the already created .gmo files)
gmofiles = glob.glob(binary_dir + '/po/*.gmo')

languages = gmofiles
languages = [os.path.basename(language) for language in languages]  # split filename from path
languages = [os.path.splitext(language)[0] for language in languages]  # split extension


# process each language sequentially
for language in languages:
    # copy .gmo file into a location where gettext can find and use it
    source = binary_dir + '/po/' + language + '.gmo'
    destination_dir = binary_dir + '/po/locale/' + language + '/LC_MESSAGES/'
    destination = destination_dir + 'inkscape.mo'

    if not os.path.isdir(destination_dir):
        os.makedirs(destination_dir)
    shutil.copy(source, destination)

# do another loop to ensure we've copied all the translations before using them
for language in languages:
    # get translation with help of gettext
    translation = gettext.translation('inkscape', localedir=binary_dir + '/po/locale', languages=[language])
    translated_string = translation.gettext(LAYER_STRING)

    if type(translated_string) != type(LAYER_STRING):  # python2 compat (make sure translation is a Unicode string)
        translated_string = translated_string.decode('utf-8')

    # now create localized version of English template file (if we have a translation)
    template_file = source_dir + '/share/templates/default.svg'
    output_file = binary_dir + '/share/templates/default.' + language + '.svg'

    if os.path.isfile(output_file):
        os.remove(output_file)
    if translated_string != LAYER_STRING:
        with open(template_file, 'r', encoding='utf-8', newline='\n') as file:
            filedata = file.read()
        filedata = filedata.replace('Layer', translated_string)
        with open(output_file, 'w', encoding='utf-8', newline='\n') as file:
            file.write(filedata)


# create timestamp file (indicates last successful creation for build system)
timestamp_file = binary_dir + '/share/templates/default_templates.timestamp'
if os.path.exists(timestamp_file):
    os.utime(timestamp_file, None)
else:
    open(timestamp_file, 'a').close()
