/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef NAV_DIJKSTRA_HPP
#define NAV_DIJKSTRA_HPP

#include "Util/NonCopyable.hpp"
#include "Dijkstra.hpp"
#include "ScanTaskPoint.hpp"
#include "Compiler.h"

#include <algorithm>
#include <assert.h>

#ifdef INSTRUMENT_TASK
extern long count_dijkstra_queries;
#endif

/**
 * Abstract class for A* /Dijkstra searches of nav points, managing
 * edges in multiple stages (corresponding to turn points).
 *
 * Expected running time, see http://www.avglab.com/andrew/pub/neci-tr-96-062.ps
 *
 * NavDijkstra<SearchPoint>
 */
template <class T>
class NavDijkstra: 
  private NonCopyable 
{
protected:
  enum {
    MAX_STAGES = 16,
  };

  Dijkstra<ScanTaskPoint> dijkstra;

  /** Number of stages in search */
  unsigned num_stages;
  T solution[MAX_STAGES];
  bool solution_valid;

protected:
  /** 
   * Constructor
   * 
   * @param _num_stages Number of stages in search
   * 
   * @return Initialised object
   */
  NavDijkstra(const bool is_min, const unsigned _num_stages, 
              const unsigned reserve_default= DIJKSTRA_QUEUE_SIZE):
    dijkstra(is_min, reserve_default),
    solution_valid(false)
  {
    set_stages(_num_stages);
  }

public:
  /**
   * Test whether two points (as previous search locations) are significantly
   * different to warrant a new search
   *
   * @param a1 First point to compare
   * @param a2 Second point to compare
   * @param dist_threshold Threshold distance for significance
   *
   * @return True if distance is significant
   */
  gcc_pure
  static bool distance_is_significant(const T& a1, const T& a2,
                                      const unsigned dist_threshold = 1) {
    return a1.flat_distance(a2)> dist_threshold;
  }

protected:
  /** 
   * Set the number of stages to search for, and clear the solution
   * array
   */
  void set_stages(const unsigned _num_stages) {
    assert(_num_stages <= MAX_STAGES);
    num_stages =_num_stages;
    if (num_stages) {
      std::fill(solution, solution + num_stages, T());
    }
  }

  /** 
   * Determine whether a finished path is valid
   * 
   * @param sp Point to check
   * 
   * @return True if this terminal point completes a valid solution
   */
  virtual bool finish_satisfied(const ScanTaskPoint &sp) const {
    return true;
  }

  /** 
   * Retrieve point
   * 
   * @param sp Index to point to retrieve
   * 
   * @return Point at index position
   */
  virtual const T &get_point(const ScanTaskPoint &sp) const = 0;

  /** 
   * Add edges from an origin node
   * 
   * @param curNode Origin node to add edges from
   */
  virtual void add_edges(const ScanTaskPoint &curNode) = 0;

  /** 
   * Determine whether a point is terminal (no further edges)
   * 
   * @param sp Point to test
   * 
   * @return True if point is terminal
   */
  gcc_pure
  bool is_final(const ScanTaskPoint &sp) const {
    assert(num_stages <= MAX_STAGES);
    return (unsigned)(sp.stage_number + 1) == num_stages;
  }

  /** 
   * Determine whether a point is a starting point (no previous edges)
   * 
   * @param sp Point to test
   * 
   * @return True if point is in first layer
   */
  gcc_pure
  bool is_first(const ScanTaskPoint &sp) const {
    return sp.stage_number == 0;
  }

  /** 
   * Iterate search algorithm
   * 
   * @param dijkstra Dijkstra structure to iterate
   * @param max_steps Maximum number of steps to update
   * 
   * @return True if algorithm returns a terminal path or no path found
   */
  bool distance_general(unsigned max_steps = 0 - 1) {
#ifdef INSTRUMENT_TASK
    count_dijkstra_queries++;
#endif

    while (!dijkstra.empty()) {
      const ScanTaskPoint destination = dijkstra.pop();

      if (is_final(destination)) {
        find_solution(destination);
        if (finish_satisfied(destination)) {
          solution_valid = true;
          dijkstra.clear();
          return true;
        }
      } else {
        add_edges(destination);
        if (dijkstra.empty())
          return true; // error, no way to reach final
      }

      if (max_steps)
        --max_steps;
      else
        return false; // Reached limit
    }

    return false; // No path found
  }

  /** 
   * Distance function for free point
   * 
   * @param curNode Destination node
   * @param currentLocation Origin location
   * 
   * @return Distance (flat) from origin to destination
   */
  gcc_pure
  unsigned distance(const ScanTaskPoint &curNode,
                    const T &currentLocation) const {
    return get_point(curNode).flat_distance(currentLocation);
  }

  /** 
   * Distance function for edges
   * 
   * @param s1 Origin node
   * @param s2 Destination node
   * 
   * @return Distance (flat) from origin to destination
   */
  gcc_pure
  unsigned distance(const ScanTaskPoint &s1, const ScanTaskPoint &s2) const {
    return get_point(s1).flat_distance(get_point(s2));
  }

  /** 
   * Determine optimal solution by backtracing the Dijkstra tree
   * 
   * @param destination Terminal point to query
   */
  void find_solution(const ScanTaskPoint &destination) {
    ScanTaskPoint p(destination); 
    ScanTaskPoint p_last(p);

    do {
      solution[p.stage_number] = get_point(p);
      p_last = p;
      p = dijkstra.get_predecessor(p);
    } while (p.stage_number != p_last.stage_number);
  }
};

#endif
