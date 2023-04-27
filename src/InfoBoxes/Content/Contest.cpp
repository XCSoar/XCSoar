// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Contest.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Language/Language.hpp"
#include "Widget/CallbackWidget.hpp"

#include <tchar.h>

static void
ShowAnalysis8() noexcept
{
  dlgAnalysisShowModal(UIGlobals::GetMainWindow(),
                       UIGlobals::GetLook(),
                       CommonInterface::Full(), *glide_computer,
                       &airspace_database,
                       terrain, AnalysisPage::CONTEST);
}

static std::unique_ptr<Widget>
LoadAnalysis8Panel([[maybe_unused]] unsigned id) noexcept
{
  return std::make_unique<CallbackWidget>(ShowAnalysis8);
}

static constexpr
InfoBoxPanel analysis8_infobox_panels[] = {
  { N_("Analysis"), LoadAnalysis8Panel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentContest::GetDialogContent() noexcept
{
  return analysis8_infobox_panels;
}

void
InfoBoxContentContest::Update(InfoBoxData &data) noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

   if (!settings_computer.contest.enable || !protected_task_manager) {
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

const InfoBoxPanel *
InfoBoxContentContestSpeed::GetDialogContent() noexcept
{
  return analysis8_infobox_panels;
}

void
InfoBoxContentContestSpeed::Update(InfoBoxData &data) noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  if (!settings_computer.contest.enable || !protected_task_manager) {
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
