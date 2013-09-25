/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_PROTECTED_ROUTE_PLANNER_HPP
#define XCSOAR_PROTECTED_ROUTE_PLANNER_HPP

#include "Thread/Guard.hpp"
#include "Task/RoutePlannerGlue.hpp"
#include "Compiler.h"

struct GlideSettings;
struct RoutePlannerConfig;
class GlidePolar;
class RasterTerrain;
class Airspaces;

/**
 * Facade to task/airspace/waypoints as used by threads,
 * to manage locking
 */
class ProtectedRoutePlanner: public Guard<RoutePlannerGlue>
{
protected:
  const Airspaces &airspaces;
  const ProtectedAirspaceWarningManager *warnings;

public:
  ProtectedRoutePlanner(RoutePlannerGlue &route, const Airspaces &_airspaces,
                        const ProtectedAirspaceWarningManager *_warnings)
    :Guard<RoutePlannerGlue>(route),
     airspaces(_airspaces), warnings(_warnings) {}

  void Reset() {
    ExclusiveLease lease(*this);
    lease->Reset();
  }

  void ClearReach() {
    ExclusiveLease lease(*this);
    lease->ClearReach();
  }

  gcc_pure
  bool IsReachEmpty() const {
    Lease lease(*this);
    return lease->IsReachEmpty();
  }

  void SetTerrain(const RasterTerrain *terrain);

  void SetPolars(const GlideSettings &settings,
                 const GlidePolar &glide_polar, const GlidePolar &safety_polar,
                 const SpeedVector &wind);

  void SolveRoute(const AGeoPoint &dest, const AGeoPoint &start,
                  const RoutePlannerConfig &config,
                  const RoughAltitude h_ceiling);

  bool Intersection(const AGeoPoint &origin,
                    const AGeoPoint &destination,
                    GeoPoint &intx) const;

  void SolveReach(const AGeoPoint &origin, const RoutePlannerConfig &config,
                  RoughAltitude h_ceiling, bool do_solve);

  void AcceptInRange(const GeoBounds &bounds,
                     TriangleFanVisitor &visitor) const;
};

#endif
