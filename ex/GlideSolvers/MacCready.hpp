#ifndef MACCREADY_HPP
#define MACCREADY_HPP

#include <math.h>

struct GLIDE_STATE;
struct GLIDE_RESULT;
struct AIRCRAFT_STATE;

class MacCready 
{
public:
  
  enum MacCreadyResult_t {
    RESULT_OK = 0,
    RESULT_PARTIAL,
    RESULT_WIND_EXCESSIVE,
    RESULT_MACCREADY_INSUFFICIENT,
    RESULT_NOSOLUTION
  };

  GLIDE_RESULT solve(const AIRCRAFT_STATE &aircraft,
                     const GLIDE_STATE &task);

private:
  typedef GLIDE_RESULT (MacCready::*Solver_t)(const AIRCRAFT_STATE &aircraft,
				   const GLIDE_STATE &task,
				   const double V);

  GLIDE_RESULT solve_vertical(const AIRCRAFT_STATE &aircraft,
                              const GLIDE_STATE &task,
			      const double V);
  GLIDE_RESULT solve_glide(const AIRCRAFT_STATE &aircraft,
                           const GLIDE_STATE &task,
			   const double V);

  GLIDE_RESULT solve_cruise(const AIRCRAFT_STATE &aircraft,
                            const GLIDE_STATE &task,
                            const double V);

  double SinkRate(double V) const;

  GLIDE_RESULT optimise(const AIRCRAFT_STATE &aircraft,
			const GLIDE_STATE &task,
			Solver_t solver);

  double cruise_bearing(const double V, const double Wn, const double theta);
};

struct GLIDE_STATE {
  double Distance; 
  double Bearing; // should be average bearing
  double MacCready; // aircraft state also provided
  double MinHeight; 
};

struct GLIDE_RESULT {
  MacCready::MacCreadyResult_t Solution;
  double TrackBearing;
  double Distance;
  double CruiseTrackBearing;
  double VOpt;
  double HeightClimb;
  double HeightGlide;
  double TimeElapsed;
  double TimeVirtual;

  // returns true if this solution is better than s2
  bool ok_or_partial() const {
    return (Solution == MacCready::RESULT_OK)
      || (Solution == MacCready::RESULT_PARTIAL);
  }
  bool superior(const GLIDE_RESULT &s2) const;
  void add(const GLIDE_RESULT &s2);
  void report();
};


#endif
