
set(libcola_SRC
	box.cpp
    cluster.cpp
	cola.cpp
	convex_hull.cpp
	compound_constraints.cpp
	conjugate_gradient.cpp
	connected_components.cpp
	gradient_projection.cpp
	shapepair.cpp
	straightener.cpp


	# -------
	# Headers
	box.h
    cluster.h
	cola.h
	compound_constraints.h
	conjugate_gradient.h
	convex_hull.h
	gradient_projection.h
	shapepair.cpp
	shortest_paths.h
	straightener.h
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")
add_inkscape_lib(cola_LIB "${libcola_SRC}")
