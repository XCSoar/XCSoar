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

#ifndef XCSOAR_AIRSPACE_ROUTE_HPP
#define XCSOAR_AIRSPACE_ROUTE_HPP

#include "RoutePlanner.hpp"
#include "Airspace/Airspaces.hpp"

class AirspaceRoute : public RoutePlanner {
  Airspaces m_airspaces;

  struct RouteAirspaceIntersection {
    const AbstractAirspace *airspace;

    RoutePoint point;

    RouteAirspaceIntersection() = default;

    constexpr
    RouteAirspaceIntersection(const AbstractAirspace *_airspace,
                              RoutePoint _point)
      :airspace(_airspace), point(_point) {}
  };

  mutable RouteAirspaceIntersection m_inx;

public:
  friend class PrintHelper;

  AirspaceRoute();
  virtual ~AirspaceRoute();

  void Synchronise(const Airspaces &master, const AirspacePredicate &condition,
                   const AGeoPoint &origin,
                   const AGeoPoint &destination);

  void Reset() override;

  unsigned AirspaceSize() const;

protected:

  void OnSolve(const AGeoPoint &origin, const AGeoPoint &destination) override;

  bool IsTrivial() const override {
    return m_airspaces.IsEmpty() && RoutePlanner::IsTrivial();
  }

private:
  bool CheckClearance(const RouteLink &e, RoutePoint &inp) const override;
  void AddNearby(const RouteLink &e) override;
  bool CheckSecondary(const RouteLink &e) override;

  void AddNearbyAirspace(const RouteAirspaceIntersection &inx,
                         const RouteLink &e);

  RouteAirspaceIntersection FirstIntersecting(const RouteLink &e) const;

  const AbstractAirspace *InsideOthers(const AGeoPoint &origin) const;

  ClearingPair FindClearingPair(const SearchPointVector &spv,
                                const SearchPointVector::const_iterator start,
                                const SearchPointVector::const_iterator end,
                                const AFlatGeoPoint &dest) const;

  ClearingPair GetPairs(const SearchPointVector &spv,
                        const RoutePoint &start,
                        const RoutePoint &dest) const;

  ClearingPair GetBackupPairs(const SearchPointVector &spv,
                              const RoutePoint &start,
                              const RoutePoint &intc) const;
};

#endif
