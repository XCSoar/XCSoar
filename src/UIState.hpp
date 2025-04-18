// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayMode.hpp"
#include "util/StaticString.hxx"
#include "PageState.hpp"
#include "Weather/WeatherUIState.hpp"

/**
 * The state of the user interface.
 */
struct UIState {
  /**
   * Is the display currently blanked?
   *
   * TODO: unimplemented
   */
  bool screen_blanked;

  /**
   * The display mode forced by the user.  If not NONE, it overrides
   * the automatic display mode.
   */
  DisplayMode force_display_mode;

  /**
   * The effective display mode.
   */
  DisplayMode display_mode;

  /**
   * Are the info boxes showing an "auxiliary" set?
   */
  bool auxiliary_enabled;

  /**
   * Which "auxiliary" set is visible if #auxiliary_enabled is true?
   */
  unsigned auxiliary_index;

  /**
   * The index of the InfoBox panel currently being displayed.  If
   * #auxiliary_enabled is true, then this is the same as
   * #auxiliary_index.
   */
  unsigned panel_index;

  /**
   * A copy of the current InfoBox panel name.  This copy is necessary
   * because the original name is in InfoBoxSettings, but MapWindow
   * does not know InfoBoxSettings or UISettings, and needs to know
   * the name for rendering the overlay.
   */
  StaticString<32u> panel_name;

  PagesState pages;

  WeatherUIState weather;

  void Clear();
};
