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
   boundary_scored(b_scored),
   search_max(GetLocation()),
   search_min(GetLocation()),
   search_reference(GetLocation())
{
  nominal_points.push_back(search_reference);
}

// SAMPLES

bool 
SampledTaskPoint::UpdateSampleNear(const AircraftState& state,
                                     TaskEvents &task_events,
                                     const TaskProjection &projection)
{
  if (IsInSector(state)) {
    // if sample is inside sample polygon
    if (PolygonInterior(state.location, sampled_points))
      // return false (no update required)
      return false;

    // add sample to polygon
    SearchPoint sp(state.location, projection);
    sampled_points.push_back(sp);

    // re-compute convex hull
    bool retval = sampled_points.prune_interior();

    // only return true if hull changed
    // return true; (update required)
    return sampled_points.thin_to_size(64) || retval;

    // thin to size is used here to ensure the sampled points vector
    // size is bounded to reasonable values for AAT calculations.
  }

  // return false (no update required)
  return false;
}

void 
SampledTaskPoint::ClearSampleAllButLast(const AircraftState& ref_last,
                                            const TaskProjection &projection)
{
  if (HasSampled()) {
    sampled_points.clear();
    SearchPoint sp(ref_last.location, projection);
    sampled_points.push_back(sp);
  }
}

// BOUNDARY

#define fixed_steps fixed(0.05)

void 
SampledTaskPoint::UpdateOZ(const TaskProjection &projection)
{ 
  search_max = search_reference;
  search_min = search_reference;
  boundary_points.clear();

  if (boundary_scored) {
    for (fixed t = fixed_zero; t <= fixed_one; t += fixed_steps) {
      SearchPoint sp(GetBoundaryParametric(t));
      boundary_points.push_back(sp);
    }

    boundary_points.prune_interior();
  } else {
    boundary_points.push_back(search_reference);
  }

  UpdateProjection(projection);
}

// SAMPLES + BOUNDARY

void 
SampledTaskPoint::UpdateProjection(const TaskProjection &projection)
{
  search_max.project(projection);
  search_min.project(projection);
  search_reference.project(projection);
  nominal_points.project(projection);
  sampled_points.project(projection);
  boundary_points.project(projection);
}

void
SampledTaskPoint::Reset() 
{
  sampled_points.clear();
}

const SearchPointVector& 
SampledTaskPoint::GetSearchPoints() const
{
  if (SearchBoundaryPoints())
    return boundary_points;

  if (HasSampled())
    return sampled_points;

  if (SearchNominalIfUnsampled())
    // this adds a point in case the waypoint was skipped
    // this is a crude way of handling the situation --- may be best
    // to de-rate the score in some way
    return nominal_points;

  return boundary_points;
}

void 
SampledTaskPoint::SetSearchMin(const GeoPoint &location,
                                 const TaskProjection &projection)
{
  SearchPoint sp(location, projection);
  SetSearchMin(sp);
}
