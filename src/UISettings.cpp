// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UISettings.hpp"
#include "Asset.hpp"

void
UISettings::SetDefaults() noexcept
{
  display.SetDefaults();

  menu_timeout = std::chrono::seconds{8 * 4};

  scale = 100;

#ifdef KOBO
  viewing_distance = Layout::ViewingDistance::HANDHELD;
#else
  viewing_distance = Layout::ViewingDistance::AUTO;
#endif

  custom_dpi = 0;  // automatic

  thermal_assistant_position = ThermalAssistantPosition::BOTTOM_LEFT_AVOID_IB;

  enable_airspace_warning_dialog = true;

  popup_message_position = PopupMessagePosition::TOP_LEFT;

  haptic_feedback = HapticFeedback::DEFAULT;

#ifdef KOBO
  show_menu_button = true;
#else
  show_menu_button = false;
#endif
  show_zoom_button = show_menu_button;
  show_quickmenu_button = HasTouchScreen();
  maccready_stf_ias = false;

#ifdef KOBO
  dark_mode = DarkMode::OFF;
#else
  dark_mode = DarkMode::AUTO;
#endif

  format.SetDefaults();
  map.SetDefaults();
  info_boxes.SetDefaults();
  vario.SetDefaults();
  traffic.SetDefaults();
  pages.SetDefaults();
  dialog.SetDefaults();
  sound.SetDefaults();
}
