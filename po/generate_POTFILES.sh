#! /bin/sh

set -e

echo "Generating updated POTFILES list..."
mydir=`dirname "$0"`
cd "$mydir"

# enforce consistent sort order and date format
export LC_ALL=C

(
 echo "../share/filters/filters.svg.h"
 echo "../share/palettes/palettes.h"
 echo "../share/patterns/patterns.svg.h"
 echo "../share/symbols/symbols.h"
 echo "../share/templates/templates.h"

 find ../src \( -name '*.cpp' -o -name '*.[ch]' \) -type f -print0 | xargs -0 egrep -l '(\<[QNC]?_|gettext) *\(' | sort

) | grep -vx -f POTFILES.skip > POTFILES.src.in


find ../share/extensions -name '*.py' -type f -print0 | xargs -0 egrep -l '(\<[QNC]?_|gettext) *\(' | sort > POTFILES.py.in
find ../share/extensions -name '*.inx' -type f -print | sort > POTFILES.inx.in
find ../share/ui -name '*.glade' -type f -print0 | sort > POTFILES.ui.in


