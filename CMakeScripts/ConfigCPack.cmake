############################
# CPack configuration file #
############################


## General ##

set(CPACK_PACKAGE_NAME "Inkscape")
set(CPACK_PACKAGE_VENDOR "Inkscape")
set(CPACK_PACKAGE_VERSION_MAJOR ${INKSCAPE_VERSION_MAJOR}) # TODO: Can be set via project(), see CMAKE_PROJECT_VERSION_PATCH
set(CPACK_PACKAGE_VERSION_MINOR ${INKSCAPE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${INKSCAPE_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${INKSCAPE_VERSION_SUFFIX}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md") # TODO: Where is this used? Do we need a better source?
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source vector graphics editor")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://inkscape.org")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.svg") # TODO: Can any generator make use of this?
set(CPACK_PACKAGE_CONTACT "Inkscape developers <inkscape-devel@lists.inkscape.org>")

set(CPACK_PACKAGE_FILE_NAME ${INKSCAPE_DIST_PREFIX})
set(CPACK_PACKAGE_CHECKSUM "SHA256")

set(CPACK_PACKAGE_EXECUTABLES "inkscape;Inkscape;inkview;Inkview")
set(CPACK_CREATE_DESKTOP_LINKS "inkscape")
if(WIN32)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Inkscape")
else()
set(CPACK_PACKAGE_INSTALL_DIRECTORY "inkscape")
endif()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
# set( CPACK_RESOURCE_FILE_WELCOME "${CMAKE_SOURCE_DIR}/README.md") # TODO: can we use this?

set(CPACK_STRIP_FILES TRUE)
set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)

# specific config for source packaging (note this is used by the 'dist' target)
set(CPACK_SOURCE_GENERATOR "TXZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${INKSCAPE_DIST_PREFIX})
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;/obj*/;cscope.*;.gitlab*;.coveragerc;*.md;")




# this allows to override above configuration per cpack generator at CPack-time
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CMakeScripts/CPack.cmake")
configure_file("${CMAKE_SOURCE_DIR}/CMakeScripts/CPack.cmake" "${CMAKE_BINARY_DIR}/CMakeScripts/CPack.cmake" @ONLY)




## Generator-specific configuration ##

# NSIS (Windows .exe installer)
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.ico")
set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_SOURCE_DIR}/packaging/nsis/header.bmp")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/packaging/nsis/welcomefinish.bmp")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin/inkscape.exe")
set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_NSIS_MENU_LINKS "${CPACK_PACKAGE_HOMEPAGE_URL}" "Inkscape Homepage")
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma") # zlib|bzip2|lzma
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
set(CPACK_NSIS_MODIFY_PATH "ON") # while the name does not suggest it, this also provides the possibility to add desktop icons
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "inkscape") # TODO: this results in instance with administrative privileges!

set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  SetCompressorDictSize 64") # hack (improve compression)
set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  BrandingText '${CPACK_PACKAGE_DESCRIPTION_SUMMARY}'") # hack (overwrite BrandingText)
set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  !define MUI_COMPONENTSPAGE_SMALLDESC") # hack (better components page layout)

file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/packaging/nsis/fileassoc.nsh" native_path)
string(REPLACE "\\" "\\\\" native_path "${native_path}")
set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "!include ${native_path}")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "\
  !insertmacro APP_ASSOCIATE 'svg' 'Inkscape.SVG' 'Scalable Vector Graphics' '$INSTDIR\\\\bin\\\\inkscape.exe,0' 'Open with Inkscape' '$INSTDIR\\\\bin\\\\inkscape.exe \\\"%1\\\"'\n\
  !insertmacro APP_ASSOCIATE 'svgz' 'Inkscape.SVGZ' 'Compressed Scalable Vector Graphics' '$INSTDIR\\\\bin\\\\inkscape.exe,0' 'Open with Inkscape' '$INSTDIR\\\\bin\\\\inkscape.exe \\\"%1\\\"'\n\
  !insertmacro UPDATEFILEASSOC")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "\
  !insertmacro APP_UNASSOCIATE 'svg' 'Inkscape.SVG'\n\
  !insertmacro APP_UNASSOCIATE 'svgz' 'Inkscape.SVGZ'\n\
  !insertmacro UPDATEFILEASSOC")

# WIX (Windows .msi installer)
set(CPACK_WIX_UPGRADE_GUID "4d5fedaa-84a0-48be-bd2a-08246398361a")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.ico")
set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/packaging/wix/Bitmaps/banner.bmp")
set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/packaging/wix/Bitmaps/dialog.bmp")
set(CPACK_WIX_PROPERTY_ARPHELPLINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_WIX_PROPERTY_ARPURLUPDATEINFO "${CPACK_PACKAGE_HOMEPAGE_URL}/release")
set(CPACK_WIX_ROOT_FEATURE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
set(CPACK_WIX_LIGHT_EXTRA_FLAGS "-dcl:high") # set high compression

set(CPACK_WIX_PATCH_FILE "${CMAKE_SOURCE_DIR}/packaging/wix/file_association.xml"
                         "${CMAKE_SOURCE_DIR}/packaging/wix/feature_attributes.xml")

# DEB (Linux .deb bundle)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libaspell15 (>= 0.60.7~20110707), libatkmm-1.6-1v5 (>= 2.24.0), libc6 (>= 2.14), libcairo2 (>= 1.14.0), libcairomm-1.0-1v5 (>= 1.12.0), libcdr-0.1-1, libdbus-glib-1-2 (>= 0.88), libfontconfig1 (>= 2.12), libfreetype6 (>= 2.2.1), libgc1c2 (>= 1:7.2d), libgcc1 (>= 1:4.0), libgdk-pixbuf2.0-0 (>= 2.22.0), libgdl-3-5 (>= 3.8.1), libglib2.0-0 (>= 2.41.1), libglibmm-2.4-1v5 (>= 2.54.0), libgomp1 (>= 4.9), libgsl23, libgslcblas0, libgtk-3-0 (>= 3.21.5), libgtkmm-3.0-1v5 (>= 3.22.0), libgtkspell3-3-0, libharfbuzz0b (>= 1.2.6), libjpeg8 (>= 8c), liblcms2-2 (>= 2.2+git20110628), libmagick++-6.q16-7 (>= 8:6.9.6.8), libpango-1.0-0 (>= 1.37.2), libpangocairo-1.0-0 (>= 1.14.0), libpangoft2-1.0-0 (>= 1.37.2), libpangomm-1.4-1v5 (>= 2.40.0), libpng16-16 (>= 1.6.2-1), libpoppler-glib8 (>= 0.18.0), libpoppler68 (>= 0.57.0), libpotrace0, librevenge-0.0-0, libsigc++-2.0-0v5 (>= 2.8.0), libsoup2.4-1 (>= 2.41.90), libstdc++6 (>= 5.2), libvisio-0.1-1, libwpg-0.3-3, libx11-6, libxml2 (>= 2.7.4), libxslt1.1 (>= 1.1.25), zlib1g (>= 1:1.1.4)")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "aspell, imagemagick, libwmf-bin, perlmagick, python-numpy, python-lxml, python-scour, python-uniconvertor")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "dia, libsvg-perl, libxml-xql-perl, pstoedit, python-uniconvertor, ruby")




## load cpack module (do this *after* all the CPACK_* variables have been set)
include(CPack)




## Component definition
#  - variable names are UPPER CASE, even if component names are lower case
#  - components/groups are ordered alphabetically by component/group name; groups always come first
#  - empty (e.g. OS-specific) components are discarded automatically

cpack_add_install_type(full    DISPLAY_NAME Full)
cpack_add_install_type(compact DISPLAY_NAME Compact) # no translations, dictionaries, examples, etc.
cpack_add_install_type(minimal DISPLAY_NAME Minimal) # minimal "working" set-up, excluding everything non-essential

cpack_add_component_group(
                    group_1_program_files
                    DISPLAY_NAME "Program Files"
                    EXPANDED)
cpack_add_component(inkscape
                    DISPLAY_NAME "Inkscape SVG Editor"
                    DESCRIPTION "Inkscape core files and dependencies"
                    GROUP "group_1_program_files"
                    REQUIRED)
cpack_add_component(python
                    DISPLAY_NAME "Python"
                    DESCRIPTION "Python interpreter (required to run Inkscape extensions)"
                    GROUP "group_1_program_files"
                    INSTALL_TYPES full compact)

cpack_add_component_group(
                    group_2_inkscape_data
                    DISPLAY_NAME "Inkscape Data"
                    EXPANDED)
cpack_add_component(extensions
                    DISPLAY_NAME "Extensions"
                    DESCRIPTION "Inkscape extensions (including many import and export plugins)"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full compact)
cpack_add_component(examples
                    DISPLAY_NAME "Examples"
                    DESCRIPTION "Example files created in Inkscape"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full)
cpack_add_component(tutorials
                    DISPLAY_NAME "Tutorials"
                    DESCRIPTION "Tutorials teaching Inkscape usage"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full)

cpack_add_component(dictionaries
                    DISPLAY_NAME "Dictionaries"
                    DESCRIPTION "Dictionaries for some common languages for spell checking in Inkscape"
                    INSTALL_TYPES full)

cpack_add_component_group(
                    group_3_translations
                    DISPLAY_NAME "Translations"
                    DESCRIPTION "Translations and localized content for Inkscape")
get_inkscape_languages()
list(LENGTH INKSCAPE_LANGUAGE_CODES length)
math(EXPR length "${length} - 1")
foreach(index RANGE ${length})
    list(GET INKSCAPE_LANGUAGE_CODES ${index} language_code)
    list(GET INKSCAPE_LANGUAGE_NAMES ${index} language_name)
    string(MAKE_C_IDENTIFIER "${language_code}" language_code_escaped)
    cpack_add_component(translations.${language_code_escaped}
                        DISPLAY_NAME "${language_name}"
                        GROUP "group_3_translations"
                        INSTALL_TYPES full)
endforeach()

# TODO: Separate themes and make optional depending on size
