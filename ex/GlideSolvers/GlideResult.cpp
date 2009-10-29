#include "GlideResult.hpp"
#include "GlideState.hpp"
#include <math.h>
#include "Math/NavFunctions.hpp"

GLIDE_RESULT::GLIDE_RESULT(const GLIDE_STATE &task, 
                           const double V):
    Solution(RESULT_NOSOLUTION),
    Distance(task.Distance),
    DistanceToFinal(task.Distance),
    TrackBearing(task.Bearing),
    CruiseTrackBearing(task.Bearing),
    VOpt(V),
    HeightClimb(0.0),
    HeightGlide(0.0),
    TimeElapsed(0.0),
    TimeVirtual(0.0),
    AltitudeDifference(task.AltitudeDifference),
    EffectiveWindSpeed(task.EffectiveWindSpeed),
    EffectiveWindAngle(task.EffectiveWindAngle)
{
}


void
GLIDE_RESULT::calc_cruise_bearing()
{
  CruiseTrackBearing=TrackBearing;
  if (EffectiveWindSpeed>0.0) {
    const double sintheta = sin(DEG_TO_RAD*EffectiveWindAngle);
    if (sintheta==0.0) {
      return;
    }
    // Wn/sin(alpha) = V/sin(theta)
    //   (Wn/V)*sin(theta) = sin(alpha)
    CruiseTrackBearing += RAD_TO_DEG*asin(sintheta*EffectiveWindSpeed/VOpt);
  }
}


void 
GLIDE_RESULT::print(std::ostream& f) const
{
  if (Solution != RESULT_OK) {
    f << "#     Solution NOT OK\n";
  }
  f << "#    Altitude Difference " << AltitudeDifference << " (m)\n";
  f << "#    Distance            " << Distance << " (m)\n";
  f << "#    TrackBearing        " << TrackBearing << " (deg)\n";
  f << "#    CruiseTrackBearing  " <<  CruiseTrackBearing << " (deg)\n";
  f << "#    VOpt                " <<  VOpt << " (m/s)\n";
  f << "#    HeightClimb         " <<  HeightClimb << " (m)\n";
  f << "#    HeightGlide         " <<  HeightGlide << " (m)\n";
  f << "#    TimeElapsed         " <<  TimeElapsed << " (s)\n";
  f << "#    TimeVirtual         " <<  TimeVirtual << " (s)\n";
  if (TimeElapsed>0) {
    f << "#    Vave remaining      " <<  Distance/TimeElapsed << " (m/s)\n";
    f << "#    EffectiveWindSpeed  " <<  EffectiveWindSpeed << " (m/s)\n";
    f << "#    EffectiveWindAngle  " <<  EffectiveWindAngle << " (deg)\n";
    f << "#    DistanceToFinal     " <<  DistanceToFinal << " (m)\n";
  }
  if (is_final_glide()) {
    f << "#    On final glide\n";
  }
}

void 
GLIDE_RESULT::add(const GLIDE_RESULT &s2) 
{
  TimeElapsed += s2.TimeElapsed;
  HeightGlide += s2.HeightGlide;
  HeightClimb += s2.HeightClimb;
  Distance += s2.Distance;
  DistanceToFinal += s2.DistanceToFinal;
  TimeVirtual += s2.TimeVirtual;

  if ((AltitudeDifference<0) || (s2.AltitudeDifference<0)) {
    AltitudeDifference= std::min(s2.AltitudeDifference+AltitudeDifference,
      AltitudeDifference);
  } else {
    AltitudeDifference= std::min(s2.AltitudeDifference, AltitudeDifference);
  }
}


double 
GLIDE_RESULT::calc_vspeed(const double mc) 
{
  if (!ok_or_partial()) {
    TimeVirtual = 0.0;
    return 1.0e6;
  }
  if (Distance>0.0) {
    if (mc>0.0) {
      // equivalent time to gain the height that was used
      TimeVirtual = HeightGlide/mc;
      return (TimeElapsed+TimeVirtual)/Distance;
    } else {
      TimeVirtual = 0.0;
      // minimise 1.0/LD over ground 
      return -HeightGlide/Distance;
    }
  } else {
    TimeVirtual = 0.0;
    return 0.0;
  }
}

double 
GLIDE_RESULT::glide_angle_ground() const
{
  if (Distance>0) {
    return HeightGlide/Distance;
  } else {
    return 100.0;
  }
}


/*
bool GLIDE_RESULT::superior(const GLIDE_RESULT &s2) const 
{
  if (Solution < s2.Solution) {
    return true;
  } else if (ok_or_partial() && (Solution == s2.Solution)) {
    if (Distance>0) {
      return (Distance/(TimeElapsed+TimeVirtual) > 
              s2.Distance/(s2.TimeElapsed+s2.TimeVirtual));
    } else {
      return (TimeElapsed+TimeVirtual < s2.TimeElapsed+s2.TimeVirtual);
    }
  } else {
    return false;
  }
}
*/


