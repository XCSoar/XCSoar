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

#ifndef XCSOAR_WINDOW_PROJECTION_HPP
#define XCSOAR_WINDOW_PROJECTION_HPP

#include "Projection.hpp"
#include "ui/dim/Rect.hpp"
#include "Geo/GeoBounds.hpp"

#include <algorithm>
#include <cassert>
#include <optional>

struct GeoQuadrilateral;

class WindowProjection:
  public Projection
{
#ifndef NDEBUG
  bool screen_size_initialised = false;
#endif

  PixelSize screen_size;

  /**
   * Geographical representation of the screen boundaries.
   *
   * This is a cached member that has to be updated manually by
   * calling UpdateScreenBounds()
   */
  GeoBounds screen_bounds;

public:
  /**
   * Converts a geographical location to a screen coordinate if the
   * location is within the visible bounds
   * @param loc Geographical location
   * @param sc Screen coordinate (output)
   * @return True if the location is within the bounds
   */
  [[gnu::pure]]
  std::optional<PixelPoint> GeoToScreenIfVisible(const GeoPoint &loc) const noexcept;

  /**
   * Checks whether a geographical location is within the visible bounds
   * @param loc Geographical location
   * @return True if the location is within the bounds
   */
  [[gnu::pure]]
  bool GeoVisible(const GeoPoint &loc) const noexcept {
    return screen_bounds.IsInside(loc);
  }

  /**
   * Checks whether a screen coordinate is within the visible bounds
   * @param P Screen coordinate
   * @return True if the screen coordinate is within the bounds
   */
  [[gnu::pure]]
  bool ScreenVisible(const PixelPoint &p) const noexcept {
    assert(screen_size_initialised);

    return GetScreenRect().Contains(p);
  }

  void SetScreenSize(PixelSize new_size) noexcept {
    assert(new_size.width > 0);
    assert(new_size.height > 0);

    screen_size = new_size;

#ifndef NDEBUG
    screen_size_initialised = true;
#endif
  }

  void SetMapRect(const PixelRect &rc) noexcept {
    SetScreenSize(rc.GetSize());
  }

  [[gnu::pure]]
  double GetMapScale() const noexcept;

  /**
   * Configure the scale so a centered circle with the specified
   * radius is visible.
   */
  void SetScaleFromRadius(double radius) noexcept;

  /**
   * Returns the size of the map area in pixels.
   */
  [[gnu::pure]]
  PixelSize GetScreenSize() const noexcept {
    assert(screen_size_initialised);

    return screen_size;
  }

  [[gnu::pure]]
  PixelRect GetScreenRect() const noexcept {
    return PixelRect{GetScreenSize()};
  }

  /**
   * Returns the raster coordinates at the center of the map.
   */
  [[gnu::pure]]
  PixelPoint GetScreenCenter() const noexcept {
    return GetScreenRect().GetCenter();
  }

  /**
   * Returns the width of the map area in meters.
   */
  double GetScreenWidthMeters() const noexcept {
    return DistancePixelsToMeters(GetScreenSize().width);
  }

  /**
   * Returns the length of the larger edge of the map area in pixels.
   */
  unsigned GetScreenDistance() const noexcept
  {
    return std::max(GetScreenSize().height, GetScreenSize().width);
  }

  /**
   * Returns the length of the smaller edge of the map area in pixels.
   */
  unsigned GetMinScreenDistance() const noexcept
  {
    return std::min(GetScreenSize().height, GetScreenSize().width);
  }

  [[gnu::pure]]
  double GetScreenDistanceMeters() const noexcept;

  /**
   * Returns the GeoPoint at the center of the screen.
   */
  [[gnu::pure]]
  GeoPoint GetGeoScreenCenter() const noexcept;

  // used by terrain renderer, topography and airspace
  [[gnu::pure]]
  const GeoBounds &GetScreenBounds() const noexcept {
    return screen_bounds;
  }

  [[gnu::pure]]
  GeoQuadrilateral GetGeoQuadrilateral() const noexcept;

  /** Updates the cached screen_bounds member */
  void UpdateScreenBounds() noexcept;

protected:
  [[gnu::pure]]
  int GetMapResolutionFactor() const noexcept {
    return GetMinScreenDistance() / 8;
  }
};

#endif
