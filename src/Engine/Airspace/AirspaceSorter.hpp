#ifndef AIRSPACE_SORTER_HPP
#define AIRSPACE_SORTER_HPP

#include "Geo/GeoVector.hpp"
#include "Geo/GeoPoint.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Predicate/AirspacePredicate.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <vector>

class AbstractAirspace;
class Airspaces;
class TaskProjection;

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
                             const TaskProjection &projection) const;
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
  fixed distance;

  void Clear() {
    cls = AirspaceClass::AIRSPACECLASSCOUNT;
    name_prefix = nullptr;
    direction = Angle::Native(fixed(-1));
    distance = fixed(-1);
  }

  gcc_pure
  bool Match(const GeoPoint &location,
             const TaskProjection &projection,
             const AbstractAirspace &as) const;
};

class AirspaceFilterPredicate : public AirspacePredicate {
  GeoPoint location;
  const TaskProjection &projection;
  const AirspaceFilterData &filter;

public:
  AirspaceFilterPredicate(const GeoPoint &_location,
                          const TaskProjection &_projection,
                          const AirspaceFilterData &_filter)
    :location(_location), projection(_projection), filter(_filter) {}

  virtual bool operator()(const AbstractAirspace &as) const gcc_override;
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
