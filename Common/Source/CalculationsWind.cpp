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

#include "CalculationsWind.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "WindZigZag.h"
#include "windanalyser.h"
#include <math.h>
#include "Logger.h"
#include "GlideComputer.hpp"
#include "Protection.hpp"

#define D_AUTOWIND_CIRCLING 1
#define D_AUTOWIND_ZIGZAG 2

int AutoWindMode= D_AUTOWIND_CIRCLING;
// 0: Manual
// 1: Circling
// 2: ZigZag
// 3: Both


void InitialiseCalculationsWind() {
  mutexGlideComputer.Lock();
  if (!GlideComputer::windanalyser) {
    GlideComputer::windanalyser = new WindAnalyser();

    //JMW TODO enhancement: seed initial wind store with start conditions
    // SetWindEstimate(Calculated->WindSpeed,Calculated->WindBearing, 1);
  }
  mutexGlideComputer.Unlock();
}

void CloseCalculationsWind() {
  mutexGlideComputer.Lock();
  if (GlideComputer::windanalyser) {
    delete GlideComputer::windanalyser;
    GlideComputer::windanalyser = NULL;
  }
  mutexGlideComputer.Unlock();
}

//// WIND

void DoWindZigZag(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // update zigzag wind
  if (((AutoWindMode & D_AUTOWIND_ZIGZAG)==D_AUTOWIND_ZIGZAG)
      && (!ReplayLogger::IsEnabled())) {
    double zz_wind_speed;
    double zz_wind_bearing;
    int quality;
    quality = WindZigZagUpdate(Basic, Calculated,
			       &zz_wind_speed,
			       &zz_wind_bearing);
    if (quality>0) {
      SetWindEstimate(zz_wind_speed, zz_wind_bearing, quality);
      Vector v_wind;
      v_wind.x = zz_wind_speed*cos(zz_wind_bearing*3.1415926/180.0);
      v_wind.y = zz_wind_speed*sin(zz_wind_bearing*3.1415926/180.0);
      mutexGlideComputer.Lock();
      if (GlideComputer::windanalyser) {
	GlideComputer::windanalyser->slot_newEstimate(Basic, Calculated, v_wind, quality);
      }
      mutexGlideComputer.Unlock();
    }
  }
}


void DoWindCirclingMode(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			bool left) {
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    mutexGlideComputer.Lock();
    if (GlideComputer::windanalyser) {
      GlideComputer::windanalyser->slot_newFlightMode(Basic, Calculated, left, 0);
    }
    mutexGlideComputer.Unlock();
  }
}

void DoWindCirclingSample(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    mutexGlideComputer.Lock();
    if (GlideComputer::windanalyser) {
      GlideComputer::windanalyser->slot_newSample(Basic, Calculated);
    }
    mutexGlideComputer.Unlock();
  }
}

void DoWindCirclingAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  if (AutoWindMode>0) {
    mutexGlideComputer.Lock();
    if (GlideComputer::windanalyser) {
      GlideComputer::windanalyser->slot_Altitude(Basic, Calculated);
    }
    mutexGlideComputer.Unlock();
  }
}

#include "Blackboard.hpp"

void  SetWindEstimate(const double wind_speed,
		      const double wind_bearing,
		      const int quality) {
  Vector v_wind;
  v_wind.x = wind_speed*cos(wind_bearing*3.1415926/180.0);
  v_wind.y = wind_speed*sin(wind_bearing*3.1415926/180.0);
  mutexGlideComputer.Lock();
  if (GlideComputer::windanalyser) {
    GlideComputer::windanalyser->slot_newEstimate(&GPS_INFO, &CALCULATED_INFO,
						  v_wind, quality);
  }
  mutexGlideComputer.Unlock();
}
