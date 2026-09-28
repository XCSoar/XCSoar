// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlideComputerConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "UtilsSettings.hpp"

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

static constexpr StaticEnumChoice aver_eff_list[] = {
  { ae15seconds, "15 s", N_("Preferred period for paragliders.") },
  { ae30seconds, "30 s" },
  { ae60seconds, "60 s" },
  { ae90seconds, "90 s", N_("Preferred period for gliders.") },
  { ae2minutes, "2 min" },
  { ae3minutes, "3 min" },
  nullptr
};

/**
 * What the glide computer takes into account: the MacCready value,
 * the switch between cruise and circling, and what it reads its
 * height and its averages from.  The switches are spread over small
 * groups, so that the explanation of the one under the cursor stays
 * close to it.
 */
class GlideComputerConfigPanel final : public ConfigListPanel {
  TaskBehaviour::AutoMCMode auto_mc_mode;
  bool block_stf, predict_wind_drift;

  bool external_trigger_cruise;
  FloatDuration cruise_to_circling, circling_to_cruise;

  bool nav_baro_altitude;
  AverageEffTime average_eff_time;
  bool wave_assistant;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
GlideComputerConfigPanel::LoadSettings() noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  auto_mc_mode = settings_computer.task.auto_mc_mode;
  block_stf = settings_computer.features.block_stf_enabled;
  predict_wind_drift = settings_computer.task.glide.predict_wind_drift;

  external_trigger_cruise =
    settings_computer.circling.external_trigger_cruise_enabled;
  cruise_to_circling =
    settings_computer.circling.cruise_to_circling_mode_switch_threshold;
  circling_to_cruise =
    settings_computer.circling.circling_to_cruise_mode_switch_threshold;

  nav_baro_altitude = settings_computer.features.nav_baro_altitude_enabled;
  average_eff_time = settings_computer.average_eff_time;
  wave_assistant = settings_computer.wave.enabled;
}

void
GlideComputerConfigPanel::Fill() noexcept
{
  AddGroup(_("MacCready"));

  AddEnumItem(_("Auto MC mode"),
              _("This option defines which auto MacCready algorithm is used."),
              auto_mc_list, auto_mc_mode);

  if (IsExpert()) {
    AddToggleItem(_("Block speed to fly"),
                  _("If enabled, the command speed in cruise is set to the MacCready speed to fly in "
                    "no vertical air-mass movement. If disabled, the command speed in cruise is set "
                    "to the dolphin speed to fly, equivalent to the MacCready speed with vertical "
                    "air-mass movement."),
                  block_stf);

    AddToggleItem(_("Predict wind drift"),
                  _("Account for wind drift for the predicted circling duration. This reduces the arrival height for legs with head wind."),
                  predict_wind_drift);

    AddGroup(_("Circling"));

    AddToggleItem(_("Flap forces cruise"),
                  _("When Vega variometer is connected and this option is true, the positive flap "
                    "setting switches the flight mode between circling and cruise."),
                  external_trigger_cruise);

    AddDurationItem(_("Cruise/Circling period"),
                    _("How many seconds of turning before changing from cruise to circling mode."),
                    2, 30, 1, cruise_to_circling);

    AddDurationItem(_("Circling/Cruise period"),
                    _("How many seconds of flying straight before changing from circling to cruise mode."),
                    2, 30, 1, circling_to_cruise);
  }

  AddGroup();

  if (IsExpert()) {
    AddToggleItem(_("Nav. by baro altitude"),
                  _("When enabled and if connected to a barometric altimeter, barometric altitude is "
                    "used for all navigation functions. Otherwise GPS altitude is used."),
                  nav_baro_altitude);

    AddEnumItem(_("GR average period"),
                _("Here you can decide on how many seconds of flight this calculation must be done. "
                  "Normally for gliders a good value is 90-120 seconds, and for paragliders 15 seconds."),
                aver_eff_list, average_eff_time);
  }

  AddToggleItem(_("Wave assistant"),
                _("Enable detection and display of wave lift. "
                  "When enabled, wave sources are identified and shown on the map."),
                wave_assistant);
}

bool
GlideComputerConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  changed |= Profile::Update(ProfileKeys::AutoMcMode,
                             settings_computer.task.auto_mc_mode,
                             auto_mc_mode);

  FeaturesSettings &features = settings_computer.features;

  changed |= Profile::Update(ProfileKeys::BlockSTF,
                             features.block_stf_enabled, block_stf);

  changed |= Profile::Update(ProfileKeys::PredictWindDrift,
                             settings_computer.task.glide.predict_wind_drift,
                             predict_wind_drift);

  CirclingSettings &circling = settings_computer.circling;

  changed |= Profile::Update(ProfileKeys::EnableExternalTriggerCruise,
                             circling.external_trigger_cruise_enabled,
                             external_trigger_cruise);

  if (circling.cruise_to_circling_mode_switch_threshold !=
      cruise_to_circling) {
    circling.cruise_to_circling_mode_switch_threshold = cruise_to_circling;
    Profile::Set(ProfileKeys::CruiseToCirclingModeSwitchThreshold,
                 std::chrono::round<std::chrono::seconds>(cruise_to_circling));
    changed = true;
  }

  if (circling.circling_to_cruise_mode_switch_threshold !=
      circling_to_cruise) {
    circling.circling_to_cruise_mode_switch_threshold = circling_to_cruise;
    Profile::Set(ProfileKeys::CirclingToCruiseModeSwitchThreshold,
                 std::chrono::round<std::chrono::seconds>(circling_to_cruise));
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::EnableNavBaroAltitude,
                             features.nav_baro_altitude_enabled,
                             nav_baro_altitude);

  if (Profile::Update(ProfileKeys::AverEffTime,
                      settings_computer.average_eff_time,
                      average_eff_time))
    require_restart = changed = true;

  changed |= Profile::Update(ProfileKeys::WaveAssistant,
                             settings_computer.wave.enabled, wave_assistant);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateGlideComputerConfigPanel()
{
  return std::make_unique<GlideComputerConfigPanel>();
}
