// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Settings for calculating a glide path.
 */
struct GlideSettings {
  /**
   * Account for wind drift for the predicted circling duration.
   */
  bool predict_wind_drift;

  void SetDefaults();
};
