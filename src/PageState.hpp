// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

#include <array>
#include <type_traits>

/**
 * The state of one configured page.
 */
struct PageState {
  /**
   * The last map scale on this page.  Negative means it's undefined.
   * This attribute is only used if PageSettings::distinct_zoom is
   * enabled.
   */
  double cruise_scale;

  /**
   * The last map scale on this page while circling.  Negative means
   * it's undefined.  This attribute is only used if
   * PageSettings::distinct_zoom and MapSettings::circle_zoom_enabled
   * are both enabled.
   */
  double circling_scale;

  /**
   * The last zoom mode on this page. This attribute is only used if
   * PageSettings::distinct_zoom is enabled.
   */
  bool auto_zoom_enabled;

  void Clear() {
    cruise_scale = -1;
    circling_scale = -1;
    auto_zoom_enabled = false;
  }
};

/**
 * Keeps track of the state of the "pages" subsystem.
 */
struct PagesState {
  static constexpr unsigned MAX_PAGES = PageSettings::MAX_PAGES;

  /**
   * The index of the current page in the list of configured pages,
   * see #PageSettings.
   *
   * This setting is only active if #special_page is not defined.
   * Otherwise, it is the page that we will return to after the user
   * decides to leave the #special_page.
   */
  unsigned current_index;

  /**
   * If this attribute is defined (see PageLayout::IsDefined()), then
   * the current page is not the one in #PageSettings, specified by
   * #page_index, but a page that was created automatically.  For
   * example, it could be a page that was created by clicking on the
   * FLARM radar.  Pressing left/right will return to the last
   * configured page.
   */
  PageLayout special_page;

  std::array<PageState, MAX_PAGES> pages;

  void Clear();
};

static_assert(std::is_trivial<PagesState>::value, "type is not trivial");
