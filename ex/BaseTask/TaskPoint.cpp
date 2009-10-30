
#include "TaskPoint.hpp"
#include "Math/Earth.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include <algorithm>

GEOPOINT 
TaskPoint::get_reference_remaining() const
{
  return getLocation();
}


const GeoVector 
TaskPoint::get_vector_remaining(const AIRCRAFT_STATE &ref) const
{
  return GeoVector(ref.Location, get_reference_remaining());
}


double 
TaskPoint::getElevation() const
{
  return Elevation; // + SAFETYARRIVALHEIGHT
}


GLIDE_RESULT 
TaskPoint::glide_solution_remaining(const AIRCRAFT_STATE &ac, 
                                    const GlidePolar &polar,
                                    const double minH) const
{
  GLIDE_STATE gs(get_vector_remaining(ac),
                 std::max(minH,getElevation()),
                 ac);
  return polar.solve(gs);
}

GLIDE_RESULT 
TaskPoint::glide_solution_planned(const AIRCRAFT_STATE &ac, 
                                  const GlidePolar &polar,
                                  const double minH) const
{
  return glide_solution_remaining(ac, polar, minH);
}

GLIDE_RESULT 
TaskPoint::glide_solution_travelled(const AIRCRAFT_STATE &ac, 
                                  const GlidePolar &polar,
                                  const double minH) const
{
  GLIDE_RESULT null_res;
  return null_res;
}

GLIDE_RESULT 
TaskPoint::glide_solution_sink(const AIRCRAFT_STATE &ac, 
                               const GlidePolar &polar,
                               const double S) const
{
  GLIDE_STATE gs(ac, get_reference_remaining(), getElevation());
  return polar.solve_sink(gs,S);
}


void 
TaskPoint::print(std::ostream& f, const AIRCRAFT_STATE &state) const
{
  f << "# Task point \n";
  f << "#   Location " << getLocation().Longitude << "," <<
    getLocation().Latitude << "\n";
}

