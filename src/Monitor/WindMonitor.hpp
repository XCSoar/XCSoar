// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Monitors the calculated/external wind, and clears the manual wind
 * setting when a new wind vector was obtained.
 */
class WindMonitor {
public:
  void Reset() {
  }

  void Check();
};
