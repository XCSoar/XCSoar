// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"

#include <cstdint>

enum class FixQuality: uint8_t {
  NO_FIX,
  GPS,
  DGPS,
  PPS,
  REAL_TIME_KINEMATIC,
  FLOAT_RTK,
  ESTIMATION,
  MANUAL_INPUT,
  SIMULATION,
};

/**
 * State of GPS fix
 */
struct GPSState
{
  static constexpr unsigned MAXSATELLITES = 12;

  //############
  //   Status
  //############

  /**
   * Fix quality
   */
  FixQuality fix_quality;
  Validity fix_quality_available;

  /**
   * Number of satellites used for gps fix.  -1 means "unknown".
   */
  int satellites_used;
  Validity satellites_used_available;

  /** GPS Satellite ids */
  int satellite_ids[MAXSATELLITES];
  Validity satellite_ids_available;

  /**
   * Horizontal dilution of precision.
   *
   * This attribute is only valid if NMEAInfo::location_available is
   * true.  A negative value means "unknown".
   */
  double hdop;

  /**
   * Position (3D) dilution of precision.
   *
   * This attribute is only valid if NMEAInfo::location_available is
   * true.  A negative value means "unknown".
   */
  double pdop;

  /**
   * Vertical dilution of precision.
   *
   * This attribute is only valid if NMEAInfo::location_available is
   * true.  A negative value means "unknown".
   */
  double vdop;

  /**
   * Is the fix real? (no replay, no simulator)
   */
  bool real;

  /** Is XCSoar in replay mode? */
  bool replay;

  /**
   * Did the simulator provide the GPS position?
   */
  bool simulator;

#if defined(ANDROID) || defined(__APPLE__)
  /**
   * Was this fix obtained from an internal GPS device for which
   * link timeout detection must be disabled? This is the case on
   * Android, iOS and OS X. On these platforms we get notifications
   * from the OS when the GPS gets disconnected.
   */
  bool nonexpiring_internal_gps;
#endif

  void Reset() noexcept;
  void Expire(TimeStamp now) noexcept;
};
