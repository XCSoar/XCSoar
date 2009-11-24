#ifndef DISTANCE_STAT_HPP
#define DISTANCE_STAT_HPP

#ifdef DO_PRINT
#include <iostream>
#endif

#include "Util/Filter.hpp"
#include "Util/AvFilter.hpp"
#include "Util/DiffFilter.hpp"

class ElementStat;

/**
 * Simple distance statistics with derived values (speed, incremental speed)
 * Incremental speeds track the short-term variation of distance with time,
 * whereas the overall speed is defined by the distance divided by a time value.
 */
class DistanceStat
{
public:
/** 
 * Constructor; initialises all to zero
 * 
 */
  DistanceStat(const bool is_positive=true);

/** 
 * Setter for distance value
 * 
 * @param d Distance value (m)
 */
  void set_distance(const double d) {
    distance = d;
  }

/** 
 * Accessor for distance value
 * 
 * @return Distance value (m)
 */
  double get_distance() const {
    return distance;
  };

/** 
 * Accessor for speed
 * 
 * @return Speed (m/s)
 */  double get_speed() const {
    return speed;
  };

/** 
 * Accessor for incremental speed (rate of change of
 * distance over dt, low-pass filtered)
 * 
 * @return Speed incremental (m/s)
 */
  double get_speed_incremental() const {
    return speed_incremental;
  };

/** 
 * Calculate bulk speed (distance/time), abstract base method
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es) = 0;

/** 
 * Calculate incremental speed from previous step.
 * Resets incremental speed to speed if dt=0
 * 
 * @param dt Time step (s)
 */
  virtual void calc_incremental_speed(const double dt);

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const DistanceStat& ds);
#endif

protected:
  double distance; /**< Distance (m) of metric */
  double speed; /**< Speed (m/s) of metric */
  double speed_incremental; /**< Incremental speed (m/s) of metric */
private:
  AvFilter av_dist;
  DiffFilter df;
  Filter v_lpf;
  const bool is_positive;
};

/**
 * Specialisation of DistanceStat for remaining distances
 */
class DistanceRemainingStat:
  public DistanceStat
{
public:
/** 
 * Calculate speed (distance remaining/time remaining)
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es);
};

/**
 * Specialisation of DistanceStat for planned distances
 */
class DistancePlannedStat:
  public DistanceStat
{
public:
/** 
 * Calculate speed (distance planned/time planned)
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es);
};

/**
 * Specialisation of DistanceStat for travelled distances
 */
class DistanceTravelledStat:
  public DistanceStat
{
public:
  DistanceTravelledStat();

/** 
 * Calculate speed (distance travelled/time elapsed)
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es);
};



#endif
