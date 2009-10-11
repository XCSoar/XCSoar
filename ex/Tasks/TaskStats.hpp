#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "Navigation/Aircraft.hpp"
#include "GlideSolvers/MacCready.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

class ElementStat;

class DistanceStat
{
public:
  DistanceStat():
    distance(0.0),
    speed(0.0) {};

  double distance;
  double speed;
  void set_distance(const double d) {
    distance = d;
  }
  void print(std::ostream &f);
private:
  virtual void calc_speed(const ElementStat* es) = 0;
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
};

class ElementStat
{
public:
  ElementStat():
    TimeStarted(-1.0),
    TimeElapsed(0.0),
    TimeRemaining(0.0),
    TimePlanned(0.0)
    {
    };

  double TimeStarted;
  double TimeElapsed;
  double TimeRemaining;
  double TimePlanned;

  DistanceRemainingStat remaining_effective;
  DistanceRemainingStat remaining;
  DistancePlannedStat planned;
  DistanceTravelledStat travelled;

  GLIDE_RESULT solution_planned;
  GLIDE_RESULT solution_travelled;
  GLIDE_RESULT solution_remaining;

  void set_times(const double ts, 
                 const AIRCRAFT_STATE& state, 
                 const double tr) {
    TimeStarted = ts;
    TimeElapsed = std::max(state.Time-ts,0.0);
    TimeRemaining = tr;
    TimePlanned = TimeElapsed+tr;

    remaining_effective.calc_speed(this);
    remaining.calc_speed(this);
    planned.calc_speed(this);
    travelled.calc_speed(this);
  }

  void print(std::ostream &f);

};



class TaskStats 
{
public:
  ElementStat total;
  ElementStat current_leg;

  // calculated values
  double cruise_efficiency;
  double mc_best;

  void print(std::ostream &f);

};


#endif
