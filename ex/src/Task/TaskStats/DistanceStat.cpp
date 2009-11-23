#include "DistanceStat.hpp"
#include "TaskStats.hpp"

#define N_AV 3

DistanceStat::DistanceStat(const bool _is_positive):
  distance(0.0),
  speed(0.0),
  av_dist(N_AV),
  df(0.0),
  v_lpf(600.0/N_AV,false),
  is_positive(_is_positive)
{

}

void 
DistanceStat::calc_incremental_speed(const double dt)
{  
  if ((dt>0) && (distance>0)) {
    if (av_dist.update(distance)) {
      for (unsigned i=0; i<(unsigned)(dt); i++) {
        double d_av = av_dist.average();
        double v = df.update(d_av)/(N_AV);
        double v_f = v_lpf.update(v);
        speed_incremental = (is_positive? -v_f:v_f);
        av_dist.reset();
      }
    }
  } else {    
    df.reset(distance,(is_positive? -1:1)*speed*(N_AV));
    v_lpf.reset((is_positive? -1:1)*speed);
    speed_incremental = speed;
    av_dist.reset();
  }
}


DistanceTravelledStat::DistanceTravelledStat():
  DistanceStat(false)
{
}

void 
DistanceRemainingStat::calc_speed(const ElementStat* es) 
{
  if (es->TimeRemaining>0) {
    speed = (distance/es->TimeRemaining);
  } else {
    speed = 0;
  }
}

void 
DistancePlannedStat::calc_speed(const ElementStat* es) 
{
  if (es->TimePlanned>0) {
    speed = (distance/es->TimePlanned);
  } else {
    speed = 0;
  }
}

void 
DistanceTravelledStat::calc_speed(const ElementStat* es) 
{
  if (es->TimeElapsed>0) {
    speed = (distance/es->TimeElapsed);
  } else {
    speed = 0;
  }
}
