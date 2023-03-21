// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Direction.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"

void
InfoBoxContentTrack::Update(InfoBoxData &data) noexcept
{
  if (!CommonInterface::Basic().track_available) {
    data.SetInvalid();
    return;
  }
  data.SetValue(CommonInterface::Basic().track);
}

bool
InfoBoxContentTrack::HandleKey(const InfoBoxKeyCodes keycode) noexcept
{
  if (!CommonInterface::Basic().gps.simulator)
    return false;

  const Angle a5 = Angle::Degrees(5);
  switch (keycode) {
  case ibkUp:
  case ibkRight:
    device_blackboard->SetTrack(
        CommonInterface::Basic().track + a5);
    return true;

  case ibkDown:
  case ibkLeft:
    device_blackboard->SetTrack(
        CommonInterface::Basic().track - a5);
    return true;
  }

  return false;
}
