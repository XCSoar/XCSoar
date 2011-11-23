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

#include "AbstractAirspace.hpp"
#include "Navigation/Aircraft.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "AirspaceInterceptSolution.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"

#include <assert.h>

#if defined(__WINE__) && !defined(_UNICODE)
#include <windows.h>
#endif

AbstractAirspace::~AbstractAirspace() {}

bool
AbstractAirspace::Inside(const AircraftState &state) const
{
  return altitude_base.IsBelow(state) &&
         altitude_top.IsAbove(state) &&
         Inside(state.location);
}

void 
AbstractAirspace::SetGroundLevel(const fixed alt) 
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
                                    const fixed &distance) const
{
  AirspaceInterceptSolution solution;
  solution.distance = distance;
  solution.elapsed_time = perf.solution_vertical(solution.distance, 
                                                 state.altitude,
                                                 altitude_base.GetAltitude(state),
                                                 altitude_top.GetAltitude(state),
                                                 solution.altitude);
  return solution;
}

AirspaceInterceptSolution 
AbstractAirspace::InterceptHorizontal(const AircraftState &state,
                                       const AirspaceAircraftPerformance &perf,
                                       const fixed &distance_start,
                                       const fixed &distance_end,
                                       const bool lower) const
{
  if (lower && altitude_base.IsTerrain())
    // impossible to be lower than terrain
    return AirspaceInterceptSolution::Invalid();

  AirspaceInterceptSolution solution;
  solution.altitude = lower? altitude_base.GetAltitude(state): altitude_top.GetAltitude(state);
  solution.elapsed_time = perf.solution_horizontal(distance_start, 
                                                   distance_end,
                                                   state.altitude,
                                                   solution.altitude,
                                                   solution.distance);
  return solution;
}

bool 
AbstractAirspace::Intercept(const AircraftState &state,
                            const AirspaceAircraftPerformance &perf,
                            AirspaceInterceptSolution &solution,
                            const GeoPoint &loc_start,
                            const GeoPoint &loc_end) const
{
  const bool only_vertical = (loc_start == loc_end) &&
                             (loc_start == state.location);

  const fixed distance_start = only_vertical ?
                               fixed_zero : state.location.Distance(loc_start);

  const fixed distance_end =
      (loc_start == loc_end) ?
      distance_start :
      (only_vertical ? fixed_zero : state.location.Distance(loc_end));

  AirspaceInterceptSolution solution_this =
    AirspaceInterceptSolution::Invalid();

  // need to scan at least three sides, top, far, bottom (if not terrain)

  AirspaceInterceptSolution solution_candidate =
    AirspaceInterceptSolution::Invalid();

  if (!only_vertical) {
    solution_candidate = InterceptVertical(state, perf, distance_start);
    // search near wall
    if (solution_candidate.IsValid() && 
        ((solution_candidate.elapsed_time < solution_this.elapsed_time) ||
         negative(solution_this.elapsed_time)))
      solution_this = solution_candidate;


    if (distance_end != distance_start) {
      // need to search far wall also
      solution_candidate = InterceptVertical(state, perf, distance_end);
      if (solution_candidate.IsValid() &&
          ((solution_candidate.elapsed_time < solution_this.elapsed_time) ||
           negative(solution_this.elapsed_time)))
        solution_this = solution_candidate;
    }
  }

  solution_candidate = InterceptHorizontal(state, perf, distance_start,
                                           distance_end, false);
  // search top wall
  if (solution_candidate.IsValid() && 
      ((solution_candidate.elapsed_time < solution_this.elapsed_time) ||
       negative(solution_this.elapsed_time)))
    solution_this = solution_candidate;
  
  // search bottom wall
  if (!altitude_base.IsTerrain()) {
    solution_candidate = InterceptHorizontal(state, perf, distance_start,
                                             distance_end, true);
    if (solution_candidate.IsValid() && 
        ((solution_candidate.elapsed_time < solution_this.elapsed_time) ||
         negative(solution_this.elapsed_time)))
      solution_this = solution_candidate;
  }

  if (solution_this.IsValid()) {
    solution = solution_this;
    if (solution.distance == distance_start)
      solution.location = loc_start;
    else if (solution.distance == distance_end)
      solution.location = loc_end;
    else if (positive(distance_end))
      solution.location =
          state.location.Interpolate(loc_end, solution.distance / distance_end);
    else
      solution.location = loc_start;

    assert(!negative(solution.distance));
    return true;
  }
  else
    solution = AirspaceInterceptSolution::Invalid();

  return false;
}


bool 
AbstractAirspace::Intercept(const AircraftState &state,
                            const GeoVector &vec,
                            const AirspaceAircraftPerformance &perf,
                            AirspaceInterceptSolution &solution) const
{
  AirspaceIntersectionVector vis = Intersects(state.location, vec);
  if (vis.empty())
    return false;

  AirspaceInterceptSolution this_solution =
    AirspaceInterceptSolution::Invalid();
  for (AirspaceIntersectionVector::const_iterator it = vis.begin();
       it != vis.end(); ++it)
    Intercept(state, perf, this_solution, it->first, it->second);

  if (!this_solution.IsValid())
    return false;

  solution = this_solution;
  return true;
}

const TCHAR *
AbstractAirspace::GetTypeText(const bool concise) const
{
  return AirspaceClassAsText(type, concise);
}

const tstring 
AbstractAirspace::GetNameText() const
{
  return name + _T(" ") + AirspaceClassAsText(type);
}

bool
AbstractAirspace::MatchNamePrefix(const TCHAR *prefix) const
{
  size_t prefix_length = _tcslen(prefix);
#if defined(__WINE__) && !defined(_UNICODE)
  if (name.length() < prefix_length)
    return false;

  /* on WINE, we don't have _tcsnicmp() */
  return CompareStringA(LOCALE_USER_DEFAULT,
                        NORM_IGNORECASE|NORM_IGNOREKANATYPE|
                        NORM_IGNORENONSPACE|NORM_IGNORESYMBOLS|
                        NORM_IGNOREWIDTH,
                        name.c_str(), prefix_length,
                        prefix, prefix_length) == CSTR_EQUAL;
#else
  return _tcsnicmp(name.c_str(), prefix, prefix_length) == 0;
#endif
}

const tstring 
AbstractAirspace::GetRadioText() const
{
  return radio;
}

const tstring
AbstractAirspace::GetVerticalText() const
{
  return _T("Base: ") + altitude_base.GetAsText(false) +
         _T(" Top: ") + altitude_top.GetAsText(false);
}

void
AbstractAirspace::Project(const TaskProjection &task_projection)
{
  m_border.Project(task_projection);
}

const FlatBoundingBox
AbstractAirspace::GetBoundingBox(const TaskProjection& task_projection)
{
  Project(task_projection);
  return m_border.CalculateBoundingbox();
}

GeoBounds
AbstractAirspace::GetGeoBounds() const
{
  return m_border.CalculateGeoBounds();
}

const SearchPointVector&
AbstractAirspace::GetClearance() const
{
  #define RADIUS 5

  if (!m_clearance.empty())
    return m_clearance;

  assert(m_task_projection != NULL);

  m_clearance = m_border;
  if (!m_is_convex)
    m_clearance.PruneInterior();

  FlatBoundingBox bb = m_clearance.CalculateBoundingbox();
  FlatGeoPoint center = bb.GetCenter();

  for (SearchPointVector::iterator i= m_clearance.begin();
       i != m_clearance.end(); ++i) {
    FlatGeoPoint p = i->get_flatLocation();
    FlatRay r(center, p);
    int mag = hypot(r.vector.Longitude, r.vector.Latitude);
    int mag_new = mag + RADIUS;
    p = r.Parametric((fixed)mag_new / mag);
    *i = SearchPoint(m_task_projection->unproject(p));
    i->project(*m_task_projection);
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
  active = days_of_operation.matches(mask);
}
