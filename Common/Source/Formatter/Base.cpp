#include "Formatter/Base.hpp"
#include "Math/FastMath.h"
#include "XCSoar.h"
#include "externs.h"
#include "Atmosphere.h"
#include "Battery.h"

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
    Value = ALTITUDEMODIFY*GPS_INFO.Altitude;
    break;
  case 1:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.AltitudeAGL  ;
    Valid = CALCULATED_INFO.TerrainValid;
    break;
  case 2:
    Value = LIFTMODIFY*CALCULATED_INFO.Average30s;
    break;
  case 3:
    Value = CALCULATED_INFO.WaypointBearing;
    Valid = CALCULATED_INFO.WaypointDistance > 10.0;
    break;
  case 4:
    if (CALCULATED_INFO.LD== 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = CALCULATED_INFO.LD;
    }
    break;
  case 5:
    if (CALCULATED_INFO.CruiseLD== 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = CALCULATED_INFO.CruiseLD;
    }
    break;
  case 6:
    Value = SPEEDMODIFY*GPS_INFO.Speed;
    break;
  case 7:
    Value = LIFTMODIFY*CALCULATED_INFO.LastThermalAverage;
    break;
  case 8:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.LastThermalGain;
    break;
  case 10:
    Value = iround(LIFTMODIFY*MACCREADY*10)/10.0;
    break;
  case 11:
    Value = DISTANCEMODIFY*CALCULATED_INFO.WaypointDistance;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 12:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeDifference;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 13:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeRequired;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 14:
    Value = 0; // Next Waypoint Text
    break;
  case 15:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeDifference;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 16:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeRequired;
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 17:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.TaskSpeed;
    if (ActiveWayPoint>=1) {
      Valid = ValidTaskPoint(ActiveWayPoint);
    } else {
      Valid = false;
    }
    break;
  case 18:
    if (CALCULATED_INFO.ValidFinish) {
      Value = DISTANCEMODIFY*CALCULATED_INFO.WaypointDistance;
    } else {
      Value = DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo;
    }
    Valid = ValidTaskPoint(ActiveWayPoint);
    break;
  case 19:
    if (CALCULATED_INFO.LDFinish== 999) {
      Valid = false;
    } else {
      Valid = ValidTaskPoint(ActiveWayPoint);
      if (CALCULATED_INFO.ValidFinish) {
        Value = 0;
      } else {
        Value = CALCULATED_INFO.LDFinish;
      }
    }
    break;
  case 20:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TerrainAlt ;
    Valid = CALCULATED_INFO.TerrainValid;
    break;
  case 21:
    Value = LIFTMODIFY*CALCULATED_INFO.AverageThermal;
    break;
  case 22:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.ThermalGain;
    break;
  case 23:
    Value = GPS_INFO.TrackBearing;
    break;
  case 24:
    if (GPS_INFO.VarioAvailable) {
      Value = LIFTMODIFY*GPS_INFO.Vario;
    } else {
      Value = LIFTMODIFY*CALCULATED_INFO.Vario;
    }
    break;
  case 25:
    Value = SPEEDMODIFY*CALCULATED_INFO.WindSpeed;
    break;
  case 26:
    Value = CALCULATED_INFO.WindBearing;
    break;
  case 28:
    Value = DISTANCEMODIFY*CALCULATED_INFO.AATMaxDistance ;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    break;
  case 29:
    Value = DISTANCEMODIFY*CALCULATED_INFO.AATMinDistance ;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    break;
  case 30:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.AATMaxSpeed;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    if (CALCULATED_INFO.AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 31:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.AATMinSpeed;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    if (CALCULATED_INFO.AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 32:
    Valid = GPS_INFO.AirspeedAvailable;
    Value = SPEEDMODIFY*GPS_INFO.IndicatedAirspeed;
    break;
  case 33:
    Valid = GPS_INFO.BaroAltitudeAvailable;
    Value = ALTITUDEMODIFY*GPS_INFO.BaroAltitude;
    break;
  case 34:
    Value = SPEEDMODIFY*CALCULATED_INFO.VMacCready;
    break;
  case 35:
    Value = CALCULATED_INFO.PercentCircling;
    break;
  case 37:
    Valid = GPS_INFO.AccelerationAvailable;
    Value = GPS_INFO.Gload;
    break;
  case 38:
    if (CALCULATED_INFO.LDNext== 999) {
      Valid = false;
    } else {
      Valid = ValidTaskPoint(ActiveWayPoint);
      Value = CALCULATED_INFO.LDNext;
    }
    break;
  case 43:
    //    Valid = GPS_INFO.AirspeedAvailable;
    Value = CALCULATED_INFO.VOpt*SPEEDMODIFY;
    break;
  case 44:
    //    Valid = GPS_INFO.AirspeedAvailable;
    Value = CALCULATED_INFO.NettoVario*LIFTMODIFY;
    break;
  case 48:
    Value = GPS_INFO.OutsideAirTemperature;
    break;
  case 49:
    Value = GPS_INFO.RelativeHumidity;
    break;
  case 50:
    Value = CuSonde::maxGroundTemperature;
    break;
  case 51:
    Value = DISTANCEMODIFY*CALCULATED_INFO.AATTargetDistance ;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    break;
  case 52:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.AATTargetSpeed;
    Valid = ValidTaskPoint(ActiveWayPoint) && AATEnabled;
    if (CALCULATED_INFO.AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 53:
    if (CALCULATED_INFO.LDvario== 999) {
      Valid = false;
    } else {
      Valid = GPS_INFO.VarioAvailable && GPS_INFO.AirspeedAvailable;
      Value = CALCULATED_INFO.LDvario;
    }
    break;
  case 54:
    Valid = GPS_INFO.AirspeedAvailable;
    Value = SPEEDMODIFY*GPS_INFO.TrueAirspeed;
    break;
  case 56: // team bearing
    Value = CALCULATED_INFO.TeammateBearing;
    Valid = true;
  case 58: // team range
	  if (TeammateCodeValid)
	  {
    Value = DISTANCEMODIFY*CALCULATED_INFO.TeammateRange;
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
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.TaskSpeedInstantaneous;
    if (ActiveWayPoint>=1) {
      Valid = ValidTaskPoint(ActiveWayPoint);
    } else {
      Valid = false;
    }
    break;
  case 60:
    Value = DISTANCEMODIFY*CALCULATED_INFO.HomeDistance ;
    if (HomeWaypoint>=0) {
      Valid = ValidWayPoint(HomeWaypoint);
    } else {
      Valid = false;
    }
    break;
  case 61:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.TaskSpeedAchieved;
    if (ActiveWayPoint>=1) {
      Valid = ValidTaskPoint(ActiveWayPoint);
    } else {
      Valid = false;
    }
    break;
  case 63:
    if (CALCULATED_INFO.timeCircling>0) {
      Value = LIFTMODIFY*CALCULATED_INFO.TotalHeightClimb
        /CALCULATED_INFO.timeCircling;
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;
  case 64:
    Value = LIFTMODIFY*CALCULATED_INFO.DistanceVario;
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
    Value = GPS_INFO.SupplyBatteryVoltage;
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
    if (CALCULATED_INFO.GRFinish== 999) {
      Valid = false;
    } else {
      Valid = ValidTaskPoint(ActiveWayPoint);
      if (CALCULATED_INFO.ValidFinish) {
	Value = 0;
      } else {
	Value = CALCULATED_INFO.GRFinish;
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
//    Valid = GPS_INFO.Altitude;
    Value = ALTITUDEMODIFY* (GPS_INFO.Altitude-QFEAltitudeOffset);
    break;
  case 71:
    if ( CALCULATED_INFO.AverageLD == 0) {
      Valid = false;
    } else {
      Valid = true;
      Value = CALCULATED_INFO.AverageLD;
      if (Value<0)
	    _tcscpy(Format, _T("^^^"));
      else if (Value>=999)
	    _tcscpy(Format, _T("+++"));
      else
	    _tcscpy(Format, _T("%2.0f"));

    }
    break;
  case 72:
    Value = CALCULATED_INFO.Experimental;
    Valid = true;
    break;
    /* TODO feature: add extra infoboxes from Lars
  case 68: // distance flown
    if (CALCULATED_INFO.TaskDistanceCovered != 0)
      {
	Value = DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceCovered;
	Valid = true;
      }
    else
      {
	Value = 0.0;
	Valid = false;
      }
    break;
  case 67: // termik liga points
    if (CALCULATED_INFO.TermikLigaPoints != 0)
      {
	Value = CALCULATED_INFO.TermikLigaPoints;
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
