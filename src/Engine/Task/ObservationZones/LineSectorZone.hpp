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

#ifndef LINESECTORZONE_HPP
#define LINESECTORZONE_HPP

#include "SymmetricSectorZone.hpp"

/**
 * Observation zone represented as a line.
 * Tests for inSector return true if the subject is behind the line
 * (within a semi-circle of diameter equal to the line length).
 * The constraint test ensures transitioning to exit only occurs if the
 * line is crossed (rather than exiting from the back semi-circle).
 */
class LineSectorZone: public SymmetricSectorZone
{
  LineSectorZone(const LineSectorZone &other, const GeoPoint &reference)
    :SymmetricSectorZone((const SymmetricSectorZone &)other, reference) {}

public:
  /**
   * Constructor
   *
   * @param loc Location of center point of line
   * @param length Length of line (m)
   *
   * @return Initialised object
   */
  LineSectorZone(const GeoPoint loc, const double length = 1000.0)
    :SymmetricSectorZone(Shape::LINE, false, false, loc,
                         length / 2, Angle::HalfCircle())
  {
    UpdateSector();
  }

  /**
   * Set length property
   *
   * @param new_length Length (m) of line
   */
  void SetLength(const double new_length) {
    SetRadius(new_length / 2);
  }
  
  /**
   * Get length property value
   *
   * @return Length (m) of line
   */
  double GetLength() const {
    return 2 * GetRadius();
  }

  /* virtual methods from class ObservationZone */
  bool TransitionConstraint(const GeoPoint &location,
                            const GeoPoint &last_location) const override {
    return CylinderZone::IsInSector(location) &&
      CylinderZone::IsInSector(last_location);
  }

  double ScoreAdjustment() const override;

  /* virtual methods from class ObservationZonePoint */
  ObservationZonePoint *Clone(const GeoPoint &_reference) const override {
    return new LineSectorZone(*this, _reference);
  }
};

#endif
