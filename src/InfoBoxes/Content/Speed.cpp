// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Speed.hpp"
#include "BackendComponents.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Formatter/UserUnits.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include <stdlib.h>

void
InfoBoxContentSpeedGround::Update(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.ground_speed_available) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromSpeed(basic.ground_speed);

  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.head_wind_available) {
    data.SetCommentInvalid();
    return;
  }

  TCHAR buffer[16];
  FormatUserWindSpeed(-info.head_wind, buffer, true, false);
  data.SetComment(buffer);
}

bool
InfoBoxContentSpeedGround::HandleKey(const InfoBoxKeyCodes keycode) noexcept
{
  if (!CommonInterface::Basic().gps.simulator)
    return false;

  auto &device_blackboard = *backend_components->device_blackboard;
  const double step = Units::ToSysSpeed(10);
  const auto a5 = Angle::Degrees(5);

  switch (keycode) {
  case ibkUp:
    device_blackboard.SetSpeed(
        CommonInterface::Basic().ground_speed + step);
    return true;

  case ibkDown:
    device_blackboard.SetSpeed(fdim(CommonInterface::Basic().ground_speed,
                                     step));
    return true;

  case ibkLeft:
    device_blackboard.SetTrack(CommonInterface::Basic().track - a5);
    return true;

  case ibkRight:
    device_blackboard.SetTrack(CommonInterface::Basic().track + a5);
    return true;
  }

  return false;
}

void
UpdateInfoBoxSpeedIndicated(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.airspeed_available) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromSpeed(basic.indicated_airspeed, false);
}

void
UpdateInfoBoxSpeed(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.airspeed_available) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromSpeed(basic.true_airspeed, false);
}

void
UpdateInfoBoxSpeedMacCready(InfoBoxData &data) noexcept
{
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  data.SetValueFromSpeed(common_stats.V_block, false);
}

void
UpdateInfoBoxSpeedDolphin(InfoBoxData &data) noexcept
{
  // Set Value
  const DerivedInfo &calculated = CommonInterface::Calculated();
  data.SetValueFromSpeed(calculated.V_stf, false);

  // Set Comment
  if (CommonInterface::GetComputerSettings().features.block_stf_enabled)
    data.SetComment(_("BLOCK"));
  else
    data.SetComment(_("DOLPHIN"));

}
