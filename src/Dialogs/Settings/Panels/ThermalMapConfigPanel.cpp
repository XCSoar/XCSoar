// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalMapConfigPanel.hpp"

#ifdef HAVE_HTTP

#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  ENABLE_TIM,
};

class ThermalMapConfigPanel final : public RowFormWidget {
public:
  ThermalMapConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
ThermalMapConfigPanel::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean("Thermal Information Map",
             _("Show thermal locations downloaded from Thermal Information "
               "Map (thermalmap.info)."),
             settings.enable_tim);
}

bool
ThermalMapConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;
  auto &settings = CommonInterface::SetComputerSettings().weather;

  changed |= SaveValue(ENABLE_TIM, ProfileKeys::EnableThermalInformationMap,
                       settings.enable_tim);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateThermalMapConfigPanel()
{
  return std::make_unique<ThermalMapConfigPanel>();
}

#endif /* HAVE_HTTP */
