#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# License checker: test that files have a proper SPDX license header.
# Author: Max Gaukler <development@maxgaukler.de>
# Licensed under GPL version 2 or any later version, read the file "COPYING" for more information.

import os
import sys
import subprocess
license = {}
hasSPDX = {}

# do not check licenses in these subdirectories:
# TODO: have look at the libraries' licenses
IGNORE_PATHS = ["./LICENSES", "./ccache", "./build", "./inst/", "./doc", "./man", "./packaging", "./po", "./share/", "./src/3rdparty/", "./src/2geom/", "./CMakeScripts/"]
# do not check licenses for the following file endings:
IGNORE_FILE_ENDINGS = ["README", "README.md", "NEWS", "NEWS.md", "AUTHORS", "CONTRIBUTING.md", "INSTALL.md", "HACKING", "COPYING", "BUILD_YOUR_OWN", "TRANSLATORS", ".ods", ".dia", ".bz2", ".bmp", ".dll", ".png", ".svg", ".po", ".rc", ".kate-swp", ".xpm", ".xml", "Notes.txt", "todo.txt"]
# permitted licenses (MUST BE compatible with licensing the compiled product as GPL3).
# IF YOU CHANGE THIS, also update the list of licenses in COPYING!
PERMITTED_LICENSES = ["GPL-2.0-or-later", "GPL-2.0-or-later OR MPL-1.1 OR LGPL-2.1-or-later", "GPL-3.0-or-later", "LGPL-2.1-or-later", "GPL-3.0-or-later", "LGPL-3.0-or-later"]

if not os.path.exists("./LICENSES"):
    print("this script must be run from the main git directory", file=sys.stderr)
    sys.exit(1)

for root, dirs, files in os.walk("."):
    for name in files:
        p = os.path.join(root,name)
        if ".git" in p:
            continue
        if sum([p.endswith(i) for i in IGNORE_FILE_ENDINGS]):
            continue
        if sum([p.startswith(i) for i in IGNORE_PATHS]):
            continue
        if subprocess.call(["git", "check-ignore", "-q", "--", p]) == 0:
            # file is in .gitignore
            continue
        license[p] = None
        hasSPDX[p] = False
        
        
        try:
            for line in open(p, encoding='utf-8').readlines():
                line = line.strip(' */#<>-!;\r\n')
                if line.startswith("SPDX-License-Identifier: "):
                    hasSPDX[p] = True
                    license[p] = line[len("SPDX-License-Identifier: "):]
        except UnicodeDecodeError:
            print("Encoding of {} is damaged (should be UTF8), cannot check license".format(p), file=sys.stderr)
            print("If you think this message is wrong, edit buildtools/check_license_header.py", file=sys.stderr)
            sys.exit(1)
        if not hasSPDX[p]:
            print("File '{}' does not have a SPDX-License-Identifier: header.".format(p), file=sys.stderr)
            print("Please have a look at the coding style: https://inkscape.org/en/develop/coding-style/", file=sys.stderr)
            print("This is required so that we can make sure all files have compatible licenses.", file=sys.stderr)
            print("If you think this message is wrong, edit buildtools/check_license_header.py", file=sys.stderr)
            sys.exit(1)
        if not license[p] in PERMITTED_LICENSES:
            print("File '{}' has an incompatible or unknown license '{}' in the SPDX-License-Identifier header.".format(p, license[p]), file=sys.stderr)
            print("Allowed licenses are: ", file=sys.stderr)
            print("\n".join(PERMITTED_LICENSES), file=sys.stderr)
            print("If you think this message is wrong, edit buildtools/check_license_header.py", file=sys.stderr)
            sys.exit(1)
        
        
            
