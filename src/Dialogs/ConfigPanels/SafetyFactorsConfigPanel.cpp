/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Form/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  ArrivalHeight,
  TerrainHeight,
  AlternateMode,
  PolarDegradation,
  SafetyMC,
  RiskFactor,
};

class SafetyFactorsConfigPanel : public RowFormWidget {
public:
  SafetyFactorsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

void
SafetyFactorsConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  AddFloat(_("Arrival height"),
           _("The height above terrain that the glider should arrive at for a safe landing."),
           _T("%.0f %s"), _T("%.0f"),
           fixed_zero, fixed(10000), fixed(100), false,
           UnitGroup::ALTITUDE, task_behaviour.safety_height_arrival);

  AddFloat(_("Terrain height"),
           _("The height above terrain that the glider must clear during final glide."),
           _T("%.0f %s"), _T("%.0f"),
           fixed_zero, fixed(10000), fixed(100), false,
           UnitGroup::ALTITUDE, task_behaviour.route_planner.safety_height_terrain);

  static constexpr StaticEnumChoice abort_task_mode_list[] = {
    { (unsigned)AbortTaskMode::SIMPLE, N_("Simple") },
    { (unsigned)AbortTaskMode::TASK, N_("Task") },
    { (unsigned)AbortTaskMode::HOME, N_("Home") },
    { 0 }
  };

  AddEnum(_("Alternates mode"),
          _("Determines sorting of alternates in the alternates dialog and in abort mode:\n[Simple] The alternates will only be sorted by waypoint type (airport/outlanding field) and arrival height.\n[Task] The sorting will also take the current task direction into account.\n[Home] The sorting will try to find landing options in the current direction to the configured home waypoint."),
          abort_task_mode_list, (unsigned)task_behaviour.abort_task_mode);

  AddFloat(_("Polar degradation"), /* xgettext:no-c-format */
           _("A permanent polar degradation. "
             "0% means no degradation, "
             "50% indicates the glider's sink rate is doubled."),
           _T("%.0f %%"), _T("%.0f"),
           fixed(0), fixed(50), fixed_one, false,
           (fixed_one - settings_computer.polar.degradation_factor) * 100);
  SetExpertRow(PolarDegradation);

  AddFloat(_("Safety MC"),
           _("The MacCready setting used, when safety MC is enabled for reach calculations, in task abort mode and for determining arrival altitude at airfields."),
           _T("%.1f %s"), _T("%.1f"),
           fixed_zero, fixed_ten, fixed(0.1), false,
           UnitGroup::VERTICAL_SPEED, task_behaviour.safety_mc);
  SetExpertRow(SafetyMC);

  AddFloat(_("STF risk factor"),
           _("The STF risk factor reduces the MacCready setting used to calculate speed to fly as the glider gets low, in order to compensate for risk. Set to 0.0 for no compensation, 1.0 scales MC linearly with current height (with reference to height of the maximum climb). If considered, 0.3 is recommended."),
           _T("%.1f %s"), _T("%.1f"),
           fixed_zero, fixed_one, fixed(0.1), false,
           task_behaviour.risk_gamma);
  SetExpertRow(RiskFactor);
}

bool
SafetyFactorsConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveValue(ArrivalHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::SafetyAltitudeArrival,
                       task_behaviour.safety_height_arrival);

  changed |= SaveValue(TerrainHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::SafetyAltitudeTerrain,
                       task_behaviour.route_planner.safety_height_terrain);

  changed |= SaveValueEnum(AlternateMode, ProfileKeys::AbortTaskMode,
                           task_behaviour.abort_task_mode);

  fixed degradation = (fixed_one - settings_computer.polar.degradation_factor) * 100;
  if (SaveValue(PolarDegradation, degradation)) {
    settings_computer.polar.SetDegradationFactor(fixed_one - degradation / 100);
    Profile::Set(ProfileKeys::PolarDegradation,
                 settings_computer.polar.degradation_factor);
    if (protected_task_manager != NULL)
      protected_task_manager->SetGlidePolar(settings_computer.polar.glide_polar_task);
    changed = true;
  }

  if (SaveValue(SafetyMC, UnitGroup::VERTICAL_SPEED, task_behaviour.safety_mc)) {
    Profile::Set(ProfileKeys::SafetyMacCready,
                 iround(task_behaviour.safety_mc * 10));
    changed = true;
  }

  if (SaveValue(RiskFactor, task_behaviour.risk_gamma)) {
    Profile::Set(ProfileKeys::RiskGamma,
                 iround(task_behaviour.risk_gamma * 10));
    changed = true;
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
