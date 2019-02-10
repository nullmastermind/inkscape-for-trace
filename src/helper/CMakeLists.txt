# SPDX-License-Identifier: GPL-2.0-or-later

include(${CMAKE_SOURCE_DIR}/CMakeScripts/UseGlibMarshal.cmake)

GLIB_MARSHAL(sp_marshal sp-marshal "${CMAKE_CURRENT_BINARY_DIR}/helper")

set(sp_marshal_SRC
	${CMAKE_CURRENT_BINARY_DIR}/sp-marshal.cpp
	${CMAKE_CURRENT_BINARY_DIR}/sp-marshal.h
)

set(helper_SRC
	action.cpp
	action-context.cpp
	geom.cpp
	geom-nodetype.cpp
	geom-pathstroke.cpp
	geom-pathvectorsatellites.cpp
	geom-satellite.cpp
	gettext.cpp
	pixbuf-ops.cpp
	png-write.cpp
	stock-items.cpp
        verb-action.cpp
	#units-test.cpp

	# we generate this file and it's .h counter-part
	${sp_marshal_SRC}


	# -------
	# Headers
	action.h
	action-context.h
	geom-curves.h
	geom-nodetype.h
	geom-pathstroke.h
	geom-pathvectorsatellites.h
	geom-satellite.h
	geom.h
	gettext.h
	mathfns.h
	pixbuf-ops.h
	png-write.h
	stock-items.h
        verb-action.h
)

set_source_files_properties(sp_marshal_SRC PROPERTIES GENERATED true)

# add_inkscape_lib(helper_LIB "${helper_SRC}")
add_inkscape_source("${helper_SRC}")
