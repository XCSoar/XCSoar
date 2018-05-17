/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "PathSolvers/NavDijkstra.hpp"
#include "Geo/SearchPoint.hpp"

#include <assert.h>

class OrderedTask;
class SearchPointVector;

/**
 * Class used to scan an OrderedTask for maximum/minimum distance
 * points.
 *
 * Uses flat-projected integer representation of search points for
 * speed, but this also makes the system approximate.
 *
 * Search points are located on OZ boundaries and each form a convex
 * hull, as this produces the minimum search vector size without loss
 * of accuracy.
 *
 * Searches are sensitive to active task point, in that task points
 * before the active task point need only be searched for maximum achieved
 * distance rather than border search points. 
 *
 * Before each calculation, set up this object with SetTaskSize() and
 * call SetBoundary() for each task point.
 *
 * This uses a Dijkstra search and so is O(N log(N)).
 */
class TaskDijkstra : protected NavDijkstra
{
  const SearchPointVector *boundaries[MAX_STAGES];

  const bool is_min;

public:
  /**
   * Constructor
   *
   * @param is_min Whether this will be used to minimise or maximise distances
   */
  TaskDijkstra(const bool is_min);

  void SetTaskSize(unsigned size) {
    SetStageCount(size);
  }

  void SetBoundary(unsigned idx, const SearchPointVector &boundary) {
    assert(idx < num_stages);

    boundaries[idx] = &boundary;
  }

  /**
   * Returns the solution point for the specified task point.  Call
   * this after run() has returned true.
   */
  const SearchPoint &GetSolution(unsigned stage) const {
    assert(stage < num_stages);

    return GetPoint(ScanTaskPoint(stage, solution[stage]));
  }

protected:
  gcc_pure
  const SearchPoint &GetPoint(ScanTaskPoint sp) const;

  bool Run();

  bool Link(const ScanTaskPoint node, const ScanTaskPoint parent,
            unsigned value) {
    if (!is_min)
      value = DIJKSTRA_MINMAX_OFFSET - value;

    return NavDijkstra::Link(node, parent, value);
  }

  /**
   * Add a zero-length start edge to each point in the first stage.
   */
  void AddZeroStartEdges();

  /**
   * Add a start edge from the given location to each point in the
   * given stage.
   */
  void AddStartEdges(unsigned stage, const SearchPoint &loc);

  /** 
   * Distance function for free point
   * 
   * @param curNode Destination node
   * @param currentLocation Origin location
   * 
   * @return Distance (flat) from origin to destination
   */
  gcc_pure
  unsigned CalcDistance(const ScanTaskPoint curNode,
                        const SearchPoint &currentLocation) const {
    /* using expensive floating point formulas here to avoid integer
       rounding errors */

    const GeoPoint &a = GetPoint(curNode).GetLocation();
    const GeoPoint &b = currentLocation.GetLocation();

    return (unsigned)a.Distance(b);
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
  unsigned CalcDistance(const ScanTaskPoint s1, const ScanTaskPoint s2) const {
    return CalcDistance(s1, GetPoint(s2));
  }

private:
  gcc_pure
  unsigned GetStageSize(const unsigned stage) const;

protected:
  /* methods from NavDijkstra */
  virtual void AddEdges(ScanTaskPoint curNode) final;
};

#endif
