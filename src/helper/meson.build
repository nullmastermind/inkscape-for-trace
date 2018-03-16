helper_sources = files([
  'action.cpp',
  'action-context.cpp',
  'geom.cpp',
  'geom-nodetype.cpp',
  'geom-pathstroke.cpp',
  'geom-pathvectorsatellites.cpp',
  'geom-satellite.cpp',
  'gettext.cpp',
  'pixbuf-ops.cpp',
  'png-write.cpp',
  'stock-items.cpp',
#units-test.cpp',
  'window.cpp'
])

helper_headers = files([
  'action.h',
  'action-context.h',
  'geom-curves.h',
  'geom-nodetype.h',
  'geom-pathstroke.h',
  'geom-pathvectorsatellites.h',
  'geom-satellite.h',
  'geom.h',
  'gettext.h',
  'mathfns.h',
  'pixbuf-ops.h',
  'png-write.h',
  'stock-items.h',
  'window.h'
])

marshallers = gnome.genmarshal('sp-marshal',
sources : 'sp-marshal.list',
extra_args : ['-UG_ENABLE_DEBUG', '--prototypes'])

marshallers_c = marshallers[0]
marshallers_h = marshallers[1]

helper_deps = [
  glibmm_dep,
  cairomm_dep,
  gtkmm_dep,
  xml2_dep,
  png_dep,
  gobject_dep
]

lib_helper = static_library('helper',
  sources: [helper_sources,
            helper_headers,
            marshallers_c,
            marshallers_h],
  dependencies: helper_deps,
  include_directories: [confinc, srcinc])

libhelper_dep = declare_dependency(link_with : lib_helper)
