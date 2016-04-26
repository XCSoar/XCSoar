/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_PROJECTION_HPP
#define XCSOAR_PROJECTION_HPP

#include "Geo/GeoPoint.hpp"
#include "Math/FastRotation.hpp"
#include "Math/Util.hpp"
#include "Screen/Point.hpp"
#include "Compiler.h"

#include <assert.h>

/**
 * This is a class that can be used for converting geographical into screen
 * coordinates and vice-versa.
 *
 * For doing so one needs to at least set a scaling factor (m/px) by calling
 * the SetScale() function.
 *
 *  Optional features
 * -------------------
 *
 * ScreenOrigin: By calling SetScreenOrigin the screen origin offset
 * can be set. This is the offset that the screen coordinates will be shifted
 * in the conversion functions. It is also the point of rotation for the
 * ScreenRotation.
 *
 * GeoLocation: By calling the SetGeoLocation() function the ScreenOrigin can
 * be mapped to a new geographical location, which will be taken into account
 * when converting other coordinates.
 *
 * ScreenRotation: By calling SetScreenAngle() the rotation angle for the
 * conversions can be set.
 */
class Projection
{
  /** This is the geographical location that the ScreenOrigin is mapped to */
  GeoPoint geo_location;

  /**
   * This is the point that the ScreenRotation will rotate around.
   * It is also the point that the GeoLocation points to.
   */
  PixelPoint screen_origin;

  /**
   * FastIntegerRotation instance for fast
   * rotation in the conversion functions
   */
  FastIntegerRotation screen_rotation;

  /** The earth's radius in screen coordinates (px) */
  double draw_scale;
  /** Inverted value of DrawScale for faster calculations */
  double inv_draw_scale;

  /** This is the scaling factor in px/m */
  double scale;

public:
  Projection();

  bool IsValid() const {
    return geo_location.IsValid();
  }

  gcc_pure
  double GetScale() const {
    return scale;
  }

  /**
   * Sets the scaling factor
   * @param _scale New scale in px/m
   */
  void SetScale(const double _scale);

  /**
   * Convert a pixel distance to a physical length in meters.
   */
  gcc_pure
  double DistancePixelsToMeters(const int x) const {
    return double(x) / GetScale();
  }

  gcc_pure
  double DistanceMetersToPixels(const double distance) const {
    return distance * GetScale();
  }

  /**
   * Convert a pixel distance to an angle on Earth's surface.
   */
  gcc_pure
  Angle PixelsToAngle(int pixels) const {
    return Angle::Radians(pixels * inv_draw_scale);
  }

  /**
   * Convert a an angle on Earth's surface to a pixel distance.
   */
  gcc_pure
  double AngleToPixels(Angle angle) const {
    return angle.Radians() * draw_scale;
  }

  /**
   * Converts screen coordinates to a GeoPoint
   * @param x x-Coordinate on the screen
   * @param y y-Coordinate on the screen
   */
  gcc_pure
  GeoPoint ScreenToGeo(int x, int y) const;

  /**
   * Converts screen coordinates to a GeoPoint
   * @param x x-Coordinate on the screen
   * @param y y-Coordinate on the screen
   */
  gcc_pure
  GeoPoint ScreenToGeo(const PixelPoint &pt) const {
    return ScreenToGeo(pt.x, pt.y);
  }

  /**
   * Converts a GeoPoint to screen coordinates
   * @param g GeoPoint to convert
   */
  gcc_pure
  PixelPoint GeoToScreen(const GeoPoint &g) const;

  /**
   * Returns the origin/rotation center in screen coordinates
   * @return The origin/rotation center in screen coordinates
   */
  const PixelPoint &GetScreenOrigin() const {
    return screen_origin;
  }

  /**
   * Set the origin/rotation center to the given screen coordinates
   * @param x Screen coordinate in x-direction
   * @param y Screen coordinate in y-direction
   */
  void SetScreenOrigin(int x, int y) {
    screen_origin.x = x;
    screen_origin.y = y;
  }

  /**
   * Set the origin/rotation center to the given screen coordinates
   * @param pt Screen coordinate
   */
  void SetScreenOrigin(PixelPoint pt) {
    screen_origin = pt;
  }

  /**
   * Returns the GeoPoint at the ScreenOrigin
   * @return GeoPoint at the ScreenOrigin
   */
  const GeoPoint &GetGeoLocation() const {
    assert(IsValid());

    return geo_location;
  }

  /**
   * Set the GeoPoint that relates to the ScreenOrigin
   * @param g The new GeoPoint
   */
  void SetGeoLocation(GeoPoint g) {
    geo_location = g;
    geo_location.Normalize();
  }

  /**
   * Converts a geographical distance (m) to a screen distance (px)
   * @param x A geographical distance (m)
   * @return The converted distance in px
   */
  unsigned GeoToScreenDistance(const double x) const {
    return uround(scale * x);
  }

  /**
   * Returns the current screen rotation angle
   * @return Screen rotation angle
   */
  Angle GetScreenAngle() const {
    return screen_rotation.GetAngle();
  }

  /**
   * Sets the screen rotation angle
   * @param angle New screen rotation angle
   */
  void SetScreenAngle(Angle angle) {
    screen_rotation.SetAngle(angle);
  }

  /**
   * Creates a FastRowRotation object base on the current screen
   * rotation angle and the specified screen row.
   */
  FastRowRotation GetScreenAngleRotation(int y) const {
    return FastRowRotation(screen_rotation, y);
  }
};

#endif
