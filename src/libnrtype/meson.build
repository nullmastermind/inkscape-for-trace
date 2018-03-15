layout_tng_sources = files([
  'Layout-TNG-Compute.cpp',
  'Layout-TNG-Output.cpp',
  'FontFactory.cpp',
  'Layout-TNG.cpp',
  'font-lister.cpp',
  'Layout-TNG-Scanline-Makers.cpp',
  'Layout-TNG-Input.cpp',
  'FontInstance.cpp',
  'Layout-TNG-OutIter.cpp'
])

layout_tng_headers = files([
  'font-instance.h',
  'Layout-TNG-Scanline-Maker.h',
  'FontFactory.h',
  'Layout-TNG.h',
  'font-glyph.h',
  'font-lister.h',
  'font-style.h'
])

#install_headers(layout_tng_headers)

layout_tng_deps = [
  glib_dep,
  gtkmm_dep,
  xml2_dep
]

layout_tng_inc = include_directories('.')

libnrtype = static_library('nrtype',
  sources: [layout_tng_sources,
            layout_tng_headers],
  dependencies: layout_tng_deps,
  include_directories: [srcinc, confinc, layout_tng_inc],
  c_args: common_cflags,
  link_args: common_ldflags)

