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

#include "WayPoint.hpp"

WAYPOINT *WayPointList = NULL;
WPCALC *WayPointCalc = NULL; // VENTA3 additional infos calculated, parallel to WPs
unsigned int NumberOfWayPoints = 0;
int WaypointsOutOfRange = 1; // include


////

void WaypointScan::scan_forward(WaypointVisitor &visitor)
{
  if (!WayPointList) return;
  for (unsigned i=0; i<NumberOfWayPoints; i++) {
    if ((WayPointList[i].Flags & AIRPORT)==AIRPORT) {
      visitor.waypoint_airport(WayPointList[i], WayPointCalc[i]);
    } else if ((WayPointList[i].Flags & LANDPOINT)==LANDPOINT) {
      visitor.waypoint_landable(WayPointList[i], WayPointCalc[i]);
    } else {
      visitor.waypoint_default(WayPointList[i], WayPointCalc[i]);
    }
  }
}
