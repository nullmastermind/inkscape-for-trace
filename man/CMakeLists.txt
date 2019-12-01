# SPDX-License-Identifier: GPL-2.0-or-later

# Load AUTHORS file contents into $INKSCAPE_AUTHORS
file(READ ${CMAKE_SOURCE_DIR}/AUTHORS content)
string(REGEX REPLACE "^([^\n#]+)\n" "\\1;\n" content_list "${content}")
join(INKSCAPE_AUTHORS "," "${content_list}")


file(GLOB podfiles *.pod.in)
foreach(podfile ${podfiles})
    get_filename_component(basename ${podfile} NAME)
    string(REPLACE ".pod.in" "" basename ${basename})

    set(POD_IN ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.pod.in)
    set(POD_OUT ${CMAKE_CURRENT_BINARY_DIR}/${basename}.pod)

    configure_file(${POD_IN} ${POD_OUT})

    if(basename MATCHES "^inkscape")
        pod2man(${POD_OUT} ${INKSCAPE_VERSION} 1 "Inkscape Commands Manual")
    elseif(basename MATCHES "^inkview")
        pod2man(${POD_OUT} ${INKSCAPE_VERSION} 1 "Inkview Commands Manual")
    else()
        message(WARNING "unexpected podfile: ${podfile}")
    endif()
endforeach()
