// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Terrain/Height.hpp"

class Canvas;
class ChartRenderer;
struct CrossSectionLook;
struct BulkPixelPoint;

/**
 * A Window which renders a terrain and airspace cross-section
 */
class TerrainXSRenderer
{
  const CrossSectionLook &look;

public:
  TerrainXSRenderer(const CrossSectionLook &_look): look(_look) {}

  void Draw(Canvas &canvas, const ChartRenderer &chart,
            const TerrainHeight *elevations) const;

private:
  void DrawPolygon(Canvas &canvas, TerrainType type,
                   const BulkPixelPoint *points, unsigned num_points) const;
};
