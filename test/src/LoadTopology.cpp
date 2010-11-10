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

/*
 * This program loads the topology from a map file and exits.  Useful
 * for valgrind and profiling.
 */

#include "Topology/TopologyStore.hpp"
#include "OS/PathName.hpp"
#include "IO/ZipLineReader.hpp"
#include "WindowProjection.hpp"

#include <zzip/zzip.h>

#include <stdio.h>
#include <tchar.h>

class TestProjection : public WindowProjection {
public:
  TestProjection() {
    SetScreenOrigin(0, 0);
    SetScale(fixed(640) / (fixed(100) * 2));
    SetGeoLocation(GeoPoint(Angle::degrees(fixed(7.7061111111111114)),
                            Angle::degrees(fixed(51.051944444444445))));
    MapRect.left = 0;
    MapRect.top = 0;
    MapRect.right = 640;
    MapRect.bottom = 480;
    UpdateScreenBounds();
  }
};

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[0]);
    return 1;
  }

  const char *path = argv[1];

  ZZIP_DIR *dir = zzip_dir_open(path, NULL);
  if (dir == NULL) {
    fprintf(stderr, "Failed to open %s\n", (const char *)path);
    return EXIT_FAILURE;
  }

  ZipLineReaderA reader(dir, "topology.tpl");
  if (reader.error()) {
    fprintf(stderr, "Failed to open %s\n", (const char *)path);
    return EXIT_FAILURE;
  }

  TopologyStore topology;
  topology.Load(reader, NULL, dir);
  zzip_dir_close(dir);

  TestProjection projection;

  topology.ScanVisibility(projection);

  return EXIT_SUCCESS;
}
