libcroco_sources = files([
  'cr-additional-sel.c',
  'cr-attr-sel.c',
  'cr-cascade.c',
  'cr-declaration.c',
  'cr-doc-handler.c',
  'cr-enc-handler.c',
  'cr-fonts.c',
  'cr-input.c',
  'cr-libxml-node-iface.c',
  'cr-num.c',
  'cr-om-parser.c',
  'cr-parser.c',
  'cr-parsing-location.c',
  'cr-prop-list.c',
  'cr-pseudo.c',
  'cr-rgb.c',
  'cr-selector.c',
  'cr-sel-eng.c',
  'cr-simple-sel.c',
  'cr-statement.c',
  'cr-string.c',
  'cr-style.c',
  'cr-stylesheet.c',
  'cr-term.c',
  'cr-tknzr.c',
  'cr-token.c',
  'cr-utils.c'
])

libcroco_headers = files([
  'cr-additional-sel.h',
  'cr-attr-sel.h',
  'cr-cascade.h',
  'cr-declaration.h',
  'cr-doc-handler.h',
  'cr-enc-handler.h',
  'cr-fonts.h',
  'cr-input.h',
  'cr-libxml-node-iface.h',
  'cr-node-iface.h',
  'cr-num.h',
  'cr-om-parser.h',
  'cr-parser.h',
  'cr-parsing-location.h',
  'cr-prop-list.h',
  'cr-pseudo.h',
  'cr-rgb.h',
  'cr-sel-eng.h',
  'cr-selector.h',
  'cr-simple-sel.h',
  'cr-statement.h',
  'cr-string.h',
  'cr-style.h',
  'cr-stylesheet.h',
  'cr-term.h',
  'cr-tknzr.h',
  'cr-token.h',
  'cr-utils.h',
  'libcroco.h'
])

libcroco_deps = [
  glib_dep,
  xml2_dep
]

libcroco = static_library('libcroco',
  sources: [libcroco_sources,
            libcroco_headers],
  dependencies: libcroco_deps,
  include_directories: [srcinc, confinc])

libcroco_dep = declare_dependency(link_with : libcroco)
