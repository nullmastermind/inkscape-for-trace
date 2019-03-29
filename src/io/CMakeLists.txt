# SPDX-License-Identifier: GPL-2.0-or-later

set(io_SRC
  dir-util.cpp
  file.cpp
  file-export-cmd.cpp
  resource.cpp
  resource-manager.cpp
  stream/bufferstream.cpp
  stream/gzipstream.cpp
  stream/inkscapestream.cpp
  stream/stringstream.cpp
  stream/uristream.cpp
  stream/xsltstream.cpp
  sys.cpp
  http.cpp

  # -------
  # Headers
  dir-util.h
  file.h
  file-export-cmd.h
  resource.h
  resource-manager.h
  stream/bufferstream.h
  stream/gzipstream.h
  stream/inkscapestream.h
  stream/stringstream.h
  stream/uristream.h
  stream/xsltstream.h
  sys.h
  http.h
)

# add_inkscape_lib(io_LIB "${io_SRC}")
add_inkscape_source("${io_SRC}")
