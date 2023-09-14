// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspacePrinting.hpp"
#include "Printing.hpp"
#include "Geo/Math.hpp"
#include "Airspace/AirspaceCircle.hpp"

#include <fstream>
#include "Airspace/AirspaceCircle.hpp"

std::ostream &
operator<<(std::ostream &os, const AirspaceAltitude &aa)
{
  switch (aa.reference) {
  case AltitudeReference::AGL:
    if (aa.altitude_above_terrain <= 0)
      os << "GND";
    else
      os << iround(aa.altitude_above_terrain) << " AGL";
    break;

  case AltitudeReference::MSL:
    os << iround(aa.altitude);
    break;

  case AltitudeReference::STD:
    os << "FL" << iround(aa.flight_level);
    break;
  }

  return os;
}

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceCircle& as)
{
  f << "# circle\n";
  for (double t=0; t<=360; t+= 30) {
    GeoPoint l = FindLatitudeLongitude(as.m_center, Angle::Degrees(t), as.m_radius);
    f << l.longitude << " " << l.latitude << " " << as.GetBase().altitude << "\n";
  }
  f << "\n";
  for (double t=0; t<=360; t+= 30) {
    GeoPoint l = FindLatitudeLongitude(as.m_center, Angle::Degrees(t), as.m_radius);
    f << l.longitude << " " << l.latitude << " " << as.GetTop().altitude << "\n";
  }
  f << "\n";
  f << "\n";
  return f;
}

#include "Airspace/AirspacePolygon.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspacePolygon& as)
{
  f << "# polygon\n";
  for (auto v = as.m_border.begin();
       v != as.m_border.end(); ++v) {
    GeoPoint l = v->GetLocation();
    f << l.longitude << " " << l.latitude << " " << as.GetBase().altitude << "\n";
  }
  f << "\n";
  for (auto v = as.m_border.begin();
       v != as.m_border.end(); ++v) {
    GeoPoint l = v->GetLocation();
    f << l.longitude << " " << l.latitude << " " << as.GetTop().altitude << "\n";
  }
  f << "\n";
  f << "\n";

  return f;
}


#include "Airspace/AbstractAirspace.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AbstractAirspace& as)
{
  switch (as.shape) {
  case AbstractAirspace::Shape::CIRCLE:
    f << (const AirspaceCircle &)as;
    break;

  case AbstractAirspace::Shape::POLYGON:
    f << (const AirspacePolygon &)as;
    break;
  }
  return f;
}

#include "Airspace/AirspaceWarning.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceWarning& aw)
{
  AirspaceWarning::State state = aw.GetWarningState();
  f << "# warning ";
  switch(state) {
  case AirspaceWarning::WARNING_CLEAR:
    f << "clear\n";
    break;
  case AirspaceWarning::WARNING_TASK:
    f << "task\n";
    break;
  case AirspaceWarning::WARNING_FILTER:
    f << "predicted filter\n";
    break;
  case AirspaceWarning::WARNING_GLIDE:
    f << "predicted glide\n";
    break;
  case AirspaceWarning::WARNING_INSIDE:
    f << "inside\n";
    break;
  };

  const AirspaceInterceptSolution &solution = aw.GetSolution();
  f << "# intercept " << solution.location.longitude << " " << solution.location.latitude
    << " dist " << solution.distance << " alt " << solution.altitude << " time "
    << solution.elapsed_time.count() << "\n";

  return f;
}
