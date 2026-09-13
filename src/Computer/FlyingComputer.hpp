// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StateClock.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/DeltaTime.hpp"
#include "time/Stamp.hpp"

#include <optional>

struct NMEAInfo;
struct DerivedInfo;
struct AircraftState;
struct FlyingState;

/**
 * Detect takeoff and landing.
 *
 * GPS alone cannot distinguish stationary ground operation from a cold-start
 * level wave flight, nor terrain contact from low-level ridge flight within
 * the 0--50 m AGL landing band.  Airspeed or sustained climb evidence is
 * needed to resolve those cases.  For an uphill foot launch, the beginning
 * of sustained launch evidence may also precede actual separation from the
 * terrain, and therefore so may the detected takeoff time.
 */
class FlyingComputer {
  class ClimbEvidence {
    StateClock<20, 5> clock;
    std::optional<double> previous_altitude;

  public:
    void Reset(std::optional<double> baseline={}) noexcept;
    bool IsActive() const noexcept { return clock.IsDefined(); }
    bool Update(FloatDuration dt, double altitude) noexcept;
  };

  struct LaunchEvidence {
    TimeStamp time;
    GeoPoint location;
    double altitude;
  };

  class SlowLaunchDetector {
    ClimbEvidence climb;
    std::optional<LaunchEvidence> candidate;

  public:
    void Reset(std::optional<double> baseline={}) noexcept;

    std::optional<LaunchEvidence>
    Update(bool eligible, FloatDuration dt, TimeStamp time,
           const GeoPoint &location,
           std::optional<double> altitude) noexcept;
  };

  DeltaTime delta_time;

  /**
   * Tracks the duration the aircraft has been stationary.
   */
  StateClock<60, 5> stationary_clock;

  /**
   * Tracks the duration the aircraft has been moving.
   */
  StateClock<30, 5> moving_clock;

  SlowLaunchDetector slow_launch;

  /** Rising evidence used only to reject a landing at low speed. */
  ClimbEvidence landing_climb;

  /**
   * If the aircraft is currenly assumed to be moving, then this
   * denotes the initial moving time stamp.  This gets reset to a
   * negative value when the aircraft is stationary for a certain
   * amount of time.
   */
  TimeStamp moving_since;

  /**
   * If the aircraft is currently assumed to be moving, then this
   * denotes the location when moving started initially.  This
   * attribute is only valid if #moving_since is non-negative.
   */
  GeoPoint moving_at;

  /**
   * If the aircraft is currently assumed to be moving, then this
   * denotes the altitude when moving started initially.  This
   * attribute is only valid if #moving_since is non-negative.
   */
  double moving_altitude;

  TimeStamp stationary_since;
  GeoPoint stationary_at;

  /**
   * If the aircraft is in powered flight, then this denotes
   * the initial powered time stamp. If the aircraft is unpowered
   * this is set to a negative value.
   */
  TimeStamp powered_since;
  GeoPoint powered_at;

  /**
   * If the aircraft is in unpowered flight, then this denotes
   * the initial unpowered time stamp. If the aircraft is powered
   * this is set to a negative value.
   */
  TimeStamp unpowered_since;
  GeoPoint unpowered_at;

  TimeStamp sinking_since;

  GeoPoint sinking_location;

  double sinking_altitude;

  /**
   * The last altitude when the aircraft was supposed to be on the
   * ground.  This is usually the elevation of the take-off airfield.
   *
   * A negative value means unknown.  Sorry for these few airfields
   * that are below MSL ...
   */
  double last_ground_altitude;

public:
  void Reset();

  void Compute(double takeoff_speed,
               const NMEAInfo &basic,
               const DerivedInfo &calculated,
               FlyingState &flying);

  void Compute(double takeoff_speed,
               const AircraftState &state, FloatDuration dt,
               FlyingState &flying);

  /**
   * Finish the landing detection.  If landing has not been detected,
   * but the aircraft has not been moving, this force-detects the
   * landing now.  Call at the end of a replay.
   */
  void Finish(FlyingState &flying, TimeStamp time) noexcept;

protected:
  void CheckRelease(FlyingState &state, TimeStamp time, const GeoPoint &location,
                    double altitude);

  /**
   * Check for powered flight.
   */
  void CheckPowered(FloatDuration dt, const NMEAInfo &basic,
                    FlyingState &flying) noexcept;

  void Check(FlyingState &state, TimeStamp time) noexcept;

  static void Takeoff(FlyingState &state, TimeStamp time,
                      const GeoPoint &location, double altitude) noexcept;

  void ConfirmSlowTakeoff(FlyingState &state,
                          const LaunchEvidence &evidence) noexcept;

  /**
   * Update flying state when moving 
   *
   * @param time Time the aircraft is moving
   */
  void Moving(FlyingState &state, TimeStamp time, FloatDuration dt,
              const GeoPoint &location, double altitude) noexcept;

  /**
   * Update flying state when stationary 
   *
   * @param time Time the aircraft is stationary
   * @param on_ground Whether the aircraft is known to be on the ground
   */
  void Stationary(FlyingState &state, TimeStamp time, FloatDuration dt,
                  const GeoPoint &location);
};
