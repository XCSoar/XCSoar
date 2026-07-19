// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <type_traits>

struct InfoBoxSettings;

class RaspStore;

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

    MAP_NORTH_UP,

    EDL_MAP,

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

    WEATHER_CONTROLS,

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

  /**
   * Optional weather overlay drawn on map pages.
   */
  enum class Overlay : uint8_t {
    NONE,
    RASP,
    EDL,
    XCTHERM,

    MAX
  } overlay;

  /**
   * Selected RASP field when #overlay is Overlay::RASP.
   */
  int rasp_field;

  /**
   * Per-page RASP forecast time when #overlay is Overlay::RASP.
   * #RASP_TIME_AUTO follows GPS local time; #RASP_TIME_NOW is manual
   * "Now" (same as Auto for rendering, but auto-advance is off);
   * otherwise minute-of-day (0..1439).
   */
  static constexpr int RASP_TIME_AUTO = -1;
  static constexpr int RASP_TIME_NOW = 24 * 60;

  int rasp_time;

  /**
   * Selected EDL isobar (Pascal) when #overlay is Overlay::EDL.
   * 0 means Auto (sync from altitude on page enter).
   */
  static constexpr int EDL_TIME_AUTO = -1;
  static constexpr int EDL_TIME_NOW = -2;

  /**
   * Per-page EDL forecast time.  Non-negative values are UTC hours
   * since the Unix epoch.
   */
  int edl_time;

  int edl_isobar;

  static constexpr int XCTHERM_LAYER_AUTO = -1;
  static constexpr int XCTHERM_TIME_AUTO = -1;

  /**
   * Per-page XCTherm cursor.  The layer is an index into the configured
   * region; time is a UTC hour (0..23).
   */
  int xctherm_layer;
  int xctherm_time;

  PageLayout() = default;

  constexpr PageLayout(bool _valid, InfoBoxConfig _infobox_config)
    :valid(_valid), main(Main::MAP),
     infobox_config(_infobox_config),
     bottom(Bottom::NOTHING),
     overlay(Overlay::NONE),
     rasp_field(-1),
     rasp_time(RASP_TIME_AUTO),
     edl_time(EDL_TIME_AUTO),
     edl_isobar(0),
     xctherm_layer(XCTHERM_LAYER_AUTO),
     xctherm_time(XCTHERM_TIME_AUTO) {}

  constexpr PageLayout(InfoBoxConfig _infobox_config)
    :valid(true), main(Main::MAP),
     infobox_config(_infobox_config),
     bottom(Bottom::NOTHING),
     overlay(Overlay::NONE),
     rasp_field(-1),
     rasp_time(RASP_TIME_AUTO),
     edl_time(EDL_TIME_AUTO),
     edl_isobar(0),
     xctherm_layer(XCTHERM_LAYER_AUTO),
     xctherm_time(XCTHERM_TIME_AUTO) {}

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

  [[gnu::const]]
  constexpr bool
  IsMapMain() const noexcept
  {
    return main == Main::MAP || main == Main::MAP_NORTH_UP ||
      main == Main::EDL_MAP;
  }

  [[gnu::const]]
  constexpr bool
  UsesEdlOverlay() const noexcept
  {
    return IsMapMain() && overlay == Overlay::EDL;
  }

  [[gnu::const]]
  constexpr bool
  UsesRaspOverlay() const noexcept
  {
    return IsMapMain() && overlay == Overlay::RASP;
  }

  [[gnu::const]]
  constexpr bool
  UsesXcthermOverlay() const noexcept
  {
    return IsMapMain() && overlay == Overlay::XCTHERM;
  }

  [[gnu::const]]
  constexpr bool
  UsesWeatherOverlay() const noexcept
  {
    return IsMapMain() &&
      (overlay == Overlay::EDL || overlay == Overlay::RASP ||
       overlay == Overlay::XCTHERM);
  }

  /**
   * Convert legacy page layouts to the current representation.
   */
  constexpr void Normalise() noexcept
  {
    if (main == Main::EDL_MAP) {
      main = Main::MAP;
      overlay = Overlay::EDL;
      if (bottom == Bottom::NOTHING)
        bottom = Bottom::WEATHER_CONTROLS;
    }

    if (unsigned(overlay) >= unsigned(Overlay::MAX))
      overlay = Overlay::NONE;

    if (IsMapMain()) {
      if (overlay != Overlay::EDL && overlay != Overlay::RASP &&
          overlay != Overlay::XCTHERM &&
          bottom == Bottom::WEATHER_CONTROLS)
        bottom = Bottom::NOTHING;
    } else {
      overlay = Overlay::NONE;
      if (bottom == Bottom::WEATHER_CONTROLS)
        bottom = Bottom::NOTHING;
    }

    if (overlay != Overlay::RASP) {
      rasp_field = -1;
      rasp_time = RASP_TIME_AUTO;
    } else {
      if (rasp_field < -1)
        rasp_field = -1;

      if (rasp_time != RASP_TIME_AUTO &&
          rasp_time != RASP_TIME_NOW &&
          (rasp_time < 0 || rasp_time >= RASP_TIME_NOW))
        rasp_time = RASP_TIME_AUTO;
    }

    if (overlay != Overlay::EDL) {
      edl_time = EDL_TIME_AUTO;
      edl_isobar = 0;
    } else {
      if (edl_time < EDL_TIME_NOW)
        edl_time = EDL_TIME_AUTO;
      if (edl_isobar < 0)
        edl_isobar = 0;
    }

    if (overlay != Overlay::XCTHERM) {
      xctherm_layer = XCTHERM_LAYER_AUTO;
      xctherm_time = XCTHERM_TIME_AUTO;
    } else {
      if (xctherm_layer < XCTHERM_LAYER_AUTO)
        xctherm_layer = XCTHERM_LAYER_AUTO;
      if (xctherm_time < XCTHERM_TIME_AUTO || xctherm_time >= 24)
        xctherm_time = XCTHERM_TIME_AUTO;
    }
  }

  [[nodiscard]]
  const char *MakeTitle(const InfoBoxSettings &info_box_settings,
                         std::span<char> buffer,
                         const RaspStore *rasp=nullptr,
                         const bool concise=false) const noexcept;

  constexpr bool operator==(const PageLayout &other) const noexcept = default;
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
