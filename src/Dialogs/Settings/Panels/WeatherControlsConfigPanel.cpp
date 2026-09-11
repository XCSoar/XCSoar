// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherControlsConfigPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Profile/Keys.hpp"
#include "Weather/Settings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  CONTROLS_HEIGHT,
};

static constexpr StaticEnumChoice controls_height_list[] = {
  { 30, "30 %" },
  { 40, "40 %" },
  { 50, "50 %" },
  { 60, "60 %" },
  { 70, "70 %" },
  { 80, "80 %" },
  { 90, "90 %" },
  { 100, "100 %" },
  nullptr
};

class WeatherControlsConfigPanel final : public RowFormWidget {
public:
  WeatherControlsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
WeatherControlsConfigPanel::Prepare(ContainerWindow &parent,
                                    const PixelRect &rc) noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  RowFormWidget::Prepare(parent, rc);

  AddEnum(_("Height"),
          _("Height of the weather overlay control rows at the bottom of "
            "the map, as a percentage of the default touch/control height."),
          controls_height_list,
          settings.controls_height_percent);
}

bool
WeatherControlsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;
  auto &settings = CommonInterface::SetComputerSettings().weather;

  changed |= SaveValueEnum(CONTROLS_HEIGHT,
                           ProfileKeys::WeatherControlsHeightPercent,
                           settings.controls_height_percent);

  if (settings.controls_height_percent < 30)
    settings.controls_height_percent = 30;
  else if (settings.controls_height_percent > 100)
    settings.controls_height_percent = 100;

  if (changed && CommonInterface::main_window != nullptr)
    CommonInterface::main_window->ReinitialiseLayout();

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateWeatherControlsConfigPanel()
{
  return std::make_unique<WeatherControlsConfigPanel>();
}
