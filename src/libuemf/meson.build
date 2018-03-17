uemf_sources = files([
  'symbol_convert.c',
  'uemf.c',
  'uemf_endian.c',
  'uemf_print.c',
  'uemf_safe.c',
  'uemf_utf.c',
  'uwmf.c',
  'uwmf_endian.c',
  'uwmf_print.c',
  'upmf.c',
  'upmf_print.c'
])

uemf_headers = files([
  'symbol_convert.h',
  'uemf.h',
  'uemf_endian.h',
  'uemf_print.h',
  'uemf_safe.h',
  'uemf_utf.h',
  'uwmf.h',
  'uwmf_endian.h',
  'uwmf_print.h',
  'upmf.h',
  'upmf_print.h'
])

uemf_deps = []

libuemf = static_library('uemf',
  sources: [uemf_sources,
            uemf_headers],
  dependencies: uemf_deps,
  include_directories: [srcinc, confinc])

libuemf_dep = declare_dependency(link_with : libuemf)
