// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
class AbstractAirspace;
class TwoTextRowsRenderer;
struct PixelRect;
struct GeoVector;
struct AirspaceLook;
struct AirspaceRendererSettings;

namespace AirspaceListRenderer
{
  /**
   * Draws an airspace list item.
   *
   * Comment is e.g. "Class C"
   */
  void Draw(Canvas &canvas, const PixelRect rc, const AbstractAirspace &airspace,
            const TwoTextRowsRenderer &row_renderer,
            const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);

  /**
   * Draws an airspace list item.
   *
   * Comment is e.g. "Class C - 20.0 km - 56 deg"
   *
   * @param vector The distance and direction that should be
   * added to the comment
   */
  void Draw(Canvas &canvas, const PixelRect rc, const AbstractAirspace &airspace,
            const GeoVector &vector,
            const TwoTextRowsRenderer &row_renderer,
            const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);
}
