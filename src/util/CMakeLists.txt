# SPDX-License-Identifier: GPL-2.0-or-later

set(util_SRC
	expression-evaluator.cpp
	share.cpp
	units.cpp
	ziptool.cpp


	# -------
	# Headers
	const_char_ptr.h
	enums.h
	expression-evaluator.h
	find-last-if.h
	fixed_point.h
	format.h
	forward-pointer-iterator.h
	longest-common-suffix.h
	reference.h
	share.h
	signal-blocker.h
	units.h
	ziptool.h
)

add_inkscape_lib(util_LIB "${util_SRC}")
target_link_libraries(util_LIB PUBLIC 2Geom::2geom)
# add_inkscape_source("${util_SRC}")
