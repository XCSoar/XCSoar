/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
ShowAnalysis8()
{
  dlgAnalysisShowModal(UIGlobals::GetMainWindow(),
                       UIGlobals::GetLook(),
                       CommonInterface::Full(), *glide_computer,
                       &airspace_database,
                       terrain, AnalysisPage::OLC);
}

static Widget *
LoadAnalysis8Panel(unsigned id)
{
  return new CallbackWidget(ShowAnalysis8);
}

static constexpr
InfoBoxPanel analysis8_infobox_panels[] = {
  { N_("Analysis"), LoadAnalysis8Panel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentOLC::GetDialogContent()
{
  return analysis8_infobox_panels;
}

void
InfoBoxContentOLC::Update(InfoBoxData &data)
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

   if (!settings_computer.contest.enable || !protected_task_manager) {
    data.SetInvalid();
    return;
  }

  int result_index =
    (settings_computer.contest.contest == Contest::OLC_LEAGUE) ? 0 : -1;

  const ContestResult& result_olc =
    CommonInterface::Calculated().contest_stats.GetResult(result_index);

  if (result_olc.score < 1) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(result_olc.distance);

  data.UnsafeFormatComment(_T("%.1f pts"), (double)result_olc.score);
}

const InfoBoxPanel *
InfoBoxContentOLCSpeed::GetDialogContent()
{
  return analysis8_infobox_panels;
}

void
InfoBoxContentOLCSpeed::Update(InfoBoxData &data)
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  if (!settings_computer.contest.enable || !protected_task_manager) {
    data.SetInvalid();
    return;
  }

  int result_index =
    (settings_computer.contest.contest == Contest::OLC_LEAGUE) ? 0 : -1;

  const ContestResult& result_olc =
    CommonInterface::Calculated().contest_stats.GetResult(result_index);

  if (result_olc.score < 1) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromTaskSpeed(result_olc.GetSpeed());

  data.UnsafeFormatComment(_T("%.1f pts"), (double)result_olc.score);
}
