// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "IGCFix.hpp"
#include "NMEA/Info.hpp"
#include "Units/System.hpp"

#include <optional>

/**
 * Apply an IGC fix to replay-style NMEA data.
 *
 * @param time an absolute timestamp chosen by the caller
 * @param derived_ground_speed ground speed to use when the IGC GSP
 * extension is absent
 */
inline void
ApplyIGCFixToNMEA(const IGCFix &fix, TimeStamp time,
                  std::optional<double> derived_ground_speed,
                  NMEAInfo &basic) noexcept
{
  basic.clock = time;
  basic.alive.Update(basic.clock);
  basic.ProvideTime(time);

  basic.location = fix.location;
  basic.location_available.Update(basic.clock);

  if (fix.gps_altitude != 0) {
    basic.gps_altitude = fix.gps_altitude;
    basic.gps_altitude_available.Update(basic.clock);
  } else
    basic.gps_altitude_available.Clear();

  if (fix.pressure_altitude != 0) {
    basic.ProvidePressureAltitude(fix.pressure_altitude);
    basic.ProvideBaroAltitudeTrue(fix.pressure_altitude);
  } else {
    basic.pressure_altitude_available.Clear();
    basic.baro_altitude_available.Clear();
  }

  if (fix.enl >= 0) {
    basic.engine_noise_level = fix.enl;
    basic.engine_noise_level_available.Update(basic.clock);
  } else
    basic.engine_noise_level_available.Clear();

  if (fix.rpm >= 0) {
    basic.engine.revolutions_per_second = fix.rpm / 60.;
    basic.engine.revolutions_per_second_available.Update(basic.clock);
  } else
    basic.engine.revolutions_per_second_available.Clear();

  if (fix.trt >= 0) {
    basic.track = Angle::Degrees(fix.trt);
    basic.track_available.Update(basic.clock);
  } else
    basic.track_available.Clear();

  if (fix.gsp >= 0) {
    basic.ground_speed =
      Units::ToSysUnit(fix.gsp, Unit::KILOMETER_PER_HOUR);
    basic.ground_speed_available.Update(basic.clock);
  } else if (derived_ground_speed) {
    basic.ground_speed = *derived_ground_speed;
    basic.ground_speed_available.Update(basic.clock);
  } else
    basic.ground_speed_available.Clear();

  basic.airspeed_available.Clear();
  basic.airspeed_real = false;
  if (fix.ias >= 0) {
    const auto ias = Units::ToSysUnit(fix.ias,
                                      Unit::KILOMETER_PER_HOUR);
    if (fix.tas >= 0)
      basic.ProvideBothAirspeeds(
        ias, Units::ToSysUnit(fix.tas, Unit::KILOMETER_PER_HOUR));
    else
      basic.ProvideIndicatedAirspeedWithAltitude(ias,
                                                 basic.pressure_altitude);
  } else if (fix.tas >= 0)
    basic.ProvideTrueAirspeed(
      Units::ToSysUnit(fix.tas, Unit::KILOMETER_PER_HOUR));

  if (fix.siu >= 0) {
    basic.gps.satellites_used = fix.siu;
    basic.gps.satellites_used_available.Update(basic.clock);
  } else
    basic.gps.satellites_used_available.Clear();

  basic.gps.real = false;
  basic.gps.replay = true;
  basic.gps.simulator = false;
}
