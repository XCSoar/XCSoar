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

#ifndef XCSOAR_PAGES_HPP
#define XCSOAR_PAGES_HPP

#include "Compiler.h"

struct PageLayout;
class GlueMapWindow;
class Widget;

namespace PageActions
{
  /**
   * Returns the configured #PageLayout that was most recently
   * visible.
   */
  gcc_pure
  const PageLayout &GetConfiguredLayout();

  /**
   * Returns the #PageLayout that is currently visible.
   */
  gcc_pure
  const PageLayout &GetCurrentLayout();

  /**
   * Opens the next page.
   */
  void Next();
  /**
   * Opens the previous page.
   */
  void Prev();

  /**
   * Opens the given layout.
   * Attention! Internally the previous page is still selected.
   * @param layout The layout to open
   */
  void OpenLayout(const PageLayout &layout);

  /**
   * Determine the index of the next configured page.
   */
  gcc_pure
  unsigned NextIndex();

  /**
   * Determine the index of the next configured page.
   */
  gcc_pure
  unsigned PrevIndex();

  /**
   * Reload the current layout.
   */
  void Update();

  /**
   * Restore the current page as it was configured.
   */
  void Restore();

  /**
   * Schedule a call to Restore().  The function returns immediately,
   * and there is no guarantee that it succeeds.
   */
  void DeferredRestore();

  /**
   * Like Restore(), but affects only the bottom area.
   */
  void RestoreBottom();

  /**
   * Show a page with the map, or restore the current page if it was
   * configured with a map (for example if the user has activated the
   * FLARM radar page).
   *
   * On success, the function returns the map window object.
   */
  GlueMapWindow *ShowMap();

  /**
   * Show a page with a full-screen map.
   *
   * On success, the function returns the map window object.
   */
  GlueMapWindow *ShowOnlyMap();

  /**
   * Show a page with the traffic radar.
   */
  void ShowTrafficRadar();

  /**
   * Show a page with the thermal assistant.
   */
  void ShowThermalAssistant();

  /**
   * Use a custom widget for the "bottom" area.  This is a wrapper for
   * MainWindow::SetBottomWidget().  Call RestoreBottom() to undo this.
   */
  void SetCustomBottom(Widget *widget);
};

#endif
