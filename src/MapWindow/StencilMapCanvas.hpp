// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifndef ENABLE_OPENGL

#include "Geo/GeoClip.hpp"
#include "util/ReusableArray.hpp"

struct PixelPoint;
struct BulkPixelPoint;
class Canvas;
class Projection;
class WindowProjection;
struct AirspaceRendererSettings;
class SearchPointVector;

/**
 * Utility class to draw multilayer items on a canvas with stencil masking
 */
class StencilMapCanvas
{
  const GeoClip clip;

  /**
   * A variable-length buffer for clipped GeoPoints.
   */
  ReusableArray<GeoPoint> geo_points_buffer;

  ReusableArray<BulkPixelPoint> pixel_points_buffer;

public:
  Canvas &buffer;
  Canvas &stencil;
  const WindowProjection &proj;
  bool buffer_drawn;
  bool use_stencil;

  const AirspaceRendererSettings &settings;

public:
  StencilMapCanvas(Canvas &_buffer,
                   Canvas &_stencil,
                   const WindowProjection &_proj,
                   const AirspaceRendererSettings &_settings);

  StencilMapCanvas(const StencilMapCanvas &other);

  void DrawSearchPointVector(const SearchPointVector &points);

  void DrawCircle(const PixelPoint &center, unsigned radius);

  void Begin();

  /**
   * Commits the calculated results
   *
   * @return true if something has been rendered, false otherwise
   */
  bool Commit();

protected:
  void ClearBuffer();
};

#endif // !ENABLE_OPENGL
