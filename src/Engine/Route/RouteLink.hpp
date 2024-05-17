// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Point.hpp"

class FlatProjection;

/**
 * Class used for primitive 3d navigation links.
 *
 * For route planning, these routes are defined in reverse time order,
 * that is, the first link is the destination (later in time), second link
 * is the origin (earlier in time).
 */
struct RouteLinkBase {
  /** Destination location */
  RoutePoint first;
  /** Origin location */
  RoutePoint second;

  constexpr RouteLinkBase(const RoutePoint _dest,
                          const RoutePoint _origin) noexcept
    :first(_dest), second(_origin) {}

  constexpr bool operator==(const RouteLinkBase &) const noexcept = default;

  /**
   * Return 2d Distance of this link
   * @return distance in FlatGeoPoint units
   */
  [[gnu::pure]]
  unsigned Distance() const noexcept {
    return first.Distance(second);
  }

  /**
   * Test whether this link is too short to be considered
   * for path planning (based on manhattan distance).
   *
   * @return true if this link is short
   */
  [[gnu::pure]]
  bool IsShort() const noexcept;

  /**
   * Calculate the dot product of this link with another.
   * Can be used to test projection of one link in direction of
   * another.
   *
   * @param o second object in dot product
   *
   * @return dot product of this object with second object
   */
  constexpr int DotProduct(const RouteLinkBase& o) const noexcept {
    return o.GetDelta().DotProduct(GetDelta());
  }

  /**
   * Calculate the cross product of this link with another.
   * Can be used to test orthogonality of two links.
   *
   * @param o second object in cross product
   *
   * @return cross product of this object with second object
   */
  constexpr int CrossProduct(const RouteLinkBase &o) const noexcept {
    return o.GetDelta().CrossProduct(GetDelta());
  }

private:
  constexpr FlatGeoPoint GetDelta() const noexcept {
    return FlatGeoPoint(second) - FlatGeoPoint(first);
  }
};

/**
 * Extension of RouteLinkBase to store additional data
 * on actual distance, reciprocal of distance, and direction indices
 * for fast lookup of performance via RoutePolars.
 */
struct RouteLink: public RouteLinkBase {
public:
  /** Distance (m) */
  double d;
  /** Reciprocal of Distance (1/m) */
  double inv_d;
  /** Direction index to be used for RoutePolar lookups */
  unsigned polar_index;

  RouteLink(const RouteLinkBase &link, const FlatProjection &proj) noexcept;
  RouteLink(const RoutePoint &_first, const RoutePoint &_second,
            const FlatProjection &proj) noexcept;

private:
  void CalcSpeedups(const FlatProjection &proj) noexcept;
};
