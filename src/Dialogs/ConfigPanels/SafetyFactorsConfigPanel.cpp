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

#include "SafetyFactorsConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;


void
SafetyFactorsConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  LoadFormProperty(*wf, _T("prpSafetyAltitudeArrival"), ugAltitude,
                   task_behaviour.safety_height_arrival);

  LoadFormProperty(*wf, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                   task_behaviour.route_planner.safety_height_terrain);

  static const StaticEnumChoice abort_task_mode_list[] = {
    { atmSimple, N_("Simple") },
    { atmTask, N_("Task") },
    { atmHome, N_("Home") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpAbortTaskMode"), abort_task_mode_list,
                   task_behaviour.abort_task_mode);

  LoadFormProperty(*wf, _T("prpSafetyMacCready"), ugVerticalSpeed,
                   task_behaviour.safety_mc);

  LoadFormProperty(*wf, _T("prpRiskGamma"), task_behaviour.risk_gamma);
}


bool
SafetyFactorsConfigPanel::Save()
{
  bool changed = false;
  WndProperty *wp;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveFormProperty(*wf, _T("prpSafetyAltitudeArrival"), ugAltitude,
                              task_behaviour.safety_height_arrival,
                              szProfileSafetyAltitudeArrival);

  changed |= SaveFormProperty(*wf, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                              task_behaviour.route_planner.safety_height_terrain,
                              szProfileSafetyAltitudeTerrain);

  changed |= SaveFormPropertyEnum(*wf, _T("prpAbortTaskMode"),
                                  szProfileAbortTaskMode,
                                  task_behaviour.abort_task_mode);

  wp = (WndProperty*)wf->FindByName(_T("prpSafetyMacCready"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    fixed val = Units::ToSysVSpeed(df.GetAsFixed());
    if (task_behaviour.safety_mc != val) {
      task_behaviour.safety_mc = val;
      Profile::Set(szProfileSafetyMacCready,
                    iround(task_behaviour.safety_mc*10));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRiskGamma"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    fixed val = df.GetAsFixed();
    if (task_behaviour.risk_gamma != val) {
      task_behaviour.risk_gamma = val;
      Profile::Set(szProfileRiskGamma, iround(task_behaviour.risk_gamma * 10));
      changed = true;
    }
  }

  return changed;
}
