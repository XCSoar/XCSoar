// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PageLayout;
class GlueMapWindow;
class Widget;

namespace PageActions
{
  /**
   * Returns the configured #PageLayout that was most recently
   * visible.
   */
  [[gnu::pure]]
  const PageLayout &GetConfiguredLayout();

  /**
   * Returns the #PageLayout that is currently visible.
   */
  [[gnu::pure]]
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
  [[gnu::pure]]
  unsigned NextIndex();

  /**
   * Determine the index of the next configured page.
   */
  [[gnu::pure]]
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
