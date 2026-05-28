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

  /**
   * Like FindVertical(), but returns the next altitude above or below
   * the aircraft where the pilot would actually start being in a
   * warning state (warning-capable, non-acked, non-cleared airspace,
   * not itself covered by a clearance). Honours the airspace
   * clearance logic.
   */
  static NearestAirspace
  FindNextWarningEntry(const MoreData &basic,
                       const DerivedInfo &calculated,
                       const ProtectedAirspaceWarningManager *airspace_warnings,
                       const Airspaces &airspace_database) noexcept;
};
