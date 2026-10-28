// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Direction.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/SpeedSimulator.hpp"
#include "Simulator.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static constexpr InfoBoxPanel track_panels[] = {
  { N_("Simulator"), LoadSpeedSimulatorPanel },
  { nullptr, nullptr }
};

void
InfoBoxContentTrack::Update(InfoBoxData &data) noexcept
{
  if (!CommonInterface::Basic().track_available) {
    data.SetInvalid();
    return;
  }
  data.SetValue(CommonInterface::Basic().track);
}

const InfoBoxPanel *
InfoBoxContentTrack::GetDialogContent() noexcept
{
  if (!is_simulator())
    return nullptr;

  return track_panels;
}
