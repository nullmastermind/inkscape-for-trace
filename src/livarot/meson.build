livarot_sources = files([
  'AlphaLigne.cpp',
  'AVL.cpp',
  'BitLigne.cpp',
  'float-line.cpp',
  'int-line.cpp',
  'PathConversion.cpp',
  'Path.cpp',
  'PathCutting.cpp',
  'path-description.cpp',
  'PathOutline.cpp',
  'PathSimplify.cpp',
  'PathStroke.cpp',
  'Shape.cpp',
  'ShapeDraw.cpp',
  'ShapeMisc.cpp',
  'ShapeRaster.cpp',
  'ShapeSweep.cpp',
  'sweep-event.cpp',
  'sweep-tree.cpp',
  'sweep-tree-list.cpp'
])

livarot_headers = files([
  'AVL.h',
  'AlphaLigne.h',
  'BitLigne.h',
  'Livarot.h',
  'LivarotDefs.h',
  'Path.h',
  'Shape.h',
  'float-line.h',
  'int-line.h',
  'path-description.h',
  'sweep-event-queue.h',
  'sweep-event.h',
  'sweep-tree-list.h',
  'sweep-tree.h'
])

livarot_deps = [
  glib_dep,
  glibmm_dep,
  gdk_dep,
  xml2_dep,
  sigcpp_dep
]

lib_livarot = static_library('livarot',
  sources: [livarot_sources,
            livarot_headers],
  dependencies: livarot_deps,
  include_directories: [srcinc, confinc])

liblivarot_dep = declare_dependency(link_with : lib_livarot)
