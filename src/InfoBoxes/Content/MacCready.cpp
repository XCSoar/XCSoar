// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/MacCready.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/MacCreadyEdit.hpp"
#include "InfoBoxes/Panel/MacCreadySetup.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"

#include <tchar.h>

static void
SetVSpeed(InfoBoxData &data, double value) noexcept
{
  TCHAR buffer[32];
  FormatUserVerticalSpeed(value, buffer, false);
  data.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  data.SetValueUnit(Units::current.vertical_speed_unit);
}

/*
 * Subpart callback function pointers
 */

static constexpr InfoBoxPanel panels[] = {
  { N_("Edit"), LoadMacCreadyEditPanel },
  { N_("Setup"), LoadMacCreadySetupPanel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentMacCready::GetDialogContent() noexcept
{
  return panels;
}

/*
 * Subpart normal operations
 */

void
InfoBoxContentMacCready::Update(InfoBoxData &data) noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  data.SetTitle(settings_computer.task.auto_mc ? _("MC AUTO") : _("MC MANUAL"));
  data.SetValueColor(settings_computer.task.auto_mc ? (2) : (3));

  SetVSpeed(data, settings_computer.polar.glide_polar_task.GetMC());

  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  data.SetCommentFromSpeed(common_stats.V_block, false);
}
