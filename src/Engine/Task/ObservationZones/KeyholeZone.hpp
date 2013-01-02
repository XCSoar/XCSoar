/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef KEYHOLEZONE_HPP
#define KEYHOLEZONE_HPP

#include "SymmetricSectorZone.hpp"

/**
 * A (default) 90 degree 10km sector centered at the bisector of
 * incoming/outgoing legs, with 500m cylinder
 */
class KeyholeZone: public SymmetricSectorZone
{
protected:
  /**
   * Constructor.
   *
   * @param loc Tip of sector location
   * @param radius Radius (m) of sector
   *
   * @return Initialised object
   */
  KeyholeZone(Shape _shape, const GeoPoint &loc,
              const fixed radius = fixed(10000.0),
              const Angle angle = Angle::QuarterCircle())
    :SymmetricSectorZone(_shape, loc, radius, angle) {}

  KeyholeZone(const KeyholeZone &other, const GeoPoint &reference)
    :SymmetricSectorZone((const SymmetricSectorZone &)other, reference) {}

public:  
  /** 
   * Constructor
   * 
   * @param loc Tip point of sector
   * @param radius Outer radius (m)
   * 
   * @return Initialised object
   */
  KeyholeZone(const GeoPoint loc, const fixed radius = fixed(10000.0))
    :SymmetricSectorZone(KEYHOLE, loc, radius, Angle::QuarterCircle())
  {
    UpdateSector();
  }

  /* virtual methods from class ObservationZone */
  virtual bool IsInSector(const GeoPoint &location) const;
  virtual GeoPoint GetBoundaryParametric(fixed t) const;
  virtual OZBoundary GetBoundary() const gcc_override;
  virtual fixed ScoreAdjustment() const;

  /* virtual methods from class ObservationZonePoint */
  virtual ObservationZonePoint *Clone(const GeoPoint &_reference) const {
    return new KeyholeZone(*this, _reference);
  }
};

#endif
