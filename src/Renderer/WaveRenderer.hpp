// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
struct WaveInfo;
struct WaveResult;
struct WaveLook;
class Canvas;
class WindowProjection;
class GeoClip;

namespace SkyLinesTracking {
  struct Data;
}

class WaveRenderer {
  const WaveLook &look;

public:
  WaveRenderer(const WaveLook &_look):look(_look) {}

  void Draw(Canvas &canvas, const WindowProjection &projection,
            const GeoClip &clip, GeoPoint a, GeoPoint b) const;

  void Draw(Canvas &canvas, const WindowProjection &projection,
            const GeoClip &clip,
            const WaveInfo &wave) const;

  void Draw(Canvas &canvas, const WindowProjection &projection,
            const WaveInfo &wave) const;

  void Draw(Canvas &canvas, const WindowProjection &projection,
            const WaveResult &result) const;

  void Draw(Canvas &canvas, const WindowProjection &projection,
            const SkyLinesTracking::Data &data) const;
};
