// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UISettings.hpp"

void
UISettings::SetDefaults() noexcept
{
  display.SetDefaults();

  menu_timeout = std::chrono::seconds{8 * 4};

  scale = 100;

  custom_dpi = 0;  // automatic

  thermal_assistant_position = ThermalAssistantPosition::BOTTOM_LEFT_AVOID_IB;

  enable_airspace_warning_dialog = true;

  popup_message_position = PopupMessagePosition::CENTER;

  haptic_feedback = HapticFeedback::DEFAULT;

  show_menu_button = true;

  dark_mode = DarkMode::AUTO;

  format.SetDefaults();
  map.SetDefaults();
  info_boxes.SetDefaults();
  vario.SetDefaults();
  traffic.SetDefaults();
  pages.SetDefaults();
  dialog.SetDefaults();
  sound.SetDefaults();
}
