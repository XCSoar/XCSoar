
#include "TaskPoint.hpp"
#include "Util.h"
#include <algorithm>

GEOPOINT TaskPoint::get_reference_remaining_destination()
{
  return getLocation();
}

double TaskPoint::get_bearing(const AIRCRAFT_STATE &ref)
{
  return ::Bearing(ref.Location, 
                   get_reference_remaining_destination());
}

double TaskPoint::get_distance(const AIRCRAFT_STATE &ref)
{
  return ::Distance(ref.Location, 
                   get_reference_remaining_destination());
}

double TaskPoint::get_distance_remaining(const AIRCRAFT_STATE &ref)
{
  return get_distance(ref);
}

double TaskPoint::get_bearing_remaining(const AIRCRAFT_STATE &ref)
{
  return get_bearing(ref);
}

double TaskPoint::getElevation()
{
  return Elevation; // + SAFETYARRIVALHEIGHT
}


GLIDE_RESULT TaskPoint::glide_solution_remaining(const AIRCRAFT_STATE &ac, 
                                                 const double mc,
                                                 const double minH)
{
  MacCready msolv; // TODO this should be passed in as a reference

  GLIDE_STATE gs;
  gs.Distance = get_distance_remaining(ac);
  gs.Bearing = get_bearing_remaining(ac);
  gs.MacCready = mc;
  gs.MinHeight = std::max(minH,getElevation());

  return msolv.solve(ac,gs);
}
