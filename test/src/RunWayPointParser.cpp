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

#include "Waypointparser.h"
#include "Waypoint/Waypoints.hpp"
#include "RasterTerrain.h"
#include "Thread/Mutex.hpp"
#include "Language.hpp"
#include "Interface.hpp"
#include "Registry.hpp"
#include "Profile.hpp"
#include "Dialogs.h"
#include "DeviceBlackboard.hpp"
#include "wcecompat/ts_string.h"

#include <stdio.h>
#include <tchar.h>
#include <stdarg.h>

/* what follows is a bunch of symbols needed by the linker - we don't
   want to compile & link the original libraries, because that would
   mean even more and more depencies */

Mutex mutexTaskData;
const TCHAR szRegistryWayPointFile[] = _T("");
const TCHAR szRegistryAdditionalWayPointFile[] = _T("");
const TCHAR szRegistryWaypointsOutOfRange[] = _T("");
const TCHAR szRegistryMapFile[] = _T("");
const TCHAR szRegistryAlternate1[] = _T("");
const TCHAR szRegistryAlternate2[] = _T("");
const TCHAR szRegistryHomeWaypoint[] = _T("");
const TCHAR szRegistryTeamcodeRefWaypoint[] = _T("");
DeviceBlackboard device_blackboard;

int WINAPI
MessageBoxX(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
  return -1;
}

void
StartupStore(const TCHAR *Str, ...)
{
  va_list ap;

  va_start(ap, Str);
  _vftprintf(stderr, Str, ap);
  va_end(ap);
}

BOOL
GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize)
{
  return false;
}

HRESULT SetToRegistry(const TCHAR *szRegValue, int nVal)
{
  return 0;
}

HRESULT
SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos)
{
  return 0;
}

void
XCSoarInterface::CreateProgressDialog(const TCHAR* text)
{
  _ftprintf(stderr, _T("%s\n"), text);
}

void
XCSoarInterface::StepProgressDialog(void)
{
}

bool
MapWindowProjection::WaypointInScaleFilter(const WAYPOINT &way_point) const
{
  return true;
}

void
DeviceBlackboard::SetStartupLocation(const GEOPOINT &loc,
                                     const double alt)
{
}

bool
RasterTerrain::GetTerrainCenter(GEOPOINT *location) const
{
  return false;
}

short
RasterTerrain::GetTerrainHeight(const GEOPOINT &Location,
                                const RasterRounding &rounding) const
{
  return 0;
}

bool
RasterTerrain::WaypointIsInTerrainRange(const GEOPOINT &location) const
{
  return true;
}

MapWindowProjection::MapWindowProjection() {}
SettingsComputerBlackboard::SettingsComputerBlackboard() {}
SettingsMapBlackboard::SettingsMapBlackboard() {}

void
Profile::StoreRegistry(void)
{
}

int
dlgWaypointOutOfTerrain(const TCHAR *Message)
{
  return -1;
}

const TCHAR *
gettext(const TCHAR *text)
{
  return text;
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[0]);
    return 1;
  }

  TCHAR path[MAX_PATH];
  Waypoints way_points;

  ascii2unicode(argv[1], path);
  ReadWayPointFile(path, way_points, NULL);

  way_points.optimise();
  printf("Size %d\n", way_points.size());

  return 0;
}
