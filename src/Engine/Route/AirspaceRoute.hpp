// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  bool IsClear(const RouteLink &e) const noexcept override;
  void AddNearby(const RouteLink &e) noexcept override;
  bool CheckSecondary(const RouteLink &e) noexcept override;

private:
  void AddNearbyAirspace(const RouteAirspaceIntersection &inx,
                         const RouteLink &e) noexcept;

  [[gnu::pure]]
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
