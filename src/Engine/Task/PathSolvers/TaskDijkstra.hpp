// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PathSolvers/NavDijkstra.hpp"
#include "Geo/SearchPoint.hpp"

#include <cassert>

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
class TaskDijkstra : protected NavDijkstra<>
{
  const SearchPointVector *boundaries[MAX_STAGES];

  const bool is_min;

public:
  /**
   * Constructor
   *
   * @param is_min Whether this will be used to minimise or maximise distances
   */
  explicit TaskDijkstra(const bool is_min) noexcept;

  void SetTaskSize(unsigned size) noexcept {
    SetStageCount(size);
  }

  void SetBoundary(unsigned idx, const SearchPointVector &boundary) noexcept {
    assert(idx < num_stages);

    boundaries[idx] = &boundary;
  }

  /**
   * Returns the solution point for the specified task point.  Call
   * this after run() has returned true.
   */
  const SearchPoint &GetSolution(unsigned stage) const noexcept {
    assert(stage < num_stages);

    return GetPoint(ScanTaskPoint(stage, solution[stage]));
  }

protected:
  [[gnu::pure]]
  const SearchPoint &GetPoint(ScanTaskPoint sp) const noexcept;

  bool Run() noexcept;

  bool Link(const ScanTaskPoint node, const ScanTaskPoint parent,
            value_type value) noexcept {
    if (!is_min)
      value = DIJKSTRA_MINMAX_OFFSET - value;

    return NavDijkstra::Link(node, parent, value);
  }

  /**
   * Add a zero-length start edge to each point in the first stage.
   */
  void AddZeroStartEdges() noexcept;

  /**
   * Add a start edge from the given location to each point in the
   * given stage.
   */
  void AddStartEdges(unsigned stage, const SearchPoint &loc) noexcept;

  /** 
   * Distance function for free point
   * 
   * @param curNode Destination node
   * @param currentLocation Origin location
   * 
   * @return Distance (flat) from origin to destination
   */
  [[gnu::pure]]
  value_type CalcDistance(const ScanTaskPoint curNode,
                          const SearchPoint &currentLocation) const noexcept {
    /* using expensive floating point formulas here to avoid integer
       rounding errors */

    const GeoPoint &a = GetPoint(curNode).GetLocation();
    const GeoPoint &b = currentLocation.GetLocation();

    return static_cast<value_type>(a.Distance(b));
  }

  /** 
   * Distance function for edges
   * 
   * @param s1 Origin node
   * @param s2 Destination node
   * 
   * @return Distance (flat) from origin to destination
   */
  [[gnu::pure]]
  value_type CalcDistance(const ScanTaskPoint s1,
                          const ScanTaskPoint s2) const noexcept {
    return CalcDistance(s1, GetPoint(s2));
  }

private:
  [[gnu::pure]]
  unsigned GetStageSize(const unsigned stage) const noexcept;

protected:
  /* methods from NavDijkstra */
  virtual void AddEdges(ScanTaskPoint curNode) noexcept final;
};
