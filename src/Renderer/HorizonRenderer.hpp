// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/MoreData.hpp"
#include "Units/Units.hpp"

#include <optional>

struct PixelRect;
struct HorizonLook;
struct AttitudeState;
class Canvas;

namespace HorizonRenderer {

/** User-unit speed and altitude, and the heading, for the full horizon. */
struct HorizonReadout {
  std::optional<double> speed;
  double speed_step = 10;
  std::optional<double> altitude;
  double altitude_step = 50;
  /** Heading in degrees, 0..360. */
  std::optional<double> course;
};

void Draw(Canvas &canvas, const PixelRect &rc,
          const HorizonLook &look,
          const AttitudeState &attitude,
          const HorizonReadout &readout = {});

/**
 * The references of an attitude indicator: indicated airspeed, the
 * navigation altitude and the heading, from a compass when one is
 * connected, otherwise the track corrected for the wind, and the track
 * itself while the wind is not known, as BasicComputer provides it.  Steps match the user unit.
 *
 * The horizon is an attitude indicator, not a navigation display, so
 * it shows what the aircraft does in the air: no ground speed in place
 * of a missing airspeed and no track; the InfoBoxes offer both.
 */
[[nodiscard]]
inline HorizonReadout
Readout(const MoreData &basic) noexcept
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

  if (basic.airspeed_available)
    readout.speed = Units::ToUserSpeed(basic.indicated_airspeed);

  if (basic.NavAltitudeAvailable())
    readout.altitude = Units::ToUserAltitude(basic.nav_altitude);

  /* where the nose points; the track differs from it by the drift
     angle */
  if (basic.attitude.heading_available)
    readout.course = basic.attitude.heading.AsBearing().Degrees();

  return readout;
}

} // namespace HorizonRenderer
