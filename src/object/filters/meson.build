spobject_filters_sources = files([
  'sp-filter-primitive.cpp',
  'blend.cpp',
  'colormatrix.cpp',
  'componenttransfer-funcnode.cpp',
  'componenttransfer.cpp',
  'composite.cpp',
  'convolvematrix.cpp',
  'diffuselighting.cpp',
  'displacementmap.cpp',
  'distantlight.cpp',
  'flood.cpp',
  'gaussian-blur.cpp',
  'image.cpp',
  'merge.cpp',
  'mergenode.cpp',
  'morphology.cpp',
  'offset.cpp',
  'pointlight.cpp',
  'specularlighting.cpp',
  'spotlight.cpp',
  'tile.cpp',
  'turbulence.cpp'
])

spobject_filters_headers = files([
  'sp-filter-primitive.h',
  'blend.h',
  'colormatrix.h',
  'componenttransfer-funcnode.h',
  'componenttransfer.h',
  'composite.h',
  'convolvematrix.h',
  'diffuselighting.h',
  'displacementmap.h',
  'distantlight.h',
  'flood.h',
  'gaussian-blur.h',
  'image.h',
  'merge.h',
  'mergenode.h',
  'morphology.h',
  'offset.h',
  'pointlight.h',
  'specularlighting.h',
  'spotlight.h',
  'tile.h',
  'turbulence.h'
])

spobject_filters_deps = [
  cairo_dep,
  xml2_dep,
  glibmm_dep
]

spobject_filters = static_library('spobject_filters',
  sources: [spobject_filters_sources,
            spobject_filters_headers],
  dependencies: spobject_filters_deps,
  include_directories: [srcinc, confinc])

libspobject_filters_dep = declare_dependency(link_with : spobject_filters)
