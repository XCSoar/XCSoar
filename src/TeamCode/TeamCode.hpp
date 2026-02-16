// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#include <type_traits>

#include <tchar.h>

struct GeoPoint;
class Angle;

class TeamCode
{
  /** The team code */
  StaticString<10> code;

public:
  void Clear() {
    code.clear();
  }

  bool IsDefined() const {
    return !code.empty();
  }

  /**
   * Returns the current team code
   * @return Current team code
   */
  const char *GetCode() const {
    return code;
  }

  /**
   * Returns the position of the team member in respect to
   * the given reference waypoint
   * @param ref Reference waypoint
   * @return The team member's position
   */
  [[gnu::pure]]
  GeoPoint GetLocation(const GeoPoint ref) const;

  /**
   * Returns the bearing from the reference point to the team member
   * @return Bearing from the reference point to the team member
   */
  Angle GetBearing() const;

  /**
   * Returns the distance from the reference point to the team member
   * @return Distance from the reference point to the team member
   */
  double GetRange() const;

  /**
   * Updates the team code with the given parameters
   * @param bearing New bearing
   * @param range New range
   */
  void Update(Angle bearing, double range);

  /**
   * Updates the team code to the given code
   * @param _code The new team code
   */
  void Update(const char* _code);
};

static_assert(std::is_trivial<TeamCode>::value, "type is not trivial");
