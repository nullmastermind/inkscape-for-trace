libvpsc_sources = files([
  'block.cpp',
  'blocks.cpp',
  'cbuffer.cpp',
  'constraint.cpp',
  'rectangle.cpp',
  'solve_VPSC.cpp',
  'variable.cpp'
])

libvpsc_headers = files([
  'assertions.h',
  'block.h',
  'blocks.h',
  'cbuffer.h',
  'constraint.h',
  'exceptions.h',
  'isnan.h',
  'linesegment.h',
  'pairing_heap.h',
  'rectangle.h',
  'solve_VPSC.h',
  'variable.h'
])

libvpsc_deps = []

libvpsc = static_library('libvpsc',
  sources: [libvpsc_sources,
            libvpsc_headers],
  dependencies: libvpsc_deps,
  include_directories: [srcinc, confinc])

libvpsc_dep = declare_dependency(link_with : libvpsc)
