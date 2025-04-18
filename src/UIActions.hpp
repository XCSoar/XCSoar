// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * This namespace provides access to several user interface actions.
 * These are usually triggered by the user, for example by
 * (configurable) InputEvents or by hard-coded button handlers.  This
 * API is meant to be lean, without too many header dependencies.
 */
namespace UIActions {
  void SignalShutdown(bool force);

  bool CheckShutdown();

  /**
   * Switch to the traffic radar page.
   */
  void ShowTrafficRadar();

  void ShowThermalAssistant();

  void ShowHorizon();
};
