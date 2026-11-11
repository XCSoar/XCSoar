// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/TriState.hpp"
#include "Origin.hpp"
#include "time/BrokenDateTime.hpp"
#include "AirspaceAltitude.hpp"
#include "AirspaceClass.hpp"
#include "AirspaceActivity.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/SearchPointVector.hpp"
#include "Radio/RadioFrequency.hpp"
#include "Radio/TransponderCode.hpp"

#ifdef DO_PRINT
#include <iosfwd>
#endif
#include <string>
#include <vector>

struct AircraftState;
struct AltitudeState;
class AtmosphericPressure;
class AirspaceAircraftPerformance;
struct AirspaceInterceptSolution;
struct FlatBoundingBox;
class FlatProjection;
class AirspaceIntersectionVector;

/**
 * One OpenAir AA period. An implausible bound is open
 * (`AA NONE/end` or `AA start/NONE`). The end instant is excluded.
 */
struct AirspaceActivationInterval {
  BrokenDateTime start = BrokenDateTime::Invalid();
  BrokenDateTime end = BrokenDateTime::Invalid();

  [[nodiscard]]
  constexpr bool Contains(BrokenDateTime now) const noexcept {
    if (start.IsPlausible() && now < start)
      return false;
    if (end.IsPlausible() && !(now < end))
      return false;
    return true;
  }
};

[[nodiscard]]
inline const AirspaceActivationInterval *
FindActivation(const std::vector<AirspaceActivationInterval> &intervals,
               BrokenDateTime now) noexcept
{
  for (const auto &interval : intervals)
    if (interval.Contains(now))
      return &interval;

  return nullptr;
}

/**
 * Weekly window, for example `AA MON-FRI 07:00Z/15:00Z`.
 * `days == 0` means every day. Bit 0 is Sunday.
 * Minutes after UTC midnight. 24*60 is 24:00.
 * An end at or before the start finishes the next day.
 */
struct AirspaceWeekdayPeriod {
  uint8_t days = 0;
  uint16_t start_minute = 0;
  uint16_t end_minute = 0;
};

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
  std::string name;

  /** Meaningful when has_activation_time is true. */
  bool has_activation_time = false;
  /** `AA NONE`: loaded, never shown as active. */
  bool never_active = false;
  /** A bad AA must not hide the airspace. */
  bool activation_unreadable = false;
  /**
   * shortcut: 64 AA lines per airspace. More than that is stored as
   * unreadable, which stays active.
   */
  std::vector<AirspaceActivationInterval> activation_intervals;
  std::vector<AirspaceWeekdayPeriod> activation_weekdays;

  /** Airspace type */
  AirspaceClass astype;

  /** Where this airspace was loaded from. */
  AirspaceOrigin origin = AirspaceOrigin::FILE;

  /** Airspace Station name */
  std::string station_name;

  /** Radio frequency (optional) */
  RadioFrequency radio_frequency = RadioFrequency::Null();

  /** Transponder code (optional) */
  TransponderCode transponder_code = TransponderCode::Null();

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
   * Distance from a reference location to the airspace boundary,
   * valid for points inside and outside the airspace.  For polygons
   * this uses the integer-projected nearest boundary point (accurate
   * to about one projection grid cell); for circles it is exact.
   * Note the difference to ClosestPoint(), which for circles returns
   * the reference location itself when it is inside.
   *
   * @param loc Reference location of observer
   *
   * @return Distance (m) to the closest point of the boundary
   */
  [[gnu::pure]]
  virtual double DistanceToBoundary(const GeoPoint &loc,
                                    const FlatProjection &projection) const noexcept;

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
   * Set fundamental properties of airspace.
   * AA periods are attached afterwards with SetActivationSchedule().
   */
  void SetProperties(std::string &&_name,
                     bool _has_activation_time, bool _never_active,
                     std::string &&_station_name,
                     TransponderCode &&_transponder_code,
                     const AirspaceClass _class, const AirspaceClass _type,
                     const AirspaceAltitude &_base,
                     const AirspaceAltitude &_top) noexcept
  {
    has_activation_time = _has_activation_time;
    never_active = _has_activation_time && _never_active;
    activation_unreadable = false;
    activation_intervals.clear();
    activation_weekdays.clear();
    name = std::move(_name);
    station_name = std::move(_station_name);
    transponder_code = std::move(_transponder_code);
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
   * Set transponder code of airspace
   *
   * @param _code Radio frequency of airspace
   */
  void SetTransponderCode(TransponderCode _code) noexcept
  {
    transponder_code = _code;
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
   * OpenAir AA periods. Also refreshes active from the current UTC time.
   */
  void SetActivationSchedule(
      std::vector<AirspaceActivationInterval> intervals,
      std::vector<AirspaceWeekdayPeriod> weekdays,
      bool unreadable) noexcept {
    activation_intervals = std::move(intervals);
    activation_weekdays = std::move(weekdays);
    activation_unreadable = unreadable;
    active = IsActiveToday();
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

  void SetOrigin(AirspaceOrigin _origin) noexcept {
    origin = _origin;
  }

  AirspaceOrigin GetOrigin() const noexcept {
    return origin;
  }

  /**
    * Returns the airspace class type. If GetType() is AirspaceClass::OTHER,
    * returns GetClass(), otherwise returns GetType()
    *
    * @return  AirspaceClass - The determined airspace class type
    */
  AirspaceClass GetTypeOrClass() const noexcept {
    return GetType() == AirspaceClass::OTHER ? GetClass() : GetType();
  }

  /**
    * Returns the airspace type. If GetClass() is AirspaceClass::UNCLASSIFIED,
    * returns GetType(), otherwise returns GetClass()
    *
    * @return  AirspaceClass - The determined airspace class or type
    */
  AirspaceClass GetClassOrType() const noexcept {
    return GetClass() == AirspaceClass::UNCLASSIFIED ? GetType() : GetClass();
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
  const char *GetName() const noexcept {
    return name.c_str();
  }

  [[gnu::pure]]
  bool HasActivationTime() const noexcept {
    return has_activation_time;
  }

  [[nodiscard]]
  BrokenDateTime GetActivationStartTime(
      BrokenDateTime now = BrokenDateTime::NowUTC()) const noexcept;

  [[nodiscard]]
  BrokenDateTime GetActivationEndTime(
      BrokenDateTime now = BrokenDateTime::NowUTC()) const noexcept;

  [[nodiscard]]
  std::size_t GetActivationIntervalCount() const noexcept {
    return activation_intervals.size();
  }

  [[nodiscard]]
  AirspaceActivationInterval
  GetActivationInterval(std::size_t i) const noexcept {
    return activation_intervals[i];
  }

  [[gnu::pure]]
  bool IsNeverActive() const noexcept {
    return never_active;
  }

  /**
   * True when the airspace is on at `now` (UTC).
   * No AA line means on. `AA NONE` means off. An AA line that did not
   * parse means on, so a typo or a sun event cannot hide it.
   */
  [[nodiscard]]
  bool IsActiveAt(BrokenDateTime now) const noexcept;

  /** @see IsActiveAt() at the current UTC time. Not the local weekday. */
  [[nodiscard]]
  bool IsActiveToday() const noexcept;

  [[gnu::pure]]
  const char *GetStationName() const noexcept {
    return station_name.c_str();
  }

   /**
   * Returns true if the name begins with the specified string.
   */
  [[gnu::pure]]
  bool MatchNamePrefix(const char *prefix) const noexcept;

  [[gnu::pure]]
  RadioFrequency GetRadioFrequency() const noexcept {
    return radio_frequency;
  }

  [[gnu::pure]] TransponderCode GetTransponderCode() const noexcept
  {
    return transponder_code;
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
