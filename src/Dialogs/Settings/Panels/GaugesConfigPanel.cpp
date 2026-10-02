// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GaugesConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "MainWindow.hpp"

enum ControlIndex {
  TAPosition,
};

static constexpr StaticEnumChoice thermal_assistant_position_list[] = {
  { UISettings::ThermalAssistantPosition::OFF,
    N_("Off"),
    N_("Disable thermal assistant.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_LEFT,
    N_("Bottom left"),
    N_("Show thermal assistant in bottom left.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_LEFT_AVOID_IB,
    N_("Bottom left (avoid InfoBoxes)"),
    N_("Show thermal assistant in bottom left, above or to the right of InfoBoxes (if present).") },
  { UISettings::ThermalAssistantPosition::BOTTOM_RIGHT,
    N_("Bottom right"),
    N_("Show thermal assistant in bottom right.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_RIGHT_AVOID_IB,
    N_("Bottom right (avoid InfoBoxes)"),
    N_("Show thermal assistant in bottom right, above or to the left of InfoBoxes (if present).") },
  { UISettings::ThermalAssistantPosition::TOP_LEFT,
    N_("Top left"),
    N_("Show thermal assistant in top left.") },
  { UISettings::ThermalAssistantPosition::TOP_RIGHT,
    N_("Top right"),
    N_("Show thermal assistant in top right.") },
  { UISettings::ThermalAssistantPosition::CENTER_TOP,
    N_("Center top"),
    N_("Show thermal assistant in center top.") },
  { UISettings::ThermalAssistantPosition::TOP_LEFT_AVOID_IB,
    N_("Top left (avoid InfoBoxes)"),
    N_("Show thermal assistant in top left (avoid InfoBoxes).") },
  { UISettings::ThermalAssistantPosition::TOP_RIGHT_AVOID_IB,
    N_("Top right (avoid InfoBoxes)"),
    N_("Show thermal assistant in top right (avoid InfoBoxes).") },
  { UISettings::ThermalAssistantPosition::CENTER_TOP_AVOID_IB,
    N_("Center top (avoid InfoBoxes)"),
    N_("Show thermal assistant in center top (avoid InfoBoxes).") },
  nullptr
};

class GaugesConfigPanel final : public RowFormWidget {
public:
  GaugesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
GaugesConfigPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  AddEnum(_("Thermal Assistant"),
          _("Enable and select the position of the thermal assistant when overlayed on the main screen."),
          thermal_assistant_position_list,
          (unsigned)ui_settings.thermal_assistant_position);
}

bool
GaugesConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  if (SaveValueEnum(TAPosition, ProfileKeys::TAPosition,
                    ui_settings.thermal_assistant_position)) {
    CommonInterface::main_window->ReinitialiseLayout();
    changed = true;
  }

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateGaugesConfigPanel()
{
  return std::make_unique<GaugesConfigPanel>();
}
