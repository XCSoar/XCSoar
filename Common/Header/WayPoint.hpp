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

#ifndef XCSOAR_WAY_POINT_HPP
#define XCSOAR_WAY_POINT_HPP

#include "XCSoar.h"
#include <tchar.h>

#define AIRPORT				0x01
#define TURNPOINT			0x02
#define LANDPOINT			0x04
#define HOME					0x08
#define START					0x10
#define FINISH				0x20
#define RESTRICTED		0x40
#define WAYPOINTFLAG	0x80

// VENTA3 note> probably it would be a good idea to separate static WP data to dynamic values,
// by moving things like Reachable, AltArival , etc to WPCALC
// Currently at 5.2.2 the whole structure is saved into the task file, so everytime we
// change the struct all old taks files become invalid... (there's a bug, btw, in this case)

typedef struct _WAYPOINT_INFO
{
  int Number;
  double Latitude;
  double Longitude;
  double Altitude;
  int Flags;
  TCHAR Name[NAME_SIZE + 1];
  TCHAR Comment[COMMENT_SIZE + 1];
  POINT	Screen;
  int Zoom;
  BOOL Reachable;
  double AltArivalAGL;
  BOOL Visible;
  bool InTask;
  TCHAR *Details;
  bool FarVisible;
  int FileNum; // which file it is in, or -1 to delete
} WAYPOINT;

// VENTA3
// This struct is separated from _WAYPOINT_INFO and will not be used in task files.
// It is managed by the same functions that manage WayPointList, only add variables here
// and use them like  WayPointCalc[n].Distance  for example.
typedef struct _WAYPOINT_CALCULATED
{
//  long timeslot;
  double GR;       // GR from current position
  short VGR;       // Visual GR
  double Distance; // distance from current position
  double Bearing;  // used for radial
  double AltReqd;  // comes free from CalculateWaypointArrivalAltitude
  double AltArriv; // Arrival Altitude
  bool Preferred;  // Flag to be used by Preferred quick selection WP page (todo) and
		   // by BestAlternate
} WPCALC;

extern WAYPOINT *WayPointList;
extern WPCALC   *WayPointCalc; // VENTA3 additional calculated infos on WPs
extern unsigned int NumberOfWayPoints;
extern int WaypointsOutOfRange;

#endif
