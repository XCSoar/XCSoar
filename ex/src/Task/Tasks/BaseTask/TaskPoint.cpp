
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
  return Elevation+task_behaviour.safety_height_arrival;
}


GlideResult 
TaskPoint::glide_solution_remaining(const AIRCRAFT_STATE &ac, 
                                    const GlidePolar &polar,
                                    const double minH) const
{
  GlideState gs(get_vector_remaining(ac),
                 std::max(minH,getElevation()),
                 ac);
  return polar.solve(gs);
}

GlideResult 
TaskPoint::glide_solution_planned(const AIRCRAFT_STATE &ac, 
                                  const GlidePolar &polar,
                                  const double minH) const
{
  return glide_solution_remaining(ac, polar, minH);
}

GlideResult 
TaskPoint::glide_solution_travelled(const AIRCRAFT_STATE &ac, 
                                  const GlidePolar &polar,
                                  const double minH) const
{
  GlideResult null_res;
  return null_res;
}

GlideResult 
TaskPoint::glide_solution_sink(const AIRCRAFT_STATE &ac, 
                               const GlidePolar &polar,
                               const double S) const
{
  GlideState gs(ac, get_reference_remaining(), getElevation());
  return polar.solve_sink(gs,S);
}


