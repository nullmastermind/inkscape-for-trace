# SPDX-License-Identifier: GPL-2.0-or-later

set(util_SRC
	ege-appear-time-tracker.cpp
	ege-tags.cpp
	expression-evaluator.cpp
	share.cpp
	units.cpp
	ziptool.cpp


	# -------
	# Headers
	const_char_ptr.h
	copy.h
	ege-appear-time-tracker.h
	ege-tags.h
	enums.h
	expression-evaluator.h
	find-if-before.h
	find-last-if.h
	fixed_point.h
	format.h
	forward-pointer-iterator.h
	list-container-test.h
	list-container.h
	list-copy.h
	list.h
	longest-common-suffix.h
	reference.h
	reverse-list.h
	share.h
	signal-blocker.h
	ucompose.hpp
	units.h
	ziptool.h
)

add_inkscape_lib(util_LIB "${util_SRC}")
# add_inkscape_source("${util_SRC}")
