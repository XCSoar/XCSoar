// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <type_traits>

#include <tchar.h>

struct InfoBoxSettings;

struct PageLayout
{
  struct InfoBoxConfig {
    /**
     * If false, InfoBoxes will not be shown.
     */
    bool enabled;

    bool auto_switch;
    unsigned panel;

    constexpr InfoBoxConfig() noexcept = default;

    constexpr InfoBoxConfig(bool _auto_switch, unsigned _panel) noexcept
      :enabled(true), auto_switch(_auto_switch), panel(_panel) {}

    constexpr void SetDefaults() noexcept {
      auto_switch = true;
      panel = 0;
    }

    constexpr bool operator==(const InfoBoxConfig &other) const noexcept = default;
    constexpr bool operator!=(const InfoBoxConfig &other) const noexcept = default;
  };

  bool valid;

  /**
   * What to show in the main area?
   */
  enum class Main : uint8_t {
    MAP,

    FLARM_RADAR,

    THERMAL_ASSISTANT,

    HORIZON,

    /**
     * A dummy entry that is used for validating profile values.
     */
    MAX
  } main;

  InfoBoxConfig infobox_config;

  /**
   * What to show below the main area (i.e. map)?
   */
  enum class Bottom : uint8_t {
    NOTHING,

    /**
     * Show a cross section below the map.
     */
    CROSS_SECTION,

    /**
     * A custom #Widget is being displayed.  This is not a
     * user-accessible option, it's only used for runtime state.
     */
    CUSTOM,

    /**
     * A dummy entry that is used for validating profile values.
     */
    MAX
  } bottom;

  PageLayout() = default;

  constexpr PageLayout(bool _valid, InfoBoxConfig _infobox_config)
    :valid(_valid), main(Main::MAP),
     infobox_config(_infobox_config),
     bottom(Bottom::NOTHING) {}

  constexpr PageLayout(InfoBoxConfig _infobox_config)
    :valid(true), main(Main::MAP),
     infobox_config(_infobox_config),
     bottom(Bottom::NOTHING) {}

  /**
   * Return an "undefined" page.  Its IsDefined() method will return
   * false.
   */
  static constexpr PageLayout Undefined() noexcept {
    return {false, InfoBoxConfig(false, 0)};
  }

  /**
   * Returns the default page that will be created initially.
   */
  static constexpr PageLayout Default() noexcept  {
    return {true, InfoBoxConfig(true, 0)};
  }

  /**
   * Returns the default page that will show the "Aux" InfoBoxes.
   */
  static constexpr PageLayout Aux() noexcept {
    return {true, InfoBoxConfig(false, 3)};
  }

  /**
   * Returns a default full-screen page.
   */
  static constexpr PageLayout FullScreen() noexcept {
    auto pl = Default();
    pl.infobox_config.enabled = false;
    return pl;
  }

  constexpr bool IsDefined() const {
    return valid;
  }

  constexpr void SetUndefined() noexcept {
    valid = false;
  }

  [[nodiscard]]
  const TCHAR *MakeTitle(const InfoBoxSettings &info_box_settings,
                         std::span<TCHAR> buffer,
                         const bool concise=false) const noexcept;

  constexpr bool operator==(const PageLayout &other) const noexcept = default;
  constexpr bool operator!=(const PageLayout &other) const noexcept = default;
};

struct PageSettings {
  static constexpr unsigned MAX_PAGES = 8;

  std::array<PageLayout, MAX_PAGES> pages;

  unsigned n_pages;

  /**
   * Each page manages its own map zoom level.  Returning to this page
   * restores the last zoom level.
   */
  bool distinct_zoom;

  void SetDefaults() noexcept;

  /**
   * Eliminate empty pages to make the array contiguous.
   */
  void Compress() noexcept;
};

static_assert(std::is_trivial<PageSettings>::value, "type is not trivial");
