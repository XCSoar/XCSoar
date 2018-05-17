#ifndef AIRSPACE_SORTER_HPP
#define AIRSPACE_SORTER_HPP

#include "Geo/GeoVector.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Compiler.h"

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
  const AbstractAirspace *airspace;

  /** From observer to waypoint */
  mutable GeoVector vec;

public:
  AirspaceSelectInfo(const AbstractAirspace &_airspace)
    :airspace(&_airspace), vec(GeoVector::Invalid()) {}

  const AbstractAirspace &GetAirspace() const {
    return *airspace;
  }

  void ResetVector();

  gcc_pure
  const GeoVector &GetVector(const GeoPoint &location,
                             const FlatProjection &projection) const;
};

typedef std::vector<AirspaceSelectInfo> AirspaceSelectInfoVector;

struct AirspaceFilterData {
  /**
   * Show only airspaces of this class.  The special value
   * #AirspaceClass::AIRSPACECLASSCOUNT disables this filter.
   */
  AirspaceClass cls;

  /**
   * Show only airspaces with a name beginning with this string.
   */
  const TCHAR *name_prefix;

  /**
   * Show only airspaces with a direction deviating less than 18
   * degrees from the aircraft.  A negative value disables this
   * filter.
   */
  Angle direction;

  /**
   * Show only airspaces less than this number of meters from the
   * aircraft.  A negative value disables this filter.
   */
  double distance;

  void Clear() {
    cls = AirspaceClass::AIRSPACECLASSCOUNT;
    name_prefix = nullptr;
    direction = Angle::Native(-1);
    distance = -1;
  }

  gcc_pure
  bool Match(const GeoPoint &location,
             const FlatProjection &projection,
             const AbstractAirspace &as) const;
};

/**
 * Returns a filtered list of airspaces, sorted by name or distance.
 *
 * @param airspaces the airspace database
 * @param location location of aircraft at time of query
 */
gcc_pure
AirspaceSelectInfoVector
FilterAirspaces(const Airspaces &airspaces, const GeoPoint &location,
                const AirspaceFilterData &filter);

#endif
