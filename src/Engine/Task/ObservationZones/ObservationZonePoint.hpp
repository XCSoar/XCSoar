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

#ifndef OBSERVATIONZONEPOINT_HPP
#define OBSERVATIONZONEPOINT_HPP

#include "ObservationZone.hpp"
#include "Geo/GeoPoint.hpp"

/**
 * \todo 
 * - add arc type for future use
 */
class ObservationZonePoint : public ObservationZone {
  GeoPoint reference;

protected:
  ObservationZonePoint(const ObservationZonePoint &other,
                       const GeoPoint &_reference)
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
  ObservationZonePoint(Shape _shape, bool _can_start_through_top,
                       const GeoPoint & _location)
    :ObservationZone(_shape, _can_start_through_top),
     reference(_location) {}

  /**
   * Update geometry when previous/next legs are modified.
   *
   * @param previous Previous task point (origin of inbound leg)
   * @param next Following task point (destination of outbound leg)
   */
  virtual void SetLegs(gcc_unused const GeoPoint *previous,
                       gcc_unused const GeoPoint *next) {}

  /**
   * Test whether an OZ is equivalent to this one
   *
   * @param other OZ to compare to
   *
   * @return True if same location and OZ
   */
  virtual bool Equals(const ObservationZonePoint &other) const;

  /**
   * Generate a random location inside the OZ (to be used for testing)
   *
   * @param mag proportional magnitude of error from center (0-1)
   *
   * @return Location of point
   */
  virtual GeoPoint GetRandomPointInSector(double mag) const = 0;

  /**
   * Clone this object with optional shift
   *
   * @param _location New location, or if NULL, uses object's location
   * @return Cloned object
   */
  virtual ObservationZonePoint *Clone(const GeoPoint &_reference) const = 0;

  ObservationZonePoint *Clone() const {
    return Clone(GetReference());
  }

  /**
   * The actual location
   */
  const GeoPoint &GetReference() const {
    return reference;
  }

protected:
  /**
   * distance from this to the reference
   */
  double DistanceTo(const GeoPoint &ref) const {
    return reference.Distance(ref);
  }
};

#endif
