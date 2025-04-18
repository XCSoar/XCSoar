// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Ptr.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"

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
  AirspacePtr airspace;

public:

  /**
   * Constructor for actual airspaces.
   *
   * @param airspace actual concrete airspace to create an envelope for
   * @param projection projection to be used for flat-earth representation
   *
   * @return airspace letter inside envelope suitable for insertion in a search structure
   */
  Airspace(AirspacePtr _airspace,
           const FlatProjection &projection) noexcept;

  /**
   * Checks whether an aircraft is inside the airspace.
   *
   * @param loc Location to check for enclosure
   *
   * @return true if aircraft is inside airspace
   */
  [[gnu::pure]]
  bool IsInside(const AircraftState &loc) const noexcept;

  /**
   * Checks whether a point is inside the airspace lateral boundary.
   *
   * @param loc Location to check for enclosure
   *
   * @return true if location is inside airspace
   */
  [[gnu::pure]]
  bool IsInside(const GeoPoint &loc) const noexcept;

  /**
   * Checks whether a line intersects with the airspace, by directing
   * the query to the enclosed concrete airspace.
   *
   * @param g1 Location of origin of search vector
   * @param end the end of the search vector
   *
   * @return true if the line intersects the airspace
   */
  [[gnu::pure]]
  AirspaceIntersectionVector Intersects(const GeoPoint &g1,
                                        const GeoPoint &end,
                                        const FlatProjection &projection) const noexcept;

  /**
   * Accessor for contained AbstractAirspace
   *
   * @return Airspace letter
   */
  AbstractAirspace &GetAirspace() const noexcept {
    return *airspace;
  };

  // TODO change to ConstAirspacePtr
  AirspacePtr GetAirspacePtr() const noexcept {
    return airspace;
  };

  /**
   * Set terrain altitude for AGL-referenced airspace altitudes
   *
   * @param alt Height above MSL of terrain (m) at center
   */
  void SetGroundLevel(double alt) const noexcept;

  /**
   * Is it necessary to call SetGroundLevel() for this AbstractAirspace?
   */
  [[gnu::pure]]
  bool NeedGroundLevel() const noexcept;

  /**
   * Set QNH pressure for FL-referenced airspace altitudes
   *
   * @param press Atmospheric pressure model and QNH
   */
  void SetFlightLevel(AtmosphericPressure press) const noexcept;

  /**
   * Set activity based on day mask
   *
   * @param days Mask of activity
   */
  void SetActivity(const AirspaceActivity mask) const noexcept;

  /**
   * Clear the convex clearance polygon
   */
  void ClearClearance() const noexcept;

  /**
   * Equality operator, matches if contained airspace is the same
   */
  bool operator==(Airspace const &a) const noexcept {
    return &GetAirspace() == &a.GetAirspace();
  }

public:
#ifdef DO_PRINT
  friend std::ostream &operator<<(std::ostream &f,
                                  const Airspace &ts);
#endif
};
