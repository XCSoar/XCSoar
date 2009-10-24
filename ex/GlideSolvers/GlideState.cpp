#include "GlideState.hpp"
#include <math.h>
#include "Util/Quadratic.hpp"
#include "Navigation/Aircraft.hpp"
#include "Math/NavFunctions.hpp"
#include "Math/Earth.hpp"

class GlideQuadratic: public Quadratic
{
public:
  GlideQuadratic(const GLIDE_STATE &task, 
                 const double V):
    Quadratic(task.dwcostheta_, task.wsq_-V*V)
    {};
  double solve() const {
    if (!check()) {
      return -1.0;
    }
    const double V = solution_max();
    return V;
  }
};

double
GLIDE_STATE::calc_ave_speed(const double Veff) const
{
  if (EffectiveWindSpeed>0.0) {
    // only need to solve if positive wind speed
    GlideQuadratic q(*this, Veff);
    return q.solve();
  } else {
    return Veff;
  }
}

// dummy task
GLIDE_STATE::GLIDE_STATE(const double distance,
                         const double bearing,
                         const double htarget,
                         const AIRCRAFT_STATE &aircraft):
  Distance(distance),
  Bearing(bearing),
  MinHeight(htarget)
{
  calc_speedups(aircraft);
}

void GLIDE_STATE::calc_speedups(const AIRCRAFT_STATE &aircraft)
{
  AltitudeDifference = (aircraft.Altitude-MinHeight);
  if (aircraft.WindSpeed>0.0) {
    WindDirection = aircraft.WindDirection;
    EffectiveWindSpeed = (aircraft.WindSpeed);
    const double theta = aircraft.WindDirection-Bearing;
    EffectiveWindAngle = theta;
    wsq_ = aircraft.WindSpeed*aircraft.WindSpeed;
    dwcostheta_ = -2.0*aircraft.WindSpeed*cos(DEG_TO_RAD*theta);
  } else {
    WindDirection = 0.0;
    EffectiveWindSpeed = 0.0;
    EffectiveWindAngle = 0.0;
    wsq_ = 0.0;
    dwcostheta_ = 0.0;
  }
}

// from target to aircraft
GLIDE_STATE::GLIDE_STATE(const GEOPOINT& target,
                         const AIRCRAFT_STATE &aircraft,
                         const double htarget):
  MinHeight(htarget)
{
  ::DistanceBearing(target, aircraft.Location, &Distance, &Bearing);
  calc_speedups(aircraft);
}

  // from aircraft to target
GLIDE_STATE::GLIDE_STATE(const AIRCRAFT_STATE &aircraft,
                         const GEOPOINT& target,
                         const double htarget):
  MinHeight(htarget)
{
  ::DistanceBearing(aircraft.Location, target, &Distance, &Bearing);
  calc_speedups(aircraft);
}
