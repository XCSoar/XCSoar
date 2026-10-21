// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "GlideComputerConfigPanel.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

using namespace std::chrono;

enum ControlIndex {
  AutoMcMode,
  WaveAssistant,
  SPACER_GLIDE,
  BlockSTF,
  PredictWindDrift,
  EnableNavBaroAltitude,
  AverEffTime,
  SPACER_MODE,
  EnableExternalTriggerCruise,
  CruiseToCirclingModeSwitchThreshold,
  CirclingToCruiseModeSwitchThreshold,
};

class GlideComputerConfigPanel final : public RowFormWidget {
public:
  GlideComputerConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
GlideComputerConfigPanel::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  RowFormWidget::Prepare(parent, rc);

  static constexpr StaticEnumChoice auto_mc_list[] = {
    { TaskBehaviour::AutoMCMode::FINALGLIDE, N_("Final glide"),
      N_("Adjusts MC for the fastest arrival. For contest sprint tasks, the MacCready is adjusted in "
          "order to cover the greatest distance in the remaining time and reach the finish height.") },
    { TaskBehaviour::AutoMCMode::CLIMBAVERAGE, N_("Trending average climb"),
      N_("Sets MC to the trending average climb rate based on all climbs.") },
    { TaskBehaviour::AutoMCMode::BOTH, N_("Both"),
      N_("Uses trending average during task, then fastest arrival when in final glide mode.") },
    nullptr
  };

  AddEnum(_("Auto MC mode"),
          _("This option defines which auto MacCready algorithm is used."),
          auto_mc_list, (unsigned)settings_computer.task.auto_mc_mode);

  AddBoolean(_("Wave assistant"),
             _("Enable detection and display of wave lift. "
               "When enabled, wave sources are identified and shown on the map."),
             settings_computer.wave.enabled);

  AddSpacer();
  SetExpertRow(SPACER_GLIDE);

  AddBoolean(_("Block speed to fly"),
             _("If enabled, the command speed in cruise is set to the MacCready speed to fly in "
                 "no vertical air-mass movement. If disabled, the command speed in cruise is set "
                 "to the dolphin speed to fly, equivalent to the MacCready speed with vertical "
                 "air-mass movement."),
             settings_computer.features.block_stf_enabled);
  SetExpertRow(BlockSTF);

  AddBoolean(_("Predict wind drift"),
             _("Account for wind drift for the predicted circling duration. This reduces the arrival height for legs with head wind."),
             task_behaviour.glide.predict_wind_drift);
  SetExpertRow(PredictWindDrift);

  AddBoolean(_("Nav. by baro altitude"),
             _("When enabled and if connected to a barometric altimeter, barometric altitude is "
                 "used for all navigation functions. Otherwise GPS altitude is used."),
             settings_computer.features.nav_baro_altitude_enabled);
  SetExpertRow(EnableNavBaroAltitude);

  static constexpr StaticEnumChoice aver_eff_list[] = {
    { ae15seconds, "15 s", N_("Preferred period for paragliders.") },
    { ae30seconds, "30 s" },
    { ae60seconds, "60 s" },
    { ae90seconds, "90 s", N_("Preferred period for gliders.") },
    { ae2minutes, "2 min" },
    { ae3minutes, "3 min" },
    nullptr
  };

  AddEnum(_("GR average period"),
          _("Here you can decide on how many seconds of flight this calculation must be done. "
              "Normally for gliders a good value is 90-120 seconds, and for paragliders 15 seconds."),
          aver_eff_list, settings_computer.average_eff_time);
  SetExpertRow(AverEffTime);

  AddSpacer();
  SetExpertRow(SPACER_MODE);

  static constexpr StaticEnumChoice speed_command_list[] = {
    { CirclingSettings::SpeedCommandSwitch::OFF, N_("Off"),
      N_("XCSoar decides circling from the turn. "
         "The variometer report is not used.") },
    { CirclingSettings::SpeedCommandSwitch::FLIGHT_MODE,
      N_("Flight mode"),
      N_("While flying, the variometer's cruise/climb report is the "
         "flight mode. "
         "Cruise ends the climb and restarts the average. "
         "Climb starts a new thermal. "
         "Final glide can still appear in cruise. "
         "On a Vega this is the circling output.") },
    { CirclingSettings::SpeedCommandSwitch::DISPLAY_MODE,
      N_("Display mode"),
      N_("The page changes and the flight mode does not. "
         "Cruise shows the cruise page, including while turning, "
         "and hides final glide. "
         "Climb shows the vario page. "
         "On a Vega this is the speed-command switch.") },
    nullptr
  };

  AddEnum(_("Speed command switch"),
          _("How XCSoar uses the variometer's cruise/climb report."),
          speed_command_list,
          unsigned(settings_computer.circling.speed_command_switch));
  SetExpertRow(EnableExternalTriggerCruise);

  AddDuration(_("Cruise/Circling period"),
              _("How many seconds of turning before changing from cruise to circling mode."),
              seconds{2}, seconds{30}, seconds{1},
              settings_computer.circling.cruise_to_circling_mode_switch_threshold);
  SetExpertRow(CruiseToCirclingModeSwitchThreshold);

  AddDuration(_("Circling/Cruise period"),
              _("How many seconds of flying straight before changing from circling to cruise mode."),
              seconds{2}, seconds{30}, seconds{1},
              settings_computer.circling.circling_to_cruise_mode_switch_threshold);
  SetExpertRow(CirclingToCruiseModeSwitchThreshold);
}

bool
GlideComputerConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveValueEnum(AutoMcMode, ProfileKeys::AutoMcMode, settings_computer.task.auto_mc_mode);

  changed |= SaveValue(WaveAssistant, ProfileKeys::WaveAssistant,
                       settings_computer.wave.enabled);

  changed |= SaveValue(BlockSTF, ProfileKeys::BlockSTF,
                       settings_computer.features.block_stf_enabled);

  changed |= SaveValue(PredictWindDrift, ProfileKeys::PredictWindDrift,
                       task_behaviour.glide.predict_wind_drift);

  changed |= SaveValue(EnableNavBaroAltitude, ProfileKeys::EnableNavBaroAltitude,
                       settings_computer.features.nav_baro_altitude_enabled);

  changed |= SaveValueEnum(AverEffTime, ProfileKeys::AverEffTime,
                           settings_computer.average_eff_time);

  changed |= SaveValueEnum(EnableExternalTriggerCruise,
                           ProfileKeys::SpeedCommandSwitch,
                           settings_computer.circling.speed_command_switch);

  changed |= SaveValue(CruiseToCirclingModeSwitchThreshold, ProfileKeys::CruiseToCirclingModeSwitchThreshold,
                       settings_computer.circling.cruise_to_circling_mode_switch_threshold);

  changed |= SaveValue(CirclingToCruiseModeSwitchThreshold, ProfileKeys::CirclingToCruiseModeSwitchThreshold,
                       settings_computer.circling.circling_to_cruise_mode_switch_threshold);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateGlideComputerConfigPanel()
{
  return std::make_unique<GlideComputerConfigPanel>();
}
