// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Gauge/HorizonSettings.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"

#include <optional>

struct PixelRect;
struct HorizonLook;
struct AttitudeState;
class Canvas;

namespace HorizonRenderer {

/**
 * User-unit speed and altitude and a bearing for the full horizon,
 * with the short titles that say which ones they are.  The titles are
 * ready to draw: the ICAO abbreviations stay as published, only
 * "Baro" is ordinary language and translated.
 */
struct HorizonReadout {
  std::optional<double> speed;
  double speed_step = 10;
  const char *speed_title = nullptr;

  std::optional<double> altitude;
  double altitude_step = 50;
  const char *altitude_title = nullptr;

  /** Degrees, 0..360. */
  std::optional<double> course;
  const char *course_title = nullptr;
};

void Draw(Canvas &canvas, const PixelRect &rc,
          const HorizonLook &look,
          const AttitudeState &attitude,
          const HorizonReadout &readout = {});

/**
 * The values the pilot chose in #HorizonSettings.  By default these
 * are the references of an attitude indicator: indicated airspeed,
 * the heading, from a compass when one is connected, otherwise the
 * track corrected for the wind and the track itself while the wind is
 * not known, as BasicComputer provides it, and the height above the
 * terrain.  A value that is not available stays
 * empty and is shown as "---"; another one is never put in its place.
 * Steps match the user unit.
 */
[[nodiscard]]
inline HorizonReadout
Readout(const MoreData &basic, const TerrainInfo &terrain,
        const HorizonSettings &settings) noexcept
{
  HorizonReadout readout;
  const Unit speed_unit = Units::GetUserSpeedUnit();
  readout.speed_step = speed_unit == Unit::METER_PER_SECOND ? 5 : 10;

  const Unit altitude_unit = Units::GetUserAltitudeUnit();
  if (altitude_unit == Unit::FEET)
    readout.altitude_step = 100;
  else if (altitude_unit == Unit::FLIGHT_LEVEL)
    readout.altitude_step = 1;
  else
    readout.altitude_step = 50;

  switch (settings.speed) {
  case HorizonSettings::Speed::INDICATED:
    readout.speed_title = "IAS";
    if (basic.airspeed_available)
      readout.speed = Units::ToUserSpeed(basic.indicated_airspeed);
    break;

  case HorizonSettings::Speed::TRUE_AIRSPEED:
    readout.speed_title = "TAS";
    if (basic.airspeed_available)
      readout.speed = Units::ToUserSpeed(basic.true_airspeed);
    break;

  case HorizonSettings::Speed::GROUND:
    readout.speed_title = "GS";
    if (basic.ground_speed_available)
      readout.speed = Units::ToUserSpeed(basic.ground_speed);
    break;
  }

  switch (settings.altitude) {
  case HorizonSettings::Altitude::ABOVE_GROUND:
    readout.altitude_title = "AGL";
    if (terrain.altitude_agl_valid)
      readout.altitude = Units::ToUserAltitude(terrain.altitude_agl);
    break;

  case HorizonSettings::Altitude::BARO:
    readout.altitude_title = _("Baro");
    if (basic.baro_altitude_available)
      readout.altitude = Units::ToUserAltitude(basic.baro_altitude);
    break;

  case HorizonSettings::Altitude::GPS:
    readout.altitude_title = "GPS";
    if (basic.gps_altitude_available)
      readout.altitude = Units::ToUserAltitude(basic.gps_altitude);
    break;
  }

  switch (settings.course) {
  case HorizonSettings::Course::HEADING:
    /* where the nose points; the track differs from it by the drift
       angle */
    readout.course_title = "HDG";
    if (basic.attitude.heading_available)
      readout.course = basic.attitude.heading.AsBearing().Degrees();
    break;

  case HorizonSettings::Course::TRACK:
    readout.course_title = "TRK";
    if (basic.track_available)
      readout.course = basic.track.AsBearing().Degrees();
    break;
  }

  return readout;
}

} // namespace HorizonRenderer
