/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/
#include "Printing.hpp"
#include "Math/Earth.hpp"
#include "Airspace/AirspaceCircle.hpp"

#include <fstream>
#include "Airspace/AirspaceCircle.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceCircle& as)
{
  f << "# circle\n";
  for (double t=0; t<=360; t+= 30) {
    GeoPoint l = FindLatitudeLongitude(as.m_center, Angle::Degrees(fixed(t)), as.m_radius);
    f << l.longitude << " " << l.latitude << " " << as.GetBase().altitude << "\n";
  }
  f << "\n";
  for (double t=0; t<=360; t+= 30) {
    GeoPoint l = FindLatitudeLongitude(as.m_center, Angle::Degrees(fixed(t)), as.m_radius);
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
  for (std::vector<SearchPoint>::const_iterator v = as.m_border.begin();
       v != as.m_border.end(); ++v) {
    GeoPoint l = v->get_location();
    f << l.longitude << " " << l.latitude << " " << as.GetBase().altitude << "\n";
  }
  f << "\n";
  for (std::vector<SearchPoint>::const_iterator v = as.m_border.begin();
       v != as.m_border.end(); ++v) {
    GeoPoint l = v->get_location();
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
  case AbstractAirspace::CIRCLE:
    f << (const AirspaceCircle &)as;
    break;

  case AbstractAirspace::POLYGON:
    f << (const AirspacePolygon &)as;
    break;
  }
  return f;
}

#include "Airspace/AirspaceWarning.hpp"

std::ostream& operator<< (std::ostream& f, 
                          const AirspaceWarning& aw)
{
  AirspaceWarning::State state = aw.get_warning_state();
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

  const AirspaceInterceptSolution &solution = aw.get_solution();
  f << "# intercept " << solution.location.longitude << " " << solution.location.latitude
    << " dist " << solution.distance << " alt " << solution.altitude << " time "
    << solution.elapsed_time << "\n";

  return f;
}

void write_border (const AbstractAirspace& as)
{
  const SearchPointVector& spv = as.GetPoints();
  for (std::vector<SearchPoint>::const_iterator v = spv.begin();
       v != spv.end(); ++v) {
    write_point(*v, v->get_flatLocation(), "polygon");
  }
  printf("polygon\n");
  write_spv(as.GetClearance());
  fflush(stdout);
}
