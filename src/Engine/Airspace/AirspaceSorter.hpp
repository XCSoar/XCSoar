#ifndef AIRSPACE_SORTER_HPP
#define AIRSPACE_SORTER_HPP

#include "Ptr.hpp"
#include "Geo/GeoVector.hpp"
#include "Airspace/AirspaceClass.hpp"

#include <tchar.h>
#include <vector>

struct GeoPoint;
class AbstractAirspace;
class Airspaces;
class FlatProjection;

/** Structure to hold Airspace sorting information */
class AirspaceSelectInfo
{
  /** Pointer to actual airspace (unprotected!) */
  ConstAirspacePtr airspace;

  /** From observer to waypoint */
  mutable GeoVector vec;

public:
  template<typename T>
  explicit AirspaceSelectInfo(T &&_airspace) noexcept
    :airspace(std::forward<T>(_airspace)), vec(GeoVector::Invalid()) {}

  const AbstractAirspace &GetAirspace() const noexcept {
    return *airspace;
  }

  const auto &GetAirspacePtr() const noexcept {
    return airspace;
  }

  void ResetVector() noexcept {
    vec.SetInvalid();
  }

  [[gnu::pure]]
  const GeoVector &GetVector(const GeoPoint &location,
                             const FlatProjection &projection) const noexcept;
};

using AirspaceSelectInfoVector = std::vector<AirspaceSelectInfo>;

struct AirspaceFilterData {
  /**
   * Show only airspaces of this class.  The special value
   * #AirspaceClass::AIRSPACECLASSCOUNT disables this filter.
   */
  AirspaceClass cls = AirspaceClass::AIRSPACECLASSCOUNT;

  /**
   * Show only airspaces with a name beginning with this string.
   */
  const TCHAR *name_prefix = nullptr;

  /**
   * Show only airspaces with a direction deviating less than 18
   * degrees from the aircraft.  A negative value disables this
   * filter.
   */
  Angle direction = Angle::Native(-1);

  /**
   * Show only airspaces less than this number of meters from the
   * aircraft.  A negative value disables this filter.
   */
  double distance = -1;

  [[gnu::pure]]
  bool Match(const GeoPoint &location,
             const FlatProjection &projection,
             const AbstractAirspace &as) const noexcept;
};

/**
 * Returns a filtered list of airspaces, sorted by name or distance.
 *
 * @param airspaces the airspace database
 * @param location location of aircraft at time of query
 */
[[gnu::pure]]
AirspaceSelectInfoVector
FilterAirspaces(const Airspaces &airspaces, const GeoPoint &location,
                const AirspaceFilterData &filter) noexcept;

#endif
