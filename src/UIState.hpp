/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_UI_STATE_HPP
#define XCSOAR_UI_STATE_HPP

#include "DisplayMode.hpp"
#include "Util/StaticString.hpp"
#include "PageSettings.hpp"

/**
 * The state of the user interface.
 */
struct UIState {
  /**
   * Is the display currently blanked?
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

  /**
   * The index of the current page in the list of configured pages,
   * see #PageSettings.
   *
   * This setting is only active if #special_page is not defined.
   * Otherwise, it is the page that we will return to after the user
   * decides to leave the #special_page.
   */
  unsigned page_index;

  /**
   * If this attribute is defined (see PageLayout::IsDefined()), then
   * the current page is not the one in #PageSettings, specified by
   * #page_index, but a page that was created automatically.  For
   * example, it could be a page that was created by clicking on the
   * FLARM radar.  Pressing left/right will return to the last
   * configured page.
   */
  PageLayout special_page;

  void Clear();
};

#endif
