# SPDX-License-Identifier: GPL-2.0-or-later

set(nrtype_SRC
	FontFactory.cpp
	FontInstance.cpp
	font-lister.cpp
	Layout-TNG.cpp
	Layout-TNG-Compute.cpp
	Layout-TNG-Input.cpp
	Layout-TNG-OutIter.cpp
	Layout-TNG-Output.cpp
	Layout-TNG-Scanline-Makers.cpp
        OpenTypeUtil.cpp

	# -------
	# Headers
	font-glyph.h
	font-instance.h
	font-lister.h
	font-style.h
	FontFactory.h
	Layout-TNG-Scanline-Maker.h
	Layout-TNG.h
        OpenTypeUtil.cpp
)

add_inkscape_lib(nrtype_LIB "${nrtype_SRC}")

# we have circular references between nrtype_LIB and inkscape_base!
# this workaround prevents undefined references in nrtype_LIB when building static libraries (likely link order problem)
if(NOT BUILD_SHARED_LIBS)
	if( WIN32 )
		target_link_libraries(nrtype_LIB PRIVATE inkscape_base -lGdi32)
	else()
		target_link_libraries(nrtype_LIB PRIVATE inkscape_base)
	endif()
else()
	if( WIN32 )
		target_link_libraries(nrtype_LIB PUBLIC 2Geom::2geom -lGdi32)
	else()
		target_link_libraries(nrtype_LIB PUBLIC 2Geom::2geom)
	endif()
endif()
