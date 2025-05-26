// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FormatSettings.hpp"
#include "MapSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Gauge/VarioSettings.hpp"
#include "Gauge/TrafficSettings.hpp"
#include "Gauge/NavigatorSettings.hpp"
#include "PageSettings.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "DisplaySettings.hpp"
#include "Audio/Settings.hpp"

#include <chrono>
#include <cstdint>
#include <type_traits>

/**
 * User interface settings.
 */
struct UISettings {
  DisplaySettings display;

  /** timeout in quarter seconds of menu button */
  std::chrono::duration<unsigned> menu_timeout;

  unsigned scale;

  /** Override OS dpi settings */
  unsigned custom_dpi;

  /** Position ThermalAssistant */
  enum class ThermalAssistantPosition: uint8_t {
    OFF,
    BOTTOM_LEFT,
    BOTTOM_LEFT_AVOID_IB,
    BOTTOM_RIGHT,
    BOTTOM_RIGHT_AVOID_IB,
  } thermal_assistant_position;

  /** Enable warning dialog */
  bool enable_airspace_warning_dialog;

  /** Show Menubutton */
  bool show_menu_button;
  bool show_zoom_button;

  enum class PopupMessagePosition : uint8_t {
    CENTER,
    TOP_LEFT,
  } popup_message_position;

  /** Haptic feedback settings */
  enum class HapticFeedback : uint8_t {
    DEFAULT,
    OFF,
    ON,
  } haptic_feedback;

  enum class DarkMode : uint_least8_t {
    OFF,
    ON,
    AUTO,
  } dark_mode;

  FormatSettings format;
  MapSettings map;
  InfoBoxSettings info_boxes;
  VarioSettings vario;
  NavigatorSettings navigator;
  TrafficSettings traffic;
  PageSettings pages;
  DialogSettings dialog;
  SoundSettings sound;

  void SetDefaults() noexcept;

  constexpr unsigned GetPercentScale() const noexcept {
    return scale;
  }
};

static_assert(std::is_trivial<UISettings>::value, "type is not trivial");
