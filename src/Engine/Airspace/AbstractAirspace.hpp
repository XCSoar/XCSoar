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
#ifndef ABSTRACTAIRSPACE_HPP
#define ABSTRACTAIRSPACE_HPP

#include "Util/TriState.hpp"
#include "Util/tstring.hpp"
#include "AirspaceAltitude.hpp"
#include "AirspaceClass.hpp"
#include "AirspaceActivity.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/SearchPointVector.hpp"
#include "Compiler.h"

#ifdef DO_PRINT
#include <iostream>
#endif

#include <tchar.h>

struct AircraftState;
struct AltitudeState;
class AtmosphericPressure;
class AirspaceAircraftPerformance;
struct AirspaceInterceptSolution;
struct FlatBoundingBox;
class FlatProjection;
class AirspaceIntersectionVector;

/** Abstract base class for airspace regions */
class AbstractAirspace {
public:
  enum class Shape: uint8_t {
    CIRCLE,
    POLYGON,
  };

private:
  const Shape shape;

  /** Airspace class */
  AirspaceClass type;

protected:
  mutable TriState is_convex;
  mutable bool active;

  /** Base of airspace */
  AirspaceAltitude altitude_base;

  /** Top of airspace */
  AirspaceAltitude altitude_top;

  /** Airspace name (identifier) */
  tstring name;

  /** Radio frequency (optional) */
  tstring radio;

  /** Actual border */
  SearchPointVector m_border;

  /** Convex clearance border */
  mutable SearchPointVector m_clearance;

  AirspaceActivity days_of_operation;

public:
  AbstractAirspace(Shape _shape):shape(_shape), active(true) {}
  virtual ~AbstractAirspace();

  Shape GetShape() const {
    return shape;
  }

  /**
   * Compute bounding box enclosing the airspace.  Rounds up/down
   * so discretisation ensures bounding box is indeed enclosing.
   *
   * @param projection Projection used for flat-earth representation
   *
   * @return Enclosing bounding box
   */
  gcc_pure
  const FlatBoundingBox GetBoundingBox(const FlatProjection &projection);

  gcc_pure
  GeoBounds GetGeoBounds() const;

  /**
   * Get arbitrary center or reference point for use in determining
   * overall center location of all airspaces
   *
   * @return Location of reference point
   */
  gcc_pure
  virtual const GeoPoint GetReferenceLocation() const = 0;

  /**
   * Get geometric center of airspace.
   *
   * @return center
   */
  gcc_pure
  virtual const GeoPoint GetCenter() const = 0;

  /**
   * Checks whether an observer is inside the airspace (no altitude taken into account)
   * This is slow because it uses geodesic calculations
   *
   * @param loc State about which to test inclusion
   *
   * @return true if observer is inside airspace boundary
   */
  gcc_pure
  virtual bool Inside(const GeoPoint &loc) const = 0;

  /**
   * Checks whether an observer is inside the airspace altitude range.
   */
  gcc_pure
  bool Inside(const AltitudeState &state) const;

  /**
   * Checks whether an observer is inside the airspace (altitude is taken into account)
   * This calls inside(state.location) so can be slow.
   *
   * @param state State about which to test inclusion
   *
   * @return true if aircraft is inside airspace boundaries
   */
  gcc_pure
  bool Inside(const AircraftState &state) const;

  /**
   * Checks whether a line intersects with the airspace.
   * Can be approximate by using flat-earth representation internally.
   *
   * @param g1 Location of origin of search vector
   * @param end the end of the search vector
   *
   * @return Vector of intersection pairs if the line intersects the airspace
   */
  gcc_pure
  virtual AirspaceIntersectionVector Intersects(const GeoPoint &g1,
                                                const GeoPoint &end,
                                                const FlatProjection &projection) const = 0;

  /**
   * Find location of closest point on boundary to a reference
   *
   * @param loc Reference location of observer
   *
   * @return Location of closest point of boundary to reference
   */
  gcc_pure
  virtual GeoPoint ClosestPoint(const GeoPoint &loc,
                                const FlatProjection &projection) const = 0;

  /**
   * Set terrain altitude for AGL-referenced airspace altitudes
   *
   * @param alt Height above MSL of terrain (m) at center
   */
  void SetGroundLevel(double alt);

  /**
   * Is it necessary to call SetGroundLevel() for this AbstractAirspace?
   */
  bool NeedGroundLevel() const {
    return altitude_base.NeedGroundLevel() || altitude_top.NeedGroundLevel();
  }

  /**
   * Set QNH pressure for FL-referenced airspace altitudes
   *
   * @param press Atmospheric pressure model and QNH
   */
  void SetFlightLevel(const AtmosphericPressure &press);

  /**
   * Set activity based on day mask
   *
   * @param days Mask of activity
   */
  void SetActivity(const AirspaceActivity mask) const;

  /**
   * Set fundamental properties of airspace
   *
   * @param _Name Name of airspace
   * @param _Type Type/class
   * @param _base Lower limit
   * @param _top Upper limit
   */
  void SetProperties(tstring &&_name, const AirspaceClass _Type,
                     const AirspaceAltitude &_base,
                     const AirspaceAltitude &_top) {
    name = std::move(_name);
    type = _Type;
    altitude_base = _base;
    altitude_top = _top;
  }

  /**
   * Set radio frequency of airspace
   *
   * @param _Radio Radio frequency of airspace
   */
  void SetRadio(const tstring &_Radio) {
    radio = _Radio;
  }

  /**
   * Set activation setting of the airspace
   *
   * @param _active New activation setting of airspace
   */
  void SetDays(const AirspaceActivity mask) {
    days_of_operation = mask;
  }

  /**
   * Get type of airspace
   *
   * @return Type/class of airspace
   */
  AirspaceClass GetType() const {
    return type;
  }

  /**
   * Test whether base is at terrain level
   *
   * @return True if base is 0 AGL
   */
  bool IsBaseTerrain() const {
    return altitude_base.IsTerrain();
  }

  const AirspaceAltitude &GetBase() const { return altitude_base; }
  const AirspaceAltitude &GetTop() const { return altitude_top; }

  /**
   * Get base altitude
   *
   * @return Altitude AMSL (m) of base
   */
  double GetBaseAltitude(const AltitudeState &state) const {
    return altitude_base.GetAltitude(state);
  }

  /**
   * Get top altitude
   *
   * @return Altitude AMSL (m) of top
   */
  double GetTopAltitude(const AltitudeState &state) const {
    return altitude_top.GetAltitude(state);
  }

  /**
   * Find time/distance/height to airspace from an observer given a
   * simplified performance model and the boundary start/end points.  If
   * inside the airspace, this will give the time etc to exit (it cares
   * not about interior/exterior, only minimum time to reach the
   * specified location)
   *
   * @param state Aircraft state
   * @param perf Aircraft performance model
   * @param solution Solution of intercept (set if intercept possible, else untouched)
   * @param loc_start Location of first point on/in airspace to query (if provided)
   * @param loc_end Location of last point on/in airspace to query (if provided)
   * @return True if intercept found
   */
  gcc_pure
  AirspaceInterceptSolution Intercept(const AircraftState &state,
                                      const AirspaceAircraftPerformance &perf,
                                      const GeoPoint &loc_start,
                                      const GeoPoint &loc_end) const;

  /**
   * Find time/distance/height to airspace from an observer given a
   * simplified performance model and the aircraft path vector.  If
   * inside the airspace, this will give the time etc to exit (it cares
   * not about interior/exterior, only minimum time to reach the
   * specified location)
   *
   * @param state Aircraft state
   * @param end end point of aircraft path vector
   * @param perf Aircraft performance model
   * @param solution Solution of intercept (set if intercept possible, else untouched)
   * @return True if intercept found
   */
  gcc_pure
  AirspaceInterceptSolution Intercept(const AircraftState &state,
                                      const GeoPoint &end,
                                      const FlatProjection &projection,
                                      const AirspaceAircraftPerformance &perf) const;

#ifdef DO_PRINT
  friend std::ostream &operator<<(std::ostream &f,
                                  const AbstractAirspace &as);
#endif

  gcc_pure
  const TCHAR *GetName() const {
    return name.c_str();
  }

  /**
   * Returns true if the name begins with the specified string.
   */
  gcc_pure
  bool MatchNamePrefix(const TCHAR *prefix) const;

  /**
   * Produce text version of radio frequency.
   *
   * @return Text version of radio frequency
   */
  gcc_pure
  const tstring &GetRadioText() const {
    return radio;
  }

  /**
   * Accessor for airspace shape
   *
   * For polygon airspaces, this is the actual boundary,
   * for circle airspaces, this is a simplified shape
   *
   * @return border of airspace
   */
  const SearchPointVector &GetPoints() const {
    return m_border;
  }

  /**
   * On-demand access of clearance border.  Generated on call,
   * to deallocate, call clear_clearance().  Uses mutable object
   * and const methods to allow visitors to generate them on demand
   * from within a visit method.
   */
  gcc_pure
  const SearchPointVector &GetClearance(const FlatProjection &projection) const;
  void ClearClearance() const;

  gcc_pure
  bool IsActive() const {
    return active;
  }

protected:
  /** Project border */
  void Project(const FlatProjection &tp);

private:
  /**
   * Find time/distance to specified point on the boundary from an observer
   * given a simplified performance model.  If inside the airspace, this will
   * give the time etc to exit (it cares not about interior/exterior, only minimum
   * time to reach the specified location)
   *
   * @param state Aircraft state
   * @param perf Aircraft performance model
   * @param distance Distance from aircraft to boundary
   * @return Solution of intercept
   */
  gcc_pure
  AirspaceInterceptSolution InterceptVertical(const AircraftState &state,
                                              const AirspaceAircraftPerformance &perf,
                                              double distance) const;

  /**
   * Find time/distance to specified horizontal boundary from an observer
   * given a simplified performance model.  If inside the airspace, this will
   * give the time etc to exit (it cares not about interior/exterior, only minimum
   * time to reach the specified location)
   *
   * @param state Aircraft state
   * @param perf Aircraft performance model
   * @param distance_start Distance from aircraft to start boundary
   * @param distance_end Distance from aircraft to end boundary
   * @param lower If true, examines lower boundary, otherwise upper boundary
   * @return Solution of intercept
   */
  gcc_pure
  AirspaceInterceptSolution InterceptHorizontal(const AircraftState &state,
                                                const AirspaceAircraftPerformance &perf,
                                                double distance_start,
                                                double distance_end,
                                                const bool lower = true) const;
};

#endif
