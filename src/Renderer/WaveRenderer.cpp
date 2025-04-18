// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaveRenderer.hpp"
#include "Computer/WaveResult.hpp"
#include "Tracking/Features.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "Look/WaveLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/GeoClip.hpp"

void
WaveRenderer::Draw(Canvas &canvas, const WindowProjection &projection,
                   const GeoClip &clip, GeoPoint ga, GeoPoint gb) const
{
  assert(ga.IsValid());
  assert(gb.IsValid());

  if (!clip.ClipLine(ga, gb))
    /* outside of the visible map area */
    return;

  const PixelPoint sa(projection.GeoToScreen(ga));
  const PixelPoint sb(projection.GeoToScreen(gb));

  canvas.Select(look.pen);
  canvas.DrawLine(sa, sb);
}

void
WaveRenderer::Draw(Canvas &canvas, const WindowProjection &projection,
                   const GeoClip &clip,
                   const WaveInfo &wave) const
{
  assert(wave.IsDefined());

  Draw(canvas, projection, clip, wave.a, wave.b);
}

void
WaveRenderer::Draw(Canvas &canvas, const WindowProjection &projection,
                   const WaveInfo &wave) const
{
  const GeoClip clip(projection.GetScreenBounds().Scale(1.1));
  Draw(canvas, projection, clip, wave);
}

void
WaveRenderer::Draw(Canvas &canvas, const WindowProjection &projection,
                   const WaveResult &result) const
{
  if (result.waves.empty())
    return;

  const GeoClip clip(projection.GetScreenBounds().Scale(1.1));
  for (const auto &wave : result.waves)
    Draw(canvas, projection, clip, wave);
}

#ifdef HAVE_SKYLINES_TRACKING

void
WaveRenderer::Draw(Canvas &canvas, const WindowProjection &projection,
                   const SkyLinesTracking::Data &data) const
{
  if (data.waves.empty())
    return;

  const GeoClip clip(projection.GetScreenBounds().Scale(1.1));
  for (const auto &i : data.waves)
    Draw(canvas, projection, clip, i.a, i.b);
}

#endif
