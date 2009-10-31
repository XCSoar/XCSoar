#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "GlideSolvers/GlideResult.hpp"
#include <iostream>

struct AIRCRAFT_STATE;

class ElementStat;

class DistanceStat
{
public:
  DistanceStat():
    distance(0.0),
    distance_last(0.0),
    speed(0.0) {};

  void set_distance(const double d) {
    distance = d;
  }
  void print(std::ostream &f) const;

  double get_speed() const {
    return speed;
  };
  double get_speed_incremental() const {
    return speed_incremental;
  };
  double get_distance() const {
    return distance;
  };

  virtual void calc_speed(const ElementStat* es) = 0;
  virtual void calc_incremental_speed(const double dt);
protected:
  double distance;
  double speed;
  double speed_incremental;
  double distance_last;
};

class DistanceRemainingStat:
  public DistanceStat
{
public:
  virtual void calc_speed(const ElementStat* es);
};

class DistancePlannedStat:
  public DistanceStat
{
public:
  virtual void calc_speed(const ElementStat* es);
};

class DistanceTravelledStat:
  public DistanceStat
{
public:
  virtual void calc_speed(const ElementStat* es);
  virtual void calc_incremental_speed(const double dt);
};

class ElementStat
{
public:
  ElementStat():
    TimeStarted(-1.0),
    TimeElapsed(0.0),
    TimeRemaining(0.0),
    TimePlanned(0.0),
    gradient(0.0),
    initialised(false)
    {
    };

  double TimeStarted;
  double TimeElapsed;
  double TimeRemaining;
  double TimePlanned;
  double gradient;

  DistanceRemainingStat remaining_effective;
  DistanceRemainingStat remaining;
  DistancePlannedStat planned;
  DistanceTravelledStat travelled;

  GLIDE_RESULT solution_planned;
  GLIDE_RESULT solution_travelled;
  GLIDE_RESULT solution_remaining;

  void set_times(const double ts, 
                 const AIRCRAFT_STATE& state);
  void calc_speeds(const double dt);
  void print(std::ostream &f) const;
  void reset();
private:
  bool initialised;
};



class TaskStats 
{
public:
  TaskStats():
    cruise_efficiency(1.0),
    glide_required(0.0),
    mc_best(0.0),
    distance_nominal(0.0),
    distance_max(0.0),
    distance_min(0.0),
    distance_scored(0.0)
    {};

  ElementStat total;
  ElementStat current_leg;

  // calculated values
  double glide_required;
  double cruise_efficiency;
  double mc_best;

  double distance_nominal;
  double distance_max;
  double distance_min;
  double distance_scored;

  void print(std::ostream &f) const;
  void reset();
};


#endif
