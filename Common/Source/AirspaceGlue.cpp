/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "AirspaceGlue.hpp"
#include "Airspace.h"
#include "AirspaceDatabase.hpp"
#include "Registry.hpp"
#include "RasterTerrain.h"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "wcecompat/ts_string.h"

void
ReadAirspace(AirspaceDatabase &airspace_database, RasterTerrain *terrain)
{
  TCHAR tpath[MAX_PATH];

  // TODO bug: add exception handler to protect parser code against chrashes
  // TODO bug: second file should be opened even if first was not okay

  // Read the airspace filenames from the registry
  GetRegistryString(szRegistryAirspaceFile, tpath, MAX_PATH);
  if (tpath[0] != 0) {
    ExpandLocalPath(tpath);

    char path[MAX_PATH];
    unicode2ascii(tpath, path, sizeof(path));

    if (!ReadAirspace(airspace_database, path))
      StartupStore(TEXT("No airspace file 1\n"));
  } else {
    // TODO feature: airspace in xcm files should be a feature
    /*
    static TCHAR  szMapFile[MAX_PATH] = TEXT("\0");
    GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
    ExpandLocalPath(szMapFile);
    wcscat(szMapFile,TEXT("/"));
    wcscat(szMapFile,TEXT("airspace.txt"));
    unicode2ascii(szMapFile, zfilename, MAX_PATH);
    fp  = zzip_fopen(zfilename, "rt");
    */
  }

  GetRegistryString(szRegistryAdditionalAirspaceFile, tpath, MAX_PATH);
  if (tpath[0] != 0) {
    ExpandLocalPath(tpath);

    char path[MAX_PATH];
    unicode2ascii(tpath, path, sizeof(path));

    if (!ReadAirspace(airspace_database, path))
      StartupStore(TEXT("No airspace file 2\n"));
  }

  // Calculate the airspace boundaries
  FindAirspaceAreaBounds(airspace_database);
  FindAirspaceCircleBounds(airspace_database);

  if (terrain != NULL) {
    terrain->Lock();
    airspace_database.UpdateAGL(*terrain);
    terrain->Unlock();
  }
}
