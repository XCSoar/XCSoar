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
#include "Units/Units.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"

class SafetyFactorsConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
SafetyFactorsConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
SafetyFactorsConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
SafetyFactorsConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_SAFETYFACTORSCONFIGPANEL") :
                               _T("IDR_XML_SAFETYFACTORSCONFIGPANEL_L"));

  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  LoadFormProperty(form, _T("prpSafetyAltitudeArrival"), ugAltitude,
                   task_behaviour.safety_height_arrival);

  LoadFormProperty(form, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                   task_behaviour.route_planner.safety_height_terrain);

  static gcc_constexpr_data StaticEnumChoice abort_task_mode_list[] = {
    { atmSimple, N_("Simple") },
    { atmTask, N_("Task") },
    { atmHome, N_("Home") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpAbortTaskMode"), abort_task_mode_list,
                   task_behaviour.abort_task_mode);

  LoadFormProperty(form, _T("prpSafetyMacCready"), ugVerticalSpeed,
                   task_behaviour.safety_mc);

  LoadFormProperty(form, _T("prpRiskGamma"), task_behaviour.risk_gamma);
}

bool
SafetyFactorsConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  WndProperty *wp;
  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveFormProperty(form, _T("prpSafetyAltitudeArrival"), ugAltitude,
                              task_behaviour.safety_height_arrival,
                              szProfileSafetyAltitudeArrival);

  changed |= SaveFormProperty(form, _T("prpSafetyAltitudeTerrain"), ugAltitude,
                              task_behaviour.route_planner.safety_height_terrain,
                              szProfileSafetyAltitudeTerrain);

  changed |= SaveFormPropertyEnum(form, _T("prpAbortTaskMode"),
                                  szProfileAbortTaskMode,
                                  task_behaviour.abort_task_mode);

  wp = (WndProperty*)form.FindByName(_T("prpSafetyMacCready"));
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

  wp = (WndProperty*)form.FindByName(_T("prpRiskGamma"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    fixed val = df.GetAsFixed();
    if (task_behaviour.risk_gamma != val) {
      task_behaviour.risk_gamma = val;
      Profile::Set(szProfileRiskGamma, iround(task_behaviour.risk_gamma * 10));
      changed = true;
    }
  }

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateSafetyFactorsConfigPanel()
{
  return new SafetyFactorsConfigPanel();
}
