// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

/**
 * What the vario gauge shows besides the needle: the values around
 * it, and the needles which stand for averages.  All of it is for
 * the expert user level.
 */
class VarioConfigPanel final : public ConfigListPanel {
  VarioSettings settings;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
VarioConfigPanel::LoadSettings() noexcept
{
  settings = CommonInterface::GetUISettings().vario;
}

void
VarioConfigPanel::Fill() noexcept
{
  if (!IsExpert())
    return;

  AddGroup();

  AddToggleItem(_("Speed arrows"),
                _("Whether to show speed command arrows on the vario gauge. In cruise mode, "
                  "arrows pointing up command slow down; arrows pointing down command speed up."),
                settings.show_speed_to_fly);

  AddToggleItem(_("Show average"),
                _("Whether to show the average climb rate. In cruise mode, this switches to showing the "
                  "average netto airmass rate."),
                settings.show_average);

  AddToggleItem(_("Show MacReady"),
                _("Whether to show the MacCready setting."),
                settings.show_mc);

  AddToggleItem(_("Show bugs"),
                _("Whether to show the bugs percentage."),
                settings.show_bugs);

  AddToggleItem(_("Show ballast"),
                _("Whether to show the ballast percentage."),
                settings.show_ballast);

  AddToggleItem(_("Show gross"),
                _("Whether to show the gross climb rate."),
                settings.show_gross);

  AddGroup();

  AddToggleItem(_("Averager needle"),
                _("If true, the vario gauge will display a hollow averager needle. During cruise, this "
                  "needle displays the average netto value. During circling, this needle displays the "
                  "average gross value."),
                settings.show_average_needle);

  AddToggleItem(_("Thermal Averager needle"),
                _("If true, the vario gauge will display a thermal averager needle instead of the current climb-rate needle. During cruise, this "
                  "needle displays the last thermal average netto value. During circling, this needle displays the "
                  "average net value."),
                settings.show_thermal_average_needle);
}

bool
VarioConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  VarioSettings &live = CommonInterface::SetUISettings().vario;

  changed |= Profile::Update(ProfileKeys::AppGaugeVarioSpeedToFly,
                             live.show_speed_to_fly,
                             settings.show_speed_to_fly);

  changed |= Profile::Update(ProfileKeys::AppGaugeVarioAvgText,
                             live.show_average, settings.show_average);

  changed |= Profile::Update(ProfileKeys::AppGaugeVarioMc,
                             live.show_mc, settings.show_mc);

  changed |= Profile::Update(ProfileKeys::AppGaugeVarioBugs,
                             live.show_bugs, settings.show_bugs);

  changed |= Profile::Update(ProfileKeys::AppGaugeVarioBallast,
                             live.show_ballast, settings.show_ballast);

  changed |= Profile::Update(ProfileKeys::AppGaugeVarioGross,
                             live.show_gross, settings.show_gross);

  changed |= Profile::Update(ProfileKeys::AppAveNeedle,
                             live.show_average_needle,
                             settings.show_average_needle);

  changed |= Profile::Update(ProfileKeys::AppAveThermalNeedle,
                             live.show_thermal_average_needle,
                             settings.show_thermal_average_needle);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateVarioConfigPanel()
{
  return std::make_unique<VarioConfigPanel>();
}
