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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "GlideComputerConfigPanel.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"

enum ControlIndex {
  AutoMcMode,
  BlockSTF,
  EnableNavBaroAltitude,
  EnableExternalTriggerCruise,
  AverEffTime,
  PredictWindDrift,
  WaveAssistant,
};

class GlideComputerConfigPanel final : public RowFormWidget {
public:
  GlideComputerConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
};

void
GlideComputerConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  RowFormWidget::Prepare(parent, rc);

  static constexpr StaticEnumChoice auto_mc_list[] = {
    { (unsigned)TaskBehaviour::AutoMCMode::FINALGLIDE, N_("Final glide"),
      N_("Adjusts MC for fastest arrival.  For OLC sprint tasks, the MacCready is adjusted in "
          "order to cover the greatest distance in the remaining time and reach the finish height.") },
    { (unsigned)TaskBehaviour::AutoMCMode::CLIMBAVERAGE, N_("Trending average climb"),
      N_("Sets MC to the trending average climb rate based on all climbs.") },
    { (unsigned)TaskBehaviour::AutoMCMode::BOTH, N_("Both"),
      N_("Uses trending average during task, then fastest arrival when in final glide mode.") },
    { 0 }
  };

  AddEnum(_("Auto MC mode"),
          _("This option defines which auto MacCready algorithm is used."),
          auto_mc_list, (unsigned)settings_computer.task.auto_mc_mode);

  AddBoolean(_("Block speed to fly"),
             _("If enabled, the command speed in cruise is set to the MacCready speed to fly in "
                 "no vertical air-mass movement. If disabled, the command speed in cruise is set "
                 "to the dolphin speed to fly, equivalent to the MacCready speed with vertical "
                 "air-mass movement."),
             settings_computer.features.block_stf_enabled);
  SetExpertRow(BlockSTF);

  AddBoolean(_("Nav. by baro altitude"),
             _("When enabled and if connected to a barometric altimeter, barometric altitude is "
                 "used for all navigation functions. Otherwise GPS altitude is used."),
             settings_computer.features.nav_baro_altitude_enabled);
  SetExpertRow(EnableNavBaroAltitude);

  AddBoolean(_("Flap forces cruise"),
             _("When Vega variometer is connected and this option is true, the positive flap "
                 "setting switches the flight mode between circling and cruise."),
             settings_computer.circling.external_trigger_cruise_enabled);
  SetExpertRow(EnableExternalTriggerCruise);

  static constexpr StaticEnumChoice aver_eff_list[] = {
    { ae15seconds, _T("15 s"), N_("Preferred period for paragliders.") },
    { ae30seconds, _T("30 s") },
    { ae60seconds, _T("60 s") },
    { ae90seconds, _T("90 s"), N_("Preferred period for gliders.") },
    { ae2minutes, _T("2 min") },
    { ae3minutes, _T("3 min") },
    { 0 }
  };

  AddEnum(_("GR average period"),
          _("Here you can decide on how many seconds of flight this calculation must be done. "
              "Normally for gliders a good value is 90-120 seconds, and for paragliders 15 seconds."),
          aver_eff_list, settings_computer.average_eff_time);
  SetExpertRow(AverEffTime);

  AddBoolean(_("Predict wind drift"),
             _("Account for wind drift for the predicted circling duration. This reduces the arrival height for legs with head wind."),
             task_behaviour.glide.predict_wind_drift);
  SetExpertRow(PredictWindDrift);

  AddBoolean(_("Wave assistant"), nullptr,
             settings_computer.wave.enabled);
}

bool
GlideComputerConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveValueEnum(AutoMcMode, ProfileKeys::AutoMcMode, settings_computer.task.auto_mc_mode);

  changed |= SaveValue(BlockSTF, ProfileKeys::BlockSTF,
                       settings_computer.features.block_stf_enabled);

  changed |= SaveValue(EnableNavBaroAltitude, ProfileKeys::EnableNavBaroAltitude,
                       settings_computer.features.nav_baro_altitude_enabled);

  changed |= SaveValue(EnableExternalTriggerCruise, ProfileKeys::EnableExternalTriggerCruise,
                       settings_computer.circling.external_trigger_cruise_enabled);

  if (SaveValueEnum(AverEffTime, ProfileKeys::AverEffTime,
                    settings_computer.average_eff_time))
    require_restart = changed = true;

  changed |= SaveValue(PredictWindDrift, ProfileKeys::PredictWindDrift,
                       task_behaviour.glide.predict_wind_drift);

  changed |= SaveValue(WaveAssistant, ProfileKeys::WaveAssistant,
                       settings_computer.wave.enabled);

  _changed |= changed;

  return true;
}

Widget *
CreateGlideComputerConfigPanel()
{
  return new GlideComputerConfigPanel();
}
