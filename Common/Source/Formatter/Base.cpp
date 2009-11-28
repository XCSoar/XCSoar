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
#include "Task.h"
#include "Math/FastMath.h"
#include "Atmosphere.h"
#include "Battery.h"
#include "McReady.h"
#include "Units.hpp"
#include "Interface.hpp"
#include <stdio.h>
#include "Math/Pressure.h"
#include "Components.hpp"
#include "WayPointList.hpp"

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
    Value = ALTITUDEMODIFY*Basic().Altitude;
    break;
  case 1:
    Value = ALTITUDEMODIFY*Calculated().AltitudeAGL  ;
    Valid = Calculated().TerrainValid;
    break;
  case 2:
    Value = LIFTMODIFY*Calculated().Average30s;
    break;
  case 3:
    Value = Calculated().WaypointBearing;
    Valid = Calculated().WaypointDistance > 10.0;
    break;
  case 4:
    if (Calculated().LD== 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().LD;
    }
    break;
  case 5:
    if (Calculated().CruiseLD== 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().CruiseLD;
    }
    break;
  case 6:
    Value = SPEEDMODIFY*Basic().Speed;
    break;
  case 7:
    Value = LIFTMODIFY*Calculated().LastThermalAverage;
    break;
  case 8:
    Value = ALTITUDEMODIFY*Calculated().LastThermalGain;
    break;
  case 10:
    Value = iround(LIFTMODIFY*GlidePolar::GetMacCready()*10)/10.0;
    break;
  case 11:
    Value = DISTANCEMODIFY*Calculated().WaypointDistance;
    Valid = task.ValidTaskPoint(task.getActiveIndex());
    break;
  case 12:
    Value = ALTITUDEMODIFY*Calculated().NextAltitudeDifference;
    Valid = task.ValidTaskPoint(task.getActiveIndex());
    break;
  case 13:
    Value = ALTITUDEMODIFY*Calculated().NextAltitudeRequired;
    Valid = task.ValidTaskPoint(task.getActiveIndex());
    break;
  case 14:
    Value = 0; // Next Waypoint Text
    break;
  case 15:
    Value = ALTITUDEMODIFY*Calculated().TaskAltitudeDifference;
    Valid = task.ValidTaskPoint(task.getActiveIndex());
    break;
  case 16:
    Value = ALTITUDEMODIFY*Calculated().TaskAltitudeRequired;
    Valid = task.ValidTaskPoint(task.getActiveIndex());
    break;
  case 17:
    Value = TASKSPEEDMODIFY*Calculated().TaskSpeed;
    if (task.getActiveIndex()>=1) {
      Valid = task.ValidTaskPoint(task.getActiveIndex());
    } else {
      Valid = false;
    }
    break;
  case 18:
    if (Calculated().ValidFinish) {
      Value = DISTANCEMODIFY*Calculated().WaypointDistance;
    } else {
      Value = DISTANCEMODIFY*Calculated().TaskDistanceToGo;
    }
    Valid = task.ValidTaskPoint(task.getActiveIndex());
    break;
  case 19:
    if (Calculated().LDFinish== 999) {
      Valid = false;
    } else {
      Valid = task.ValidTaskPoint(task.getActiveIndex());
      if (Calculated().ValidFinish) {
        Value = 0;
      } else {
        Value = Calculated().LDFinish;
      }
    }
    break;
  case 20:
    Value = ALTITUDEMODIFY*Calculated().TerrainAlt ;
    Valid = Calculated().TerrainValid;
    break;
  case 21:
    Value = LIFTMODIFY*Calculated().AverageThermal;
    break;
  case 22:
    Value = ALTITUDEMODIFY*Calculated().ThermalGain;
    break;
  case 23:
    Value = Basic().TrackBearing;
    break;
  case 24:
    if (Basic().VarioAvailable) {
      Value = LIFTMODIFY*Basic().Vario;
    } else {
      Value = LIFTMODIFY*Calculated().Vario;
    }
    break;
  case 25:
    Value = SPEEDMODIFY*Calculated().WindSpeed;
    break;
  case 26:
    Value = Calculated().WindBearing;
    break;
  case 28:
    Value = DISTANCEMODIFY*Calculated().AATMaxDistance ;
    Valid = task.ValidTaskPoint(task.getActiveIndex()) && task.getSettings().AATEnabled;
    break;
  case 29:
    Value = DISTANCEMODIFY*Calculated().AATMinDistance ;
    Valid = task.ValidTaskPoint(task.getActiveIndex()) && task.getSettings().AATEnabled;
    break;
  case 30:
    Value = TASKSPEEDMODIFY*Calculated().AATMaxSpeed;
    Valid = task.ValidTaskPoint(task.getActiveIndex()) && task.getSettings().AATEnabled;
    if (Calculated().AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 31:
    Value = TASKSPEEDMODIFY*Calculated().AATMinSpeed;
    Valid = task.ValidTaskPoint(task.getActiveIndex()) && task.getSettings().AATEnabled;
    if (Calculated().AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 32:
    Valid = Basic().AirspeedAvailable;
    Value = SPEEDMODIFY*Basic().IndicatedAirspeed;
    break;
  case 33:
    Valid = Basic().BaroAltitudeAvailable;
    Value = ALTITUDEMODIFY*Basic().BaroAltitude;
    break;
  case 34:
    Value = SPEEDMODIFY*Calculated().VMacCready;
    break;
  case 35:
    Value = Calculated().PercentCircling;
    break;
  case 37:
    Valid = Basic().AccelerationAvailable;
    Value = Basic().Gload;
    break;
  case 38:
    if (Calculated().LDNext== 999) {
      Valid = false;
    } else {
      Valid = task.ValidTaskPoint(task.getActiveIndex());
      Value = Calculated().LDNext;
    }
    break;
  case 43:
    //    Valid = Basic().AirspeedAvailable;
    Value = Calculated().VOpt*SPEEDMODIFY;
    break;
  case 44:
    //    Valid = Basic().AirspeedAvailable;
    Value = Calculated().NettoVario*LIFTMODIFY;
    break;
  case 48:
    Value = Basic().OutsideAirTemperature;
    break;
  case 49:
    Value = Basic().RelativeHumidity;
    break;
  case 50:
    Value = CuSonde::maxGroundTemperature;
    break;
  case 51:
    Value = DISTANCEMODIFY*Calculated().AATTargetDistance ;
    Valid = task.ValidTaskPoint(task.getActiveIndex()) && task.getSettings().AATEnabled;
    break;
  case 52:
    Value = TASKSPEEDMODIFY*Calculated().AATTargetSpeed;
    Valid = task.ValidTaskPoint(task.getActiveIndex()) && task.getSettings().AATEnabled;
    if (Calculated().AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 53:
    if (Calculated().LDvario== 999) {
      Valid = false;
    } else {
      Valid = Basic().VarioAvailable && Basic().AirspeedAvailable;
      Value = Calculated().LDvario;
    }
    break;
  case 54:
    Valid = Basic().AirspeedAvailable;
    Value = SPEEDMODIFY*Basic().TrueAirspeed;
    break;
  case 56: // team bearing
    Value = Calculated().TeammateBearing;
    Valid = true;
  case 58: // team range
    if (SettingsComputer().TeammateCodeValid)
      {
	Value = DISTANCEMODIFY*Calculated().TeammateRange;
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
    Value = TASKSPEEDMODIFY*Calculated().TaskSpeedInstantaneous;
    if (task.getActiveIndex()>=1) {
      Valid = task.ValidTaskPoint(task.getActiveIndex());
    } else {
      Valid = false;
    }
    break;
  case 60:
    Value = DISTANCEMODIFY*Calculated().HomeDistance ;
    if (SettingsComputer().HomeWaypoint>=0) {
      Valid = way_points.verify_index(SettingsComputer().HomeWaypoint);
    } else {
      Valid = false;
    }
    break;
  case 61:
    Value = TASKSPEEDMODIFY*Calculated().TaskSpeedAchieved;
    if (task.getActiveIndex()>=1) {
      Valid = task.ValidTaskPoint(task.getActiveIndex());
    } else {
      Valid = false;
    }
    break;
  case 63:
    if (Calculated().timeCircling>0) {
      Value = LIFTMODIFY*Calculated().TotalHeightClimb
        /Calculated().timeCircling;
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;
  case 64:
    Value = LIFTMODIFY*Calculated().DistanceVario;
    if (task.getActiveIndex()>=1) {
      Valid = task.ValidTaskPoint(task.getActiveIndex());
    } else {
      Valid = false;
    }
    break;
  case 65: // battery voltage
#if !defined(WINDOWSPC) && !defined(HAVE_POSIX)
#ifndef GNAV
    Value = PDABatteryPercent;
    Valid = true;
#else
    Value = Basic().SupplyBatteryVoltage;
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
    if (Calculated().GRFinish== 999) {
      Valid = false;
    } else {
      Valid = task.ValidTaskPoint(task.getActiveIndex());
      if (Calculated().ValidFinish) {
	Value = 0;
      } else {
	Value = Calculated().GRFinish;
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
//    Valid = Basic().Altitude;
    Value = ALTITUDEMODIFY* (Basic().Altitude-QFEAltitudeOffset);
    break;
  case 71:
    if ( Calculated().AverageLD == 0) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().AverageLD;
      if (Value<0)
	    _tcscpy(Format, _T("^^^"));
      else if (Value>=999)
	    _tcscpy(Format, _T("+++"));
      else
	    _tcscpy(Format, _T("%2.0f"));

    }
    break;
  case 72:
    Value = Calculated().Experimental;
    Valid = true;
    break;
    /* TODO feature: add extra infoboxes from Lars
  case 68: // distance flown
    if (Calculated().TaskDistanceCovered != 0)
      {
	Value = DISTANCEMODIFY*Calculated().TaskDistanceCovered;
	Valid = true;
      }
    else
      {
	Value = 0.0;
	Valid = false;
      }
    break;
  case 67: // termik liga points
    if (Calculated().TermikLigaPoints != 0)
      {
	Value = Calculated().TermikLigaPoints;
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
