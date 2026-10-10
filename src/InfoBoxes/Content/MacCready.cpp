// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/MacCready.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/MacCreadyEdit.hpp"
#include "InfoBoxes/Panel/MacCreadySetup.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Formatter/GlideRatioFormatter.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"

static void
SetVSpeed(InfoBoxData &data, double value) noexcept
{
  char buffer[32];
  FormatUserVerticalSpeed(value, buffer, false);
  data.SetValue(buffer[0] == '+' ? buffer + 1 : buffer);
  data.SetValueUnit(Units::current.vertical_speed_unit);
}

/*
 * Subpart callback function pointers
 */

static constexpr InfoBoxPanel panels[] = {
  { N_("Edit"), LoadMacCreadyEditPanel },
  { NC_("Menu", "Setup"), LoadMacCreadySetupPanel },
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

static void
SetSafetyMacCready(InfoBoxData &data, const DerivedInfo &calculated,
                   const ComputerSettings &settings) noexcept
{
  data.SetTitle(_("Safety MC"));
  data.SetValueColor(InfoBoxData::COLOR_ORANGE);

  const GlidePolar &safety = calculated.glide_polar_safety;
  const double mc = safety.IsValid()
    ? safety.GetMC()
    : settings.task.safety_mc;
  SetVSpeed(data, mc);

  /* Still-air glide ratio at the Safety MC speed, the same figure the
     Safety Factors picker shows beside the value. The comment has no
     unit slot, so the :1 is part of the text. */
  if (!safety.IsValid()) {
    data.SetCommentInvalid();
    return;
  }

  char ratio[16];
  FormatGlideRatio(ratio, sizeof(ratio), safety.GetBestLD());
  data.FmtComment("{}:1", ratio);
}

void
InfoBoxContentMacCready::Update(InfoBoxData &data) noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (calculated.common_stats.task_type == TaskType::ABORT) {
    SetSafetyMacCready(data, calculated, settings_computer);
    return;
  }

  const bool auto_mc = settings_computer.task.auto_mc;
  data.SetTitle(auto_mc ? _("MC AUTO") : _("MC MANUAL"));
  data.SetValueColor(auto_mc ? 2 : 3);

  SetVSpeed(data, settings_computer.polar.glide_polar_task.GetMC());

  data.SetCommentFromSpeed(calculated.common_stats.V_block, false);
}
