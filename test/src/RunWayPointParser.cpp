/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "WayPointFile.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "OS/PathName.hpp"

#include <stdio.h>
#include <tchar.h>

/* what follows is a bunch of symbols needed by the linker - we don't
   want to compile & link the original libraries, because that would
   mean even more and more depencies */

class DumpVisitor : public WaypointVisitor {
public:
  void Visit(const Waypoint &wp) {
    _ftprintf(stdout, _T("%s\n"), wp.Name.c_str());
  }
};

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[0]);
    return 1;
  }

  Waypoints way_points;

  WayPointFile* parser = NULL;
  PathName path(argv[1]);
  parser = WayPointFile::create(path, 0);
  if (!parser) {
    fprintf(stderr, "WayPointParser::SetFile() has failed\n");
    return 1;
  }

  if (!parser->Parse(way_points, NULL)) {
    fprintf(stderr, "WayPointParser::Parse() has failed\n");
    return 1;
  }

  delete parser;

  way_points.optimise();
  printf("Size %d\n", way_points.size());

  DumpVisitor visitor;
  way_points.visit_name_prefix(_T(""), visitor);

  return 0;
}
