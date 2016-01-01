/* Copyright_License {

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
#ifndef AIRSPACE_HPP
#define AIRSPACE_HPP

#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Compiler.h"

#ifdef DO_PRINT
#include <iostream>
#endif

struct GeoPoint;
struct AircraftState;
class AtmosphericPressure;
class AbstractAirspace;
class AirspaceActivity;
class AirspaceIntersectionVector;
class FlatProjection;

/**
 * Single object container for actual airspaces, to be stored in Airspaces object
 * This class manages the bounding box of the actual airspace.
 *
 * This follows envelope-letter
 * idiom, in which the AbstractAirspace is the letter and this class
 * Airspace is an envelope, containing bounding box information for
 * use with high performance search structures.
 */
class Airspace final : public FlatBoundingBox
{
  AbstractAirspace *airspace;

public:

  /**
   * Constructor for actual airspaces.
   *
   * @param airspace actual concrete airspace to create an envelope for
   * @param projection projection to be used for flat-earth representation
   *
   * @return airspace letter inside envelope suitable for insertion in a search structure
   */
  Airspace(AbstractAirspace &airspace,
           const FlatProjection &projection);

  /**
   * Checks whether an aircraft is inside the airspace.
   *
   * @param loc Location to check for enclosure
   *
   * @return true if aircraft is inside airspace
   */
  gcc_pure
  bool IsInside(const AircraftState &loc) const;

  /**
   * Checks whether a point is inside the airspace lateral boundary.
   *
   * @param loc Location to check for enclosure
   *
   * @return true if location is inside airspace
   */
  gcc_pure
  bool IsInside(const GeoPoint &loc) const;

  /**
   * Checks whether a line intersects with the airspace, by directing
   * the query to the enclosed concrete airspace.
   *
   * @param g1 Location of origin of search vector
   * @param end the end of the search vector
   *
   * @return true if the line intersects the airspace
   */
  gcc_pure
  AirspaceIntersectionVector Intersects(const GeoPoint &g1,
                                        const GeoPoint &end,
                                        const FlatProjection &projection) const;

  /**
   * Destroys concrete airspace enclosed by this instance if present.
   * Note that this should not be called by clients but only by the
   * master store.  Many copies of this airspace may point to the same
   * concrete airspace so have to be careful here.
   *
   */
  void Destroy();

  /**
   * Accessor for contained AbstractAirspace
   *
   * @return Airspace letter
   */
  AbstractAirspace &GetAirspace() const {
    return *airspace;
  };

  /**
   * Set terrain altitude for AGL-referenced airspace altitudes
   *
   * @param alt Height above MSL of terrain (m) at center
   */
  void SetGroundLevel(double alt) const;

  /**
   * Is it necessary to call SetGroundLevel() for this AbstractAirspace?
   */
  gcc_pure
  bool NeedGroundLevel() const;

  /**
   * Set QNH pressure for FL-referenced airspace altitudes
   *
   * @param press Atmospheric pressure model and QNH
   */
  void SetFlightLevel(const AtmosphericPressure &press) const;

  /**
   * Set activity based on day mask
   *
   * @param days Mask of activity
   */
  void SetActivity(const AirspaceActivity mask) const;

  /**
   * Clear the convex clearance polygon
   */
  void ClearClearance() const;

  /**
   * Equality operator, matches if contained airspace is the same
   */
  bool operator==(Airspace const &a) const {
    return &GetAirspace() == &a.GetAirspace();
  }

public:
#ifdef DO_PRINT
  friend std::ostream &operator<<(std::ostream &f,
                                  const Airspace &ts);
#endif
};

#endif
