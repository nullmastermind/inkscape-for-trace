# SPDX-License-Identifier: GPL-2.0-or-later

set(extension_SRC
	db.cpp
	dependency.cpp
	effect.cpp
	error-file.cpp
	execution-env.cpp
	extension.cpp
	init.cpp
	input.cpp
	output.cpp
	patheffect.cpp
	print.cpp
	system.cpp
	timer.cpp
    loader.cpp

	implementation/implementation.cpp
	implementation/xslt.cpp
	implementation/script.cpp

	internal/bluredge.cpp
	internal/cairo-ps-out.cpp
	internal/cairo-render-context.cpp
	internal/cairo-renderer.cpp
	internal/cairo-renderer-pdf-out.cpp
	internal/emf-inout.cpp
	internal/emf-print.cpp
	internal/gdkpixbuf-input.cpp
	internal/gimpgrad.cpp
	internal/grid.cpp
	internal/image-resolution.cpp
	internal/latex-pstricks.cpp
	internal/latex-pstricks-out.cpp
	internal/metafile-inout.cpp
	internal/metafile-print.cpp
	internal/odf.cpp
	internal/latex-text-renderer.cpp
	internal/pov-out.cpp
	internal/svg.cpp
	internal/svgz.cpp
	internal/text_reassemble.c
	internal/wmf-inout.cpp
	internal/wmf-print.cpp

	internal/filter/filter-all.cpp
	internal/filter/filter-file.cpp
	internal/filter/filter.cpp

	internal/pdfinput/pdf-input.cpp
	internal/pdfinput/pdf-parser.cpp
	internal/pdfinput/svg-builder.cpp

	prefdialog/prefdialog.cpp
	prefdialog/parameter.cpp
	prefdialog/parameter-bool.cpp
	prefdialog/parameter-color.cpp
	prefdialog/parameter-float.cpp
	prefdialog/parameter-int.cpp
	prefdialog/parameter-notebook.cpp
	prefdialog/parameter-optiongroup.cpp
	prefdialog/parameter-path.cpp
	prefdialog/parameter-string.cpp
	prefdialog/widget.cpp
	prefdialog/widget-box.cpp
	prefdialog/widget-image.cpp
	prefdialog/widget-label.cpp
	prefdialog/widget-separator.cpp
	prefdialog/widget-spacer.cpp

	# ------
	# Header
	db.h
	dependency.h
	effect.h
	error-file.h
	execution-env.h
	extension.h
	init.h
	input.h
	output.h
	patheffect.h
	print.h
	system.h
	timer.h
    loader.h

	implementation/implementation.h
	implementation/script.h
	implementation/xslt.h

	internal/bluredge.h
	internal/cairo-ps-out.h
	internal/cairo-render-context.h
	internal/cairo-renderer-pdf-out.h
	internal/cairo-renderer.h
	internal/clear-n_.h
	internal/emf-inout.h
	internal/emf-print.h
	internal/filter/bevels.h
	internal/filter/blurs.h
	internal/filter/bumps.h
	internal/filter/color.h
	internal/filter/distort.h
	internal/filter/filter.h
	internal/filter/image.h
	internal/filter/morphology.h
	internal/filter/overlays.h
	internal/filter/paint.h
	internal/filter/protrusions.h
	internal/filter/shadows.h
	internal/filter/textures.h
	internal/filter/transparency.h
	internal/gdkpixbuf-input.h
	internal/gimpgrad.h
	internal/grid.h
	internal/image-resolution.h
	internal/latex-pstricks-out.h
	internal/latex-pstricks.h
	internal/latex-text-renderer.h
	internal/metafile-inout.h
	internal/metafile-print.h
	internal/odf.h
	internal/pdfinput/pdf-input.h
	internal/pdfinput/pdf-parser.h
	internal/pdfinput/svg-builder.h
	internal/pov-out.h
	internal/svg.h
	internal/svgz.h
	internal/text_reassemble.h
	internal/wmf-inout.h
	internal/wmf-print.h

	prefdialog/prefdialog.h
	prefdialog/parameter.h
	prefdialog/parameter-bool.h
	prefdialog/parameter-color.h
	prefdialog/parameter-float.h
	prefdialog/parameter-int.h
	prefdialog/parameter-notebook.h
	prefdialog/parameter-optiongroup.h
	prefdialog/parameter-path.h
	prefdialog/parameter-string.h
	prefdialog/widget.h
	prefdialog/widget-box.h
	prefdialog/widget-image.h
	prefdialog/widget-label.h
	prefdialog/widget-separator.h
	prefdialog/widget-spacer.h
)

if(WIN32)
	list(APPEND extension_SRC
	)
endif()

if(WITH_LIBCDR)
	list(APPEND extension_SRC
		internal/cdr-input.cpp
		internal/cdr-input.h
	)
endif()

if(WITH_LIBVISIO)
	list(APPEND extension_SRC
		internal/vsd-input.cpp
		internal/vsd-input.h
	)
endif()

if(WITH_LIBWPG)
	list(APPEND extension_SRC
		internal/wpg-input.cpp
		internal/wpg-input.h
	)
endif()

if(WITH_MAGICK)
	list(APPEND extension_SRC
		internal/bitmap/adaptiveThreshold.cpp
		internal/bitmap/adaptiveThreshold.h
		internal/bitmap/addNoise.cpp
		internal/bitmap/addNoise.h
		internal/bitmap/blur.cpp
		internal/bitmap/blur.h
		internal/bitmap/channel.cpp
		internal/bitmap/channel.h
		internal/bitmap/charcoal.cpp
		internal/bitmap/charcoal.h
		internal/bitmap/colorize.cpp
		internal/bitmap/colorize.h
		internal/bitmap/contrast.cpp
		internal/bitmap/contrast.h
		internal/bitmap/crop.cpp
		internal/bitmap/crop.h
		internal/bitmap/cycleColormap.cpp
		internal/bitmap/cycleColormap.h
		internal/bitmap/despeckle.cpp
		internal/bitmap/despeckle.h
		internal/bitmap/edge.cpp
		internal/bitmap/edge.h
		internal/bitmap/emboss.cpp
		internal/bitmap/emboss.h
		internal/bitmap/enhance.cpp
		internal/bitmap/enhance.h
		internal/bitmap/equalize.cpp
		internal/bitmap/equalize.h
		internal/bitmap/gaussianBlur.cpp
		internal/bitmap/gaussianBlur.h
		internal/bitmap/imagemagick.cpp
		internal/bitmap/imagemagick.h
		internal/bitmap/implode.cpp
		internal/bitmap/implode.h
		internal/bitmap/level.cpp
		internal/bitmap/level.h
		internal/bitmap/levelChannel.cpp
		internal/bitmap/levelChannel.h
		internal/bitmap/medianFilter.cpp
		internal/bitmap/medianFilter.h
		internal/bitmap/modulate.cpp
		internal/bitmap/modulate.h
		internal/bitmap/negate.cpp
		internal/bitmap/negate.h
		internal/bitmap/normalize.cpp
		internal/bitmap/normalize.h
		internal/bitmap/oilPaint.cpp
		internal/bitmap/oilPaint.h
		internal/bitmap/opacity.cpp
		internal/bitmap/opacity.h
		internal/bitmap/raise.cpp
		internal/bitmap/raise.h
		internal/bitmap/reduceNoise.cpp
		internal/bitmap/reduceNoise.h
		internal/bitmap/sample.cpp
		internal/bitmap/sample.h
		internal/bitmap/shade.cpp
		internal/bitmap/shade.h
		internal/bitmap/sharpen.cpp
		internal/bitmap/sharpen.h
		internal/bitmap/solarize.cpp
		internal/bitmap/solarize.h
		internal/bitmap/spread.cpp
		internal/bitmap/spread.h
		internal/bitmap/swirl.cpp
		internal/bitmap/swirl.h
		internal/bitmap/threshold.cpp
		internal/bitmap/threshold.h
		internal/bitmap/unsharpmask.cpp
		internal/bitmap/unsharpmask.h
		internal/bitmap/wave.cpp
		internal/bitmap/wave.h
	)
endif()

if(WITH_DBUS)
        add_subdirectory(dbus)
endif()

# add_inkscape_lib(extension_LIB "${extension_SRC}")
add_inkscape_source("${extension_SRC}")

add_subdirectory( plugins )
