# SPDX-License-Identifier: GPL-2.0-or-later
# Load AUTHORS file contents into $INKSCAPE_AUTHORS
file(READ ${CMAKE_SOURCE_DIR}/AUTHORS content)
string(REGEX REPLACE "^([^\n#]+)\n" "\\1;\n" content_list "${content}")

join(INKSCAPE_AUTHORS "," "${content_list}")

foreach(podfile
        inkscape.pod
        inkscape.de.pod
        inkscape.el.pod
        inkscape.fr.pod
        inkscape.ja.pod
        inkscape.sk.pod
        inkscape.zh_TW.pod
        inkview.pod)

    set(POD_IN ${CMAKE_CURRENT_SOURCE_DIR}/${podfile}.in)
    set(POD_OUT ${CMAKE_CURRENT_BINARY_DIR}/${podfile})

    configure_file(${POD_IN} ${POD_OUT})

    pod2man(${POD_OUT} ${INKSCAPE_VERSION} 1 "Inkscape Commands Manual")
endforeach()
