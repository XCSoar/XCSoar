/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_PAGE_STATE_HPP
#define XCSOAR_PAGE_STATE_HPP

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

  void Clear() {
    cruise_scale = -1;
    circling_scale = -1;
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

#endif
