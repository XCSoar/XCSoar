// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Time.hpp"
#include "InfoBoxes/Data.hpp"
#include "Dialogs/Dialogs.h"
#include "Interface.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/LocalTimeFormatter.hpp"

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
  data.FmtComment("{:02}", basic.date_time_utc.second);
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
  data.FmtValue("{:02}:{:02}", t.hour, t.minute);

  // Set Comment
  data.FmtComment("{:02}", t.second);
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

void
InfoBoxContentTimeFlight::Update(InfoBoxData &data) noexcept
{
  UpdateInfoBoxTimeFlight(data);
}

bool
InfoBoxContentTimeFlight::HandleClick() noexcept
{
  dlgStatusShowModal(4);
  return true;
}

void
InfoBoxContentTimeLocal::Update(InfoBoxData &data) noexcept
{
  UpdateInfoBoxTimeLocal(data);
}

bool
InfoBoxContentTimeLocal::HandleClick() noexcept
{
  dlgStatusShowModal(4);
  return true;
}

void
InfoBoxContentTimeUTC::Update(InfoBoxData &data) noexcept
{
  UpdateInfoBoxTimeUTC(data);
}

bool
InfoBoxContentTimeUTC::HandleClick() noexcept
{
  dlgStatusShowModal(4);
  return true;
}
