/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Waypoint/WaypointReader.hpp"
#include "Waypoint/Factory.hpp"
#include "Waypoint/Waypoints.hpp"
#include "system/Args.hpp"
#include "Operation/Operation.hpp"

#include <stdio.h>
#include <tchar.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "PATH\n");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  Waypoints way_points;

  NullOperationEnvironment operation;
  if (!ReadWaypointFile(path, way_points,
                        WaypointFactory(WaypointOrigin::NONE),
                        operation)) {
    fprintf(stderr, "ReadWaypointFile() has failed\n");
    return EXIT_FAILURE;
  }

  way_points.Optimise();
  printf("Size %d\n", way_points.size());

  way_points.VisitNamePrefix(_T(""), [](const auto &p){
    const auto &wp = *p;
    _ftprintf(stdout, _T("%s, %f, %f, %.0fm\n"), wp.name.c_str(),
              (double)wp.location.latitude.Degrees(),
              (double)wp.location.longitude.Degrees(),
              (double)wp.elevation);
  });

  return EXIT_SUCCESS;
}
