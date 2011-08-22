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
#include "SampledTaskPoint.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"

SampledTaskPoint::SampledTaskPoint(Type _type, const Waypoint & wp,
                                   const bool b_scored)
  :TaskWaypoint(_type, wp),
   m_boundary_scored(b_scored),
   m_search_max(GetLocation()),
   m_search_min(GetLocation()),
   m_search_reference(GetLocation())
{
  m_nominal_points.push_back(m_search_reference);
}

// SAMPLES

bool 
SampledTaskPoint::update_sample_near(const AircraftState& state,
                                     TaskEvents &task_events,
                                     const TaskProjection &projection)
{
  if (isInSector(state)) {
    // if sample is inside sample polygon
    if (PolygonInterior(state.location, m_sampled_points))
      // return false (no update required)
      return false;

    // add sample to polygon
    SearchPoint sp(state.location, projection);
    m_sampled_points.push_back(sp);

    // re-compute convex hull
    bool retval = m_sampled_points.prune_interior();

    // only return true if hull changed
    // return true; (update required)
    return m_sampled_points.thin_to_size(64) || retval;

    // thin to size is used here to ensure the sampled points vector
    // size is bounded to reasonable values for AAT calculations.
  }

  // return false (no update required)
  return false;
}

void 
SampledTaskPoint::clear_sample_all_but_last(const AircraftState& ref_last,
                                            const TaskProjection &projection)
{
  if (has_sampled()) {
    m_sampled_points.clear();
    SearchPoint sp(ref_last.location, projection);
    m_sampled_points.push_back(sp);
  }
}

// BOUNDARY

#define fixed_steps fixed(0.05)

void 
SampledTaskPoint::update_oz(const TaskProjection &projection)
{ 
  m_search_max = m_search_reference;
  m_search_min = m_search_reference;
  m_boundary_points.clear();

  if (m_boundary_scored) {
    for (fixed t = fixed_zero; t <= fixed_one; t += fixed_steps) {
      SearchPoint sp(get_boundary_parametric(t));
      m_boundary_points.push_back(sp);
    }

    m_boundary_points.prune_interior();
  } else {
    m_boundary_points.push_back(m_search_reference);
  }

  update_projection(projection);
}

// SAMPLES + BOUNDARY

void 
SampledTaskPoint::update_projection(const TaskProjection &projection)
{
  m_search_max.project(projection);
  m_search_min.project(projection);
  m_search_reference.project(projection);
  m_nominal_points.project(projection);
  m_sampled_points.project(projection);
  m_boundary_points.project(projection);
}

void
SampledTaskPoint::reset() 
{
  m_sampled_points.clear();
}

const SearchPointVector& 
SampledTaskPoint::get_search_points() const
{
  if (search_boundary_points())
    return m_boundary_points;

  if (has_sampled())
    return m_sampled_points;

  if (search_nominal_if_unsampled())
    // this adds a point in case the waypoint was skipped
    // this is a crude way of handling the situation --- may be best
    // to de-rate the score in some way
    return m_nominal_points;

  return m_boundary_points;
}

void 
SampledTaskPoint::set_search_min(const GeoPoint &location,
                                 const TaskProjection &projection)
{
  SearchPoint sp(location, projection);
  set_search_min(sp);
}
