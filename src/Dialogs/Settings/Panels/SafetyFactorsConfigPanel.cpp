// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SafetyFactorsConfigPanel.hpp"
#include "ActionInterface.hpp"
#include "ConfigListPanel.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Formatter/GlideRatioFormatter.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Units/Units.hpp"
#include "util/StaticString.hxx"
#include "util/StringFormat.hpp"

#include <algorithm>

static constexpr StaticEnumChoice abort_task_mode_list[] = {
  { AbortTaskMode::SIMPLE, N_("Simple"),
    N_("Reachable airfields are listed first (nearest at top), then "
       "outlanding sites (nearest at top).") },
  { AbortTaskMode::TASK, N_("Task"),
    N_("Reachable airfields are listed first (smallest detour to the "
       "active turnpoint at top), then outlanding sites.") },
  { AbortTaskMode::HOME, N_("Home"),
    N_("Reachable airfields are listed first (smallest detour toward "
       "home at top), then outlanding sites.") },
  nullptr
};

/** the largest safety MacCready value, in m/s */
static constexpr double SAFETY_MC_MAX = 10;

/**
 * Format the best glide ratio in still air at the given MacCready
 * setting.  The polar must be valid.
 *
 * @param user_mc the MacCready setting in the user's vertical speed unit
 */
static void
FormatGlideRatioAtMC(char *buffer, size_t size,
                     const GlidePolar &polar, double user_mc) noexcept
{
  GlidePolar mc_polar = polar;
  /* Same point the safety reach polar flies: past maximum cruise. */
  mc_polar.SetMCLimitedByCruise(false);
  mc_polar.SetMC(Units::ToSysVSpeed(user_mc));

  char ratio[16];
  FormatGlideRatio(ratio, sizeof(ratio), mc_polar.GetBestLD());
  StringFormat(buffer, size, "%s:1", ratio);
}

/**
 * The margins which the glide computer keeps: the heights, what it
 * expects of the glider, and the MacCready values it falls back to.
 */
class SafetyFactorsConfigPanel final : public ConfigListPanel {
  double arrival_height, terrain_height;
  AbortTaskMode abort_task_mode;

  /** the polar degradation as a percentage */
  int degradation;

  bool auto_bugs;
  double safety_mc;
  double task_mc;

  /** Help of the safety MC row, including the active polar. */
  StaticString<1024> safety_mc_help;

  /** the STF risk factor in tenths */
  int risk_gamma;

  bool turn_back_marker;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
SafetyFactorsConfigPanel::LoadSettings() noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  arrival_height = task_behaviour.safety_height_arrival;
  terrain_height = task_behaviour.route_planner.safety_height_terrain;
  abort_task_mode = task_behaviour.abort_task_mode;
  degradation =
    iround((1 - settings_computer.polar.degradation_factor) * 100);
  auto_bugs = settings_computer.polar.auto_bugs;
  safety_mc = task_behaviour.safety_mc;
  task_mc = task_behaviour.task_mc;
  risk_gamma = iround(task_behaviour.risk_gamma * 10);
  turn_back_marker = task_behaviour.turn_back_marker_enabled;
}

void
SafetyFactorsConfigPanel::Fill() noexcept
{
  AddGroup();

  AddAltitudeItem(_("Arrival height"),
                  _("The height above terrain that the glider should arrive at for a safe landing."),
                  0, 2000, 10, arrival_height);

  AddAltitudeItem(_("Terrain height"),
                  _("The height above terrain that the glider must clear during final glide."),
                  0, 1000, 10, terrain_height);

  AddEnumItem(_("Alternates mode"),
              _("Determines sorting of alternates in the alternates dialog "
                "and in abort mode."),
              abort_task_mode_list, abort_task_mode);

  AddVerticalSpeedItem(_("Startup Task MC"),
                       _("The MacCready used at startup for speed-to-fly "
                         "and task calculations. Changing it here also "
                         "sets the live MacCready. Terrain reach, landable "
                         "colours, abort and the Alternates list follow "
                         "Reach polar, not this value unless Reach polar "
                         "is Task MC."),
                       0, TaskBehaviour::TASK_MC_MAX, task_mc);

  if (IsExpert()) {
    AddGroup();

    AddPercentItem(_("Polar degradation"), /* xgettext:no-c-format */
                   _("A permanent polar degradation. "
                     "0% means no degradation, "
                     "50% indicates the glider's sink rate is doubled."),
                   0, 50, 1, degradation);

    AddToggleItem(_("Auto bugs"), /* xgettext:no-c-format */
                  _("If enabled, adds 1% to the bugs setting after each full hour while flying."),
                  auto_bugs);

    const ComputerSettings &settings =
      CommonInterface::GetComputerSettings();

    safety_mc_help = _("The MacCready used for terrain reach, landable "
                       "colours, abort and the Alternates list when Reach "
                       "polar is Safety MC. Alternate InfoBoxes and "
                       "waypoint Alt. diff. Safety MC always use this "
                       "value. Speed-to-fly is not affected. Higher values "
                       "treat fewer fields as reachable.");
    safety_mc_help += "\n\n";
    if (!settings.polar.glide_polar_task.IsValid()) {
      safety_mc_help += _("No glide polar is configured, so no glide ratio "
                          "can be shown. Select a plane first.");
    } else {
      safety_mc_help += _("The glide ratio next to each value is the best "
                          "glide ratio in still air at that MacCready "
                          "setting, including bugs, ballast and polar "
                          "degradation.");
      if (!settings.plane.polar_name.empty()) {
        safety_mc_help += "\n";
        safety_mc_help.AppendFormat(_("Active polar: %s"),
                                    settings.plane.polar_name.c_str());
      }
    }

    AddVerticalSpeedItem(_("Safety MC"), safety_mc_help.c_str(),
                         0, SAFETY_MC_MAX, safety_mc);

    GlidePolar ratio_polar = settings.polar.glide_polar_task;
    StaticString<16> glide_ratio;
    if (!ratio_polar.IsValid())
      glide_ratio = _("Unknown");
    else {
      const double factor =
        1 - std::clamp(degradation, 0, 50) / 100.;
      ratio_polar.SetBugs(factor * settings.polar.bugs);
      FormatGlideRatioAtMC(glide_ratio.data(), glide_ratio.capacity(),
                           ratio_polar,
                           Units::ToUserVSpeed(safety_mc));
    }

    AddItem(_("Safety MC GR"), {.value = glide_ratio.c_str()});

    StaticString<8> risk;
    risk.Format("%.1f", risk_gamma / 10.);

    AddItem(_("STF risk factor"), [this](){
      if (PickNumber(_("STF risk factor"),
                     _("The STF risk factor reduces the MacCready setting used to calculate speed to fly as the glider gets low, in order to compensate for risk. Set to 0.0 for no compensation, 1.0 scales MC linearly with current height (with reference to height of the maximum climb). If considered, 0.3 is recommended."),
                     0, 10, 1, risk_gamma,
                     [](StaticString<32> &s, int v){
                       s.Format("%.1f", v / 10.);
                     }))
        Refresh();
    }, {.value = risk.c_str(), .chevron = true});
  }

  AddGroup();

  AddToggleItem(C_("Setting", "Turn back marker"),
                _("Show a green triangle on the map along the current track "
                  "indicating the furthest point from which the active task "
                  "waypoint or Goto target can still be reached with the "
                  "current altitude and conditions. "
                  "The triangle is only shown during cruise when the target "
                  "is reachable."),
                turn_back_marker);
}

bool
SafetyFactorsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= Profile::Update(ProfileKeys::SafetyAltitudeArrival,
                             task_behaviour.safety_height_arrival,
                             arrival_height);

  RoutePlannerConfig &route_planner = task_behaviour.route_planner;
  changed |= Profile::Update(ProfileKeys::SafetyAltitudeTerrain,
                             route_planner.safety_height_terrain,
                             terrain_height);

  changed |= Profile::Update(ProfileKeys::AbortTaskMode,
                             task_behaviour.abort_task_mode, abort_task_mode);

  PolarSettings &polar = settings_computer.polar;
  if (iround((1 - polar.degradation_factor) * 100) != degradation) {
    polar.SetDegradationFactor(1 - degradation / 100.);
    Profile::Set(ProfileKeys::PolarDegradation, polar.degradation_factor);
    backend_components->SetTaskPolar(polar);
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::AutoBugs, polar.auto_bugs,
                             auto_bugs);

  if (task_behaviour.task_mc != task_mc) {
    task_behaviour.task_mc = task_mc;
    Profile::Set(ProfileKeys::TaskMacCready, iround(task_mc * 10));
    ActionInterface::SetMacCready(task_mc);
    changed = true;
  }

  if (task_behaviour.safety_mc != safety_mc) {
    task_behaviour.safety_mc = safety_mc;
    Profile::Set(ProfileKeys::SafetyMacCready, iround(safety_mc * 10));
    changed = true;
  }

  if (iround(task_behaviour.risk_gamma * 10) != risk_gamma) {
    task_behaviour.risk_gamma = risk_gamma / 10.;
    Profile::Set(ProfileKeys::RiskGamma, risk_gamma);
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::TurnBackMarkerEnabled,
                             task_behaviour.turn_back_marker_enabled,
                             turn_back_marker);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSafetyFactorsConfigPanel()
{
  return std::make_unique<SafetyFactorsConfigPanel>();
}
