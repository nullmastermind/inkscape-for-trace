libdepixelize_sources = files([
  'kopftracer2011.cpp'])

libdepixelize_headers = files([
  'kopftracer2011.h',
  'splines.h',
  'priv/branchless.h',
  'priv/colorspace.h',
  'priv/curvature.h',
  'priv/homogeneoussplines.h',
  'priv/integral.h',
  'priv/iterator.h',
  'priv/optimization-kopf2011.h',
  'priv/pixelgraph.h',
  'priv/point.h',
  'priv/simplifiedvoronoi.h',
  'priv/splines-kopf2011.h'
])

libdepixelize_deps = [
  glibmm_dep,
  gdkmm_dep
]

libdepixelize = static_library('depixelize',
  sources: [libdepixelize_sources,
            libdepixelize_headers],
  dependencies: libdepixelize_deps,
  include_directories: [srcinc, confinc])

libdepixelize_dep = declare_dependency(link_with : libdepixelize)
