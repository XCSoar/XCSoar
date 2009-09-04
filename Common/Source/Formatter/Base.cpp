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

#include "Formatter/Base.hpp"
#include "XCSoar.h"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "Math/FastMath.h"
#include "Atmosphere.h"
#include "Battery.h"
#include "McReady.h"
#include "Units.hpp"
#include "Interface.hpp"
#include <stdio.h>

InfoBoxFormatter::InfoBoxFormatter(const TCHAR *theformat) {
  _tcscpy(Format, theformat);
  Valid = true;
  Value = 0.0;
  Text[0] = 0;
  CommentText[0] = 0;
}

const TCHAR *InfoBoxFormatter::Render(int *color) {
  if (Valid) {
    _stprintf(Text,
              Format,
              Value );
    *color = 0;
  } else {
    RenderInvalid(color);
  }
  return(Text);
}

const TCHAR *InfoBoxFormatter::RenderTitle(int *color) { // VENTA3
  if (Valid) {
    _stprintf(Text,
              Format,
              Value );
    *color = 0;
  } else {
    RenderInvalid(color);
  }
  return(Text);
}

void InfoBoxFormatter::RenderInvalid(int *color) {
  _tcscpy(CommentText, _T(""));
  _tcscpy(Text, _T("---"));
  *color = -1;
}

// TODO enhancement: crop long text or provide alternate
// e.g. 10300 ft ==> 10,3
// e.g. "Ardlethan" => "Ardl."


void InfoBoxFormatter::AssignValue(int i) {
  switch (i) {
  case 0:
    Value = ALTITUDEMODIFY*XCSoarInterface::Basic().Altitude;
    break;
  case 1:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().AltitudeAGL  ;
    Valid = XCSoarInterface::Calculated().TerrainValid;
    break;
  case 2:
    Value = LIFTMODIFY*XCSoarInterface::Calculated().Average30s;
    break;
  case 3:
    Value = XCSoarInterface::Calculated().WaypointBearing;
    Valid = XCSoarInterface::Calculated().WaypointDistance > 10.0;
    break;
  case 4:
    if (XCSoarInterface::Calculated().LD== 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = XCSoarInterface::Calculated().LD;
    }
    break;
  case 5:
    if (XCSoarInterface::Calculated().CruiseLD== 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = XCSoarInterface::Calculated().CruiseLD;
    }
    break;
  case 6:
    Value = SPEEDMODIFY*XCSoarInterface::Basic().Speed;
    break;
  case 7:
    Value = LIFTMODIFY*XCSoarInterface::Calculated().LastThermalAverage;
    break;
  case 8:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().LastThermalGain;
    break;
  case 10:
    Value = iround(LIFTMODIFY*GlidePolar::GetMacCready()*10)/10.0;
    break;
  case 11:
    Value = DISTANCEMODIFY*XCSoarInterface::Calculated().WaypointDistance;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 12:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().NextAltitudeDifference;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 13:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().NextAltitudeRequired;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 14:
    Value = 0; // Next Waypoint Text
    break;
  case 15:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().TaskAltitudeDifference;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 16:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().TaskAltitudeRequired;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 17:
    Value = TASKSPEEDMODIFY*XCSoarInterface::Calculated().TaskSpeed;
    if (ActiveWayPoint>=1) {
      Valid = ValidTaskPoint(ActiveWayPoint);
    } else {
      Valid = false;
    }
    break;
  case 18:
    if (XCSoarInterface::Calculated().ValidFinish) {
      Value = DISTANCEMODIFY*XCSoarInterface::Calculated().WaypointDistance;
    } else {
      Value = DISTANCEMODIFY*XCSoarInterface::Calculated().TaskDistanceToGo;
    }
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 19:
    if (XCSoarInterface::Calculated().LDFinish== 999) {
      Valid = false;
    } else {
      Valid = ValidTaskPoint(ActiveWayPoint);
      if (XCSoarInterface::Calculated().ValidFinish) {
        Value = 0;
      } else {
        Value = XCSoarInterface::Calculated().LDFinish;
      }
    }
    break;
  case 20:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().TerrainAlt ;
    Valid = XCSoarInterface::Calculated().TerrainValid;
    break;
  case 21:
    Value = LIFTMODIFY*XCSoarInterface::Calculated().AverageThermal;
    break;
  case 22:
    Value = ALTITUDEMODIFY*XCSoarInterface::Calculated().ThermalGain;
    break;
  case 23:
    Value = XCSoarInterface::Basic().TrackBearing;
    break;
  case 24:
    if (XCSoarInterface::Basic().VarioAvailable) {
      Value = LIFTMODIFY*XCSoarInterface::Basic().Vario;
    } else {
      Value = LIFTMODIFY*XCSoarInterface::Calculated().Vario;
    }
    break;
  case 25:
    Value = SPEEDMODIFY*XCSoarInterface::Calculated().WindSpeed;
    break;
  case 26:
    Value = XCSoarInterface::Calculated().WindBearing;
    break;
  case 28:
    Value = DISTANCEMODIFY*XCSoarInterface::Calculated().AATMaxDistance ;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    break;
  case 29:
    Value = DISTANCEMODIFY*XCSoarInterface::Calculated().AATMinDistance ;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    break;
  case 30:
    Value = TASKSPEEDMODIFY*XCSoarInterface::Calculated().AATMaxSpeed;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    if (XCSoarInterface::Calculated().AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 31:
    Value = TASKSPEEDMODIFY*XCSoarInterface::Calculated().AATMinSpeed;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    if (XCSoarInterface::Calculated().AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 32:
    Valid = XCSoarInterface::Basic().AirspeedAvailable;
    Value = SPEEDMODIFY*XCSoarInterface::Basic().IndicatedAirspeed;
    break;
  case 33:
    Valid = XCSoarInterface::Basic().BaroAltitudeAvailable;
    Value = ALTITUDEMODIFY*XCSoarInterface::Basic().BaroAltitude;
    break;
  case 34:
    Value = SPEEDMODIFY*XCSoarInterface::Calculated().VMacCready;
    break;
  case 35:
    Value = XCSoarInterface::Calculated().PercentCircling;
    break;
  case 37:
    Valid = XCSoarInterface::Basic().AccelerationAvailable;
    Value = XCSoarInterface::Basic().Gload;
    break;
  case 38:
    if (XCSoarInterface::Calculated().LDNext== 999) {
      Valid = false;
    } else {
      Valid = ValidTaskPoint(ActiveWayPoint);
      Value = XCSoarInterface::Calculated().LDNext;
    }
    break;
  case 43:
    //    Valid = XCSoarInterface::Basic().AirspeedAvailable;
    Value = XCSoarInterface::Calculated().VOpt*SPEEDMODIFY;
    break;
  case 44:
    //    Valid = XCSoarInterface::Basic().AirspeedAvailable;
    Value = XCSoarInterface::Calculated().NettoVario*LIFTMODIFY;
    break;
  case 48:
    Value = XCSoarInterface::Basic().OutsideAirTemperature;
    break;
  case 49:
    Value = XCSoarInterface::Basic().RelativeHumidity;
    break;
  case 50:
    Value = CuSonde::maxGroundTemperature;
    break;
  case 51:
    Value = DISTANCEMODIFY*XCSoarInterface::Calculated().AATTargetDistance ;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    break;
  case 52:
    Value = TASKSPEEDMODIFY*XCSoarInterface::Calculated().AATTargetSpeed;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    if (XCSoarInterface::Calculated().AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 53:
    if (XCSoarInterface::Calculated().LDvario== 999) {
      Valid = false;
    } else {
      Valid = XCSoarInterface::Basic().VarioAvailable && XCSoarInterface::Basic().AirspeedAvailable;
      Value = XCSoarInterface::Calculated().LDvario;
    }
    break;
  case 54:
    Valid = XCSoarInterface::Basic().AirspeedAvailable;
    Value = SPEEDMODIFY*XCSoarInterface::Basic().TrueAirspeed;
    break;
  case 56: // team bearing
    Value = XCSoarInterface::Calculated().TeammateBearing;
    Valid = true;
  case 58: // team range
	  if (TeammateCodeValid)
	  {
    Value = DISTANCEMODIFY*XCSoarInterface::Calculated().TeammateRange;
    if (Value > 100)
      {
        _tcscpy(Format, _T("%.0lf"));
      }
    else
      {
        _tcscpy(Format, _T("%.1lf"));
      }
    Valid = true;
	  }
	  else
	  {
		  Valid = false;
	  }
    break;
  case 59:
    Value = TASKSPEEDMODIFY*XCSoarInterface::Calculated().TaskSpeedInstantaneous;
    if (ActiveWayPoint>=1) {
      Valid = ValidTaskPoint(ActiveWayPoint);
    } else {
      Valid = false;
    }
    break;
  case 60:
    Value = DISTANCEMODIFY*XCSoarInterface::Calculated().HomeDistance ;
    if (HomeWaypoint>=0) {
      Valid = ValidWayPoint(HomeWaypoint);
    } else {
      Valid = false;
    }
    break;
  case 61:
    Value = TASKSPEEDMODIFY*XCSoarInterface::Calculated().TaskSpeedAchieved;
    if (ActiveWayPoint>=1) {
      Valid = ValidTaskPoint(ActiveWayPoint);
    } else {
      Valid = false;
    }
    break;
  case 63:
    if (XCSoarInterface::Calculated().timeCircling>0) {
      Value = LIFTMODIFY*XCSoarInterface::Calculated().TotalHeightClimb
        /XCSoarInterface::Calculated().timeCircling;
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;
  case 64:
    Value = LIFTMODIFY*XCSoarInterface::Calculated().DistanceVario;
    if (ActiveWayPoint>=1) {
      Valid = ValidTaskPoint(ActiveWayPoint);
    } else {
      Valid = false;
    }
    break;
  case 65: // battery voltage
#if (WINDOWSPC<1)
#ifndef GNAV
    Value = PDABatteryPercent;
    Valid = true;
#else
    Value = XCSoarInterface::Basic().SupplyBatteryVoltage;
    if (Value>0.0) {
      Valid = true;
    } else {
      Valid = false;
    }
#endif
#else
    Value = 0.0;
    Valid = false;
#endif
    break;
  case 66: // VENTA-ADDON added GR Final
    if (XCSoarInterface::Calculated().GRFinish== 999) {
      Valid = false;
    } else {
      Valid = ValidTaskPoint(ActiveWayPoint);
      if (XCSoarInterface::Calculated().ValidFinish) {
	Value = 0;
      } else {
	Value = XCSoarInterface::Calculated().GRFinish;
	if (Value >100 )
	  {
	    _tcscpy(Format, _T("%1.0f"));
	  }
	else
	  {
	    _tcscpy(Format, _T("%1.1f"));
	  }
      }
    }
    break;
  case 70:	// VENTA3 QFE
//    Valid = XCSoarInterface::Basic().Altitude;
    Value = ALTITUDEMODIFY* (XCSoarInterface::Basic().Altitude-QFEAltitudeOffset);
    break;
  case 71:
    if ( XCSoarInterface::Calculated().AverageLD == 0) {
      Valid = false;
    } else {
      Valid = true;
      Value = XCSoarInterface::Calculated().AverageLD;
      if (Value<0)
	    _tcscpy(Format, _T("^^^"));
      else if (Value>=999)
	    _tcscpy(Format, _T("+++"));
      else
	    _tcscpy(Format, _T("%2.0f"));

    }
    break;
  case 72:
    Value = XCSoarInterface::Calculated().Experimental;
    Valid = true;
    break;
    /* TODO feature: add extra infoboxes from Lars
  case 68: // distance flown
    if (XCSoarInterface::Calculated().TaskDistanceCovered != 0)
      {
	Value = DISTANCEMODIFY*XCSoarInterface::Calculated().TaskDistanceCovered;
	Valid = true;
      }
    else
      {
	Value = 0.0;
	Valid = false;
      }
    break;
  case 67: // termik liga points
    if (XCSoarInterface::Calculated().TermikLigaPoints != 0)
      {
	Value = XCSoarInterface::Calculated().TermikLigaPoints;
	Valid = true;
      }
    else
      {
	Value = 0.0;
	Valid = false;
      }
    break;
    */
  default:
    break;
  };
}

const TCHAR *InfoBoxFormatter::GetCommentText(void) {
  return CommentText;
}

bool InfoBoxFormatter::isValid(void) {
  return Valid;
}
