#ifndef MACCREADY_HPP
#define MACCREADY_HPP

#include <iostream>

struct GLIDE_STATE;
struct GLIDE_RESULT;
struct AIRCRAFT_STATE;

class MacCready 
{
public:
  MacCready():mc(0.0),
              cruise_efficiency(1.0) {

  }
  
  enum MacCreadyResult_t {
    RESULT_OK = 0,
    RESULT_PARTIAL,
    RESULT_WIND_EXCESSIVE,
    RESULT_MACCREADY_INSUFFICIENT,
    RESULT_NOSOLUTION
  };

  GLIDE_RESULT solve(const AIRCRAFT_STATE &aircraft,
                     const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_glide(const AIRCRAFT_STATE &aircraft,
                           const GLIDE_STATE &task,
			   const double V) const;

  GLIDE_RESULT solve_glide_zerowind(const AIRCRAFT_STATE &aircraft,
                                    const GLIDE_STATE &task,
                                    const double V) const;

  GLIDE_RESULT solve_glide(const AIRCRAFT_STATE &aircraft,
                           const GLIDE_STATE &task,
			   const double V,
                           const double S) const;

  GLIDE_RESULT solve_sink(const AIRCRAFT_STATE &aircraft,
                          const GLIDE_STATE &task,
                          const double S) const;

  void set_mc(double _mc);

  double get_mc() const {
    return mc;
  }

  void set_cruise_efficiency(double _ef) {
    cruise_efficiency = _ef;
  }

  double get_cruise_efficiency() const {
    return cruise_efficiency;
  }

private:
  GLIDE_RESULT optimise_glide(const AIRCRAFT_STATE &aircraft,
                              const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_vertical(const AIRCRAFT_STATE &aircraft,
                              const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_cruise(const AIRCRAFT_STATE &aircraft,
                            const GLIDE_STATE &task) const;

  double SinkRate(const double V) const;

  double cruise_bearing(const double V, const double Wn, 
                        const double theta) const;

  void solve_vopt();
  double mc;
  double VOpt;
  double cruise_efficiency;
};

struct GLIDE_STATE {
  double Distance; 
  double Bearing; // should be average bearing
  double MinHeight; 
};

struct GLIDE_RESULT {
  GLIDE_RESULT():
    Solution(MacCready::RESULT_NOSOLUTION),
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

  GLIDE_RESULT(const GLIDE_STATE &task, const double V);

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
  MacCready::MacCreadyResult_t Solution;

  void calc_cruise_bearing();

  // returns true if this solution is better than s2
  bool ok_or_partial() const {
    return (Solution == MacCready::RESULT_OK)
      || (Solution == MacCready::RESULT_PARTIAL);
  }
/*
  bool superior(const GLIDE_RESULT &s2) const;
*/
  void add(const GLIDE_RESULT &s2);
  void print(std::ostream& f) const;
  double calc_vspeed(const double mc);
};


#endif
