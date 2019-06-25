############################
# CPack configuration file #
############################


## General ##

set(CPACK_PACKAGE_NAME "Inkscape")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source vector graphics editor")
set(CPACK_PACKAGE_VENDOR "Inkscape")
set(CPACK_PACKAGE_CONTACT "Inkscape developers <inkscape-devel@lists.sourceforge.net>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://inkscape.org")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION_MAJOR ${INKSCAPE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${INKSCAPE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${INKSCAPE_VERSION_PATCH})
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.ico")

set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}~${INKSCAPE_VERSION_SUFFIX}")
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;/obj*/;tags;cscope.*")
set(INKSCAPE_CPACK_PREFIX ${PROJECT_NAME}-${INKSCAPE_VERSION}_${INKSCAPE_REVISION_DATE}_${INKSCAPE_REVISION_HASH})
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${INKSCAPE_CPACK_PREFIX})
set(CPACK_PACKAGE_FILE_NAME ${INKSCAPE_CPACK_PREFIX})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "inkscape")
set(CPACK_SOURCE_GENERATOR "TXZ")
set(CPACK_PACKAGE_EXECUTABLES "inkscape;Inkscape;inkview;Inkview")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)
configure_file("${CMAKE_SOURCE_DIR}/CMakeScripts/CPack.cmake" "${CMAKE_BINARY_DIR}/CMakeScripts/CPack.cmake" @ONLY)
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CMakeScripts/CPack.cmake")

## Windows ##

if (WIN32)
    set(CPACK_GENERATOR "ZIP")
    ### nsis generator
    find_package(NSIS)
    if (NSIS_MAKE)
        set(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
        set(CPACK_NSIS_DISPLAY_NAME "Inkscape")
        set(CPACK_NSIS_COMPRESSOR "/SOLID zlib")
        set(CPACK_NSIS_MENU_LINKS "https://inkscape.org/" "Inkscape homepage")
    endif (NSIS_MAKE)

    #WIX
    set(CPACK_WIX_UPGRADE_GUID "4d5fedaa-84a0-48be-bd2a-08246398361a")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.ico")
    set(CPACK_WIX_UI_BANNER  "${CMAKE_SOURCE_DIR}/packaging/wix/Bitmaps/banner.bmp")
    set(CPACK_WIX_UI_DIALOG  "${CMAKE_SOURCE_DIR}/packaging/wix/Bitmaps/dialog.bmp")
endif (WIN32)

## Linux ##

### DEB ###
if(UNIX)
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libaspell15 (>= 0.60.7~20110707), libatkmm-1.6-1v5 (>= 2.24.0), libc6 (>= 2.14), libcairo2 (>= 1.14.0), libcairomm-1.0-1v5 (>= 1.12.0), libcdr-0.1-1, libdbus-glib-1-2 (>= 0.88), libfontconfig1 (>= 2.12), libfreetype6 (>= 2.2.1), libgc1c2 (>= 1:7.2d), libgcc1 (>= 1:4.0), libgdk-pixbuf2.0-0 (>= 2.22.0), libgdl-3-5 (>= 3.8.1), libglib2.0-0 (>= 2.41.1), libglibmm-2.4-1v5 (>= 2.54.0), libgomp1 (>= 4.9), libgsl23, libgslcblas0, libgtk-3-0 (>= 3.21.5), libgtkmm-3.0-1v5 (>= 3.22.0), libgtkspell3-3-0, libharfbuzz0b (>= 1.2.6), libjpeg8 (>= 8c), liblcms2-2 (>= 2.2+git20110628), libmagick++-6.q16-7 (>= 8:6.9.6.8), libpango-1.0-0 (>= 1.37.2), libpangocairo-1.0-0 (>= 1.14.0), libpangoft2-1.0-0 (>= 1.37.2), libpangomm-1.4-1v5 (>= 2.40.0), libpng16-16 (>= 1.6.2-1), libpoppler-glib8 (>= 0.18.0), libpoppler68 (>= 0.57.0), libpotrace0, librevenge-0.0-0, libsigc++-2.0-0v5 (>= 2.8.0), libsoup2.4-1 (>= 2.41.90), libstdc++6 (>= 5.2), libvisio-0.1-1, libwpg-0.3-3, libx11-6, libxml2 (>= 2.7.4), libxslt1.1 (>= 1.1.25), zlib1g (>= 1:1.1.4)")
    IF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
      FIND_PROGRAM(DPKG_CMD dpkg)
      IF(NOT DPKG_CMD)
        MESSAGE(STATUS "Can not find dpkg in your path, default to amd64.")
        SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
      ENDIF(NOT DPKG_CMD)
        EXECUTE_PROCESS(COMMAND "${DPKG_CMD}" --print-architecture
            OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    SET(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "aspell, imagemagick, libwmf-bin, perlmagick, python-numpy, python-lxml, python-scour, python-uniconvertor")
    SET(CPACK_DEBIAN_PACKAGE_SUGGESTS "dia, libsvg-perl, libxml-xql-perl, pstoedit, python-uniconvertor, ruby")
    install(FILES ${CMAKE_SOURCE_DIR}/COPYING RENAME copyright DESTINATION ${SHARE_INSTALL}/doc/inkscape/)
endif(UNIX)

### RPM ###


## MacOS ##

#[[
.. variable:: CPACK_DMG_VOLUME_NAME

 The volume name of the generated disk image. Defaults to
 CPACK_PACKAGE_FILE_NAME.

.. variable:: CPACK_DMG_FORMAT

 The disk image format. Common values are ``UDRO`` (UDIF read-only), ``UDZO`` (UDIF
 zlib-compressed) or ``UDBZ`` (UDIF bzip2-compressed). Refer to ``hdiutil(1)`` for
 more information on other available formats. Defaults to ``UDZO``.

.. variable:: CPACK_DMG_DS_STORE

 Path to a custom ``.DS_Store`` file. This ``.DS_Store`` file can be used to
 specify the Finder window position/geometry and layout (such as hidden
 toolbars, placement of the icons etc.). This file has to be generated by
 the Finder (either manually or through AppleScript) using a normal folder
 from which the ``.DS_Store`` file can then be extracted.

.. variable:: CPACK_DMG_DS_STORE_SETUP_SCRIPT

 Path to a custom AppleScript file.  This AppleScript is used to generate
 a ``.DS_Store`` file which specifies the Finder window position/geometry and
 layout (such as hidden toolbars, placement of the icons etc.).
 By specifying a custom AppleScript there is no need to use
 ``CPACK_DMG_DS_STORE``, as the ``.DS_Store`` that is generated by the AppleScript
 will be packaged.

.. variable:: CPACK_DMG_BACKGROUND_IMAGE

 Path to an image file to be used as the background.  This file will be
 copied to ``.background``/``background.<ext>``, where ``<ext>`` is the original image file
 extension.  The background image is installed into the image before
 ``CPACK_DMG_DS_STORE_SETUP_SCRIPT`` is executed or ``CPACK_DMG_DS_STORE`` is
 installed.  By default no background image is set.

.. variable:: CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK

 Default behaviour is to include a symlink to ``/Applications`` in the DMG.
 Set this option to ``ON`` to avoid adding the symlink.

.. variable:: CPACK_DMG_SLA_DIR

  Directory where license and menu files for different languages are stored.
  Setting this causes CPack to look for a ``<language>.menu.txt`` and
  ``<language>.license.txt`` file for every language defined in
  ``CPACK_DMG_SLA_LANGUAGES``. If both this variable and
  ``CPACK_RESOURCE_FILE_LICENSE`` are set, CPack will only look for the menu
  files and use the same license file for all languages.

.. variable:: CPACK_DMG_SLA_LANGUAGES

  Languages for which a license agreement is provided when mounting the
  generated DMG. A menu file consists of 9 lines of text. The first line is
  is the name of the language itself, uppercase, in English (e.g. German).
  The other lines are translations of the following strings:

  - Agree
  - Disagree
  - Print
  - Save...
  - You agree to the terms of the License Agreement when you click the
    "Agree" button.
  - Software License Agreement
  - This text cannot be saved. The disk may be full or locked, or the file
    may be locked.
  - Unable to print. Make sure you have selected a printer.

  For every language in this list, CPack will try to find files
  ``<language>.menu.txt`` and ``<language>.license.txt`` in the directory
  specified by the ``CPACK_DMG_SLA_DIR`` variable.

.. variable:: CPACK_COMMAND_HDIUTIL

 Path to the ``hdiutil(1)`` command used to operate on disk image files on
 macOS. This variable can be used to override the automatically detected
 command (or specify its location if the auto-detection fails to find it).

.. variable:: CPACK_COMMAND_SETFILE

 Path to the ``SetFile(1)`` command used to set extended attributes on files and
 directories on macOS. This variable can be used to override the
 automatically detected command (or specify its location if the
 auto-detection fails to find it).

 .. variable:: CPACK_COMMAND_REZ

 Path to the ``Rez(1)`` command used to compile resources on macOS. This
 variable can be used to override the automatically detected command (or
 specify its location if the auto-detection fails to find it).



#Variables specific to CPack Bundle generator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Installers built on macOS using the Bundle generator use the
aforementioned DragNDrop (``CPACK_DMG_xxx``) variables, plus the following
Bundle-specific parameters (``CPACK_BUNDLE_xxx``).

.. variable:: CPACK_BUNDLE_NAME

 The name of the generated bundle. This appears in the OSX finder as the
 bundle name. Required.

.. variable:: CPACK_BUNDLE_PLIST

 Path to an OSX plist file that will be used for the generated bundle. This
 assumes that the caller has generated or specified their own Info.plist
 file. Required.

.. variable:: CPACK_BUNDLE_ICON

 Path to an OSX icon file that will be used as the icon for the generated
 bundle. This is the icon that appears in the OSX finder for the bundle, and
 in the OSX dock when the bundle is opened. Required.

.. variable:: CPACK_BUNDLE_STARTUP_COMMAND

 Path to a startup script. This is a path to an executable or script that
 will be run whenever an end-user double-clicks the generated bundle in the
 OSX Finder. Optional.

.. variable:: CPACK_BUNDLE_APPLE_CERT_APP

 The name of your Apple supplied code signing certificate for the application.
 The name usually takes the form ``Developer ID Application: [Name]`` or
 ``3rd Party Mac Developer Application: [Name]``. If this variable is not set
 the application will not be signed.

.. variable:: CPACK_BUNDLE_APPLE_ENTITLEMENTS

 The name of the ``Plist`` file that contains your apple entitlements for sandboxing
 your application. This file is required for submission to the Mac App Store.

 .. variable:: CPACK_BUNDLE_APPLE_CODESIGN_FILES

 A list of additional files that you wish to be signed. You do not need to
 list the main application folder, or the main executable. You should
 list any frameworks and plugins that are included in your app bundle.

.. variable:: CPACK_BUNDLE_APPLE_CODESIGN_PARAMETER

 Additional parameter that will passed to ``codesign``.
 Default value: ``--deep -f``

.. variable:: CPACK_COMMAND_CODESIGN

 Path to the ``codesign(1)`` command used to sign applications with an
 Apple cert. This variable can be used to override the automatically
 detected command (or specify its location if the auto-detection fails
 to find it).



 See also : PackageMaker 

 #]]

include(CPack)
