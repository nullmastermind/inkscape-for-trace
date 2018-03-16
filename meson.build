project('inkscape', 'cpp',
        version: '0.92+devel',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=c99',
        ],
        meson_version : '>= 0.43.0',
        license: 'GPLv3+')

sigcpp_req   = '>= 2.6.2' #TODO: review required version: 2.8.0 ?
gdl_req      = '>= 3.6.0'
pango_req    = '>= 1.41.0'
pangoft2_req = '>= 1.37.2'
cairo_req    = '>= 1.14.0'
glib_req     = '>= 2.36.0'
glibmm_req   = '>= 2.46.3' #TODO review: 2.54.0 ?
gtkmm_req    = '>= 3.16.0'
xml2_req     = '>= 2.7.4'
harfbuzz_req = '>= 0.9'
bohemgc_req  = '>= 7.4.2'

# Dependencies
sigcpp_dep     = dependency('sigc++-2.0', version: sigcpp_req, required : true)
gdl_dep        = dependency('gdl-3.0', version: gdl_req, required : true)
cairo_dep      = dependency('cairo', version: cairo_req, required : true)
xml2_dep       = dependency('libxml-2.0', version: xml2_req, required : true)
glib_dep       = dependency('glib-2.0', version: glib_req, required : true)
glibmm_dep     = dependency('glibmm-2.4', version: glibmm_req, required : true)
gtkmm_dep      = dependency('gtkmm-3.0', version: gtkmm_req, required : true)
pango_dep      = dependency('pango', version: pango_req, required : true)
pangoft2_dep   = dependency('pangoft2', version: pangoft2_req, required : true)
pangocairo_dep = dependency('pangocairo', version: cairo_req, required : true)
harfbuzz_dep   = dependency('harfbuzz', version: harfbuzz_req, required: false)
boehmgc_dep    = dependency('bdw-gc', version: bohemgc_req, required: true)
gsl_dep        = dependency('gsl', required: true)

common_cflags = ''
common_ldflags = ''
add_project_arguments('-DHAVE_CONFIG_H', language: 'cpp')

confinc = include_directories('.')
srcinc = include_directories('src')

subdir('src')

#if get_option('build-tests')
#  subdir('tests')
#endif

cdata = configuration_data()
cdata.set('HAVE_HARFBUZZ', harfbuzz_dep.found())
#cdata.set('HAVE_PANGOFT', pangoft_dep.found())

if gdl_dep.found()
  cdata.set('WITH_GDL_3_6', 1)
endif

gtkmm_version = gtkmm_dep.version()
cdata.set('INKSCAPE_GTKMM_MAJOR_VERSION', gtkmm_version.split('.')[0].to_int())
cdata.set('INKSCAPE_GTKMM_MICRO_VERSION', gtkmm_version.split('.')[1].to_int())
cdata.set('INKSCAPE_GTKMM_MINOR_VERSION', gtkmm_version.split('.')[2].to_int())

cdata.set('INKSCAPE_DATADIR', '/usr/local/share') #TODO: should this be INSTALL_DIR + '/share' ?

# config.h
configure_file(input: 'config.h.meson',
               output: 'config.h',
               configuration: cdata)

inkscape_sources = [
  ui_sources, ui_headers
]

inkscape_deps = [
  gtkmm_dep,
  xml2_dep,
  gdl_dep,
  libnrtype_dep,
  libinkgc_dep,
  lib2geom_dep
]

executable('inkscape',
  sources: inkscape_sources,
  dependencies : inkscape_deps,
  include_directories: [srcinc, confinc]
)

