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

#include "Waypoints.hpp"
#include "WaypointVisitor.hpp"
#include "Navigation/TaskProjection.hpp"
#include <vector>

#ifdef INSTRUMENT_TASK
unsigned n_queries = 0;
extern long count_intersections;
#endif

/**
 * Container accessor to allow a WaypointVisitor to visit WaypointEnvelopes 
 */
class WaypointEnvelopeVisitor:
  public Visitor<WaypointEnvelope> 
{
public:
/** 
 * Constructor
 * 
 * @param wve Contained visitor
 * 
 * @return Initialised object
 */
  WaypointEnvelopeVisitor(WaypointVisitor* wve):waypoint_visitor(wve) {};

/** 
 * Accessor operator to perform visit
 * 
 */
  void operator()(const WaypointEnvelope& as) {
    Visit(as);
  }

/** 
 * Visit item inside envelope
 * 
 */
  void Visit(const WaypointEnvelope& as) {
    as.get_waypoint().Accept(*waypoint_visitor);
  };
private:
  WaypointVisitor *waypoint_visitor;
};


Waypoints::Waypoints():
  m_file0_writable(false),
  m_home(NULL)
{
}

void
Waypoints::optimise()
{
  if (task_projection.update_fast()) {
    // task projection changed, so need to push items back onto stack
    std::copy(begin(),end(),std::back_inserter(tmp_wps));
    waypoint_tree.clear();
  }

  if (!tmp_wps.empty()) {
    while (!tmp_wps.empty()) {
      WaypointEnvelope w = (tmp_wps.front());
      w.project(task_projection);
      waypoint_tree.insert(w);
      tmp_wps.pop_front();
    }
    waypoint_tree.optimize();
  } 
}

void
Waypoints::append(const Waypoint& wp)
{
  if (empty()) {
    task_projection.reset(wp.Location);
  }
  task_projection.scan_location(wp.Location);

  tmp_wps.push_back(WaypointEnvelope(wp));
}


const Waypoint*
Waypoints::get_nearest(const GEOPOINT &loc) const 
{
  WaypointTree::const_iterator it = find_nearest(loc);
  if (it != waypoint_tree.end()) {
    return &it->get_waypoint();
  }
  return NULL;
}


Waypoints::WaypointTree::const_iterator 
Waypoints::find_nearest(const GEOPOINT &loc) const 
{
  WaypointEnvelope bb_target(loc, task_projection);
  std::pair<WaypointTree::const_iterator, double> 
    found = waypoint_tree.find_nearest(bb_target);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  return found.first;
}

const Waypoint*
Waypoints::lookup_name(const tstring &name) const
{
  WaypointTree::const_iterator found = waypoint_tree.begin();
  while (found != waypoint_tree.end()) {
    if ((*found).get_waypoint().Name == name) {
      return &(*found).get_waypoint();
    }
    found++;
  }
  return NULL;
}

const Waypoint*
Waypoints::lookup_location(const GEOPOINT &loc, const fixed range) const
{
  WaypointEnvelope bb_target(loc, task_projection);
  std::pair<WaypointTree::const_iterator, double> 
    found = waypoint_tree.find_nearest(bb_target);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  if (found.first != waypoint_tree.end()) {
    const Waypoint* wp = &(found.first)->get_waypoint();
    if (wp->Location == loc)
      return wp;
    else if (positive(range) && (wp->Location.distance(loc)<=range)) {
      return wp;
    }
  } 
  return NULL;
}


const Waypoint* 
Waypoints::find_home() const
{
  if (!m_home || !m_home->Flags.Home) {

    WaypointTree::const_iterator found = waypoint_tree.begin();
    while (found != waypoint_tree.end()) {
      const Waypoint* wp = &(*found).get_waypoint();
      if (wp->Flags.Home) {
        m_home = wp;
        return wp;
      }
      found++;
    }
  } 
  if (m_home) {
    return m_home;
  }
  return NULL;
}

bool
Waypoints::set_home(const unsigned id) 
{
  bool ok = false;
  WaypointTree::iterator found = waypoint_tree.begin();
  while (found != waypoint_tree.end()) {
    const WaypointEnvelope* wp = &(*found);
    wp->set_home(wp->get_waypoint().id == id);
    found++;
  }
  return ok;
}

const Waypoint*
Waypoints::lookup_id(const unsigned id) const
{
  WaypointTree::const_iterator found = waypoint_tree.begin();
  while (found != waypoint_tree.end()) {
    if (found->get_waypoint().id == id) {
      return &found->get_waypoint();
    }
    found++;
  }
  return NULL;
}


Waypoints::WaypointTree::const_iterator
Waypoints::find_id(const unsigned id) const
{
  WaypointTree::const_iterator found = waypoint_tree.begin();
  while (found != waypoint_tree.end()) {
    if (found->get_waypoint().id == id) {
      break;
    }
    found++;
  }
#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  return found;
}


std::vector< WaypointEnvelope >
Waypoints::find_within_range(const GEOPOINT &loc, 
                             const fixed range) const
{
  WaypointEnvelope bb_target(loc, task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);

  std::vector< WaypointEnvelope > vectors;
  waypoint_tree.find_within_range(bb_target, mrange, 
                                  std::back_inserter(vectors));
#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
  return vectors;
}


void 
Waypoints::visit_within_range(const GEOPOINT &loc, 
                              const fixed range,
                              WaypointVisitor& visitor) const
{
  WaypointEnvelope bb_target(loc, task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);

  WaypointEnvelopeVisitor wve(&visitor);  

  waypoint_tree.visit_within_range(bb_target, mrange, wve);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
}


void
Waypoints::visit_within_radius(const GEOPOINT &loc, 
                               const fixed range,
                               WaypointVisitor& visitor) const
{
  const unsigned mrange = task_projection.project_range(loc, range);
  FLAT_GEOPOINT floc = task_projection.project(loc);

  std::vector < WaypointEnvelope > vectors = find_within_range(loc, range);

  for (std::vector< WaypointEnvelope >::iterator v=vectors.begin();
       v != vectors.end(); ) {

#ifdef INSTRUMENT_TASK
        count_intersections++;
#endif

    if ((*v).flat_distance_to(floc)> mrange) {
      vectors.erase(v);
    } else {
      visitor(v->get_waypoint());
      v++;
    }
  }
}


Waypoints::WaypointTree::const_iterator
Waypoints::begin() const
{
  return waypoint_tree.begin();
}

Waypoints::WaypointTree::const_iterator
Waypoints::end() const
{
  return waypoint_tree.end();
}



void
Waypoints::clear()
{
  waypoint_tree.clear();
  m_file0_writable = false;
}

unsigned
Waypoints::size() const
{
  return waypoint_tree.size();
}

bool
Waypoints::empty() const
{
  return waypoint_tree.empty() && tmp_wps.empty();
}


void
Waypoints::erase(const Waypoint& wp)
{
  WaypointEnvelope w(wp);
  w.project(task_projection);
  waypoint_tree.erase_exact(w);
}

void
Waypoints::replace(const Waypoint& orig, Waypoint& replacement)
{
  erase(orig);
  append(replacement);
}

Waypoint
Waypoints::create(const GEOPOINT &location) 
{
  Waypoint edit_waypoint;
  edit_waypoint.Location = location;
  edit_waypoint.id = size()+tmp_wps.size()+1;
  
  if (edit_waypoint.id == 1) {
    // first waypoint, put into primary file (this will be auto-generated)
    edit_waypoint.FileNum = 0;
    m_file0_writable = true;
  } else if (m_file0_writable) {
    edit_waypoint.FileNum = 0; 
  } else {
    edit_waypoint.FileNum = 1; 
  }

  return edit_waypoint;
}

void
Waypoints::set_file0_writable(const bool set)
{
  m_file0_writable = set;
}

bool
Waypoints::get_writable(const Waypoint& wp) const
{
  if (wp.FileNum == 0) {
    return m_file0_writable || empty();
  } else {
    return true;
  }
}
