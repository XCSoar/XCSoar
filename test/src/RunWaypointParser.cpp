/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Waypoint/Waypoints.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "OS/PathName.hpp"
#include "OS/Args.hpp"
#include "Operation/Operation.hpp"

#include <stdio.h>
#include <tchar.h>

class DumpVisitor : public WaypointVisitor {
public:
  void Visit(const Waypoint &wp) {
    _ftprintf(stdout, _T("%s, %f, %f, %.0fm\n"), wp.name.c_str(),
              (double)wp.location.latitude.Degrees(),
              (double)wp.location.longitude.Degrees(),
              (double)wp.elevation);
  }
};

int main(int argc, char **argv)
{
  Args args(argc, argv, "PATH\n");

  const char *path_arg = args.ExpectNext();
  args.ExpectEnd();

  Waypoints way_points;

  PathName path(path_arg);
  WaypointReader parser(path, 0);
  if (parser.Error()) {
    fprintf(stderr, "WayPointParser::SetFile() has failed\n");
    return EXIT_FAILURE;
  }

  NullOperationEnvironment operation;
  if (!parser.Parse(way_points, operation)) {
    fprintf(stderr, "WayPointParser::Parse() has failed\n");
    return EXIT_FAILURE;
  }

  way_points.Optimise();
  printf("Size %d\n", way_points.size());

  DumpVisitor visitor;
  way_points.VisitNamePrefix(_T(""), visitor);

  return EXIT_SUCCESS;
}
