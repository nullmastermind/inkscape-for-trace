debug_sources = files([
  'demangle.cpp',
  'heap.cpp',
  'log-display-config.cpp',
  'logger.cpp',
  'sysv-heap.cpp',
  'timestamp.cpp',
  'gdk-event-latency-tracker.cpp'
])

debug_headers = files([
  'demangle.h',
  'event-tracker.h',
  'event.h',
  'gc-heap.h',
  'gdk-event-latency-tracker.h',
  'heap.h',
  'log-display-config.h',
  'logger.h',
  'simple-event.h',
  'sysv-heap.h',
  'timestamp.h'
])


debug_deps = [
  glibmm_dep,
  gtk_dep
]

libdebug = static_library('libdebug',
  sources: [debug_sources,
            debug_headers],
  dependencies: debug_deps,
  include_directories: [srcinc, confinc])

libdebug_dep = declare_dependency(link_with : libdebug)
