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

#include "Calculations.h"
#include "Blackboard.hpp"
#include "Airspace.h"
#include "AirspaceWarning.h"
#include "Math/Geometry.hpp"
#include "Message.h"
#include "Math/Earth.hpp"
#include "MapWindow.h"

extern int AIRSPACEWARNINGS;
extern int WarningTime;
extern int AcknowledgementTime;


void PredictNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if(Calculated->Circling)
    {
      Calculated->NextLatitude = Basic->Latitude;
      Calculated->NextLongitude = Basic->Longitude;
      Calculated->NextAltitude =
        Calculated->NavAltitude + Calculated->Average30s * WarningTime;
    }
  else
    {
      FindLatitudeLongitude(Basic->Latitude,
                            Basic->Longitude,
                            Basic->TrackBearing,
                            Basic->Speed*WarningTime,
                            &Calculated->NextLatitude,
                            &Calculated->NextLongitude);

      if (Basic->BaroAltitudeAvailable) {
        Calculated->NextAltitude =
          Basic->BaroAltitude + Calculated->Average30s * WarningTime;
      } else {
        Calculated->NextAltitude =
          Calculated->NavAltitude + Calculated->Average30s * WarningTime;
      }
    }
    // MJJ TODO Predict terrain altitude
    Calculated->NextAltitudeAGL = Calculated->NextAltitude - Calculated->TerrainAlt;

}


//////////////////////////////////////////////

bool GlobalClearAirspaceWarnings = false;

// JMW this code is deprecated
bool ClearAirspaceWarnings(const bool acknowledge, const bool ack_all_day) {
  unsigned int i;
  if (acknowledge) {
    GlobalClearAirspaceWarnings = true;
    if (AirspaceCircle) {
      for (i=0; i<NumberOfAirspaceCircles; i++) {
        if (AirspaceCircle[i].WarningLevel>0) {
          AirspaceCircle[i].Ack.AcknowledgementTime = GPS_INFO.Time;
          if (ack_all_day) {
            AirspaceCircle[i].Ack.AcknowledgedToday = true;
          }
          AirspaceCircle[i].WarningLevel = 0;
        }
      }
    }
    if (AirspaceArea) {
      for (i=0; i<NumberOfAirspaceAreas; i++) {
        if (AirspaceArea[i].WarningLevel>0) {
          AirspaceArea[i].Ack.AcknowledgementTime = GPS_INFO.Time;
          if (ack_all_day) {
            AirspaceArea[i].Ack.AcknowledgedToday = true;
          }
          AirspaceArea[i].WarningLevel = 0;
        }
      }
    }
    return Message::Acknowledge(MSG_AIRSPACE);
  }
  return false;
}


void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated){
  unsigned int i;

  if(!AIRSPACEWARNINGS)
      return;

  static bool position_is_predicted = false;

  //  LockFlightData(); Not necessary, airspace stuff has its own locking

  if (GlobalClearAirspaceWarnings == true) {
    GlobalClearAirspaceWarnings = false;
    Calculated->IsInAirspace = false;
  }

  position_is_predicted = !position_is_predicted;
  // every second time step, do predicted position rather than
  // current position

  double alt;
  double agl;
  double lat;
  double lon;

  if (position_is_predicted) {
    alt = Calculated->NextAltitude;
    agl = Calculated->NextAltitudeAGL;
    lat = Calculated->NextLatitude;
    lon = Calculated->NextLongitude;
  } else {
    if (Basic->BaroAltitudeAvailable) {
      alt = Basic->BaroAltitude;
    } else {
      alt = Basic->Altitude;
    }
    agl = Calculated->AltitudeAGL;
    lat = Basic->Latitude;
    lon = Basic->Longitude;
  }

  // JMW TODO enhancement: FindAirspaceCircle etc should sort results, return
  // the most critical or closest.

  if (AirspaceCircle) {
    for (i=0; i<NumberOfAirspaceCircles; i++) {

      if ((((AirspaceCircle[i].Base.Base != abAGL) && (alt >= AirspaceCircle[i].Base.Altitude))
           || ((AirspaceCircle[i].Base.Base == abAGL) && (agl >= AirspaceCircle[i].Base.AGL)))
          && (((AirspaceCircle[i].Top.Base != abAGL) && (alt < AirspaceCircle[i].Top.Altitude))
           || ((AirspaceCircle[i].Top.Base == abAGL) && (agl < AirspaceCircle[i].Top.AGL)))) {

        if ((MapWindow::iAirspaceMode[AirspaceCircle[i].Type] >= 2) &&
	    InsideAirspaceCircle(lon, lat, i)) {

          AirspaceWarnListAdd(Basic, Calculated, position_is_predicted, 1, i, false);
        }

      }

    }
  }

  // repeat process for areas

  if (AirspaceArea) {
    for (i=0; i<NumberOfAirspaceAreas; i++) {

      if ((((AirspaceArea[i].Base.Base != abAGL) && (alt >= AirspaceArea[i].Base.Altitude))
           || ((AirspaceArea[i].Base.Base == abAGL) && (agl >= AirspaceArea[i].Base.AGL)))
          && (((AirspaceArea[i].Top.Base != abAGL) && (alt < AirspaceArea[i].Top.Altitude))
           || ((AirspaceArea[i].Top.Base == abAGL) && (agl < AirspaceArea[i].Top.AGL)))) {

        if ((MapWindow::iAirspaceMode[AirspaceArea[i].Type] >= 2)
            && InsideAirspaceArea(lon, lat, i)){

          AirspaceWarnListAdd(Basic, Calculated, position_is_predicted, 0, i, false);
        }

      }
    }
  }

  AirspaceWarnListProcess(Basic, Calculated);

  //  UnlockFlightData();

}

