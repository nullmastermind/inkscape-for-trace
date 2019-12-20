if(UNIX)
    # The install directive for the binaries and libraries are found in src/CMakeList.txt

    install(FILES
        AUTHORS
        TRANSLATORS
        DESTINATION ${INKSCAPE_SHARE_INSTALL}/doc)

    # TODO: install license

    # desktop file / appdata / etc.
    install(FILES
      ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop
      DESTINATION ${SHARE_INSTALL}/applications)
    install(FILES ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml
      DESTINATION ${SHARE_INSTALL}/metainfo)
elseif(WIN32)
    include(CMakeScripts/InstallMSYS2.cmake)
endif()
