// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Contest.hpp"
#include "ShowAnalysis.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Language/Language.hpp"
#include "BackendComponents.hpp"

#include <tchar.h>

bool
InfoBoxContentContest::HandleClick() noexcept
{
  return ShowAnalysis(AnalysisPage::CONTEST);
}

void
InfoBoxContentContest::Update(InfoBoxData &data) noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

   if (!settings_computer.contest.enable ||
       !backend_components || !backend_components->protected_task_manager) {
    data.SetInvalid();
    return;
  }

  int result_index =
    (settings_computer.contest.contest == Contest::OLC_LEAGUE) ? 0 : -1;

  const ContestResult& result_contest =
    CommonInterface::Calculated().contest_stats.GetResult(result_index);

  if (result_contest.score < 1) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(result_contest.distance);

  data.FmtComment(_T("{:.1f} pts"), result_contest.score);
}

bool
InfoBoxContentContestSpeed::HandleClick() noexcept
{
  return ShowAnalysis(AnalysisPage::CONTEST);
}

void
InfoBoxContentContestSpeed::Update(InfoBoxData &data) noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  if (!settings_computer.contest.enable ||
      !backend_components || !backend_components->protected_task_manager) {
    data.SetInvalid();
    return;
  }

  int result_index =
    (settings_computer.contest.contest == Contest::OLC_LEAGUE) ? 0 : -1;

  const ContestResult& result_contest =
    CommonInterface::Calculated().contest_stats.GetResult(result_index);

  if (result_contest.score < 1) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromTaskSpeed(result_contest.GetSpeed());

  data.FmtComment(_T("{:.1f} pts"), result_contest.score);
}
