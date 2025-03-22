// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class SlopeShading: uint8_t {
  OFF,
  FIXED,
  SUN,
  WIND,
};

enum class Contours: uint8_t {
  OFF,
  ON
};

struct TerrainRendererSettings {
  /** Number of available color ramps */
  static constexpr unsigned NUM_RAMPS = 18;

  /** Map will show terrain */
  bool enable;

  /**
   * Apply slope shading to the terrain?
   */
  SlopeShading slope_shading;

  /** Terrain contrast percentage */
  short contrast;

  /** Terrain brightness percentage */
  short brightness;

  unsigned short ramp;

  /**
   * Draw contours on terrain?
   */
  Contours contours;

  /**
   * Set all attributes to the default values.
   */
  void SetDefaults();

  bool operator==(const TerrainRendererSettings &other) const {
    return enable == other.enable &&
      slope_shading == other.slope_shading &&
      contrast == other.contrast &&
      brightness == other.brightness &&
      ramp == other.ramp &&
      contours == other.contours;
  }

  bool operator!=(const TerrainRendererSettings &other) const {
    return !(*this == other);
  }
};
