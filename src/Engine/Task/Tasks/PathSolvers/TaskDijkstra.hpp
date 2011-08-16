/* Copyright_License {

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

#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "NavDijkstra.hpp"
#include "Navigation/SearchPoint.hpp"

class OrderedTask;

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
 * This uses a Dijkstra search and so is O(N log(N)).
 */
class TaskDijkstra: 
  public NavDijkstra<SearchPoint>
{
protected:
  OrderedTask &task;
  unsigned active_stage;

private:
  unsigned sp_sizes[MAX_STAGES];

public:
  /**
   * Constructor
   *
   * @param _task The task to find max/min distances for
   * @param is_min Whether this will be used to minimise or maximise distances
   * @param do_reserve Whether to reserve dijkstra queue
   */
  TaskDijkstra(OrderedTask& _task, const bool is_min,
               const bool do_reserve=false);

protected:
  gcc_pure
  const SearchPoint &GetPointFast(const ScanTaskPoint &sp) const;

  const SearchPoint &get_point(const ScanTaskPoint &sp) const;
  bool run();
  virtual void save() = 0;

  /**
   * Update internal details required from the task
   */
  bool refresh_task();

  void add_start_edges(const SearchPoint &loc);

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
                    const SearchPoint &currentLocation) const {
    return GetPointFast(curNode).flat_distance(currentLocation);
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
    return GetPointFast(s1).flat_distance(GetPointFast(s2));
  }

private:
  void calculate_sizes();
  unsigned get_size(const unsigned stage) const;

  void add_edges(const ScanTaskPoint &curNode);
};

#endif
