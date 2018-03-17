project('inkscape', 'cpp', 'c',
        version: '0.92+devel',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=c99',
        ],
        meson_version : '>= 0.43.0',
        license: 'GPLv3+')

gnome = import('gnome')

sigcpp_req   = '>= 2.6.2' #TODO: review required version: 2.8.0 ?
gdl_req      = '>= 3.6.0'
pango_req    = '>= 1.41.0'
pangoft2_req = '>= 1.37.2'
cairo_req    = '>= 1.14.0'
cairomm_req  = '>= 1.12.0'
glib_req     = '>= 2.36.0'
glibmm_req   = '>= 2.46.3' #TODO review: 2.54.0 ?
gtk_req      = '>= 3.18.9' #TODO review: 3.21.5 ?
gtkmm_req    = '>= 3.16.0'
gdk_req      = '>= 3.8'
gdkmm_req    = '>= 3.8'
xml2_req     = '>= 2.7.4'
harfbuzz_req = '>= 0.9'
bohemgc_req  = '>= 7.4.2'
png_req      = '>= 1.2.54' #TODO: 1.6.2 ?
soup_req     = '>= 2.41.90'
zlib_req     = '>= 1.1.4'
xslt_req     = '>= 1.1.25'

# Dependencies
sigcpp_dep     = dependency('sigc++-2.0',  version: sigcpp_req,   required : true)
gdl_dep        = dependency('gdl-3.0',     version: gdl_req,      required : true)
cairo_dep      = dependency('cairo',       version: cairo_req,    required : true)
cairopdf_dep   = dependency('cairo-pdf',                           required : true)
cairomm_dep    = dependency('cairomm-1.0', version: cairomm_req,  required : true)
xml2_dep       = dependency('libxml-2.0',  version: xml2_req,     required : true)
glib_dep       = dependency('glib-2.0',    version: glib_req,     required : true)
gobject_dep    = dependency('gobject-2.0',                        required : true)
glibmm_dep     = dependency('glibmm-2.4',  version: glibmm_req,   required : true)
gtk_dep        = dependency('gtk+-3.0',    version: gtk_req,      required : true)
gtkmm_dep      = dependency('gtkmm-3.0',   version: gtkmm_req,    required : true)
gdk_dep        = dependency('gdk-3.0',     version: gdk_req,      required : true)
gdkmm_dep      = dependency('gdkmm-3.0',   version: gdkmm_req,    required : true)
gmodule_dep    = dependency('gmodule-2.0',                        required : true)
pango_dep      = dependency('pango',       version: pango_req,    required : true)
pangoft2_dep   = dependency('pangoft2',    version: pangoft2_req, required : true)
pangocairo_dep = dependency('pangocairo',  version: cairo_req,    required : true)
harfbuzz_dep   = dependency('harfbuzz',    version: harfbuzz_req, required : false)
boehmgc_dep    = dependency('bdw-gc',      version: bohemgc_req,  required : true)
gsl_dep        = dependency('gsl',                                required : true)
png_dep        = dependency('libpng',      version: png_req,      required : true)
soup_dep       = dependency('libsoup-2.4', version: soup_req,     required : true)
zlib_dep       = dependency('zlib',        version: zlib_req,     required : true)
x11_dep        = dependency('x11',                                required : true)
xslt_dep       = dependency('libxslt',     version: xslt_req,     required : true)

add_project_arguments('-DHAVE_CONFIG_H', language: 'cpp')

confinc = include_directories('.')
srcinc = include_directories('src')
helperinc = include_directories('src/helper')

subdir('src')

#if get_option('build-tests')
#  subdir('tests')
#endif

cdata = configuration_data()
cdata.set('HAVE_HARFBUZZ', harfbuzz_dep.found())
cdata.set('HAVE_CAIRO_PDF', cairopdf_dep.found())
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

common_deps = [
  libsrcdir_dep,
  libui_dep,
  libnrtype_dep,
  libinkgc_dep,
  lib2geom_dep,
  libspobject_dep,
  libhelper_dep,
  libxml_dep,
]

inkscape_deps = [
  glibmm_dep,
  gtkmm_dep,
  xml2_dep,
  x11_dep
]

executable('inkscape',
  sources: 'src/inkscape.cpp',
  dependencies : [common_deps,
                  inkscape_deps],
  include_directories: [srcinc, confinc]
)

