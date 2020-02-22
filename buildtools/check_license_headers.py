#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# License checker: test that files have a proper SPDX license header.
# Author: Max Gaukler <development@maxgaukler.de>
# Licensed under GPL version 2 or any later version, read the file "COPYING" for more information.

from __future__ import print_function

import fnmatch
import os
import sys
import subprocess
license = {}
hasSPDX = {}

if sys.version_info[0] < 3:
    from io import open

# do not check licenses in these subdirectories:
# TODO: have look at the libraries' licenses
IGNORE_PATHS = [
    ".git*",
    "CMakeScripts",
    "LICENSES",
    "ccache",
    "build*",
    "doc",
    "inst",
    "man",
    "packaging",
    "patches",
    "po",
    "share",
    "src/2geom",
    "src/3rdparty",
    "testfiles/cli_tests/testcases",
    "testfiles/rendering_tests/fonts/LICENSES",
]

# do not check licenses for the following file endings:
IGNORE_FILE_ENDINGS = [
    ".bmp",
    ".bz2",
    ".dia",
    ".dll",
    ".eps",
    ".kate-swp",
    ".ods",
    ".otf",
    ".pdf",
    ".png",
    ".po",
    ".ps",
    ".rc",
    ".svg",
    ".ttf",
    ".xml",
    ".xpm",
    "AUTHORS",
    "BUILD_YOUR_OWN",
    "CONTRIBUTING.md",
    "COPYING",
    "HACKING",
    "INSTALL.md",
    "NEWS",
    "NEWS.md",
    "Notes.txt",
    "README",
    "README.md",
    "TRANSLATORS",
    "todo.txt",
]

# permitted licenses (MUST BE compatible with licensing the compiled product as GPL3).
# IF YOU CHANGE THIS, also update the list of licenses in COPYING!
PERMITTED_LICENSES = [
    "GPL-2.0-or-later",
    "GPL-3.0-or-later",
    "LGPL-2.1-or-later",
    "LGPL-3.0-or-later",
]


class LicenseCheckError(Exception):
    pass


if not os.path.exists("./LICENSES"):
    print("this script must be run from the main git directory", file=sys.stderr)
    sys.exit(1)


def files_all():
    ignore_paths = [('./' + p) for p in IGNORE_PATHS]
    ignore_paths += [(p + '/*') for p in ignore_paths]

    for root, dirs, files in os.walk("."):
        for name in files:
            p = os.path.join(root,name)
            if any(p.endswith(i) for i in IGNORE_FILE_ENDINGS):
                continue
            if any(fnmatch.fnmatch(p, i) for i in ignore_paths):
                continue
            if subprocess.call(["git", "check-ignore", "-q", "--", p]) == 0:
                # file is in .gitignore
                continue
            yield p


def main(filenames):
    for p in filenames:
        license[p] = None
        hasSPDX[p] = False
        
        
        try:
            for line in open(p, encoding='utf-8').readlines():
                line = line.strip(' */#<>-!;\r\n')
                if line.startswith("SPDX-License-Identifier: "):
                    hasSPDX[p] = True
                    license[p] = line[len("SPDX-License-Identifier: "):]
        except IOError:
            print("Cannot open {} (ignored)".format(p), file=sys.stderr)
            continue
        except UnicodeDecodeError:
            raise LicenseCheckError(
                "Encoding of {} is damaged (should be UTF8), cannot check license"
                .format(p))

        if not hasSPDX[p]:
            raise LicenseCheckError(
                "File '{}' does not have a SPDX-License-Identifier: header.\n"
                "Please have a look at the coding style: https://inkscape.org/en/develop/coding-style/\n"
                "This is required so that we can make sure all files have compatible licenses."
                .format(p))

        if not any(lic in PERMITTED_LICENSES for lic in license[p].split(' OR ')):
            raise LicenseCheckError(
                "File '{}' has an incompatible or unknown license '{}' in the SPDX-License-Identifier header.\n"
                "Allowed licenses are:\n"
                "{}".format(p, license[p], "\n".join(PERMITTED_LICENSES)))


if __name__ == '__main__':
    try:
        main(files_all())
    except LicenseCheckError as e:
        print(e, file=sys.stderr)
        print("If you think this message is wrong, edit buildtools/check_license_header.py", file=sys.stderr)
        sys.exit(1)

# vi:sw=4:expandtab:
