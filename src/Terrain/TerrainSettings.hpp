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

#ifndef XCSOAR_TERRAIN_SETTINGS_HPP
#define XCSOAR_TERRAIN_SETTINGS_HPP

#include <stdint.h>

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
  static constexpr unsigned NUM_RAMPS = 15;

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

#endif
