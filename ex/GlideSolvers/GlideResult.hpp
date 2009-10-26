#ifndef GLIDERESULT_HPP
#define GLIDERESULT_HPP

#include <iostream>

struct GLIDE_STATE;

struct GLIDE_RESULT {

  enum GlideResult_t {
    RESULT_OK = 0,
    RESULT_PARTIAL,
    RESULT_WIND_EXCESSIVE,
    RESULT_MACCREADY_INSUFFICIENT,
    RESULT_NOSOLUTION
  };

  GLIDE_RESULT():
    Solution(RESULT_NOSOLUTION),
    TrackBearing(0.0),
    CruiseTrackBearing(0.0),
    Distance(0.0),
    VOpt(0.0),
    HeightClimb(0.0),
    HeightGlide(0.0),
    TimeElapsed(0.0),
    TimeVirtual(0.0),
    AltitudeDifference(0.0),
    EffectiveWindSpeed(0.0),
    EffectiveWindAngle(0.0)
    {
      // default is null result
    }

  GLIDE_RESULT(const GLIDE_STATE &task, 
               const double V);

  double TrackBearing;
  double Distance;
  double CruiseTrackBearing;
  double VOpt;
  double HeightClimb;
  double HeightGlide;
  double TimeElapsed;
  double TimeVirtual;
  double AltitudeDifference;
  double EffectiveWindSpeed;
  double EffectiveWindAngle;
  GlideResult_t Solution;

  void calc_cruise_bearing();

  // returns true if this solution is better than s2
  bool ok_or_partial() const {
    return (Solution == RESULT_OK)
      || (Solution == RESULT_PARTIAL);
  }

  bool glide_reachable() const {
    return (Solution==RESULT_OK) &&
      (AltitudeDifference>=0) &&
      (HeightClimb==0);
  }
/*
  bool superior(const GLIDE_RESULT &s2) const;
*/
  void add(const GLIDE_RESULT &s2);
  void print(std::ostream& f) const;
  double calc_vspeed(const double mc);
};

#endif
