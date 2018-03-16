libio_sources = files([
  'base64stream.cpp',
  'bufferstream.cpp',
  'gzipstream.cpp',
  'inkscapestream.cpp',
  'resource.cpp',
  'stringstream.cpp',
  'sys.cpp',
  'http.cpp',
  'uristream.cpp',
  'xsltstream.cpp'
])

libio_headers = files([
  'base64stream.h',
  'bufferstream.h',
  'gzipstream.h',
  'inkscapestream.h',
  'resource.h',
  'stringstream.h',
  'sys.h',
  'http.h',
  'uristream.h',
  'xsltstream.h'
])

libio_deps = [
  soup_dep,
  glibmm_dep,
  gtk_dep,
  xml2_dep,
  zlib_dep
]

libio = static_library('libio',
  sources: [libio_sources,
            libio_headers],
  dependencies: libio_deps,
  include_directories: [srcinc, confinc])

libio_dep = declare_dependency(link_with : libio)
