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

#include "SnailTrail.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

#include <math.h>


SnailTrail::SnailTrail(): clock(2.0)
{
  indexNext = 0;
  memset(&TrailPoints[0],0,TRAILSIZE*sizeof(SNAIL_POINT));
}

// called by calculation thread

void
SnailTrail::AddPoint(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
  ScopeLock protect(mutexSnail);

  TrailPoints[indexNext].Latitude = (float)(Basic->Location.Latitude);
  TrailPoints[indexNext].Longitude = (float)(Basic->Location.Longitude);
  TrailPoints[indexNext].Time = Basic->Time;
  TrailPoints[indexNext].FarVisible = true; // hasn't been filtered out yet.
  if (Calculated->TerrainValid) {
    double hr = max(0,Calculated->AltitudeAGL)/100.0;
    TrailPoints[indexNext].DriftFactor = 2.0/(1.0+exp(-hr))-1.0;
  } else {
    TrailPoints[indexNext].DriftFactor = 1.0;
  }

  if (Calculated->Circling) {
    TrailPoints[indexNext].Vario = (float)(Calculated->NettoVario) ;
  } else {
    TrailPoints[indexNext].Vario = (float)(Calculated->NettoVario) ;
  }
  TrailPoints[indexNext].Colour = -1; // need to have colour calculated
  TrailPoints[indexNext].Circling = Calculated->Circling;

  indexNext ++;
  indexNext %= TRAILSIZE;

}


void SnailTrail::ScanVisibility(rectObj *bounds_active) {
  ScopeLock protect(mutexSnail);
  SNAIL_POINT *sv= TrailPoints;
  const rectObj bounds = *bounds_active;
  const SNAIL_POINT *se = sv+TRAILSIZE;
  while (sv<se) {
    sv->FarVisible = ((sv->Longitude> bounds.minx) &&
		      (sv->Longitude< bounds.maxx) &&
		      (sv->Latitude> bounds.miny) &&
		      (sv->Latitude< bounds.maxy));
    sv++;
  }
}

////

