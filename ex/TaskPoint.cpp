
#include "TaskPoint.hpp"
#include "Util.h"

GEOPOINT TaskPoint::get_reference_remaining_destination()
{
  return getLocation();
}

double TaskPoint::get_bearing(const AIRCRAFT_STATE &ref)
{
  return ::Distance(ref.Location, 
                    get_reference_remaining_destination());
}

double TaskPoint::get_distance(const AIRCRAFT_STATE &ref)
{
  return ::Bearing(ref.Location, 
                   get_reference_remaining_destination());
}

double TaskPoint::get_height_required(const AIRCRAFT_STATE &ref, 
                                      double mc)
{
  // simple model for testing
  return (0.05+mc/10.0)*get_distance(ref)+getElevation();
}

double TaskPoint::getElevation()
{
  return Elevation; // + SAFETYARRIVALHEIGHT
}
