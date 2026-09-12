// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SafetyFactorsConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Interface.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/GlideRatioFormatter.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "util/StaticString.hxx"

#include <algorithm>

enum ControlIndex {
  ArrivalHeight,
  TerrainHeight,
  AlternateMode,
  PolarDegradation,
  AutoBugs,
  SafetyMC,
  SafetyMCGlideRatio,
  RiskFactor,
  TurnBackMarker,
};

/**
 * Format the best glide ratio in still air at the given MacCready
 * setting, i.e. the glide ratio achieved when flying at the speed to
 * fly for it.  The polar must be valid.
 *
 * @param user_mc the MacCready setting in the user's vertical speed unit
 */
static void
FormatGlideRatioAtMC(char *buffer, size_t size,
                     const GlidePolar &polar, double user_mc) noexcept
{
  GlidePolar mc_polar = polar;
  mc_polar.SetMC(Units::ToSysVSpeed(user_mc));
  FormatGlideRatio(buffer, size, mc_polar.GetBestLD());
}

/**
 * The safety MC data field, which shows the glide ratio resulting from
 * each MacCready value next to it in the combo list.
 */
class SafetyMCDataField final : public DataFieldFloat {
  /**
   * The polar the glide ratios are calculated with; invalid when no
   * polar is configured, in which case no glide ratio is shown.
   */
  GlidePolar polar;

public:
  using DataFieldFloat::DataFieldFloat;

  void SetPolar(const GlidePolar &_polar) noexcept {
    polar = _polar;
  }

protected:
  /* virtual methods from class DataFieldFloat */
  void AppendComboValue(ComboList &combo_list,
                        double value) const noexcept override;
};

void
SafetyMCDataField::AppendComboValue(ComboList &combo_list,
                                    double value) const noexcept
{
  if (!polar.IsValid()) {
    DataFieldFloat::AppendComboValue(combo_list, value);
    return;
  }

  StaticString<decltype(edit_format)::capacity()> edit;
  edit.Format(edit_format, value);

  StaticString<decltype(display_format)::capacity()> display;
  display.Format(display_format, value, GetUnits());

  char glide_ratio[16];
  FormatGlideRatioAtMC(glide_ratio, sizeof(glide_ratio), polar, value);

  const unsigned i = combo_list.Append(edit, display);
  combo_list.SetAnnotation(i, glide_ratio);
}

class SafetyFactorsConfigPanel final
  : public RowFormWidget, DataFieldListener {
  /**
   * The help text of the safety MC row, which explains the glide ratio
   * and names the active polar.  #WndProperty does not copy the help
   * text, so it is kept here; the fixed capacity keeps the pointer
   * handed to it valid even when the text is rebuilt.
   */
  StaticString<1024> safety_mc_help;

public:
  SafetyFactorsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /**
   * @return the polar the safety MC glide ratio is calculated with,
   * including the polar degradation currently entered in this dialog;
   * invalid when no polar is configured
   */
  GlidePolar GetGlideRatioPolar() const noexcept;

  void BuildSafetyMCHelp() noexcept;
  void UpdateSafetyMCGlideRatio() noexcept;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
SafetyFactorsConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(SafetyMC, df) || IsDataField(PolarDegradation, df))
    UpdateSafetyMCGlideRatio();
}

GlidePolar
SafetyFactorsConfigPanel::GetGlideRatioPolar() const noexcept
{
  const PolarSettings &settings = CommonInterface::GetComputerSettings().polar;
  GlidePolar polar = settings.glide_polar_task;

  if (!polar.IsValid())
    return polar;

  /* preview the degradation value currently entered in this dialog,
     applied to the bugs setting like PolarSettings::SetDegradationFactor();
     the clamp is redundant for values entered here, but keeps SetBugs()
     inside its assertion range */
  const double degradation_factor =
    1 - std::clamp(GetValueFloat(PolarDegradation), 0., 50.) / 100;
  polar.SetBugs(degradation_factor * settings.bugs);

  return polar;
}

void
SafetyFactorsConfigPanel::BuildSafetyMCHelp() noexcept
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  safety_mc_help = _("The MacCready setting used, when safety MC is enabled for reach calculations, in task abort mode and for determining arrival altitude at airfields.");
  safety_mc_help += "\n\n";

  if (!settings.polar.glide_polar_task.IsValid()) {
    safety_mc_help += _("No glide polar is configured, so no glide ratio "
                        "can be shown. Select a plane first.");
    return;
  }

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

void
SafetyFactorsConfigPanel::UpdateSafetyMCGlideRatio() noexcept
{
  const GlidePolar polar = GetGlideRatioPolar();

  auto &df = (SafetyMCDataField &)GetDataField(SafetyMC);
  df.SetPolar(polar);

  if (!polar.IsValid()) {
    SetText(SafetyMCGlideRatio, _("Unknown"));
    return;
  }

  char buffer[16];
  FormatGlideRatioAtMC(buffer, sizeof(buffer), polar, df.GetValue());
  SetText(SafetyMCGlideRatio, buffer);
}

void
SafetyFactorsConfigPanel::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  AddFloat(_("Arrival height"),
           _("The height above terrain that the glider should arrive at for a safe landing."),
           "%.0f %s", "%.0f",
           0, 2000, 10, false,
           UnitGroup::ALTITUDE, task_behaviour.safety_height_arrival);

  AddFloat(_("Terrain height"),
           _("The height above terrain that the glider must clear during final glide."),
           "%.0f %s", "%.0f",
           0, 1000, 10, false,
           UnitGroup::ALTITUDE, task_behaviour.route_planner.safety_height_terrain);

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

  AddEnum(_("Alternates mode"),
          _("Determines sorting of alternates in the alternates dialog "
            "and in abort mode."),
          abort_task_mode_list, (unsigned)task_behaviour.abort_task_mode);

  AddFloat(_("Polar degradation"), /* xgettext:no-c-format */
           _("A permanent polar degradation. "
             "0% means no degradation, "
             "50% indicates the glider's sink rate is doubled."),
           "%.0f %%", "%.0f",
           0, 50, 1, false,
           (1 - settings_computer.polar.degradation_factor) * 100,
           this);
  SetExpertRow(PolarDegradation);

  AddBoolean(_("Auto bugs"), /* xgettext:no-c-format */
           _("If enabled, adds 1% to the bugs setting after each full hour while flying."),
             settings_computer.polar.auto_bugs);
  SetExpertRow(AutoBugs);

  BuildSafetyMCHelp();

  auto *safety_mc =
    new SafetyMCDataField("%.1f", "%.1f %s",
                          0, Units::ToUserVSpeed(10),
                          Units::ToUserVSpeed(task_behaviour.safety_mc),
                          GetUserVerticalSpeedStep(), false, this);
  safety_mc->SetUnits(Units::GetVerticalSpeedName());
  safety_mc->SetFormat(GetUserVerticalSpeedFormat(false, false));
  Add(_("Safety MC"), safety_mc_help, safety_mc);
  SetExpertRow(SafetyMC);

  /* no help text: this row always carries a value, and
     WndProperty::BeginEditing() then shows the value instead of the
     help; the explanation is on the safety MC row above */
  AddReadOnly(_("Safety MC glide ratio"));
  SetExpertRow(SafetyMCGlideRatio);
  UpdateSafetyMCGlideRatio();

  AddFloat(_("STF risk factor"),
           _("The STF risk factor reduces the MacCready setting used to calculate speed to fly as the glider gets low, in order to compensate for risk. Set to 0.0 for no compensation, 1.0 scales MC linearly with current height (with reference to height of the maximum climb). If considered, 0.3 is recommended."),
           "%.1f %s", "%.1f",
           0, 1, 0.1, false,
           task_behaviour.risk_gamma);
  SetExpertRow(RiskFactor);

  AddBoolean(C_("Setting", "Turn back marker"),
             _("Show a green triangle on the map along the current track "
               "indicating the furthest point from which the active task "
               "waypoint or Goto target can still be reached with the "
               "current altitude and conditions. "
               "The triangle is only shown during cruise when the target "
               "is reachable."),
             task_behaviour.turn_back_marker_enabled);
}

bool
SafetyFactorsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveValue(ArrivalHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::SafetyAltitudeArrival,
                       task_behaviour.safety_height_arrival);

  changed |= SaveValue(TerrainHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::SafetyAltitudeTerrain,
                       task_behaviour.route_planner.safety_height_terrain);

  changed |= SaveValueEnum(AlternateMode, ProfileKeys::AbortTaskMode,
                           task_behaviour.abort_task_mode);

  double degradation = (1 - settings_computer.polar.degradation_factor) * 100;
  if (SaveValue(PolarDegradation, degradation)) {
    settings_computer.polar.SetDegradationFactor(1 - degradation / 100);
    Profile::Set(ProfileKeys::PolarDegradation,
                 settings_computer.polar.degradation_factor);
    backend_components->SetTaskPolar(settings_computer.polar);
    changed = true;
  }

  if (SaveValue(AutoBugs, settings_computer.polar.auto_bugs)) {
    Profile::Set(ProfileKeys::AutoBugs, settings_computer.polar.auto_bugs);
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

  if (SaveValue(TurnBackMarker, task_behaviour.turn_back_marker_enabled)) {
    Profile::Set(ProfileKeys::TurnBackMarkerEnabled, task_behaviour.turn_back_marker_enabled);
    changed = true;
  }

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSafetyFactorsConfigPanel()
{
  return std::make_unique<SafetyFactorsConfigPanel>();
}
