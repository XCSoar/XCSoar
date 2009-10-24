#ifndef GLIDESTATE_HPP
#define GLIDESTATE_HPP

struct AIRCRAFT_STATE;
struct GEOPOINT;

struct GLIDE_STATE {
  // dummy task
  GLIDE_STATE(const double distance,
              const double bearing,
              const double htarget,
              const AIRCRAFT_STATE &aircraft);
  // from target to aircraft
  GLIDE_STATE(const GEOPOINT& target,
              const AIRCRAFT_STATE &aircraft,
              const double htarget);
  // from aircraft to target
  GLIDE_STATE(const AIRCRAFT_STATE &aircraft,
              const GEOPOINT& target,
              const double htarget);
  double Distance; 
  double Bearing; // TODO should be average bearing
  double MinHeight; 
  void calc_speedups(const AIRCRAFT_STATE &aircraft);
  double WindDirection;
  double AltitudeDifference;
  double EffectiveWindSpeed;
  double EffectiveWindAngle;
  double calc_ave_speed(const double Veff) const;

  double wsq_; // speedup
  double dwcostheta_; // speedup
};

#endif
