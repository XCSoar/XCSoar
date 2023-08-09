// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "IntermediatePoint.hpp"
#include "Math/Angle.hpp"

struct RangeAndRadial {
  /**
   * Thesigned range [-1,1] from near point on perimeter through
   * center to far side of the oz perimeter.
   */
  double range;

  /**
   * The bearing of the target.
   */
  Angle radial;

  static constexpr RangeAndRadial Zero() noexcept {
    return {0., Angle::Zero()};
  }
};

/**
 * An AATPoint is an abstract IntermediatePoint,
 * can manage a target within the observation zone
 * but does not yet have an observation zone.
 *
 * \todo
 * - Elevation may vary with target shift
 */
class AATPoint final : public IntermediateTaskPoint {
  /** Location of target within OZ */
  GeoPoint target_location;
  /** Whether target can float */
  bool target_locked;

public:
  /**
   * Constructor.  Initialises to unlocked target, target is
   * initially set to origin.
   *
   * @param _oz Observation zone for this task point
   * @param wp Waypoint origin of turnpoint
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Partially-initialised object
   */
  AATPoint(std::unique_ptr<ObservationZonePoint> &&_oz,
           WaypointPtr &&wp,
           const TaskBehaviour &tb) noexcept
    :IntermediateTaskPoint(TaskPointType::AAT, std::move(_oz), std::move(wp),
                           tb, true),
     target_location(GetLocation()),
     target_locked(false)
  {
  }

  /**
   * Lock/unlock the target from automatic shifts
   *
   * @param do_lock Whether to lock the target
   */
  void LockTarget(bool do_lock) noexcept {
    target_locked = do_lock;
  }

  const GeoPoint &GetTarget() const noexcept {
    return target_location;
  }

  /**
   * Set target location explicitly
   *
   * @param loc Location of new target
   * @param override_lock If false, won't set the target if it is locked
   */
  void SetTarget(const GeoPoint &loc, const bool override_lock=false) noexcept;

  /**
   * Set target location from a signed range & radial as bearing
   * referenced from the previous target
   * used by dlgTarget
   */
  void SetTarget(RangeAndRadial rar, const FlatProjection &projection) noexcept;

  /**
   * returns position of the target in signed range & radial as
   * bearing referenced from the previous target
   * used by dlgTarget.
   *
   * @param &range returns signed range [-1,1] from near point on
   * perimeter through center to far side of the oz perimeter
   *
   * @param &radial returns the bearing in degrees of
   * the target
   */
  [[gnu::pure]]
  RangeAndRadial GetTargetRangeRadial(double old_range=0) const noexcept;

  /**
   * Accessor to get target location
   *
   * @return Target location
   */
  const GeoPoint &GetTargetLocation() const noexcept {
    return target_location;
  }

  /**
   * Test whether aircraft has travelled close to isoline of target
   * within threshold
   *
   * @param state Aircraft state
   * @param threshold Threshold for distance comparision (m)
   *
   * @return True if double leg distance from state is within
   * threshold of target
   */
  [[gnu::pure]]
  bool IsCloseToTarget(const AircraftState& state,
                       double threshold=0) const noexcept;

  /**
   * Set target to parametric value between min and max locations.
   * Targets are only moved for current or after taskpoints, unless
   * force_if_current is true.
   *
   * @param p Parametric range (0:1) to set target
   * @param force_if_current If current active, force range move
   * (otherwise ignored)
   *
   * @return True if target was moved
   */
  bool SetRange(double p, bool force_if_current) noexcept;

  /**
   * If this TaskPoint has the capability to adjust the
   * target/range, this indicates whether it is locked from
   * being updated by the optimizer
   * Only valid for TaskPoints where has_target() returns true
   *
   * @return True if target is locked
   *    or False if target is unlocked or tp has no target
   */
  bool IsTargetLocked() const noexcept {
    return target_locked;
  }

private:
  /**
   * Check whether target needs to be moved and if so, to
   * perform the move.  Makes no assumption as to whether the aircraft
   * within or outside the observation zone.
   *
   * @param state Current aircraft state
   * @param known_outside set to true if known to be outside the sector
   *
   * @return True if target was moved
   */
  bool CheckTarget(const AircraftState& state, bool known_outside) noexcept;

  /**
   * Check whether target needs to be moved and if so, to
   * perform the move, where aircraft is inside the observation zone
   * of the current active taskpoint.
   *
   * @param state Current aircraft state
   *
   * @return True if target was moved
   */
  bool CheckTargetInside(const AircraftState &state) noexcept;

  /**
   * Check whether target needs to be moved and if so, to
   * perform the move, where aircraft is outside the observation zone
   * of the current active taskpoint.
   *
   * @param state Current aircraft state
   *
   * @return True if target was moved
   */
  bool CheckTargetOutside(const AircraftState& state) noexcept;

public:
  /* virtual methods from class TaskPoint */
  const GeoPoint &GetLocationRemaining() const noexcept override;

  /* virtual methods from class ObservationZoneClient */
  double ScoreAdjustment() const noexcept override {
    return 0;
  }

  /* virtual methods from class OrderedTaskPoint */
  bool Equals(const OrderedTaskPoint &other) const noexcept override;
  bool UpdateSampleNear(const AircraftState &state,
                        const FlatProjection &projection) noexcept override;
  bool UpdateSampleFar(const AircraftState &state,
                       const FlatProjection &projection) noexcept override;
};
