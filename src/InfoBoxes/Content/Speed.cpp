// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Speed.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/SpeedSimulator.hpp"
#include "Simulator.hpp"
#include "Computer/STF.hpp"
#include "Formatter/UserUnits.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"

static constexpr InfoBoxPanel speed_ground_panels[] = {
  { N_("Simulator"), LoadSpeedSimulatorPanel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentSpeedGround::GetDialogContent() noexcept
{
  if (!is_simulator())
    return nullptr;

  return speed_ground_panels;
}

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

  char buffer[16];
  FormatUserWindSpeed(-info.head_wind, buffer, true, false);
  data.SetComment(buffer);
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
  data.SetValueColor(basic.airspeed_real ? 0 : 2);
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
  data.SetValueColor(basic.airspeed_real ? 0 : 2);
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
  const auto stf = GetSTFSpeed(CommonInterface::Basic(),
                               CommonInterface::Calculated());
  if (!stf) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromSpeed(*stf, false);

  // Set Comment
  if (CommonInterface::GetComputerSettings().features.block_stf_enabled)
    data.SetComment(_("BLOCK"));
  else
    data.SetComment(_("DOLPHIN"));

}
