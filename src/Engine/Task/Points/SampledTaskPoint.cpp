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

#include "SampledTaskPoint.hpp"
#include "Task/ObservationZones/Boundary.hpp"
#include "Navigation/Aircraft.hpp"

SampledTaskPoint::SampledTaskPoint(const GeoPoint &location,
                                   const bool b_scored)
  :boundary_scored(b_scored), past(false),
   nominal_points(1, location)
{
#ifndef NDEBUG
  search_max.SetInvalid();
  search_min.SetInvalid();
#endif
}

// SAMPLES

bool
SampledTaskPoint::AddInsideSample(const AircraftState& state,
                                  const FlatProjection &projection)
{
  assert(state.location.IsValid());

  // if sample is inside sample polygon
  if (sampled_points.IsInside(state.location))
    // return false (no update required)
    return false;

  // add sample to polygon
  SearchPoint sp(state.location, projection);
  sampled_points.push_back(sp);

  // re-compute convex hull
  bool retval = sampled_points.PruneInterior();

  // only return true if hull changed
  // return true; (update required)
  return sampled_points.ThinToSize(64) || retval;

  /* thin to size is used here to ensure the sampled points vector
     size is bounded to reasonable values for AAT calculations */
}

void
SampledTaskPoint::ClearSampleAllButLast(const AircraftState& ref_last,
                                        const FlatProjection &projection)
{
  if (HasSampled()) {
    sampled_points.clear();
    SearchPoint sp(ref_last.location, projection);
    sampled_points.push_back(sp);
  }
}

// BOUNDARY

void
SampledTaskPoint::UpdateOZ(const FlatProjection &projection,
                           const OZBoundary &_boundary)
{
  search_max = search_min = nominal_points.front();
  boundary_points.clear();

  for (const SearchPoint sp : _boundary)
    boundary_points.push_back(sp);

  UpdateProjection(projection);
}

// SAMPLES + BOUNDARY

void
SampledTaskPoint::UpdateProjection(const FlatProjection &projection)
{
  search_max.Project(projection);
  search_min.Project(projection);
  nominal_points.Project(projection);
  sampled_points.Project(projection);
  boundary_points.Project(projection);
}

void
SampledTaskPoint::Reset()
{
  sampled_points.clear();
}

const SearchPointVector &
SampledTaskPoint::GetSearchPoints() const
{
  assert(!boundary_points.empty());

  if (HasSampled())
    return sampled_points;

  if (past)
    // this adds a point in case the waypoint was skipped
    // this is a crude way of handling the situation --- may be best
    // to de-rate the score in some way
    return nominal_points;

  return boundary_points;
}
