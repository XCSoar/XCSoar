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
#include "SampledTaskPoint.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"

SampledTaskPoint::SampledTaskPoint(const TaskProjection& tp,
                                   const Waypoint & wp,
                                   const TaskBehaviour &tb,
                                   const bool b_scored):
    TaskPoint(wp,tb),
    m_boundary_scored(b_scored),
    m_task_projection(tp),
    m_search_max(get_location(),tp),
    m_search_min(get_location(),tp),
    m_search_reference(get_location(),tp)
{
  m_nominal_point.push_back(m_search_reference);
}

// SAMPLES

bool 
SampledTaskPoint::update_sample(const AIRCRAFT_STATE& state,
                                const TaskEvents &task_events)
{
  if (isInSector(state)) {
    // if sample is inside sample polygon
    //   return false (no update required)
    // else
    //   add sample to polygon
    //   re-compute convex hull
    //   return true; (update required)
    //
    if (PolygonInterior(state.Location, m_sampled_points)) {
      // do nothing
      return false;
    } else {
      SearchPoint sp(state.Location, m_task_projection, true);
      m_sampled_points.push_back(sp);
      // only return true if hull changed 
      return prune_interior(m_sampled_points);
    }
  }
  return false;
}


void 
SampledTaskPoint::clear_sample_all_but_last(const AIRCRAFT_STATE& ref_last) 
{
  if (!m_sampled_points.empty()) {
    m_sampled_points.clear();
    SearchPoint sp(ref_last.Location, m_task_projection, true);
    m_sampled_points.push_back(sp);
  }
}

// BOUNDARY

static const fixed fixed_steps(0.05);

void 
SampledTaskPoint::update_oz() 
{ 
  m_search_max = m_search_reference;
  m_search_min = m_search_reference;
  m_boundary_points.clear();
  if (m_boundary_scored) {
    for (fixed t=fixed_zero; t<= fixed_one; t+= fixed_steps) {
      SearchPoint sp(get_boundary_parametric(t), m_task_projection);
      m_boundary_points.push_back(sp);
    }
    prune_interior(m_boundary_points);
  } else {
    m_boundary_points.push_back(m_search_reference);
  }
  update_projection();
}

// SAMPLES + BOUNDARY

void 
SampledTaskPoint::update_projection()
{
  m_search_max.project(m_task_projection);
  m_search_min.project(m_task_projection);
  m_search_reference.project(m_task_projection);
  project(m_nominal_point, m_task_projection);
  project(m_sampled_points, m_task_projection);
  project(m_boundary_points, m_task_projection);
}


void
SampledTaskPoint::reset() 
{
  m_sampled_points.clear();
}


const SearchPointVector& 
SampledTaskPoint::get_search_points() const
{
  if (search_boundary_points()) {
    return m_boundary_points;
  } else {
    if (m_sampled_points.empty()) {
      if (search_nominal_if_unsampled()) {
        // this adds a point in case the waypoint was skipped
        // this is a crude way of handling the situation --- may be best
        // to de-rate the score in some way
        return m_nominal_point;
      } else {
        return m_boundary_points;
      }
    } else {
      return m_sampled_points;
    }
  }
}


const SearchPointVector& 
SampledTaskPoint::get_sample_points() const
{
  return m_sampled_points;
}
