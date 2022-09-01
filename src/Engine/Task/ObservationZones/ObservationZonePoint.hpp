/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "ObservationZone.hpp"
#include "Geo/GeoPoint.hpp"

#include <memory>

/**
 * \todo 
 * - add arc type for future use
 */
class ObservationZonePoint : public ObservationZone {
  GeoPoint reference;

protected:
  constexpr ObservationZonePoint(const ObservationZonePoint &other,
                                 const GeoPoint &_reference) noexcept
    :ObservationZone(other.GetShape(), other.CanStartThroughTop()),
     reference(_reference) {}

public:
  /**
   * Constructor
   *
   * @param _location Location used as reference point for this OZ
   *
   * @return Initialised object
   */
  constexpr ObservationZonePoint(Shape _shape, bool _can_start_through_top,
                                 const GeoPoint & _location) noexcept
    :ObservationZone(_shape, _can_start_through_top),
     reference(_location) {}

  /**
   * Update geometry when previous/next legs are modified.
   *
   * @param previous Previous task point (origin of inbound leg)
   * @param next Following task point (destination of outbound leg)
   */
  virtual void SetLegs([[maybe_unused]] const GeoPoint *previous,
                       [[maybe_unused]] const GeoPoint *next) noexcept {}

  /**
   * Test whether an OZ is equivalent to this one
   *
   * @param other OZ to compare to
   *
   * @return True if same location and OZ
   */
  [[gnu::pure]]
  virtual bool Equals(const ObservationZonePoint &other) const noexcept;

  /**
   * Generate a random location inside the OZ (to be used for testing)
   *
   * @param mag proportional magnitude of error from center (0-1)
   *
   * @return Location of point
   */
  virtual GeoPoint GetRandomPointInSector(double mag) const noexcept = 0;

  /**
   * Clone this object with optional shift
   *
   * @param _location New location, or if NULL, uses object's location
   * @return Cloned object
   */
  virtual std::unique_ptr<ObservationZonePoint> Clone(const GeoPoint &_reference) const noexcept = 0;

  std::unique_ptr<ObservationZonePoint> Clone() const noexcept {
    return Clone(GetReference());
  }

  /**
   * The actual location
   */
  constexpr const GeoPoint &GetReference() const noexcept {
    return reference;
  }

protected:
  /**
   * distance from this to the reference
   */
  [[gnu::pure]]
  double DistanceTo(const GeoPoint &ref) const noexcept {
    return reference.Distance(ref);
  }
};
