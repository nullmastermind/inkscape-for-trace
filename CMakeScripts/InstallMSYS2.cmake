if(WIN32)
  install(PROGRAMS
    ${EXECUTABLE_OUTPUT_PATH}/inkscape.exe
    ${EXECUTABLE_OUTPUT_PATH}/inkview.exe
    DESTINATION ${CMAKE_INSTALL_PREFIX}
  )

  install(PROGRAMS
	${EXECUTABLE_OUTPUT_PATH}/inkscape_com.exe
	DESTINATION ${CMAKE_INSTALL_PREFIX}
	RENAME inkscape.com
  )

  install(FILES
    ${LIBRARY_OUTPUT_PATH}/libinkscape_base.dll
    DESTINATION ${CMAKE_INSTALL_PREFIX}
  )

  install(FILES
    AUTHORS
    COPYING
    NEWS
    README
    TRANSLATORS
	GPL2.txt
	GPL3.txt
	LGPL2.1.txt
    DESTINATION ${CMAKE_INSTALL_PREFIX})

  install(DIRECTORY doc
    DESTINATION ${CMAKE_INSTALL_PREFIX})

  # mingw dlls
  install(FILES
    ${MINGW_BIN}/LIBEAY32.dll
    ${MINGW_BIN}/SSLEAY32.dll
    ${MINGW_BIN}/libMagick++-6.Q16HDRI-6.dll
    ${MINGW_BIN}/libMagickCore-6.Q16HDRI-2.dll
    ${MINGW_BIN}/libMagickWand-6.Q16HDRI-2.dll
    ${MINGW_BIN}/libaspell-15.dll
    ${MINGW_BIN}/libatk-1.0-0.dll
    ${MINGW_BIN}/libatkmm-1.6-1.dll
    ${MINGW_BIN}/libbz2-1.dll
    ${MINGW_BIN}/libcairo-2.dll
    ${MINGW_BIN}/libcairo-gobject-2.dll
    ${MINGW_BIN}/libcairomm-1.0-1.dll
    ${MINGW_BIN}/libcdr-0.1.dll
    ${MINGW_BIN}/libcurl-4.dll
    ${MINGW_BIN}/libenchant.dll
    ${MINGW_BIN}/libepoxy-0.dll
    ${MINGW_BIN}/libexpat-1.dll
    ${MINGW_BIN}/libffi-6.dll
    ${MINGW_BIN}/libfftw3-3.dll
    ${MINGW_BIN}/libfontconfig-1.dll
    ${MINGW_BIN}/libfreetype-6.dll
    ${MINGW_BIN}/libgc-1.dll
    ${MINGW_BIN}/libgdk-3-0.dll
    ${MINGW_BIN}/libgdk_pixbuf-2.0-0.dll
    ${MINGW_BIN}/libgdkmm-3.0-1.dll
    ${MINGW_BIN}/libgdl-3-5.dll
    ${MINGW_BIN}/libgio-2.0-0.dll
    ${MINGW_BIN}/libgiomm-2.4-1.dll
    ${MINGW_BIN}/libglib-2.0-0.dll
    ${MINGW_BIN}/libglibmm-2.4-1.dll
    ${MINGW_BIN}/libgmodule-2.0-0.dll
    ${MINGW_BIN}/libgmp-10.dll
    ${MINGW_BIN}/libgnutls-30.dll
    ${MINGW_BIN}/libgobject-2.0-0.dll
    ${MINGW_BIN}/libgomp-1.dll
    ${MINGW_BIN}/libgraphite2.dll
    ${MINGW_BIN}/libgsl-19.dll
    ${MINGW_BIN}/libgslcblas-0.dll
    ${MINGW_BIN}/libgtk-3-0.dll
    ${MINGW_BIN}/libgtkmm-3.0-1.dll
    ${MINGW_BIN}/libgtkspell3-3-0.dll
    ${MINGW_BIN}/libharfbuzz-0.dll
    ${MINGW_BIN}/libhogweed-4.dll
    ${MINGW_BIN}/libiconv-2.dll
    ${MINGW_BIN}/libicudt57.dll
    ${MINGW_BIN}/libicuin57.dll
    ${MINGW_BIN}/libicuuc57.dll
    ${MINGW_BIN}/libidn-11.dll
    ${MINGW_BIN}/libintl-8.dll
    ${MINGW_BIN}/libjpeg-8.dll
    ${MINGW_BIN}/liblcms2-2.dll
    ${MINGW_BIN}/liblqr-1-0.dll
    ${MINGW_BIN}/libltdl-7.dll
    ${MINGW_BIN}/liblzma-5.dll
    ${MINGW_BIN}/libnettle-6.dll
    ${MINGW_BIN}/libnghttp2-14.dll
    ${MINGW_BIN}/libnspr4.dll
    ${MINGW_BIN}/libopenjp2-7.dll
    ${MINGW_BIN}/libp11-kit-0.dll
    ${MINGW_BIN}/libpango-1.0-0.dll
    ${MINGW_BIN}/libpangocairo-1.0-0.dll
    ${MINGW_BIN}/libpangoft2-1.0-0.dll
    ${MINGW_BIN}/libpangomm-1.4-1.dll
    ${MINGW_BIN}/libpangowin32-1.0-0.dll
    ${MINGW_BIN}/libpcre-1.dll
    ${MINGW_BIN}/libpixman-1-0.dll
    ${MINGW_BIN}/libplc4.dll
    ${MINGW_BIN}/libplds4.dll
    ${MINGW_BIN}/libpng16-16.dll
    ${MINGW_BIN}/libpoppler-66.dll
    ${MINGW_BIN}/libpoppler-glib-8.dll
    ${MINGW_BIN}/libpopt-0.dll
    ${MINGW_BIN}/libpotrace-0.dll
    ${MINGW_BIN}/librevenge-0.0.dll
    ${MINGW_BIN}/librevenge-stream-0.0.dll
    ${MINGW_BIN}/librtmp-1.dll
    ${MINGW_BIN}/libsigc-2.0-0.dll
    ${MINGW_BIN}/libssh2-1.dll
    ${MINGW_BIN}/libstdc++-6.dll
    ${MINGW_BIN}/libtasn1-6.dll
    ${MINGW_BIN}/libtiff-5.dll
    ${MINGW_BIN}/libunistring-2.dll
    ${MINGW_BIN}/libvisio-0.1.dll
    ${MINGW_BIN}/libwinpthread-1.dll
    ${MINGW_BIN}/libwpd-0.10.dll
    ${MINGW_BIN}/libwpg-0.3.dll
    ${MINGW_BIN}/libxml2-2.dll
    ${MINGW_BIN}/libxslt-1.dll
    ${MINGW_BIN}/nss3.dll
    ${MINGW_BIN}/nssutil3.dll
    ${MINGW_BIN}/smime3.dll
    ${MINGW_BIN}/zlib1.dll
    # these are not picked up by 'ldd' but are required for SVG support in gdk-pixbuf-2.0
    ${MINGW_BIN}/libcroco-0.6-3.dll
    ${MINGW_BIN}/librsvg-2-2.dll
    # required by python2-lxml
    ${MINGW_BIN}/libexslt-0.dll
    # required by python2-numpy
    ${MINGW_BIN}/libgfortran-3.dll
    ${MINGW_BIN}/libopenblas.dll
    ${MINGW_BIN}/libquadmath-0.dll
    DESTINATION ${CMAKE_INSTALL_PREFIX})
  # There are differences for 64-Bit and 32-Bit build environments.
  if(HAVE_MINGW64)
    install(FILES
      ${MINGW_BIN}/libgcc_s_seh-1.dll
      DESTINATION ${CMAKE_INSTALL_PREFIX})
  else()
    install(FILES
      ${MINGW_BIN}/libgcc_s_dw2-1.dll
      DESTINATION ${CMAKE_INSTALL_PREFIX})
  endif()

  # Install hicolor/index.theme to avoid bug 1635207
  install(FILES
    ${MINGW_PATH}/share/icons/hicolor/index.theme
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor)

  install(DIRECTORY ${MINGW_PATH}/share/icons/Adwaita
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons)

  # translations for libraries (we usually shouldn't need many)
  install(DIRECTORY ${MINGW_PATH}/share/locale
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share
    FILES_MATCHING
    PATTERN "*gtk30.mo"
    PATTERN "*gtkspell3.mo")

  install(DIRECTORY ${MINGW_PATH}/share/poppler
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share)

  install(DIRECTORY ${MINGW_PATH}/share/glib-2.0/schemas
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/glib-2.0)

  # fontconfig
  install(DIRECTORY ${MINGW_PATH}/etc/fonts
    DESTINATION ${CMAKE_INSTALL_PREFIX}/etc
    FILES_MATCHING PATTERN "fonts.conf" EXCLUDE)
  # adjust fonts.conf to store font cache in AppData
  set(cachedir_default "\\t^<cachedir^>/var/cache/fontconfig^</cachedir^>") # the '^' are needed to escape angle brackets on Windows command shell
  set(cachedir_appdata "\\t^<cachedir^>LOCAL_APPDATA_FONTCONFIG_CACHE^</cachedir^>")
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/etc/fonts/fonts.conf
    COMMAND sed 's!${cachedir_default}!${cachedir_appdata}\\n${cachedir_default}!' ${MINGW_PATH}/etc/fonts/fonts.conf  > ${CMAKE_BINARY_DIR}/etc/fonts/fonts.conf
    MAIN_DEPENDENCY ${MINGW_PATH}/etc/fonts/fonts.conf
  )
  add_custom_target(fonts_conf ALL DEPENDS ${CMAKE_BINARY_DIR}/etc/fonts/fonts.conf)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/etc/fonts
    DESTINATION ${CMAKE_INSTALL_PREFIX}/etc)

  # GTK 3.0
  install(DIRECTORY ${MINGW_LIB}/gtk-3.0
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    FILES_MATCHING
    PATTERN "*.dll"
    PATTERN "*.cache")

  install(DIRECTORY ${MINGW_PATH}/etc/gtk-3.0
    DESTINATION ${CMAKE_INSTALL_PREFIX}/etc)

  install(DIRECTORY ${MINGW_LIB}/gdk-pixbuf-2.0
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    FILES_MATCHING
    PATTERN "*.dll"
    PATTERN "*.cache")

  # Aspell dictionaries
  install(DIRECTORY ${MINGW_LIB}/aspell-0.60
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

  # Aspell backend for Enchant (gtkspell uses Enchant to access Aspell dictionaries)
  install(FILES
    ${MINGW_LIB}/enchant/libenchant_aspell.dll
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/enchant)

  # Necessary to run extensions on windows if it is not in the path
  if (HAVE_MINGW64)
    install(FILES
      ${MINGW_BIN}/gspawn-win64-helper.exe
      ${MINGW_BIN}/gspawn-win64-helper-console.exe
      DESTINATION ${CMAKE_INSTALL_PREFIX})
  else()
    install(FILES
      ${MINGW_BIN}/gspawn-win32-helper.exe
      ${MINGW_BIN}/gspawn-win32-helper-console.exe
      DESTINATION ${CMAKE_INSTALL_PREFIX})
  endif()

  # Python (a bit hacky for backwards compatibility with devlibs at this point)
  install(FILES
    ${MINGW_BIN}/python2.exe
    RENAME python.exe
    DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(FILES
    ${MINGW_BIN}/python2w.exe
    RENAME pythonw.exe
    DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(FILES
    ${MINGW_BIN}/libpython2.7.dll
    DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(DIRECTORY ${MINGW_LIB}/python2.7
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    PATTERN "python2.7/site-packages" EXCLUDE # specify individual packages to install below
    PATTERN "python2.7/test" EXCLUDE # we don't need the Python testsuite
  )

  set(site_packages "lib/python2.7/site-packages")
  # Python packages installed via pacman
  set(packages "python2-lxml" "python2-numpy")
  foreach(package ${packages})
    list_files_pacman(${package} paths)
    install_list(FILES ${paths}
      ROOT ${MINGW_PATH}
      INCLUDE ${site_packages} # only include content from "site-packages" (we might consider to install everything)
    )
  endforeach()
  # Python packages installed via pip
  set(packages "coverage" "pyserial" "scour" "six")
  foreach(package ${packages})
    list_files_pip(${package} paths)
    install_list(FILES ${paths}
      ROOT ${MINGW_PATH}/${site_packages}
      DESTINATION ${site_packages}/
      EXCLUDE "^\\.\\.\\/" # exclude content in parent directories (notably scripts installed to /bin)
    )
  endforeach()
endif()