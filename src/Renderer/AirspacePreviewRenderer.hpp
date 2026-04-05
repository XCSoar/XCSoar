// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceClass.hpp"
#include "ui/canvas/Color.hpp"

struct PixelPoint;
class Canvas;
class AbstractAirspace;
struct AirspaceRendererSettings;
struct AirspaceLook;

namespace AirspacePreviewRenderer
{
  bool PrepareFill(Canvas &canvas, AirspaceClass type,
                   const AirspaceLook &look,
                   const AirspaceRendererSettings &settings);

  /**
   * Restore text drawing state after #PrepareFill. Pass the text color that
   * was active before #PrepareFill (e.g. from Canvas::GetTextColor()).
   */
  void UnprepareFill(Canvas &canvas, Color text_color) noexcept;

  bool PrepareOutline(Canvas &canvas, AirspaceClass type,
                      const AirspaceLook &look,
                      const AirspaceRendererSettings &settings);

  /** Draw a scaled preview of the given airspace */
  void Draw(Canvas &canvas, const AbstractAirspace &airspace,
            const PixelPoint pt, unsigned radius,
            const AirspaceRendererSettings &settings,
            const AirspaceLook &look);
}
