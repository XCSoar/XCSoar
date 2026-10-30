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

/** User-unit speed and altitude for the full-horizon tapes. */
struct HorizonReadout {
  std::optional<double> speed;
  double speed_step = 10;
  std::optional<double> altitude;
  double altitude_step = 50;
};

void Draw(Canvas &canvas, const PixelRect &rc,
          const HorizonLook &look,
          const AttitudeState &attitude,
          const HorizonReadout &readout = {});

/**
 * Indicated airspeed when it is available, otherwise ground speed.
 * Altitude is the navigation altitude.  Steps match the user unit.
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
  else if (basic.ground_speed_available)
    readout.speed = Units::ToUserSpeed(basic.ground_speed);

  if (basic.NavAltitudeAvailable())
    readout.altitude = Units::ToUserAltitude(basic.nav_altitude);

  return readout;
}

} // namespace HorizonRenderer
