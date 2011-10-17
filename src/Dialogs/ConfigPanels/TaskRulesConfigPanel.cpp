/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "TaskRulesConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;


void
TaskRulesConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  LoadFormProperty(*wf, _T("prpStartMaxSpeed"), ugHorizontalSpeed,
                   task_behaviour.ordered_defaults.start_max_speed);

  LoadFormProperty(*wf, _T("prpStartMaxSpeedMargin"), ugHorizontalSpeed,
                   task_behaviour.start_max_speed_margin);

  static gcc_constexpr_data StaticEnumChoice start_max_height_ref_list[] = {
    { hrAGL, N_("AGL"), N_("Reference AGL for start maximum height rule (above start point)") },
    { hrMSL, N_("MSL"), N_("Reference MSL for start maximum height rule (above sea level)") },
    { 0 }
  };
  LoadFormProperty(*wf, _T("prpStartHeightRef"), start_max_height_ref_list,
                   task_behaviour.ordered_defaults.start_max_height_ref);

  LoadFormProperty(*wf, _T("prpStartMaxHeight"), ugAltitude,
                   task_behaviour.ordered_defaults.start_max_height);

  LoadFormProperty(*wf, _T("prpStartMaxHeightMargin"), ugAltitude,
                   task_behaviour.start_max_height_margin);

  static gcc_constexpr_data StaticEnumChoice finish_min_height_ref_list[] = {
    { hrAGL, N_("AGL"), N_("Reference AGL for finish minimum height rule (above finish point)") },
    { hrMSL, N_("MSL"), N_("Reference MSL for finish minimum height rule (above sea level)") },
    { 0 }
  };
  LoadFormProperty(*wf, _T("prpFinishHeightRef"), finish_min_height_ref_list,
                   task_behaviour.ordered_defaults.finish_min_height_ref);

  LoadFormProperty(*wf, _T("prpFinishMinHeight"), ugAltitude,
                   task_behaviour.ordered_defaults.finish_min_height);

  const StaticEnumChoice contests_list[] = {
    { OLC_FAI, ContestToString(OLC_FAI) },
    { OLC_Classic, ContestToString(OLC_Classic) },
    { OLC_League, ContestToString(OLC_League) },
    { OLC_Plus, ContestToString(OLC_Plus) },
    { OLC_XContest, ContestToString(OLC_XContest) },
    { OLC_DHVXC, ContestToString(OLC_DHVXC) },
    { OLC_SISAT, ContestToString(OLC_SISAT) },
    { 0 }
  };
  LoadFormProperty(*wf, _T("prpContests"), contests_list,
                   task_behaviour.contest);
}


bool
TaskRulesConfigPanel::Save()
{
  bool changed = false;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();
  TaskBehaviour &task_behaviour = settings_computer.task;

  OrderedTaskBehaviour &otb = task_behaviour.ordered_defaults;
  changed |= SaveFormProperty(*wf, _T("prpStartMaxSpeed"),
                              ugHorizontalSpeed,
                              otb.start_max_speed,
                              szProfileStartMaxSpeed);

  changed |= SaveFormProperty(*wf, _T("prpStartMaxSpeedMargin"),
                              ugHorizontalSpeed,
                              task_behaviour.start_max_speed_margin,
                              szProfileStartMaxSpeedMargin);

  changed |= SaveFormProperty(*wf, _T("prpStartMaxHeight"), ugAltitude,
                              otb.start_max_height,
                              szProfileStartMaxHeight);

  changed |= SaveFormProperty(*wf, _T("prpStartMaxHeightMargin"), ugAltitude,
                              task_behaviour.start_max_height_margin,
                              szProfileStartMaxHeightMargin);

  changed |= SaveFormPropertyEnum(*wf, _T("prpStartHeightRef"),
                                  szProfileStartHeightRef,
                                  otb.start_max_height_ref);

  changed |= SaveFormProperty(*wf, _T("prpFinishMinHeight"), ugAltitude,
                              otb.finish_min_height,
                              szProfileFinishMinHeight);

  changed |= SaveFormPropertyEnum(*wf, _T("prpFinishHeightRef"),
                                  szProfileFinishHeightRef,
                                  otb.finish_min_height_ref);

  changed |= SaveFormPropertyEnum(*wf, _T("prpContests"), szProfileOLCRules,
                                  task_behaviour.contest);

  return changed;
}
