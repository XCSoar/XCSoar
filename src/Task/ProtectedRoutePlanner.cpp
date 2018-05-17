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

#include "ProtectedRoutePlanner.hpp"

void
ProtectedRoutePlanner::SetTerrain(const RasterTerrain *terrain)
{
  ExclusiveLease lease(*this);
  lease->SetTerrain(terrain);
}

void
ProtectedRoutePlanner::SetPolars(const GlideSettings &settings,
                                 const RoutePlannerConfig &config,
                                 const GlidePolar &glide_polar,
                                 const GlidePolar &safety_polar,
                                 const SpeedVector &wind,
                                 const int height_min_working)
{
  ExclusiveLease lease(*this);
  lease->UpdatePolar(settings, config, glide_polar, safety_polar,
                     wind, height_min_working);
}

void
ProtectedRoutePlanner::SolveRoute(const AGeoPoint &dest,
                                  const AGeoPoint &start,
                                  const RoutePlannerConfig &config,
                                  const int h_ceiling)
{
  ExclusiveLease lease(*this);
  lease->Synchronise(airspaces, warnings, dest, start);
  lease->Solve(dest, start, config, h_ceiling);
}

GeoPoint
ProtectedRoutePlanner::Intersection(const AGeoPoint &origin,
                                    const AGeoPoint &destination) const
{
  Lease lease(*this);
  return lease->Intersection(origin, destination);
}

void
ProtectedRoutePlanner::SolveReach(const AGeoPoint &origin,
                                  const RoutePlannerConfig &config,
                                  const int h_ceiling,
                                  const bool do_solve)
{
  ExclusiveLease lease(*this);
  lease->SolveReach(origin, config, h_ceiling, do_solve);
}

const FlatProjection
ProtectedRoutePlanner::GetTerrainReachProjection() const
{
  Lease lease(*this);
  return lease->GetTerrainReachProjection();
}
