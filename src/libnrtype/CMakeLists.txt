
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

	# -------
	# Headers
	font-glyph.h
	font-instance.h
	font-lister.h
	font-style.h
	FontFactory.h
	Layout-TNG-Scanline-Maker.h
	Layout-TNG.h
)

add_inkscape_lib(nrtype_LIB "${nrtype_SRC}")
