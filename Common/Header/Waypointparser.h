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

#if !defined(AFX_WAYPOINTPARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_WAYPOINTPARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include <tchar.h>

#define wpTerrainBoundsYes    100
#define wpTerrainBoundsYesAll 101
#define wpTerrainBoundsNo     102
#define wpTerrainBoundsNoAll  103

class WayPointList;
class MapWindowProjection;
class RasterTerrain;

struct WAYPOINT;
struct GEOPOINT;
struct SETTINGS_COMPUTER;

/**
 * Reads a text file, and appends its way points to the specified
 * WayPointList.
 */
bool
ReadWayPointFile(const TCHAR *path, WayPointList &way_points,
                 RasterTerrain &terrain);

void
ReadWayPoints(WayPointList &way_points, RasterTerrain &terrain);

void
SetHome(const WayPointList &way_points, const RasterTerrain &terrain,
        SETTINGS_COMPUTER &settings,
        const bool reset, const bool set_location=false);

int
FindNearestWayPoint(const WayPointList &way_points,
                    MapWindowProjection &map_projection,
                    const GEOPOINT &location,
                    double MaxRange, bool exhaustive=false);

int dlgWaypointOutOfTerrain(const TCHAR *Message);

void
WaypointWriteFiles(WayPointList &way_points,
                   const SETTINGS_COMPUTER &settings_computer);

void
WaypointAltitudeFromTerrain(WAYPOINT* wpt, RasterTerrain &terrain);

#endif
