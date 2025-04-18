// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct MoreData;
struct DerivedInfo;
class Airspaces;
class AbstractAirspace;
class ProtectedAirspaceWarningManager;

class NearestAirspace {

public:
  const AbstractAirspace *airspace;

  /**
   * The horizontal or vertical distance [m], depending on which
   * function filled this object.
   */
  double distance;

  NearestAirspace() noexcept:airspace(nullptr) {}
  NearestAirspace(const AbstractAirspace &_airspace, double _distance) noexcept
    :airspace(&_airspace), distance(_distance) {}

  bool IsDefined() const noexcept {
    return airspace != nullptr;
  }

  [[gnu::pure]]
  static NearestAirspace
  FindHorizontal(const MoreData &basic,
                 const ProtectedAirspaceWarningManager *airspace_warnings,
                 const Airspaces &airspace_database) noexcept;

  static NearestAirspace
  FindVertical(const MoreData &basic,
               const DerivedInfo &calculated,
               const ProtectedAirspaceWarningManager *airspace_warnings,
               const Airspaces &airspace_database) noexcept;
};
