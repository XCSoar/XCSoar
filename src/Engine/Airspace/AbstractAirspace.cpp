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

#include "AbstractAirspace.hpp"
#include "Navigation/Aircraft.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "AirspaceInterceptSolution.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Geo/Flat/FlatRay.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/GeoBounds.hpp"
#include "AirspaceIntersectionVector.hpp"
#include "Util/StringAPI.hxx"

#include <assert.h>

AbstractAirspace::~AbstractAirspace() {}

bool
AbstractAirspace::Inside(const AltitudeState &state) const
{
  return altitude_base.IsBelow(state) && altitude_top.IsAbove(state);
}

bool
AbstractAirspace::Inside(const AircraftState &state) const
{
  return altitude_base.IsBelow(state) &&
         altitude_top.IsAbove(state) &&
         Inside(state.location);
}

void
AbstractAirspace::SetGroundLevel(const double alt)
{
  altitude_base.SetGroundLevel(alt);
  altitude_top.SetGroundLevel(alt);
}

void
AbstractAirspace::SetFlightLevel(const AtmosphericPressure &press)
{
  altitude_base.SetFlightLevel(press);
  altitude_top.SetFlightLevel(press);
}

AirspaceInterceptSolution
AbstractAirspace::InterceptVertical(const AircraftState &state,
                                    const AirspaceAircraftPerformance &perf,
                                    double distance) const
{
  AirspaceInterceptSolution solution;
  solution.distance = distance;
  solution.elapsed_time = perf.SolutionVertical(solution.distance,
                                                state.altitude,
                                                altitude_base.GetAltitude(state),
                                                altitude_top.GetAltitude(state),
                                                solution.altitude);
  return solution;
}

AirspaceInterceptSolution
AbstractAirspace::InterceptHorizontal(const AircraftState &state,
                                      const AirspaceAircraftPerformance &perf,
                                      double distance_start,
                                      double distance_end,
                                      const bool lower) const
{
  if (lower && altitude_base.IsTerrain())
    // impossible to be lower than terrain
    return AirspaceInterceptSolution::Invalid();

  AirspaceInterceptSolution solution;
  solution.altitude = lower
    ? altitude_base.GetAltitude(state)
    : altitude_top.GetAltitude(state);
  solution.elapsed_time = perf.SolutionHorizontal(distance_start,
                                                  distance_end,
                                                  state.altitude,
                                                  solution.altitude,
                                                  solution.distance);
  return solution;
}

AirspaceInterceptSolution
AbstractAirspace::Intercept(const AircraftState &state,
                            const AirspaceAircraftPerformance &perf,
                            const GeoPoint &loc_start,
                            const GeoPoint &loc_end) const
{
  const bool only_vertical = (loc_start == loc_end) &&
    (loc_start == state.location);

  const auto distance_start = only_vertical
    ? double(0)
    : state.location.Distance(loc_start);

  const auto distance_end = loc_start == loc_end
    ? distance_start
    : (only_vertical ? double(0) : state.location.Distance(loc_end));

  AirspaceInterceptSolution solution =
    AirspaceInterceptSolution::Invalid();

  // need to scan at least three sides, top, far, bottom (if not terrain)

  AirspaceInterceptSolution solution_candidate =
    AirspaceInterceptSolution::Invalid();

  if (!only_vertical) {
    solution_candidate = InterceptVertical(state, perf, distance_start);
    // search near wall
    if (solution_candidate.IsEarlierThan(solution))
      solution = solution_candidate;

    if (distance_end != distance_start) {
      // need to search far wall also
      solution_candidate = InterceptVertical(state, perf, distance_end);
      if (solution_candidate.IsEarlierThan(solution))
        solution = solution_candidate;
    }
  }

  solution_candidate = InterceptHorizontal(state, perf, distance_start,
                                           distance_end, false);
  // search top wall
  if (solution_candidate.IsEarlierThan(solution))
    solution = solution_candidate;

  // search bottom wall
  if (!altitude_base.IsTerrain()) {
    solution_candidate = InterceptHorizontal(state, perf, distance_start,
                                             distance_end, true);
    if (solution_candidate.IsEarlierThan(solution))
      solution = solution_candidate;
  }

  if (solution.IsValid()) {
    if (solution.distance == distance_start)
      solution.location = loc_start;
    else if (solution.distance == distance_end)
      solution.location = loc_end;
    else if (distance_end > 0)
      solution.location =
        state.location.Interpolate(loc_end, solution.distance / distance_end);
    else
      solution.location = loc_start;

    assert(solution.distance >= 0);
  }

  return solution;
}

AirspaceInterceptSolution
AbstractAirspace::Intercept(const AircraftState &state,
                            const GeoPoint &end,
                            const FlatProjection &projection,
                            const AirspaceAircraftPerformance &perf) const
{
  AirspaceInterceptSolution solution = AirspaceInterceptSolution::Invalid();
  for (const auto &i : Intersects(state.location, end, projection)) {
    auto new_solution = Intercept(state, perf, i.first, i.second);
    if (new_solution.IsEarlierThan(solution))
      solution = new_solution;
  }

  return solution;
}

bool
AbstractAirspace::MatchNamePrefix(const TCHAR *prefix) const
{
  size_t prefix_length = _tcslen(prefix);
  return StringIsEqualIgnoreCase(name.c_str(), prefix, prefix_length);
}

void
AbstractAirspace::Project(const FlatProjection &projection)
{
  m_border.Project(projection);
}

const FlatBoundingBox
AbstractAirspace::GetBoundingBox(const FlatProjection &projection)
{
  Project(projection);
  return m_border.CalculateBoundingbox();
}

GeoBounds
AbstractAirspace::GetGeoBounds() const
{
  return m_border.CalculateGeoBounds();
}

const SearchPointVector&
AbstractAirspace::GetClearance(const FlatProjection &projection) const
{
  #define RADIUS 5

  if (!m_clearance.empty())
    return m_clearance;

  m_clearance = m_border;
  if (is_convex != TriState::FALSE)
    is_convex = m_clearance.PruneInterior() ? TriState::FALSE : TriState::TRUE;

  FlatBoundingBox bb = m_clearance.CalculateBoundingbox();
  FlatGeoPoint center = bb.GetCenter();

  for (SearchPoint &i : m_clearance) {
    FlatGeoPoint p = i.GetFlatLocation();
    FlatRay r(center, p);
    int mag = r.Magnitude();
    int mag_new = mag + RADIUS;
    p = r.Parametric((double)mag_new / mag);
    i = SearchPoint(projection.Unproject(p), p);
  }

  return m_clearance;
}

void
AbstractAirspace::ClearClearance() const
{
  m_clearance.clear();
}

void
AbstractAirspace::SetActivity(const AirspaceActivity mask) const
{
  active = days_of_operation.Matches(mask);
}
