inkgc_sources = files([
  'gc.cpp'
])

inkgc_headers = files([
  'gc-alloc.h',
  '../gc-anchored.h',
  'gc-core.h',
  'gc-managed.h',
  'gc-soft-ptr.h'
])

inkgc_deps = [
  glib_dep,
  glibmm_dep,
  sigcpp_dep,
  boehmgc_dep
]

libinkgc = static_library('inkgc',
  sources: [inkgc_sources,
            inkgc_headers],
  dependencies: inkgc_deps,
  include_directories: [srcinc, confinc])

libinkgc_dep = declare_dependency(link_with : libinkgc)
