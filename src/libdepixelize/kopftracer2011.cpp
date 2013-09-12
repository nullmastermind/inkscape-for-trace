/*  This file is part of the libdepixelize project
    Copyright (C) 2013 Vinícius dos Santos Oliveira <vini.ipsmaker@gmail.com>

    GNU Lesser General Public License Usage
    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation; either version 2.1 of the License, or (at your
    option) any later version.
    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.

    GNU General Public License Usage
    Alternatively, this library may be used under the terms of the GNU General
    Public License as published by the Free Software Foundation, either version
    2 of the License, or (at your option) any later version.
    You should have received a copy of the GNU General Public License along with
    this library.  If not, see <http://www.gnu.org/licenses/>.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

// Build fix under Inkscape build tree
#if GLIBMM_DISABLE_DEPRECATED && HAVE_GLIBMM_THREADS_H
#include <glibmm/threads.h>
#endif

#include <utility>
#include <algorithm>
#include "kopftracer2011.h"
#include "priv/colorspace.h"
#include "priv/homogeneoussplines.h"
#include "priv/branchless.h"
#include "priv/splines.h"

namespace Tracer {
namespace Heuristics {

int curves(PixelGraph &graph, PixelGraph::Node *a, PixelGraph::Node *b);
bool islands(PixelGraph &graph, PixelGraph::Node *a, PixelGraph::Node *b);

struct SparsePixels
{
    typedef std::pair<PixelGraph::Node*, PixelGraph::Node*> Edge;
    typedef std::pair<Edge, int> EdgeWeight;

    void operator()(PixelGraph &graph, unsigned radius);

    static bool similar_colors(PixelGraph::Node *n, Edge edge);

    EdgeWeight diagonals[2];
};

} // namespace Heuristics

Splines Kopf2011::to_voronoi(const std::string &filename,
                             const Options &options)
{
    return to_voronoi(Gdk::Pixbuf::create_from_file(filename), options);
}

Splines Kopf2011::to_voronoi(const Glib::RefPtr<Gdk::Pixbuf const> &buf,
                             const Options &options)
{
    return Splines(_voronoi<Precision>(buf, options));
}

Splines Kopf2011::to_splines(const std::string &filename,
                             const Options &options)
{
    return to_splines(Gdk::Pixbuf::create_from_file(filename), options);
}

Splines Kopf2011::to_splines(const Glib::RefPtr<Gdk::Pixbuf const> &buf,
                             const Options &options)
{
    HomogeneousSplines<Precision> splines(_voronoi<Precision>(buf, options));
    return Splines(splines, options.optimize, options.nthreads);
}

template<class T>
SimplifiedVoronoi<T> Kopf2011::_voronoi(const Glib::RefPtr<Gdk::Pixbuf const> &buf,
                                        const Options &options)
{
    PixelGraph graph(buf);

    /*if ( !graph.width() || !graph.height() )
        return;*/

#ifndef NDEBUG
    graph.checkConsistency();
#endif

    // This step could be part of the initialization of PixelGraph
    // and decrease the necessary number of passes
    graph.connectAllNeighbors();

#ifndef NDEBUG
    graph.checkConsistency();
#endif

    // This step can't be part of PixelGraph initilization without adding some
    // cache misses due to random access patterns that might be injected
    _disconnect_neighbors_with_dissimilar_colors(graph);

#ifndef NDEBUG
    graph.checkConsistency();
#endif

    // This and below steps must be executed in separate.
    // Otherwise, there will be colateral effects due to misassumption about the
    // data being read.
    _remove_crossing_edges_safe(graph);

#ifndef NDEBUG
    graph.checkConsistency();
#endif

    _remove_crossing_edges_unsafe(graph, options);

#ifndef NDEBUG
    graph.checkConsistency();
#endif

    return SimplifiedVoronoi<T>(graph);
}

// TODO: move this function (plus connectAllNeighbors) to PixelGraph constructor
inline void
Kopf2011::_disconnect_neighbors_with_dissimilar_colors(PixelGraph &graph)
{
    using colorspace::similar_colors;
    for ( PixelGraph::iterator it = graph.begin(), end = graph.end() ; it != end
              ; ++it ) {
        if ( it->adj.top )
            it->adj.top = similar_colors(it->rgba, (it - graph.width())->rgba);
        if ( it->adj.topright ) {
            it->adj.topright
                = similar_colors(it->rgba, (it - graph.width() + 1)->rgba);
        }
        if ( it->adj.right )
            it->adj.right = similar_colors(it->rgba, (it + 1)->rgba);
        if ( it->adj.bottomright ) {
            it->adj.bottomright
                = similar_colors(it->rgba, (it + graph.width() + 1)->rgba);
        }
        if ( it->adj.bottom ) {
            it->adj.bottom
                = similar_colors(it->rgba, (it + graph.width())->rgba);
        }
        if ( it->adj.bottomleft ) {
            it->adj.bottomleft
                = similar_colors(it->rgba, (it + graph.width() - 1)->rgba);
        }
        if ( it->adj.left )
            it->adj.left = similar_colors(it->rgba, (it - 1)->rgba);
        if ( it->adj.topleft ) {
            it->adj.topleft = similar_colors(it->rgba,
                                             (it - graph.width() - 1)->rgba);
        }
    }
}

/**
 * This method removes crossing edges if the 2x2 block is fully connected.
 *
 * In this case the two diagonal connections can be safely removed without
 * affecting the final result.
 *
 * \TODO: It should remember/cache who are the unsafe crossing edges?
 */
inline void Kopf2011::_remove_crossing_edges_safe(PixelGraph &graph)
{
    if ( graph.width() < 2 || graph.height() < 2 )
        return;

    PixelGraph::iterator it = graph.begin();
    for ( int i = 0 ; i != graph.height() - 1 ; ++i, ++it ) {
        for ( int j = 0 ; j != graph.width() - 1 ; ++j, ++it ) {
            // this <-> right
            if ( !it->adj.right )
                continue;

            // this <-> down
            if ( !it->adj.bottom )
                continue;

            PixelGraph::iterator down_right = it + graph.width() + 1;

            // down_right <-> right
            if ( !down_right->adj.top )
                continue;

            // down_right <-> down
            if ( !down_right->adj.left )
                continue;

            // main diagonal
            // this <-> down_right
            it->adj.bottomright = 0;
            down_right->adj.topleft = 0;

            // secondary diagonal
            // right <-> down
            (it + 1)->adj.bottomleft = 0;
            (it + graph.width())->adj.topright = 0;
        }
    }
}

/**
 * This method removes crossing edges using the heuristics.
 */
inline
void Kopf2011::_remove_crossing_edges_unsafe(PixelGraph &graph,
                                             const Options &options)
{
    if ( graph.width() < 2 || graph.height() < 2 )
        return;

    // Iterate over the graph, 2x2 blocks at time
    PixelGraph::iterator it = graph.begin();
    for (int i = 0 ; i != graph.height() - 1 ; ++i, ++it ) {
        for ( int j = 0 ; j != graph.width() - 1 ; ++j, ++it ) {
            using std::pair;
            using std::make_pair;

            typedef pair<PixelGraph::Node*, PixelGraph::Node*> Edge;
            typedef pair<Edge, int> EdgeWeight;

            EdgeWeight diagonals[2] = {
                make_pair(make_pair(&*it, &*(it + graph.width() + 1)), 0),
                make_pair(make_pair(&*(it + 1), &*(it + graph.width())), 0)
            };

            // Check if there are crossing edges
            if ( !diagonals[0].first.first->adj.bottomright
                 || !diagonals[1].first.first->adj.bottomleft ) {
                continue;
            }

            // Compute weights
            for ( int i = 0 ; i != 2 ; ++i ) {
                // Curves and islands heuristics
                PixelGraph::Node *a = diagonals[i].first.first;
                PixelGraph::Node *b = diagonals[i].first.second;

                diagonals[i].second += Heuristics::curves(graph, a, b)
                    * options.curvesMultiplier;

                diagonals[i].second += Heuristics::islands(graph, a, b)
                    * options.islandsWeight;
            }

            {
                // Sparse pixels heuristic
                Heuristics::SparsePixels sparse_pixels;

                for ( int i = 0 ; i != 2 ; ++i )
                    sparse_pixels.diagonals[i] = diagonals[i];

                sparse_pixels(graph, options.sparsePixelsRadius);

                for ( int i = 0 ; i != 2 ; ++i ) {
                    diagonals[i].second += sparse_pixels.diagonals[i].second
                        * options.sparsePixelsMultiplier;
                }
            }

            // Remove edges with lower weight
            if ( diagonals[0].second > diagonals[1].second ) {
                diagonals[1].first.first->adj.bottomleft = 0;
                diagonals[1].first.second->adj.topright = 0;
            } else if ( diagonals[0].second < diagonals[1].second ) {
                diagonals[0].first.first->adj.bottomright = 0;
                diagonals[0].first.second->adj.topleft = 0;
            } else {
                diagonals[0].first.first->adj.bottomright = 0;
                diagonals[0].first.second->adj.topleft = 0;
                diagonals[1].first.first->adj.bottomleft = 0;
                diagonals[1].first.second->adj.topright = 0;
            }
        }
    }
}

inline int Heuristics::curves(PixelGraph &graph, PixelGraph::Node *a,
                              PixelGraph::Node *b)
{
    int count = 1;

    // b -> a
    // and then a -> b
    for ( int i = 0 ; i != 2 ; ++i ) {
        PixelGraph::Node *it = i ? a : b;
        PixelGraph::Node *prev = i ? b : a;
        int local_count = 0;

        // Used to avoid inifinite loops in circular-like edges
        PixelGraph::Node *const initial = it;

        while ( it->adjsize() == 2 ) {
            ++local_count;

            // Iterate to next
            {
                // There are only two values that won't be zero'ed
                // and one of them has the same value of prev
                guintptr aux = guintptr(it);
                aux = it->adj.top * guintptr(it - graph.width())
                    + it->adj.topright * guintptr(it - graph.width() + 1)
                    + it->adj.right * guintptr(it + 1)
                    + it->adj.bottomright * guintptr(it + graph.width() + 1)
                    + it->adj.bottom * guintptr(it + graph.width())
                    + it->adj.bottomleft * guintptr(it + graph.width() - 1)
                    + it->adj.left * guintptr(it - 1)
                    + it->adj.topleft * guintptr(it - graph.width() - 1)
                    - guintptr(prev);
                prev = it;
                it = reinterpret_cast<PixelGraph::Node*>(aux);
            }

            // Break infinite loops
            if ( it == initial )
                return local_count;
        }
        count += local_count;
    }

    return count;
}

inline void Heuristics::SparsePixels::operator ()(PixelGraph &graph,
                                                  unsigned radius)
{
    if ( !graph.width() || !graph.height() )
        return;

    // Clear weights
    for ( int i = 0 ; i != 2 ; ++i )
        diagonals[i].second = 0;

    // Compute begin and end nodes under a (2*radius)^2 window
    std::pair<unsigned, unsigned> begin(0, 0);

    // Get midpoint
    {
        using branchless::max_consecutive;

        unsigned x1 = graph.toX(diagonals[0].first.first);
        unsigned x2 = graph.toX(diagonals[0].first.second);
        begin.first = max_consecutive(x1, x2);

        unsigned y1 = graph.toY(diagonals[0].first.first);
        unsigned y2 = graph.toY(diagonals[0].first.second);
        begin.second = max_consecutive(y1, y2);
    }

    std::pair<unsigned, unsigned> end(begin.first + radius,
                                      begin.second + radius);

    // Get beginning point
    {
        using branchless::first_if;

        begin.first = first_if(begin.first - radius, 0u, begin.first >= radius);

        begin.second = first_if(begin.second - radius, 0u,
                                begin.second >= radius);
    }

    // Fix ending point if misplaced
    {
        using branchless::first_if;

        end.first = first_if(unsigned(graph.width()), end.first,
                             end.first > graph.width());

        end.second = first_if(unsigned(graph.height()), end.second,
                              end.second > graph.height());
    }

    // Iterate over nodes and count them
    for ( int i = begin.second ; i != end.second ; ++i ) {
        PixelGraph::Node *it = &graph[begin.first][i];
        for ( int j = begin.first ; j != end.first ; ++j, ++it ) {
            for ( int k = 0 ; k != 2 ; ++k )
                diagonals[k].second += similar_colors(it, diagonals[k].first);
        }
    }

    int minor = branchless::min(diagonals[0].second, diagonals[1].second);
    for ( int i = 0 ; i != 2 ; ++i )
        diagonals[i].second -= minor;
    std::swap(diagonals[0].second, diagonals[1].second);
}

inline bool
Heuristics::SparsePixels::similar_colors(PixelGraph::Node *n,
                                         Heuristics::SparsePixels::Edge edge)
{
    using colorspace::similar_colors;
    return (similar_colors(n->rgba, edge.first->rgba)
            || similar_colors(n->rgba, edge.second->rgba));
}

inline bool Heuristics::islands(PixelGraph &graph, PixelGraph::Node *a,
                               PixelGraph::Node *b)
{
    if ( a->adjsize() == 1 || b->adjsize() == 1 )
        return true;

    return false;
}

} // namespace Tracer

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
