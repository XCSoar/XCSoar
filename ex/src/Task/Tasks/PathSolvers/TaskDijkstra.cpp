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

#include "TaskDijkstra.hpp"
#include "Dijkstra.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include <algorithm>

#ifdef INSTRUMENT_TASK
unsigned num_dijkstra = 0;
#endif

bool operator == (const ScanTaskPoint &p1, const ScanTaskPoint &p2) 
{
  return (p1.first == p2.first) && (p1.second == p2.second);
}


unsigned TaskDijkstra::extremal_distance(const unsigned d) const
{
  if (shortest) {
    return d;
  } else {
    return 50000-d;
  }
}

TaskDijkstra::~TaskDijkstra() {
}


TaskDijkstra::TaskDijkstra(OrderedTask& _task):
  task(_task),
  shortest(false),
  num_taskpoints(_task.task_size())
{
  solution.reserve(num_taskpoints);
  sp_sizes.reserve(num_taskpoints);
  activeStage = task.getActiveTaskPointIndex();

  for (unsigned stage=0; stage<num_taskpoints; stage++) {
    sp_sizes[stage]= task.get_tp_search_points(stage).size();
  }
}


const SearchPoint &
TaskDijkstra::get_point(const ScanTaskPoint &sp) const
{
  return task.get_tp_search_points(sp.first)[sp.second];
}


bool 
TaskDijkstra::distance_is_significant(const SearchPoint& a1,
                                      const SearchPoint& a2)
{
  return a1.flat_distance(a2)>1;
}


unsigned TaskDijkstra::distance(const ScanTaskPoint &curNode,
                              const SearchPoint &currentLocation) const
{
#ifdef INSTRUMENT_TASK
  num_dijkstra++;
#endif
  return extremal_distance(get_point(curNode).flat_distance(currentLocation));
}

unsigned TaskDijkstra::distance(const ScanTaskPoint &s1,
                              const ScanTaskPoint &s2) const
{
#ifdef INSTRUMENT_TASK
  num_dijkstra++;
#endif
/*
  printf("p%d.%d p%d.%d = %d\n", s1.first, s1.second, s2.first, s2.second, 
         get_point(s1).flat_distance(get_point(s2)));
*/
  return extremal_distance(get_point(s1).flat_distance(get_point(s2)));
}

unsigned 
TaskDijkstra::get_size(const unsigned stage) const
{
  return sp_sizes[stage];
}

void TaskDijkstra::add_edges(DijkstraTaskPoint &dijkstra,
                             const ScanTaskPoint& curNode) 
{
  ScanTaskPoint destination;
  destination.first = curNode.first+1;

  const unsigned dsize = get_size(destination.first);

  for (destination.second=0; 
       destination.second< dsize; destination.second++) {

    dijkstra.link(destination, curNode, (distance(curNode, destination)));
  }
}

void TaskDijkstra::add_start_edges(DijkstraTaskPoint &dijkstra,
                                   const SearchPoint &currentLocation) 
{
  ScanTaskPoint destination;
  destination.first = activeStage;

  dijkstra.pop(); // need to remove dummy first point

  const unsigned dsize = get_size(destination.first);

  for (destination.second=0; 
       destination.second< dsize; destination.second++) {

    dijkstra.link(destination, destination, (distance(destination, currentLocation)));
  }
}


unsigned TaskDijkstra::distance_max()
{
  if (num_taskpoints<2) {
    return 0;
  }

  shortest = false;

  const ScanTaskPoint start(0,0);
  DijkstraTaskPoint dijkstra(start);

  unsigned d= distance_general(dijkstra);
  save_max();
  return d;
}

unsigned 
TaskDijkstra::distance_min(const SearchPoint &currentLocation)
{
  if (num_taskpoints<2) {
    return 0;
  }
  shortest = true; 

  const ScanTaskPoint start(max(1,(int)activeStage)-1,0);
  DijkstraTaskPoint dijkstra(start);
  if (activeStage) {
    add_start_edges(dijkstra, currentLocation);
  }
  unsigned d = distance_general(dijkstra);
  save_min();
  return d;
}

unsigned 
TaskDijkstra::distance_general(DijkstraTaskPoint &dijkstra)
{
  unsigned lastStage = 0-1;
  while (!dijkstra.empty()) {

    const ScanTaskPoint curNode = dijkstra.pop();

    if (curNode.first != lastStage) {
      lastStage = curNode.first;

      if (curNode.first+1 == num_taskpoints) {

        ScanTaskPoint p = curNode; 
        ScanTaskPoint p_last;
        do {
          p_last = p;
          solution[p_last.first] = get_point(p_last);
          p = dijkstra.get_predecessor(p_last);
        } while (!(p == p_last));

        return extremal_distance(dijkstra.dist());
      }
    }
    add_edges(dijkstra, curNode);
  }

  return 0-1; // No path found
}


void 
TaskDijkstra::save_min()
{
  for (unsigned j=activeStage; j<num_taskpoints; j++) {
    task.set_tp_search_min(j, solution[j]);
  }
}


void 
TaskDijkstra::save_max()
{
  for (unsigned j=0; j<num_taskpoints; j++) {
    task.set_tp_search_max(j, solution[j]);
    if (j<=activeStage) {
      task.set_tp_search_min(j, solution[j]);
    }
  }
}


/**
 * \todo 
 * - only scan parts that are required, and prune out points
 *   that become irrelevant (strictly under-performing) 
 * - if in sector, prune out all default points that result in
 *   lower distance than current achieved max
 *
 * - if searching min 
 *   first search max actual up to current
 *     (taking into account aircraft location?)
 *   then search min after that from aircraft location
 *
 * - also update saved rank for potential pruning operations
 *
 */
