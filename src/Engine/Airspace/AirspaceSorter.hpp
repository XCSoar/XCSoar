#ifndef AIRSPACE_SORTER_HPP
#define AIRSPACE_SORTER_HPP

#include "Geo/GeoVector.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <vector>

class AbstractAirspace;
class Airspaces;
struct GeoPoint;
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

/**
 * Utility class to manage sorting of waypoints (e.g. for dlgWaypointSelect)
 * Note that distance queries do not yet make use of the kdtree, but this system
 * keeps a local master list so won't need to lock the waypoints for long.
 */
class AirspaceSorter
{
  AirspaceSelectInfoVector m_airspaces_all;

  const TaskProjection &projection;
  const GeoPoint &location;

public:
  /**
   * Constructor.  Sorts master list of waypoints by name
   *
   * @param _airspaces Airspaces store
   * @param Location Location of aircraft at time of query
   * @param distance_factor Units factor to apply to distance calculations
   */
  AirspaceSorter(const Airspaces &_airspaces,
                 const GeoPoint &Location);

  /**
   * Return master list
   *
   * @return Master list
   */
  gcc_pure
  const AirspaceSelectInfoVector& GetList() const;

  /**
   * Remove airspaces not of specified class
   *
   * @param vec List of airspaces to filter (read-write)
   * @param t Class of airspace to match
   */
  void FilterByClass(AirspaceSelectInfoVector& vec, const AirspaceClass t) const;

  /**
   * Remove airspaces not matching the specifid name prefix
   *
   * @param v List of airspaces to filter (read-write)
   * @param prefix the name prefix
   */
  void FilterByNamePrefix(AirspaceSelectInfoVector &v,
                          const TCHAR *prefix) const;

  /**
   * Remove airspaces bearing greater than 18 degrees from supplied direction
   *
   * @param vec List of airspaces to filter (read-write)
   * @param direction Bearing (degrees) of desired direction
   */
  void FilterByDirection(AirspaceSelectInfoVector& vec, const Angle direction) const;

  /**
   * Remove airspaces further than desired direction
   *
   * @param vec List of airspaces to filter (read-write)
   * @param distance Distance (user units) of limit
   */
  void FilterByDistance(AirspaceSelectInfoVector& vec, const fixed distance) const;

  /**
   * Sort airspaces by distance
   *
   * @param vec List of airspaces to sort (read-write)
   */
  void SortByDistance(AirspaceSelectInfoVector& vec) const;

  /**
   * Sort airspaces alphabetically
   *
   * @param vec List of airspaces to sort (read-write)
   */
  void SortByName(AirspaceSelectInfoVector& vec) const;
};

#endif
