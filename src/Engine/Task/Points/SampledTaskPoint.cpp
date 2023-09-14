// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SampledTaskPoint.hpp"
#include "Task/ObservationZones/Boundary.hpp"
#include "Navigation/Aircraft.hpp"

SampledTaskPoint::SampledTaskPoint(const GeoPoint &location,
                                   const bool b_scored) noexcept
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
                                  const FlatProjection &projection) noexcept
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
                                        const FlatProjection &projection) noexcept
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
                           const OZBoundary &_boundary) noexcept
{
  search_max = search_min = nominal_points.front();
  boundary_points.clear();

  for (const SearchPoint sp : _boundary)
    boundary_points.push_back(sp);

  UpdateProjection(projection);
}

// SAMPLES + BOUNDARY

void
SampledTaskPoint::UpdateProjection(const FlatProjection &projection) noexcept
{
  search_max.Project(projection);
  search_min.Project(projection);
  nominal_points.Project(projection);
  sampled_points.Project(projection);
  boundary_points.Project(projection);
}

void
SampledTaskPoint::Reset() noexcept
{
  sampled_points.clear();
}

const SearchPointVector &
SampledTaskPoint::GetSearchPoints() const noexcept
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
