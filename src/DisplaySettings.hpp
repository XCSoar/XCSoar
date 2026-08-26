// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayOrientation.hpp"
#include "DisplayType.hpp"

#include <cstdint>
#include <type_traits>

/**
 * Display settings.
 */
struct DisplaySettings {
  DisplayOrientation orientation;
  uint8_t cursor_size;
  bool invert_cursor_colors;
  bool full_screen;

  /**
   * The screen edges up to which everything except the map itself -
   * the InfoBoxes, the gauges, the overlay buttons, the compass, the
   * final glide bar - may extend.
   *
   * @see safe_area_stretch
   */
  enum SafeAreaStretchEdge : uint8_t {
    SAFE_AREA_STRETCH_TOP = 0x1,
    SAFE_AREA_STRETCH_RIGHT = 0x2,
    SAFE_AREA_STRETCH_BOTTOM = 0x4,
    SAFE_AREA_STRETCH_LEFT = 0x8,

    SAFE_AREA_STRETCH_NONE = 0,
    SAFE_AREA_STRETCH_ALL = SAFE_AREA_STRETCH_TOP|SAFE_AREA_STRETCH_RIGHT|
      SAFE_AREA_STRETCH_BOTTOM|SAFE_AREA_STRETCH_LEFT,
  };

  /**
   * Bit mask of #SafeAreaStretchEdge: on these edges, the InfoBoxes,
   * gauges and map overlays use the whole screen.  On the others they
   * stay inside the safe area, clear of the display cutout ("notch")
   * and the system bars.
   *
   * Each edge is decided separately because the insets move when the
   * device is rotated: leaving an edge on the safe area costs nothing
   * in an orientation that has no inset there, but keeps the display
   * clear of the cutout once it turns up on that edge.
   *
   * Only relevant while #full_screen is enabled; without it, the
   * whole user interface is inside the safe area anyway.
   */
  uint8_t safe_area_stretch;

  /**
   * Shall the iOS status bar be visible?
   */
  enum class StatusBar : uint8_t {
    /**
     * Visible while there is room for it above the user interface,
     * i.e. unless the safe area is stretched to the top screen edge.
     */
    AUTO,

    VISIBLE,

    HIDDEN,
  };

  StatusBar status_bar;

  /**
   * Resolve #status_bar against #full_screen.
   */
  constexpr bool IsStatusBarVisible() const noexcept {
    switch (status_bar) {
    case StatusBar::VISIBLE:
      return true;

    case StatusBar::HIDDEN:
      return false;

    case StatusBar::AUTO:
      break;
    }

    /* without full screen mode the whole user interface is inside the
       safe area anyway; with it, the status bar would sit on top of
       the InfoBoxes as soon as they reach the top screen edge */
    return !full_screen ||
      (safe_area_stretch & SAFE_AREA_STRETCH_TOP) == 0;
  }

  /**
   * Display technology for refresh-sensitive UI.  Defaults to e-ink
   * on Kobo and LCD on other platforms.
   */
  DisplayType display_type;

  void SetDefaults();
};

static_assert(std::is_trivial<DisplaySettings>::value, "type is not trivial");
