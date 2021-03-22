/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "WindowProjection.hpp"

bool
WindowProjection::GeoVisible(const GeoPoint &loc) const noexcept
{
  return screen_bounds.IsInside(loc);
}

std::optional<PixelPoint>
WindowProjection::GeoToScreenIfVisible(const GeoPoint &loc) const noexcept
{
  if (!GeoVisible(loc))
    return {};

  auto p = GeoToScreen(loc);
  if (!ScreenVisible(p))
    return {};

  return p;
}

bool
WindowProjection::ScreenVisible(const PixelPoint &P) const noexcept
{
  assert(screen_size_initialised);

  return GetScreenRect().Contains(P);
}

void
WindowProjection::SetScaleFromRadius(double radius) noexcept
{
  SetScale(double(GetMinScreenDistance()) / (radius * 2));
}

double
WindowProjection::GetMapScale() const noexcept
{
  return DistancePixelsToMeters(GetMapResolutionFactor());
}

double
WindowProjection::GetScreenDistanceMeters() const noexcept
{
  return DistancePixelsToMeters(GetScreenDistance());
}

GeoPoint
WindowProjection::GetGeoScreenCenter() const noexcept
{
  return ScreenToGeo(GetScreenWidth() / 2, GetScreenHeight() / 2);
}

void
WindowProjection::UpdateScreenBounds() noexcept
{
  assert(screen_size_initialised);

  if (!IsValid())
    return;

  const auto r = GetScreenRect();

  GeoBounds sb(ScreenToGeo(r.GetTopLeft()));
  sb.Extend(ScreenToGeo(r.GetTopRight()));
  sb.Extend(ScreenToGeo(r.GetBottomRight()));
  sb.Extend(ScreenToGeo(r.GetBottomLeft()));

  screen_bounds = sb;
}
