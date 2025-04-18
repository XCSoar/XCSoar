// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Settings for #WaveComputer.
 */
struct WaveSettings {
  bool enabled;

  void SetDefaults() {
    enabled = false;
  }
};
