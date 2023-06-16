// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/ARange.hpp"
#include "GeoPoint.hpp"

/**
 * A rectangle on earth's surface with very simple semantics.  Similar
 * to the RECT struct, it is bounded by four orthogonal lines.  Its
 * goal is to perform fast overlap checks, e.g. to determine if an
 * object is visible on the screen.
 */
class GeoBounds {
  /**
   * The range from west to east.
   */
  AngleRange longitude;

  /**
   * The range from south to north.
   */
  AngleRange latitude;

public:
  constexpr GeoBounds() noexcept = default;

  constexpr GeoBounds(const GeoPoint pt) noexcept
    :longitude(pt.longitude, pt.longitude),
     latitude(pt.latitude, pt.latitude) {}

  constexpr GeoBounds(GeoPoint north_west, GeoPoint south_east) noexcept
    :longitude(north_west.longitude, south_east.longitude),
     latitude(south_east.latitude, north_west.latitude) {}

  /**
   * Construct an instance that is "invalid", i.e. IsValid() will
   * return false.  The return value must not be used in any
   * calculation.
   */
  constexpr static GeoBounds Invalid() noexcept {
    return GeoBounds(GeoPoint::Invalid());
  }

  /**
   * Set this instance to "invalid", i.e. IsValid() will return false.
   * The return value must not be used in any calculation.
   */
  constexpr void SetInvalid() noexcept {
    latitude.end = Angle::FullCircle();
  }

  constexpr Angle GetWest() const noexcept {
    return longitude.start;
  }

  constexpr Angle GetEast() const noexcept {
    return longitude.end;
  }

  constexpr Angle GetSouth() const noexcept {
    return latitude.start;
  }

  constexpr Angle GetNorth() const noexcept {
    return latitude.end;
  }

  constexpr GeoPoint GetNorthWest() const noexcept {
    return GeoPoint(GetWest(), GetNorth());
  }

  constexpr GeoPoint GetNorthEast() const noexcept {
    return GeoPoint(GetEast(), GetNorth());
  }

  constexpr GeoPoint GetSouthWest() const noexcept {
    return GeoPoint(GetWest(), GetSouth());
  }

  constexpr GeoPoint GetSouthEast() const noexcept {
    return GeoPoint(GetEast(), GetSouth());
  }

  /**
   * Check if this object is "valid".  Returns false when it was
   * constructed by Invalid().  This is not an extensive plausibility
   * check; it is only designed to catch instances created by
   * Invalid().  If you want a real check, call Check().
   */
  constexpr bool IsValid() const noexcept {
    return latitude.end <= Angle::HalfCircle();
  }

  /**
   * Check if this object is "valid".  This is an extensive test; if
   * all you want is check if this object was constructed by
   * Invalid(), then call the cheaper method IsValid().
   */
  constexpr bool Check() const noexcept {
    return GetSouthWest().Check() && GetNorthEast().Check() &&
      GetNorth() >= GetSouth();
  }

  constexpr bool IsEmpty() const noexcept {
    return longitude.IsEmpty() || latitude.IsEmpty();
  }

  Angle GetWidth() const noexcept {
    return longitude.GetLength();
  }

  Angle GetHeight() const noexcept {
    return latitude.GetLength();
  }

  /**
   * Returns the geographic width of the object (west to east) at its
   * center in metres.
   */
  [[gnu::pure]]
  double GetGeoWidth() const noexcept {
    const Angle middle_latitude = latitude.GetMiddle();
    return GeoPoint(GetWest(), middle_latitude)
      .Distance(GeoPoint(GetEast(), middle_latitude));
  }

  /**
   * Returns the geographic height of the object (south to north) in
   * metres.
   */
  [[gnu::pure]]
  double GetGeoHeight() const noexcept {
    return GetNorthWest().Distance(GetSouthWest());
  }

  /**
   * Extend the bounds so the given point is inside.
   *
   * @return true if the bounds have been modified
   */
  bool Extend(const GeoPoint pt) noexcept;

  bool IsInside(Angle _longitude, Angle _latitude) const noexcept {
    return longitude.IsInside(_longitude) && latitude.IsInside(_latitude);
  }

  bool IsInside(const GeoPoint pt) const noexcept {
    return IsInside(pt.longitude, pt.latitude);
  }

  bool IsInside(const GeoBounds &interior) const noexcept {
    return longitude.IsInside(interior.longitude) &&
      latitude.IsInside(interior.latitude);
  }

  /**
   * Does this GeoBounds instance overlap with the specified one?
   */
  [[gnu::pure]]
  bool Overlaps(const GeoBounds &other) const noexcept {
    return longitude.Overlaps(other.longitude) &&
      latitude.Overlaps(other.latitude);
  }

  /**
   * Set this object to the intersection of this and the other object.
   *
   * @return false if the two objects do not overlap; in this case,
   * the object is left in an undefined state
   */
  bool IntersectWith(const GeoBounds &other) noexcept;

  [[gnu::pure]]
  GeoPoint GetCenter() const noexcept;

  /**
   * Returns a scaled version of the GeoBounds.
   * The bounds are scaled around the center by the given factor.
   * @param factor The scaling factor
   * @return A scaled version of the GeoBounds
   */
  [[gnu::pure]]
  GeoBounds Scale(double factor) const noexcept;
};
