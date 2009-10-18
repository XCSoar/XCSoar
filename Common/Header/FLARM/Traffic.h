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

#ifndef XCSOAR_FLARM_TRAFFIC_H
#define XCSOAR_FLARM_TRAFFIC_H

#include "Sizes.h"
#include <tchar.h>
#include "GeoPoint.hpp"

typedef struct _FLARM_TRAFFIC
{
  /** Location of the FLARM target */
  GEOPOINT Location;
  /** TrackBearing of the FLARM target */
  double TrackBearing;
  /** Speed of the FLARM target */
  double Speed;
  /** Altitude of the FLARM target */
  double Altitude;
  /** Turnrate of the FLARM target */
  double TurnRate;
  /** Climbrate of the FLARM target */
  double ClimbRate;
  /** Latitude-based distance of the FLARM target */
  double RelativeNorth;
  /** Longitude-based distance of the FLARM target */
  double RelativeEast;
  /** Altidude-based distance of the FLARM target */
  double RelativeAltitude;
  /** FLARM id of the FLARM target */
  long ID;
  /** (if exists) Name of the FLARM target */
  TCHAR Name[FLARM_NAME_SIZE];
  unsigned short IDType;
  unsigned short AlarmLevel;
  /** Last time the FLARM target was seen */
  double Time_Fix;
  unsigned short Type;
#ifdef FLARM_AVERAGE
  double Average30s;
#endif
} FLARM_TRAFFIC;

#endif
