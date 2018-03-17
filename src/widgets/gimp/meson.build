libgimpwidgets_sources = files([
  'gimpcolorwheel.c',
  'ruler.cpp'
])

libgimpwidgets_headers = files([
  'gimpcolorwheel.h',
  'ruler.h'
])

libgimpwidgets_deps = [
  glibmm_dep,
  gtk_dep
]

libgimpwidgets = static_library('gimpwidgets',
  sources: [libgimpwidgets_sources,
            libgimpwidgets_headers],
  dependencies: libgimpwidgets_deps,
  include_directories: [srcinc, confinc])

libgimpwidgets_dep = declare_dependency(link_with : libgimpwidgets)
