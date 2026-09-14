// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FormatSettings.hpp"
#include "MapSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Gauge/VarioSettings.hpp"
#include "Gauge/TrafficSettings.hpp"
#include "PageSettings.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "DisplaySettings.hpp"
#include "Audio/Settings.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

/**
 * The value of UISettings::antialiasing which disables MSAA.
 */
inline constexpr unsigned ANTIALIASING_OFF = 0;

/**
 * The MSAA sample counts XCSoar offers in its user interface.  Keep
 * in sync with OpenGL::ANTIALIASING_SAMPLE_COUNTS, which the display
 * backends use; this copy exists because the setting must also be
 * available in builds without OpenGL.
 */
inline constexpr unsigned ANTIALIASING_SAMPLE_COUNTS[] = { 2, 4, 8, 16 };

/**
 * Is this a value the user may store in UISettings::antialiasing?
 */
constexpr bool
IsValidAntialiasing(unsigned samples) noexcept
{
  if (samples == ANTIALIASING_OFF)
    return true;

  for (const unsigned n : ANTIALIASING_SAMPLE_COUNTS)
    if (n == samples)
      return true;

  return false;
}

/**
 * Given the number of MSAA samples requested (0 = disabled, or one
 * of #ANTIALIASING_SAMPLE_COUNTS) and a bit mask of the levels one
 * channel (the window surface or a framebuffer object) can actually
 * provide (bit n set means n samples work; bit 0 set means the mask
 * is known at all), return the highest level that does not exceed
 * the request: the "next lower available setting".  Returns 0 if
 * nothing at or below the request is available.
 *
 * If @a available is 0 ("unknown", i.e. this channel could not be
 * probed), the request is returned unchanged.
 *
 * Keep in sync with OpenGL::SelectAntialiasingSamples(), which the
 * display backends use; this copy exists for the same reason as
 * #ANTIALIASING_SAMPLE_COUNTS above.
 */
constexpr unsigned
SelectAntialiasingSamples(unsigned requested, unsigned available) noexcept
{
  if (requested == 0 || available == 0)
    return requested;

  for (std::size_t i = std::size(ANTIALIASING_SAMPLE_COUNTS); i-- > 0;) {
    const unsigned n = ANTIALIASING_SAMPLE_COUNTS[i];
    if (n <= requested && (available & (1u << n)) != 0)
      return n;
  }

  return 0;
}

/**
 * User interface settings.
 */
struct UISettings {
  DisplaySettings display;

  /** timeout in quarter seconds of menu button */
  std::chrono::duration<unsigned> menu_timeout;

  /** UI scale in percent; same bounds as the Text size setting. */
  static constexpr unsigned SCALE_MIN = 75;
  static constexpr unsigned SCALE_MAX = 200;
  static constexpr unsigned SCALE_STEP = 5;

  unsigned scale;

  /** Override OS dpi settings */
  unsigned custom_dpi;

  /** Anti-aliasing (MSAA) samples; see IsValidAntialiasing() */
  unsigned antialiasing;

  /** Position ThermalAssistant */
  enum class ThermalAssistantPosition: uint8_t {
    OFF,
    BOTTOM_LEFT,
    BOTTOM_LEFT_AVOID_IB,
    BOTTOM_RIGHT,
    BOTTOM_RIGHT_AVOID_IB,
    TOP_LEFT,
    TOP_RIGHT,
    CENTER_TOP,
    TOP_LEFT_AVOID_IB,
    TOP_RIGHT_AVOID_IB,
    CENTER_TOP_AVOID_IB,
  } thermal_assistant_position;

  /** Enable warning dialog */
  bool enable_airspace_warning_dialog;

  /** Show Menubutton */
  bool show_menu_button;
  bool show_zoom_button;
  bool show_quickmenu_button;

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
    COUNT
  } dark_mode;

  FormatSettings format;
  MapSettings map;
  InfoBoxSettings info_boxes;
  VarioSettings vario;
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
