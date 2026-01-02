// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Time.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Math/SunEphemeris.hpp"

#include <tchar.h>

void
UpdateInfoBoxTimeLocal(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  if (!basic.time_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  FormatLocalTimeHHMM(data.value.buffer(), basic.time, settings.utc_offset);

  // Set Comment

  SunEphemeris::Result sun =
    SunEphemeris::CalcSunTimes(basic.location, basic.date_time_utc,
                               settings.utc_offset);

  const unsigned sunsethours = (int)sun.time_of_sunset;
  const unsigned sunsetmins = (int)((sun.time_of_sunset - sunsethours) * 60);

  data.FmtComment(_T("{:02}:{:02}"), sunsethours, sunsetmins);
}

void
UpdateInfoBoxTimeUTC(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!basic.time_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  const BrokenDateTime t = basic.date_time_utc;
  data.FmtValue(_T("{:02}:{:02}"), t.hour, t.minute);

  // Set Comment
  data.FmtComment(_T("{:02}"), t.second);
}

void
UpdateInfoBoxTimeFlight(InfoBoxData &data) noexcept
{
  const FlyingState &flight = CommonInterface::Calculated().flight;

  if (flight.flight_time.count() <= 0) {
    data.SetInvalid();
    return;
  }
  data.SetValueFromTimeTwoLines(flight.flight_time);
}
