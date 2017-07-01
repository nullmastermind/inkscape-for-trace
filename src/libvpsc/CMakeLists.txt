
set(libvpsc_SRC
	block.cpp
	blocks.cpp
	cbuffer.cpp
	constraint.cpp
	rectangle.cpp
	solve_VPSC.cpp
	variable.cpp


	# -------
	# Headers
	assertions.h
	block.h
	blocks.h
	cbuffer.h
	constraint.h
	exceptions.h
	isnan.h
	linesegment.h
	pairing_heap.h
	rectangle.h
	solve_VPSC.h
	variable.h
)

add_inkscape_lib(vpsc_LIB "${libvpsc_SRC}")
