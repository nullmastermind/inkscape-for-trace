libcola_sources = files([
  'box.cpp',
  'cluster.cpp',
  'cola.cpp',
  'convex_hull.cpp',
  'compound_constraints.cpp',
  'conjugate_gradient.cpp',
  'connected_components.cpp',
  'gradient_projection.cpp',
  'shapepair.cpp',
  'straightener.cpp'
])

libcola_headers = files([
  'box.h',
  'cluster.h',
  'cola.h',
  'compound_constraints.h',
  'conjugate_gradient.h',
  'convex_hull.h',
  'gradient_projection.h',
  'shapepair.cpp',
  'shortest_paths.h',
  'straightener.h'
])

libcola_deps = [
  libvpsc_dep
]

libcola = static_library('libcola',
  sources: [libcola_sources,
            libcola_headers],
  dependencies: libcola_deps,
  include_directories: [srcinc, confinc])

libcola_dep = declare_dependency(link_with : libcola)
