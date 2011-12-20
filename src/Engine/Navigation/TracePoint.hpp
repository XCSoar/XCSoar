#ifndef TRACE_POINT_HPP
#define TRACE_POINT_HPP

#include "Util/TypeTraits.hpp"
#include "SearchPoint.hpp"
#include "Rough/RoughAltitude.hpp"
#include "Compiler.h"

#include <assert.h>
#include <vector>

struct AircraftState;

/**
 * Class for points used in traces (snail trail, OLC scans)
 * Internally, keeps track of predecessors as a kind of a linked-list
 */
class TracePoint : public SearchPoint
{
  /** Time of sample */
  unsigned time;
  /**
   * Thermal drift factor:
   * 256 indicates drift rate equal to wind speed
   * 0 indicates no drift.
   */
  unsigned short drift_factor;

  /**
   * The NavAltitude [m].
   */
  RoughAltitude altitude;

  /**
   * The NettoVario value [m/256s].  This is a "short" fixed-point
   * integer to save memory.
   */
  signed short vario;

public:
  /**
   * Non-initialising constructor.
   */
  TracePoint() = default;

  /**
   * Constructor for a TracePoint which is only used as parameter to
   * TraceTree::find_within_range().  It initializes only the
   * SearchPoint base class.
   */
  TracePoint(const GeoPoint &location):
    SearchPoint(location) {}

  /**
   * Constructor for actual trace points
   *
   * @param state State of aircraft
   * @param tp Projection used internally
   *
   * @return Initialised object
   */
  TracePoint(const AircraftState &state);

  void Clear() {
    time = (unsigned)(0 - 1);
  }

  bool IsDefined() const {
    return time != (unsigned)(0 - 1);
  }

  unsigned GetTime() const {
    return time;
  }

  bool IsOlderThan(const TracePoint &other) const {
    return time < other.time;
  }

  bool IsNewerThan(const TracePoint &other) const {
    return time > other.time;
  }

  unsigned DeltaTime(const TracePoint &previous) const {
    assert(!IsOlderThan(previous));

    return time - previous.time;
  }

  fixed CalculateDrift(fixed now) const {
    const fixed dt = now - fixed(time);
    return dt * drift_factor / 256;
  }

  fixed GetAltitude() const {
    return altitude;
  }

  /**
   * Returns the altitude as an integer.  Some calculations may not
   * need the fractional part.
   */
  int GetIntegerAltitude() const {
    return (int)altitude;
  }

  fixed GetVario() const {
    return fixed(vario) / 256;
  }

  /**
   * Structure for STL sorting by time
   */
  struct time_sort {
    /** 
     * Comparison operator
     * 
     * @return True if s1 is earlier than s2
     */
    bool operator()(const TracePoint& s1, const TracePoint& s2) {
      return s1.time < s2.time;
    }
  };

  /** 
   * Test match based on time (since time of a sample must be unique)
   * 
   * @param a Point to compare to
   * 
   * @return True if time matches
   */
  bool operator==(TracePoint const &a) const {
    return time == a.time; 
  }
};

static_assert(is_trivial_ndebug<TracePoint>::value, "type is not trivial");

class TracePointVector : public std::vector<TracePoint> {
};

class TaskProjection;

gcc_pure
TaskProjection get_bounds(const TracePointVector& vec,
                          const GeoPoint &fallback_location);

#endif

