/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Navigation/GeoPoint.hpp"
#include "Math/FastRotation.hpp"
#include "Screen/Point.hpp"
#include "Compiler.h"

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
class Projection {
  /** This is the geographical location that the ScreenOrigin is mapped to */
  GeoPoint GeoLocation;

  /**
   * This is the point that the ScreenRotation will rotate around.
   * It is also the point that the GeoLocation points to.
   */
  RasterPoint ScreenOrigin;

  /**
   * FastIntegerRotation instance for fast
   * rotation in the conversion functions
   */
  FastIntegerRotation ScreenRotation;

  /** The earth's radius in screen coordinates (px) */
  fixed DrawScale;
  /** Inverted value of DrawScale for faster calculations */
  fixed InvDrawScale;

  /** This is the scaling factor in px/m */
  fixed scale;

public:
  Projection();

  gcc_pure
  fixed GetMapScale() const;

  gcc_pure
  fixed GetScale() const;

  /**
   * Sets the scaling factor
   * @param _scale New scale in px/m
   */
  void SetScale(const fixed _scale);

  /**
   * Convert a pixel distance to an angle on Earth's surface.
   */
  gcc_pure
  Angle PixelsToAngle(int pixels) const {
    return Angle::radians(pixels * InvDrawScale);
  }

  /**
   * Convert a an angle on Earth's surface to a pixel distance.
   */
  gcc_pure
  fixed AngleToPixels(Angle angle) const {
    return fast_mult(angle.value_radians(), DrawScale, 12);
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
  GeoPoint ScreenToGeo(const RasterPoint &pt) const {
    return ScreenToGeo(pt.x, pt.y);
  }

  /**
   * Converts a GeoPoint to screen coordinates
   * @param g GeoPoint to convert
   */
  gcc_pure
  RasterPoint GeoToScreen(const GeoPoint &g) const;

  /**
   * Returns the origin/rotation center in screen coordinates
   * @return The origin/rotation center in screen coordinates
   */
  const RasterPoint &GetScreenOrigin() const {
    return ScreenOrigin;
  }

  /**
   * Set the origin/rotation center to the given screen coordinates
   * @param x Screen coordinate in x-direction
   * @param y Screen coordinate in y-direction
   */
  void SetScreenOrigin(int x, int y) {
    ScreenOrigin.x = x;
    ScreenOrigin.y = y;
  }

  /**
   * Set the origin/rotation center to the given screen coordinates
   * @param pt Screen coordinate
   */
  void SetScreenOrigin(RasterPoint pt) {
    ScreenOrigin = pt;
  }

  /**
   * Returns the GeoPoint at the ScreenOrigin
   * @return GeoPoint at the ScreenOrigin
   */
  const GeoPoint &GetGeoLocation() const {
    return GeoLocation;
  }

  /**
   * Set the GeoPoint that relates to the ScreenOrigin
   * @param g The new GeoPoint
   */
  void SetGeoLocation(GeoPoint g) {
    GeoLocation = g;
  }

  /**
   * Converts a geographical distance (m) to a screen distance (px)
   * @param x A geographical distance (m)
   * @return The converted distance in px
   */
  unsigned GeoToScreenDistance(const fixed x) const {
    return iround(scale * x);
  }

  /**
   * Returns the current screen rotation angle
   * @return Screen rotation angle
   */
  Angle GetScreenAngle() const {
    return ScreenRotation.GetAngle();
  }

  /**
   * Sets the screen rotation angle
   * @param angle New screen rotation angle
   */
  void SetScreenAngle(Angle angle) {
    ScreenRotation.SetAngle(angle);
  }

  /**
   * Creates a FastRowRotation object base on the current screen
   * rotation angle and the specified screen row.
   */
  FastRowRotation GetScreenAngleRotation(int y) const {
    return FastRowRotation(ScreenRotation, y);
  }

protected:
  gcc_const
  static int GetMapResolutionFactor();
};

#endif
