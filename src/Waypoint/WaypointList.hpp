// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoVector.hpp"
#include "Engine/Waypoint/Ptr.hpp"

#include <vector>

/**
 * Structure to hold Waypoint sorting information
 */
struct WaypointListItem
{
  WaypointPtr waypoint;

private:
  /** From observer to waypoint */
  mutable GeoVector vec = GeoVector::Invalid();

public:
  template<typename W>
  explicit WaypointListItem(W &&_waypoint) noexcept
    :waypoint(std::forward<W>(_waypoint)) {}

  void ResetVector() noexcept;

  [[gnu::pure]]
  const GeoVector &GetVector(const GeoPoint &location) const noexcept;
};

class WaypointList: public std::vector<WaypointListItem>
{
public:
  void SortByName() noexcept;
  void SortByDistance(const GeoPoint &location) noexcept;
  void MakeUnique() noexcept;
};
