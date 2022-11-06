/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "TerrainRoute.hpp"
#include "Airspace/Airspaces.hpp"

class AirspaceRoute : public TerrainRoute {
  Airspaces m_airspaces;

  struct RouteAirspaceIntersection {
    const AbstractAirspace *airspace;

    RoutePoint point;

    RouteAirspaceIntersection() = default;

    constexpr
    RouteAirspaceIntersection(const AbstractAirspace *_airspace,
                              RoutePoint _point) noexcept
      :airspace(_airspace), point(_point) {}
  };

  mutable RouteAirspaceIntersection m_inx;

public:
  friend class PrintHelper;

  AirspaceRoute() noexcept;
  ~AirspaceRoute() noexcept;

  void Synchronise(const Airspaces &master, AirspacePredicate condition,
                   const AGeoPoint &origin,
                   const AGeoPoint &destination) noexcept;

  void Reset() noexcept override;

  [[gnu::pure]]
  unsigned AirspaceSize() const noexcept;

protected:

  void OnSolve(const AGeoPoint &origin, const AGeoPoint &destination) noexcept override;

  bool IsTrivial() const noexcept override {
    return m_airspaces.IsEmpty() && RoutePlanner::IsTrivial();
  }

  std::optional<RoutePoint> CheckClearance(const RouteLink &e) const noexcept override;
  void AddNearby(const RouteLink &e) noexcept override;
  bool CheckSecondary(const RouteLink &e) noexcept override;

private:
  void AddNearbyAirspace(const RouteAirspaceIntersection &inx,
                         const RouteLink &e) noexcept;

  RouteAirspaceIntersection FirstIntersecting(const RouteLink &e) const noexcept;

  [[gnu::pure]]
  const AbstractAirspace *InsideOthers(const AGeoPoint &origin) const noexcept;

  [[gnu::pure]]
  ClearingPair FindClearingPair(const SearchPointVector &spv,
                                const SearchPointVector::const_iterator start,
                                const SearchPointVector::const_iterator end,
                                const AFlatGeoPoint &dest) const noexcept;

  [[gnu::pure]]
  ClearingPair GetPairs(const SearchPointVector &spv,
                        const RoutePoint &start,
                        const RoutePoint &dest) const noexcept;

  [[gnu::pure]]
  ClearingPair GetBackupPairs(const SearchPointVector &spv,
                              const RoutePoint &start,
                              const RoutePoint &intc) const noexcept;
};
