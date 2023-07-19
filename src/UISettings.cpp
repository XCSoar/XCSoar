// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UISettings.hpp"
#include "GlobalSettings.hpp"

void
UISettings::SetDefaults() noexcept
{
  display.SetDefaults();

  menu_timeout = std::chrono::seconds{8 * 4};

  scale = 100;

  custom_dpi = 0;  // automatic

  thermal_assistant_position = ThermalAssistantPosition::BOTTOM_LEFT;

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


bool UISettings::GetDarkMode() const noexcept
{
  switch (this->dark_mode) {
  case UISettings::DarkMode::OFF:
    break;

  case UISettings::DarkMode::ON:
    return true;

  case UISettings::DarkMode::AUTO:
    return GlobalSettings::dark_mode;
  }

  return false;
}

uint8_t UISettings::GetBgColorVal() const noexcept
{
  /* Get background (brightness) color value for infoboxes, vario etc.
  Contrast is any integer 0 to 5 (max contrast). */
  uint8_t contrast = this->info_boxes.contrast;
  uint8_t bg = (contrast>5) ? 0 : 16*(5-contrast);
  bool dark_mode = this->GetDarkMode();
  return dark_mode ? bg : 255-bg;
}
