// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadarRenderer.hpp"
#include "Math/Angle.hpp"
#include "Math/Util.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm> // for std::min()

static constexpr unsigned
HalfWithPadding(unsigned size, unsigned padding) noexcept
{
  unsigned half = size / 2;
  if (half > padding)
    half -= padding;
  return half;
}

void
RadarRenderer::UpdateLayout(const PixelRect &rc) noexcept
{
  center = rc.GetCenter();
  view_radius = std::min(HalfWithPadding(rc.GetWidth(), h_padding),
                         HalfWithPadding(rc.GetHeight(), v_padding));
}

PixelPoint
RadarRenderer::At(Angle angle, unsigned radius) const noexcept
{
  return At(iround(radius * angle.fastsine()),
            -iround(radius * angle.fastcosine()));

}

void
RadarRenderer::DrawCircle(Canvas &canvas, unsigned circle_radius) const noexcept
{
  canvas.DrawCircle(center, circle_radius);
}
