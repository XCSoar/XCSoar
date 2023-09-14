// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct GeoPoint;
class OZBoundary;

/**
 * Abstract class giving properties of a zone which is used to measure
 * transitions in/out of and interior/exterior checking.
 */
class ObservationZone
{
public:
  enum class Shape : uint8_t {
    LINE,
    CYLINDER,
    MAT_CYLINDER,
    SECTOR,
    FAI_SECTOR,
    DAEC_KEYHOLE,
    BGAFIXEDCOURSE,
    BGAENHANCEDOPTION,
    BGA_START,
    ANNULAR_SECTOR,
    SYMMETRIC_QUADRANT,
    CUSTOM_KEYHOLE,
  };

private:
  const Shape shape;

  const bool can_start_through_top;

protected:
  constexpr ObservationZone(Shape _shape, bool _can_start_through_top) noexcept
    :shape(_shape), can_start_through_top(_can_start_through_top) {}

public:
  virtual ~ObservationZone() noexcept = default;

  ObservationZone(const ObservationZone &) = delete;
  ObservationZone &operator=(const ObservationZone &) = delete;

  constexpr Shape GetShape() const noexcept {
    return shape;
  }

  /** 
   * Check whether observer is within OZ
   *
   * @return True if reference point is inside sector
   */
  [[gnu::pure]]
  virtual bool IsInSector(const GeoPoint &location) const noexcept = 0;

  /**
   * If zone when used for start can trigger task start via vertical exit
   *
   * @return True if zone type can have a valid start through top
   */
  constexpr bool CanStartThroughTop() const noexcept {
    return can_start_through_top;
  }

  /**
   * Check transition constraints
   *
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if constraints are satisfied
   */
  [[gnu::pure]]
  virtual bool TransitionConstraint(const GeoPoint &location,
                                    const GeoPoint &last_location) const noexcept = 0;

  /**
   * Return an unordered list of boundary points for evaluation by the
   * class #TaskDijkstra.  It shall contain all corners, all middle
   * points of straight lines and approximations for arcs.
   *
   * Even though there is no specific ordering, the most important
   * points should be at the front of the list if possible, to avoid
   * rounding problems as much as possible; for example, the
   * LineSectorZone has its center point at the beginning, so
   * TaskDijkstra chooses the outer points only if there is a
   * measurable advantage.
   */
  [[gnu::pure]]
  virtual OZBoundary GetBoundary() const noexcept = 0;

  /**
   * Distance reduction for scoring when outside this OZ
   * (used because FAI cylinders, for example, have their
   *  radius subtracted from scored distances)
   *
   * @return Distance (m) to subtract from score
   */
  [[gnu::pure]]
  virtual double ScoreAdjustment() const noexcept = 0;
};
