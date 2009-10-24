
#include "TaskPoint.hpp"
#include "Math/Earth.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include <algorithm>

GEOPOINT 
TaskPoint::get_reference_remaining() const
{
  return getLocation();
}

double 
TaskPoint::get_bearing(const AIRCRAFT_STATE &ref) const
{
  return ::Bearing(ref.Location, 
                   get_reference_remaining());
}

double 
TaskPoint::get_distance(const AIRCRAFT_STATE &ref) const
{
  return ::Distance(ref.Location, 
                   get_reference_remaining());
}

double 
TaskPoint::get_distance_remaining(const AIRCRAFT_STATE &ref) const
{
  return get_distance(ref);
}

double 
TaskPoint::get_bearing_remaining(const AIRCRAFT_STATE &ref) const
{
  return get_bearing(ref);
}

double 
TaskPoint::getElevation() const
{
  return Elevation; // + SAFETYARRIVALHEIGHT
}


GLIDE_RESULT 
TaskPoint::glide_solution_remaining(const AIRCRAFT_STATE &ac, 
                                    const MacCready &msolv,
                                    const double minH) const
{
  GLIDE_STATE gs(get_distance_remaining(ac),
                 get_bearing_remaining(ac),
                 std::max(minH,getElevation()),
                 ac);
  return msolv.solve(gs);
}

GLIDE_RESULT 
TaskPoint::glide_solution_planned(const AIRCRAFT_STATE &ac, 
                                  const MacCready &msolv,
                                  const double minH) const
{
  return glide_solution_remaining(ac, msolv, minH);
}

GLIDE_RESULT 
TaskPoint::glide_solution_travelled(const AIRCRAFT_STATE &ac, 
                                  const MacCready &msolv,
                                  const double minH) const
{
  GLIDE_RESULT null_res;
  return null_res;
}

GLIDE_RESULT 
TaskPoint::glide_solution_sink(const AIRCRAFT_STATE &ac, 
                               const MacCready &msolv,
                               const double S) const
{
  GLIDE_STATE gs(ac, get_reference_remaining(), getElevation());
  return msolv.solve_sink(gs,S);
}


void 
TaskPoint::print(std::ostream& f) const
{
  f << "# Task point \n";
  f << "#   Location " << getLocation().Longitude << "," <<
    getLocation().Latitude << "\n";
}

