/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Navigation/SearchPointVector.hpp"

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
public:
/**
 * Constructor
 *
 * @param _task The task to find max/min distances for
 */
  TaskDijkstra(OrderedTask& _task);

/**
 * Destructor, frees local variables
 */
  ~TaskDijkstra();

/** 
 * Search task points for targets within OZs to produce the
 * maximum-distance task.  Saves the max-distance solution 
 * in the corresponding task points for later accurate distance
 * measurement.
 * 
 * @return True if succeeded
 */  
  bool distance_max();

/** 
 * Search task points for targets within OZs to produce the
 * minimum-distance task.  Saves the minimum-distance solution 
 * in the corresponding task points for later accurate distance
 * measurement.
 * 
 * Note that the minimum distance task is the minimum distance
 * remaining and is therefore sensitive to the specified aircraft
 * location.
 *
 * @param location Location of aircraft
 * @return True if succeeded
 */  
  bool distance_min(const SearchPoint& location);

protected:
  const SearchPoint &get_point(const ScanTaskPoint &sp) const;

private:
  unsigned get_size(const unsigned stage) const;

  void add_edges(DijkstraTaskPoint &dijkstra,
                 const ScanTaskPoint &curNode);

  void add_start_edges(DijkstraTaskPoint &dijkstra,
                 const SearchPoint &loc);

  void save_max();
  void save_min();

  void get_sizes();

  OrderedTask& task;
  std::vector<unsigned> sp_sizes;
  unsigned active_stage;
};

#endif
