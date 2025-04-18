// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelPoint;
class Canvas;
class ObservationZonePoint;
struct TaskLook;
struct AirspaceRendererSettings;
struct AirspaceLook;

namespace OZPreviewRenderer
{
  /** Draw a scaled preview of the given airspace */
  void Draw(Canvas &canvas, const ObservationZonePoint &oz,
            PixelPoint pt, unsigned radius,
            const TaskLook &look,
            const AirspaceRendererSettings &airspace_settings,
            const AirspaceLook &airspace_look);
}
