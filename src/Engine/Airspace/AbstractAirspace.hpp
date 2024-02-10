// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "util/TriState.hpp"
#include "util/tstring.hpp"
#include "AirspaceAltitude.hpp"
#include "AirspaceClass.hpp"
#include "AirspaceActivity.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/SearchPointVector.hpp"
#include "RadioFrequency.hpp"

#ifdef DO_PRINT
#include <iosfwd>
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
  AirspaceClass asclass;

protected:
  mutable TriState is_convex;
  mutable bool active;

  /** Base of airspace */
  AirspaceAltitude altitude_base;

  /** Top of airspace */
  AirspaceAltitude altitude_top;

  /** Airspace name (identifier) */
  tstring name;

  /** Airspace type */
  AirspaceClass astype;

  /** Radio frequency (optional) */
  RadioFrequency radio_frequency = RadioFrequency::Null();

  /** Actual border */
  SearchPointVector m_border;

  /** Convex clearance border */
  mutable SearchPointVector m_clearance;

  AirspaceActivity days_of_operation;

public:
  AbstractAirspace(Shape _shape) noexcept:shape(_shape), active(true) {}
  virtual ~AbstractAirspace() noexcept;

  Shape GetShape() const noexcept {
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
  [[gnu::pure]]
  const FlatBoundingBox GetBoundingBox(const FlatProjection &projection) noexcept;

  [[gnu::pure]]
  GeoBounds GetGeoBounds() const noexcept;

  /**
   * Get arbitrary center or reference point for use in determining
   * overall center location of all airspaces
   *
   * @return Location of reference point
   */
  [[gnu::pure]]
  virtual const GeoPoint GetReferenceLocation() const noexcept = 0;

  /**
   * Get geometric center of airspace.
   *
   * @return center
   */
  [[gnu::pure]]
  virtual const GeoPoint GetCenter() const noexcept = 0;

  /**
   * Checks whether an observer is inside the airspace (no altitude taken into account)
   * This is slow because it uses geodesic calculations
   *
   * @param loc State about which to test inclusion
   *
   * @return true if observer is inside airspace boundary
   */
  [[gnu::pure]]
  virtual bool Inside(const GeoPoint &loc) const noexcept = 0;

  /**
   * Checks whether an observer is inside the airspace altitude range.
   */
  [[gnu::pure]]
  bool Inside(const AltitudeState &state) const noexcept;

  /**
   * Checks whether an observer is inside the airspace (altitude is taken into account)
   * This calls inside(state.location) so can be slow.
   *
   * @param state State about which to test inclusion
   *
   * @return true if aircraft is inside airspace boundaries
   */
  [[gnu::pure]]
  bool Inside(const AircraftState &state) const noexcept;

  /**
   * Checks whether a line intersects with the airspace.
   * Can be approximate by using flat-earth representation internally.
   *
   * @param g1 Location of origin of search vector
   * @param end the end of the search vector
   *
   * @return Vector of intersection pairs if the line intersects the airspace
   */
  [[gnu::pure]]
  virtual AirspaceIntersectionVector Intersects(const GeoPoint &g1,
                                                const GeoPoint &end,
                                                const FlatProjection &projection) const noexcept = 0;

  /**
   * Find location of closest point on boundary to a reference
   *
   * @param loc Reference location of observer
   *
   * @return Location of closest point of boundary to reference
   */
  [[gnu::pure]]
  virtual GeoPoint ClosestPoint(const GeoPoint &loc,
                                const FlatProjection &projection) const noexcept = 0;

  /**
   * Set terrain altitude for AGL-referenced airspace altitudes
   *
   * @param alt Height above MSL of terrain (m) at center
   */
  void SetGroundLevel(double alt) noexcept;

  /**
   * Is it necessary to call SetGroundLevel() for this AbstractAirspace?
   */
  bool NeedGroundLevel() const noexcept {
    return altitude_base.NeedGroundLevel() || altitude_top.NeedGroundLevel();
  }

  /**
   * Set QNH pressure for FL-referenced airspace altitudes
   *
   * @param press Atmospheric pressure model and QNH
   */
  void SetFlightLevel(AtmosphericPressure press) noexcept;

  /**
   * Set activity based on day mask
   *
   * @param days Mask of activity
   */
  void SetActivity(const AirspaceActivity mask) const noexcept;

  /**
   * Set fundamental properties of airspace
   *
   * @param _Name Name of airspace
   * @param _classs Class
   * @param _type Type
   * @param _base Lower limit
   * @param _top Upper limit
   */
  void SetProperties(tstring &&_name, const AirspaceClass _class,
                     const AirspaceClass _type,
                     const AirspaceAltitude &_base,
                     const AirspaceAltitude &_top) noexcept {
    name = std::move(_name);
    asclass = _class;
    astype = _type;
    altitude_base = _base;
    altitude_top = _top;
  }

  /**
   * Set radio frequency of airspace
   *
   * @param _Radio Radio frequency of airspace
   */
  void SetRadioFrequency(RadioFrequency _radio) noexcept {
    radio_frequency = _radio;
  }

  /**
   * Set activation setting of the airspace
   *
   * @param _active New activation setting of airspace
   */
  void SetDays(const AirspaceActivity mask) noexcept {
    days_of_operation = mask;
  }

  /**
   * Get asclass of airspace
   *
   * @return Class of airspace
   */
  AirspaceClass GetClass() const noexcept {
    return asclass;
  }

  /**
   * Get Type of airspace
   *
   * @return Type of airspace
   */
  AirspaceClass GetType() const noexcept {
    return astype;
  }

  /**
   * Test whether base is at terrain level
   *
   * @return True if base is 0 AGL
   */
  bool IsBaseTerrain() const noexcept {
    return altitude_base.IsTerrain();
  }

  const AirspaceAltitude &GetBase() const noexcept { return altitude_base; }
  const AirspaceAltitude &GetTop() const noexcept { return altitude_top; }

  /**
   * Get base altitude
   *
   * @return Altitude AMSL (m) of base
   */
  double GetBaseAltitude(const AltitudeState &state) const noexcept {
    return altitude_base.GetAltitude(state);
  }

  /**
   * Get top altitude
   *
   * @return Altitude AMSL (m) of top
   */
  double GetTopAltitude(const AltitudeState &state) const noexcept {
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
  [[gnu::pure]]
  AirspaceInterceptSolution Intercept(const AircraftState &state,
                                      const AirspaceAircraftPerformance &perf,
                                      const GeoPoint &loc_start,
                                      const GeoPoint &loc_end) const noexcept;

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
  [[gnu::pure]]
  AirspaceInterceptSolution Intercept(const AircraftState &state,
                                      const GeoPoint &end,
                                      const FlatProjection &projection,
                                      const AirspaceAircraftPerformance &perf) const noexcept;

#ifdef DO_PRINT
  friend std::ostream &operator<<(std::ostream &f,
                                  const AbstractAirspace &as);
#endif

  [[gnu::pure]]
  const TCHAR *GetName() const noexcept {
    return name.c_str();
  }

  /**
   * Returns true if the name begins with the specified string.
   */
  [[gnu::pure]]
  bool MatchNamePrefix(const TCHAR *prefix) const noexcept;

  [[gnu::pure]]
  RadioFrequency GetRadioFrequency() const noexcept {
    return radio_frequency;
  }

  /**
   * Accessor for airspace shape
   *
   * For polygon airspaces, this is the actual boundary,
   * for circle airspaces, this is a simplified shape
   *
   * @return border of airspace
   */
  const SearchPointVector &GetPoints() const noexcept {
    return m_border;
  }

  /**
   * On-demand access of clearance border.  Generated on call,
   * to deallocate, call clear_clearance().  Uses mutable object
   * and const methods to allow visitors to generate them on demand
   * from within a visit method.
   */
  [[gnu::pure]]
  const SearchPointVector &GetClearance(const FlatProjection &projection) const noexcept;
  void ClearClearance() const noexcept;

  [[gnu::pure]]
  bool IsActive() const noexcept {
    return active;
  }

protected:
  /** Project border */
  void Project(const FlatProjection &tp) noexcept;

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
  [[gnu::pure]]
  AirspaceInterceptSolution InterceptVertical(const AircraftState &state,
                                              const AirspaceAircraftPerformance &perf,
                                              double distance) const noexcept;

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
  [[gnu::pure]]
  AirspaceInterceptSolution InterceptHorizontal(const AircraftState &state,
                                                const AirspaceAircraftPerformance &perf,
                                                double distance_start,
                                                double distance_end,
                                                bool lower = true) const noexcept;
};
