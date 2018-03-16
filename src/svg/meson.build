svg_sources = files([
  'css-ostringstream.cpp',
  'path-string.cpp',
#  'sp-svg.def',
  'stringstream.cpp',
  'strip-trailing-zeros.cpp',
  'svg-affine.cpp',
  'svg-color.cpp',
  'svg-angle.cpp',
  'svg-length.cpp',
  'svg-path.cpp'
#  'test-stubs.cpp'
])

svg_headers = files([
  'css-ostringstream-test.h',
  'css-ostringstream.h',
  'path-string.h',
  'stringstream-test.h',
  'stringstream.h',
  'strip-trailing-zeros.h',
  'svg-affine-test.h',
  'svg-color-test.h',
  'svg-color.h',
  'svg-icc-color.h',
  'svg-angle.h',
  'svg-length-test.h',
  'svg-length.h',
  'svg-path-geom-test.h',
  'svg.h'
#  'test-stubs.h'
])

svg_deps = [
  glibmm_dep
]

libsvg = static_library('libsvg',
  sources: [svg_sources,
            svg_headers],
  dependencies: svg_deps,
  include_directories: [srcinc, confinc])

libsvg_dep = declare_dependency(link_with : libsvg)
