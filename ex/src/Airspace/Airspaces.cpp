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
#include "Airspaces.hpp"
#include "AirspaceVisitor.hpp"
#include <deque>

#ifdef INSTRUMENT_TASK
extern unsigned n_queries;
extern long count_intersections;
#endif

const std::vector<Airspace>
Airspaces::scan_nearest(const AIRCRAFT_STATE &state) const 
{
  Airspace bb_target(state.Location, task_projection);

  std::pair<AirspaceTree::const_iterator, double> 
    found = airspace_tree.find_nearest(bb_target);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  std::vector<Airspace> res;
  if (found.first != airspace_tree.end()) {
    // also should do scan_range with range = 0 since there
    // could be more than one with zero dist
    if (found.second==0) {
      return scan_range(state, 0);
    } else {
      res.push_back(*found.first);
    }
  }
  return res;
}

void 
Airspaces::visit_within_range(const GEOPOINT &loc, 
                              const double range,
                              AirspaceVisitor& visitor) const
{
  Airspace bb_target(loc, task_projection);
  int mrange = task_projection.project_range(loc, range);
  airspace_tree.visit_within_range(bb_target, -mrange, visitor);
}

void 
Airspaces::visit_intersecting(const GEOPOINT &loc, 
                              const GeoVector &vec,
                              AirspaceVisitor& visitor) const
{
  FlatRay ray(task_projection.project(loc), 
              task_projection.project(vec.end_point(loc)));

  std::deque< Airspace > vectors;
  {
    GEOPOINT c = vec.mid_point(loc);
    Airspace bb_target(c, task_projection);
    int mrange = task_projection.project_range(c, vec.Distance/2.0);
    airspace_tree.find_within_range(bb_target, -mrange, 
                                    std::back_inserter(vectors));
  }

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
  for (std::deque<Airspace>::iterator v=vectors.begin();
       v != vectors.end(); v++) {
    if (v->intersects(ray)) {
      if (v->intersects(loc, vec, task_projection)) {
        visitor(*v);
      }
    }
  }
}


const std::vector<Airspace>
Airspaces::scan_range(const AIRCRAFT_STATE &state, 
                      const double range) const
{
  Airspace bb_target(state.Location, task_projection);
  int mrange = task_projection.project_range(state.Location, range);
  
  std::deque< Airspace > vectors;
  airspace_tree.find_within_range(bb_target, -mrange, std::back_inserter(vectors));

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  std::vector<Airspace> res;

  for (std::deque<Airspace>::iterator v=vectors.begin();
       v != vectors.end(); v++) {
    if ((*v).distance(bb_target)<= range) {
      if ((*v).inside(state) || (range>0)) {
        res.push_back(*v);
      }
    }        
  }
  return res;
}


std::vector< Airspace >
Airspaces::find_inside(const AIRCRAFT_STATE &state) const
{
  Airspace bb_target(state.Location, task_projection);

  std::vector< Airspace > vectors;
  airspace_tree.find_within_range(bb_target, 0, std::back_inserter(vectors));

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  for (std::vector<Airspace>::iterator v=vectors.begin();
       v != vectors.end(); ) {

#ifdef INSTRUMENT_TASK
        count_intersections++;
#endif

    if (!(*v).inside(state)) {
      vectors.erase(v);
    } else {
      v++;
    }
  }
  return vectors;
}

void 
Airspaces::optimise()
{
  task_projection.update_fast();

  while (!tmp_as.empty()) {
    Airspace as(*tmp_as.front(), task_projection);
    airspace_tree.insert(as);
    tmp_as.pop_front();
  }
  airspace_tree.optimise();
}

void 
Airspaces::insert(AbstractAirspace* asp)
{
  if (!asp) {
    // nothing to add
    return;
  }
  if (tmp_as.empty() && airspace_tree.empty()) {
    task_projection.reset(asp->get_center());
  }
  task_projection.scan_location(asp->get_center());

  tmp_as.push_back(asp);

  /**
   * \todo
   * if range changed, need to re-pack airspace
   * will have to remove all from the list, recalculate projections,
   * then add them again!
   * (can just insert() them all, then clear the tree, then run optimise()
   */
}


Airspaces::~Airspaces()
{
  clear();
}


void
Airspaces::clear()
{

  // delete temporaries in case they were added without an optimise() call
  while (!tmp_as.empty()) {
    AbstractAirspace *aa = tmp_as.front();
    delete aa;
    tmp_as.pop_front();
  }

  // delete items in the tree
  for (AirspaceTree::iterator v = airspace_tree.begin();
       v != airspace_tree.end(); v++) {
    Airspace a = *v;
    a.destroy();
  }

  // then delete the tree
  airspace_tree.clear();
}
