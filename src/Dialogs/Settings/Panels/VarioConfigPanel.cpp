// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  AppGaugeVarioSpeedToFly,
  AppGaugeVarioAvgText,
  AppGaugeVarioMc,
  AppGaugeVarioBugs,
  AppGaugeVarioBallast,
  AppGaugeVarioGross,
  AppAveNeedle,
  AppAveThermalNeedle,
};


class VarioConfigPanel final : public RowFormWidget {
public:
  VarioConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
VarioConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  const VarioSettings &settings = CommonInterface::GetUISettings().vario;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("Speed arrows"),
             _("Whether to show speed command arrows on the vario gauge. In cruise mode, "
                 "arrows pointing up command slow down; arrows pointing down command speed up."),
             settings.show_speed_to_fly);
  SetExpertRow(AppGaugeVarioSpeedToFly);

  AddBoolean(_("Show average"),
             _("Whether to show the average climb rate. In cruise mode, this switches to showing the "
                 "average netto airmass rate."),
             settings.show_average);
  SetExpertRow(AppGaugeVarioAvgText);

  AddBoolean(_("Show MacReady"), _("Whether to show the MacCready setting."), settings.show_mc);
  SetExpertRow(AppGaugeVarioMc);

  AddBoolean(_("Show bugs"), _("Whether to show the bugs percentage."), settings.show_bugs);
  SetExpertRow(AppGaugeVarioBugs);

  AddBoolean(_("Show ballast"), _("Whether to show the ballast percentage."), settings.show_ballast);
  SetExpertRow(AppGaugeVarioBallast);

  AddBoolean(_("Show gross"), _("Whether to show the gross climb rate."), settings.show_gross);
  SetExpertRow(AppGaugeVarioGross);

  AddBoolean(_("Averager needle"),
             _("If true, the vario gauge will display a hollow averager needle. During cruise, this "
                 "needle displays the average netto value. During circling, this needle displays the "
                 "average gross value."),
             settings.show_average_needle);
  SetExpertRow(AppAveNeedle);

  AddBoolean(_("Thermal Averager needle"),
             _("If true, the vario gauge will display a thermal averager needle instead of the current climb-rate needle. During cruise, this "
               "needle displays the last thermal average netto value. During circling, this needle displays the "
               "average net value."),
             settings.show_thermal_average_needle);
  SetExpertRow(AppAveThermalNeedle);
}

bool
VarioConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  VarioSettings &settings = CommonInterface::SetUISettings().vario;

  changed |= SaveValue(AppGaugeVarioSpeedToFly, ProfileKeys::AppGaugeVarioSpeedToFly, settings.show_speed_to_fly);

  changed |= SaveValue(AppGaugeVarioAvgText, ProfileKeys::AppGaugeVarioAvgText, settings.show_average);

  changed |= SaveValue(AppGaugeVarioMc, ProfileKeys::AppGaugeVarioMc, settings.show_mc);

  changed |= SaveValue(AppGaugeVarioBugs, ProfileKeys::AppGaugeVarioBugs, settings.show_bugs);

  changed |= SaveValue(AppGaugeVarioBallast, ProfileKeys::AppGaugeVarioBallast, settings.show_ballast);

  changed |= SaveValue(AppGaugeVarioGross, ProfileKeys::AppGaugeVarioGross, settings.show_gross);

  changed |= SaveValue(AppAveNeedle, ProfileKeys::AppAveNeedle, settings.show_average_needle);

  changed |= SaveValue(AppAveThermalNeedle, ProfileKeys::AppAveThermalNeedle, settings.show_thermal_average_needle);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateVarioConfigPanel()
{
  return std::make_unique<VarioConfigPanel>();
}
