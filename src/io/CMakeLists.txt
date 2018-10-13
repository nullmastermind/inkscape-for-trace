
set(io_SRC
  base64stream.cpp
  bufferstream.cpp
  dir-util.cpp
  gzipstream.cpp
  inkscapestream.cpp
  resource.cpp
  resource-manager.cpp
  stringstream.cpp
  sys.cpp
  http.cpp
  uristream.cpp
  xsltstream.cpp

  # -------
  # Headers
  base64stream.h
  bufferstream.h
  dir-util.h
  gzipstream.h
  inkscapestream.h
  resource.h
  resource-manager.h
  stringstream.h
  sys.h
  http.h
  uristream.h
  xsltstream.h
)

if(WIN32)
  # Sources for the inkscape executable on Windows.
  list(APPEND io_SRC
    registrytool.h
    registrytool.cpp
  )
endif()

# add_inkscape_lib(io_LIB "${io_SRC}")
add_inkscape_source("${io_SRC}")
