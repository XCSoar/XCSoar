// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/TerrainSettings.hpp"

void
TerrainRendererSettings::SetDefaults()
{
  enable = true;
  slope_shading = SlopeShading::WIND;
  contrast = 65;
  brightness = 192;
  ramp = 0;
  contours = Contours::OFF;
}
