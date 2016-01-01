/*
Copyright_License {

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

#include "SoonestAirspace.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceInterceptSolution.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "Minimum.hpp"
#include "Navigation/Aircraft.hpp"

struct SoonestAirspace {
  const AbstractAirspace *airspace = nullptr;
  double time = -1;

  SoonestAirspace() = default;
  SoonestAirspace(const AbstractAirspace &_airspace,
                  double _time)
    :airspace(&_airspace), time(_time) {}


  bool IsDefined() const {
    return airspace != nullptr;
  }
};

gcc_pure
__attribute__((always_inline))
static inline SoonestAirspace
CalculateSoonestAirspace(const AircraftState &state,
                         const AirspaceAircraftPerformance &perf,
                         const double max_time,
                         const FlatProjection &projection,
                         const AbstractAirspace &airspace)
{
  const auto closest = airspace.ClosestPoint(state.location, projection);
  assert(closest.IsValid());

  const auto solution = airspace.Intercept(state, perf, closest, closest);
  if (!solution.IsValid() ||
      solution.elapsed_time > max_time)
    return SoonestAirspace();

  return SoonestAirspace(airspace, solution.elapsed_time);
}

struct CompareSoonestAirspace {
  gcc_pure
  bool operator()(const SoonestAirspace &a, const SoonestAirspace &b) const {
    return a.IsDefined() && (!b.IsDefined() || a.time < b.time);
  }
};

const AbstractAirspace *
FindSoonestAirspace(const Airspaces &airspaces,
                    const AircraftState &state,
                    const AirspaceAircraftPerformance &perf,
                    const AirspacePredicate &predicate,
                    const double max_time)
{
  const auto &projection = airspaces.GetProjection();
  const auto range = perf.GetMaxSpeed() * max_time;
  return FindMinimum(airspaces, state.location, range, predicate,
                     [&state, &perf, max_time,
                      &projection](const AbstractAirspace &airspace){
                       return CalculateSoonestAirspace(state, perf, max_time,
                                                       projection, airspace);
                     },
                     CompareSoonestAirspace()).airspace;
}
