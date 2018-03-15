project('inkscape', 'cpp',
        version: '0.92+devel',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=c99',
        ],
        meson_version : '>= 0.43.0',
        license: 'GPLv3+')

pango_req          = '>= 1.41.0'
cairo_req          = '>= 1.14.0'
glib_req           = '>= 2.36.0'
gtkmm_req          = '>= 3.16.0'
xml2_req           = '>= 2.7.4'

# Dependencies
cairo_dep      = dependency('cairo', version: cairo_req, required : true)
xml2_dep       = dependency('libxml-2.0', version: xml2_req, required : true)
glib_dep       = dependency('glib-2.0', version: glib_req, required : true)
gtkmm_dep      = dependency('gtkmm-3.0', version: gtkmm_req, required : true)
pango_dep      = dependency('pango', version: pango_req,
                            fallback : ['pango', 'libpango_dep'])
pangocairo_dep = dependency('pangocairo', version: cairo_req,
                            fallback : ['pango', 'libpangocairo_dep'])
harfbuzz_dep   = dependency('harfbuzz', version: '>= 0.9', required: false)

common_cflags = ''
common_ldflags = ''

confinc = include_directories('.')
srcinc = include_directories('src')

subdir('src')

#if get_option('build-tests')
#  subdir('tests')
#endif

cdata = configuration_data()
cdata.set('HAVE_HARFBUZZ', harfbuzz_dep.found())
#cdata.set('HAVE_PANGOFT', pangoft_dep.found())

# config.h
configure_file(input: 'config.h.meson',
               output: 'config.h',
               configuration: cdata)

