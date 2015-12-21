/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "WaveRenderer.hpp"
#include "Computer/WaveResult.hpp"
#include "Look/WaveLook.hpp"
#include "Screen/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/GeoClip.hpp"

void
WaveRenderer::Draw(Canvas &canvas, const WindowProjection &projection,
                   const GeoClip &clip,
                   const WaveInfo &wave) const
{
  assert(wave.IsDefined());

  GeoPoint ga(wave.a), gb(wave.b);
  if (!clip.ClipLine(ga, gb))
    /* outside of the visible map area */
    return;

  const RasterPoint sa(projection.GeoToScreen(ga));
  const RasterPoint sb(projection.GeoToScreen(gb));

  canvas.Select(look.pen);
  canvas.DrawLine(sa, sb);
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
