/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005  

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#include "stdafx.h"
#include <windows.h>

#include "Process.h"
#include "externs.h"
#include "Utils.h"
#include "device.h"
#include "Dialogs.h"
#include "Port.h"
#include "Atmosphere.h"

// JMW added key codes,
// so -1 down
//     1 up
//     0 enter

void	AirspeedProcessing(int UpDown)
{
  if (UpDown==0) {
    EnableCalibration = !EnableCalibration;
	// XXX InputEvents - Is this an automatic or user thing - either way, needs moving
    if (EnableCalibration) 
      DoStatusMessage(TEXT("Calibrate ON"));
    else 
      DoStatusMessage(TEXT("Calibrate OFF"));
  }

}

void	AltitudeProcessing(int UpDown)
{
	#ifdef _SIM_
	if(UpDown==1) {
	  GPS_INFO.Altitude += (100/ALTITUDEMODIFY);
	}	else if (UpDown==-1)
	  {
	    GPS_INFO.Altitude -= (100/ALTITUDEMODIFY);
	    if(GPS_INFO.Altitude < 0)
	      GPS_INFO.Altitude = 0;
	  } else if (UpDown==-2) {
	  DirectionProcessing(-1);
	} else if (UpDown==2) {
	  DirectionProcessing(1);
	}
#endif
	return;
}

void	SpeedProcessing(int UpDown)
{
	#ifdef _SIM_
		if(UpDown==1)
			GPS_INFO.Speed += (10/SPEEDMODIFY);
		else if (UpDown==-1)
		{
			GPS_INFO.Speed -= (10/SPEEDMODIFY);
			if(GPS_INFO.Speed < 0)
				GPS_INFO.Speed = 0;
		} else if (UpDown==-2) {
			DirectionProcessing(-1);
		} else if (UpDown==2) {
			DirectionProcessing(1);
		}
	#endif
	return;
}


extern TCHAR szRegistryAccelerometerZero[];


void	AccelerometerProcessing(int UpDown)
{
  DWORD Temp;
  if (UpDown==0) {
    AccelerometerZero*= GPS_INFO.Gload;
    if (AccelerometerZero<1) {
      AccelerometerZero = 100;
    }
    Temp = (int)AccelerometerZero;
    SetToRegistry(szRegistryAccelerometerZero,Temp);
  }
}

void	WindDirectionProcessing(int UpDown)
{
	
	if(UpDown==1)
	{
		CALCULATED_INFO.WindBearing  += 5;
		while (CALCULATED_INFO.WindBearing  >= 360)
		{
			CALCULATED_INFO.WindBearing  -= 360;
		}
	}
	else if (UpDown==-1)
	{
		CALCULATED_INFO.WindBearing  -= 5;
		while (CALCULATED_INFO.WindBearing  < 0)
		{
			CALCULATED_INFO.WindBearing  += 360;
		}
	} else if (UpDown == 0) {
	  SaveWindToRegistry();
	}
	return;
}


void	WindSpeedProcessing(int UpDown)
{
	if(UpDown==1)
		CALCULATED_INFO.WindSpeed += (1/SPEEDMODIFY);
	else if (UpDown== -1)
	{
		CALCULATED_INFO.WindSpeed -= (1/SPEEDMODIFY);
		if(CALCULATED_INFO.WindSpeed < 0)
			CALCULATED_INFO.WindSpeed = 0;
	} 
	// JMW added faster way of changing wind direction
	else if (UpDown== -2) {
		WindDirectionProcessing(-1);
	} else if (UpDown== 2) {
		WindDirectionProcessing(1);
	} else if (UpDown == 0) {
	  SaveWindToRegistry();
	}
	return;
}

void	DirectionProcessing(int UpDown)
{
	#ifdef _SIM_
	
		if(UpDown==1)
		{
			GPS_INFO.TrackBearing   += 5;
			while (GPS_INFO.TrackBearing  >= 360)
			{
				GPS_INFO.TrackBearing  -= 360;
			}
		}
		else if (UpDown==-1)
		{
			GPS_INFO.TrackBearing  -= 5;
			while (GPS_INFO.TrackBearing  < 0)
			{
				GPS_INFO.TrackBearing  += 360;
			}
		}
	#endif
	return;
}

void	MacCreadyProcessing(int UpDown)
{

  if(UpDown==1) {
    
    MACCREADY += (double)0.1;
    
    if (MACCREADY>5.0) { // JMW added sensible limit
      MACCREADY=5.0;
    }
  }
  else if(UpDown==-1)
    {
      MACCREADY -= (double)0.1;
      if(MACCREADY < 0)
	{
	  MACCREADY = 0;
	}

  } else if (UpDown==0)
    {
      CALCULATED_INFO.AutoMacCready = !CALCULATED_INFO.AutoMacCready; 
      // JMW toggle automacready
	} 
  else if (UpDown==-2)
    {
      CALCULATED_INFO.AutoMacCready = false;  // SDP on auto maccready
      
    }
  else if (UpDown==+2)
    {
      CALCULATED_INFO.AutoMacCready = true;	// SDP off auto maccready
      
    }
  
  devPutMacCready(devA(), MACCREADY); 
  devPutMacCready(devB(), MACCREADY);
  
  return;
}


void	ForecastTemperatureProcessing(int UpDown)
{
  if (UpDown==1) {
    CuSonde::adjustForecastTemperature(0.5);
  }
  if (UpDown== -1) {
    CuSonde::adjustForecastTemperature(-0.5);
  }
}


extern void PopupWaypointDetails();

/*
	1	Next waypoint
	0	Show waypoint details
	-1	Previous waypoint
	2	Next waypoint with wrap around
	-2	Previous waypoint with wrap around
*/
void NextUpDown(int UpDown)
{
  LockTaskData();

  if(UpDown>0) {
    if(ActiveWayPoint < MAXTASKPOINTS) {
      // Increment Waypoint
      if(Task[ActiveWayPoint+1].Index >= 0) {
	if(ActiveWayPoint == 0)	{
	  // manual start
	  // JMW: TODO allow restart
	  // JMW: make this work only for manual
	  if (CALCULATED_INFO.TaskStartTime==0) {
	    CALCULATED_INFO.TaskStartTime = GPS_INFO.Time;
	  }
	}
	ActiveWayPoint ++;
	AdvanceArmed = false;
	CALCULATED_INFO.LegStartTime = GPS_INFO.Time ;
      }
      // No more, try first
      else 
        if((UpDown == 2) && (Task[0].Index >= 0)) {
          /* ****DISABLED****
          if(ActiveWayPoint == 0)	{
            // JMW: TODO allow restart
            // JMW: make this work only for manual
            
            // TODO: This should trigger reset of flight stats, but 
            // should ask first...
            if (CALCULATED_INFO.TaskStartTime==0) {
              CALCULATED_INFO.TaskStartTime = GPS_INFO.Time ;
            }            
          }
          */
          AdvanceArmed = false;
          ActiveWayPoint = 0;
          CALCULATED_INFO.LegStartTime = GPS_INFO.Time ;
        }
    }
  }
  else if (UpDown<0){
    if(ActiveWayPoint >0) {

      ActiveWayPoint --;
      /*
	XXX How do we know what the last one is?
	} else if (UpDown == -2) {
	ActiveWayPoint = MAXTASKPOINTS;
      */
    } else {
      if (ActiveWayPoint==0) {

        RotateStartPoints();

	// restarted task..
	// JMW TODO: ask if user wants to restart task?
	//	CALCULATED_INFO.TaskStartTime = 0;
      }
    }
    
  } 
  else if (UpDown==0) {
    SelectedWaypoint = Task[ActiveWayPoint].Index;
    PopupWaypointDetails();
  }
  if (ActiveWayPoint>=0) {
    SelectedWaypoint = Task[ActiveWayPoint].Index;
  }
  UnlockTaskData();
}


void NoProcessing(int UpDown)
{
	(void)UpDown;
	return;
}




/////////////////////////////////////////////

void FormatterLowWarning::AssignValue(int i) {
  InfoBoxFormatter::AssignValue(i);
  switch (i) {
  case 1:
    minimum = ALTITUDEMODIFY*SAFETYALTITUDETERRAIN;
    break;
  case 2:
    minimum = 0.5*LIFTMODIFY*MACCREADY;
    break;
  default:
    break;
  }
}


void FormatterTime::SecsToDisplayTime(int d) {
  int dd = d % (3600*24);

  hours = (dd/3600);
  mins = (dd/60-hours*60);
  seconds = (dd-mins*60+hours*3600);
  hours = hours % 24;
  if (dd<0) {
    Valid = FALSE;
  } else {
    Valid = TRUE;
  }
}


int TimeLocal(int localtime) {
  localtime += GetUTCOffset();
  if (localtime<0) {
    localtime+= 3600*24;
  }
  return localtime;
}

int DetectCurrentTime() {
  int localtime = (int)GPS_INFO.Time;
  return TimeLocal(localtime);
}


int DetectStartTime() {
  // JMW added restart ability
  //
  // we want this to display landing time until next takeoff 

  static int starttime = -1;
  static int lastflighttime = -1;

  if (CALCULATED_INFO.Flying) {
    if (starttime == -1) {
      // hasn't been started yet
      
      starttime = (int)GPS_INFO.Time;
      
      lastflighttime = -1;
    }
    return (int)GPS_INFO.Time-starttime;

  } else {

    if (lastflighttime == -1) {
      // hasn't been stopped yet
      if (starttime>=0) {
	lastflighttime = (int)GPS_INFO.Time-starttime;
      } else {
	return 0; // no last flight time
      }
      // reset for next takeoff
      starttime = -1;
    }
  }

  // return last flighttime if it exists
  return max(0,lastflighttime);
}


void FormatterTime::AssignValue(int i) {
  switch (i) {
  case 9:
    SecsToDisplayTime((int)CALCULATED_INFO.LastThermalTime);
    break;
  case 27:
    SecsToDisplayTime((int)CALCULATED_INFO.AATTimeToGo);
    break;
  case 36:
    SecsToDisplayTime((int)CALCULATED_INFO.FlightTime);
    break;
  case 39:
    SecsToDisplayTime(DetectCurrentTime());
    break;
  case 40:
    SecsToDisplayTime((int)(GPS_INFO.Time));
    break;
  case 41:
    SecsToDisplayTime((int)(CALCULATED_INFO.TaskTimeToGo));
    break;
  case 42:
    SecsToDisplayTime((int)(CALCULATED_INFO.LegTimeToGo));
    break;
  case 45:
    SecsToDisplayTime((int)(CALCULATED_INFO.TaskTimeToGo+DetectCurrentTime()));
    break;
  case 46:
    SecsToDisplayTime((int)(CALCULATED_INFO.LegTimeToGo+DetectCurrentTime()));
    break;
  default:
    break;
  }

}

// TODO crop long text or provide alternate
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
    if (ActiveWayPoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 12:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeDifference;
    if (ActiveWayPoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 13:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeRequired; 
    if (ActiveWayPoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 14:
    Value = 0; // Next Waypoint Text
    break;
  case 15:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeDifference;
    if (ActiveWayPoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 16:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeRequired;
    if (ActiveWayPoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 17:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.TaskSpeed;
    if (ActiveWayPoint>=1) {
      Valid = true;
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
    if (ActiveWayPoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 19:
    if (CALCULATED_INFO.LDFinish== 999) {
      Valid = false;
    } else {
      Valid = true;
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
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
      Valid = false;
    }
    break;
  case 29:
    Value = DISTANCEMODIFY*CALCULATED_INFO.AATMinDistance ; 
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
      Valid = false;
    }
    break;
  case 30:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.AATMaxSpeed;
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
      Valid = false;
    }
    if (CALCULATED_INFO.AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 31:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.AATMinSpeed;
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
      Valid = false;
    }
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
      Valid = true;
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
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
      Valid = false;
    }
    break;
  case 52:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.AATTargetSpeed;
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
      Valid = false;
    }
    if (CALCULATED_INFO.AATTimeToGo<1) {
      Valid = false;
    }
    break;
  case 53:
    if (CALCULATED_INFO.LDvario== 999) {
      Valid = false;
    } else {
      Valid = true;
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
    break;
  case 59:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.TaskSpeedInstantaneous;
    if (ActiveWayPoint>=1) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 60:
    Value = DISTANCEMODIFY*CALCULATED_INFO.HomeDistance ; 
    if (HomeWaypoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 61:
    Value = TASKSPEEDMODIFY*CALCULATED_INFO.TaskSpeedAchieved;
    if (ActiveWayPoint>=1) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  default:
    break;

  };
}


TCHAR *InfoBoxFormatter::Render(int *color) {
  if (Valid) {
    _stprintf(Text,
              Format,
              Value );
  } else {
    _stprintf(Text,TEXT("---"));
  }
  *color = 0;
  return(Text);
}

TCHAR *FormatterLowWarning::Render(int *color) {

  if (Valid) {
    _stprintf(Text,
              Format,
              Value );
  } else {
    _stprintf(Text,TEXT("---"));
  }
  if (Value<minimum) {
    *color = 1; // red
  } else {
    *color = 0;
  }
  return(Text);
}


TCHAR *FormatterTime::Render(int *color) {
  if (Valid==FALSE) {
    _stprintf(Text,TEXT("--:--"));
  } else {
    if (hours<1) {
      _stprintf(Text,
                TEXT("%02d:%02d"),
                mins, seconds );
    } else {
      _stprintf(Text,
                TEXT("%02d:%02d"),
                hours, mins );

    }
  }
  *color = 0;
  return(Text);
}

TCHAR *FormatterWaypoint::Render(int *color) {
  int thewaypoint = ActiveWayPoint;
  LockTaskData();
  if((thewaypoint >=0)&&(WayPointList))
    {
      int index = Task[thewaypoint].Index;
      if ((index>=0) && (WayPointList[index].Reachable)) {
	*color = 2; // blue text
      } else {
	*color = 0; // black text
      }
      if ( DisplayTextType == DISPLAYFIRSTTHREE)
        {
          _tcsncpy(Text,WayPointList[index].Name,3);
          Text[3] = '\0';
        }
      else if( DisplayTextType == DISPLAYNUMBER)
        {
          _stprintf(Text,TEXT("%d"),
		    WayPointList[index].Number );
        }
      else
        {
          _tcsncpy(Text,WayPointList[index].Name,
                   (sizeof(Text)/sizeof(TCHAR))-1);
          Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
        }
    }
  else
    {
      *color = 0;
      Text[0] = '\0';
    }
  UnlockTaskData();

  return(Text);
}

TCHAR *FormatterDiffBearing::Render(int *color) {

  if (ActiveWayPoint>=0 && CALCULATED_INFO.WaypointDistance > 10.0) {
    Valid = true;

    Value = CALCULATED_INFO.WaypointBearing -  GPS_INFO.TrackBearing;

    if (Value < -180.0)
      Value += 360.0;
    else
    if (Value > 180.0)
      Value -= 360.0;

    if (Value > 1)
      _stprintf(Text, TEXT("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("«%2.0f°"), -Value);
    else
      _tcscpy(Text, TEXT("«»"));

  } else {
    Valid = false;
    _tcscpy(Text, TEXT("---"));
  }

  *color = 0;
  return(Text);
}




TCHAR *FormatterTeamCode::Render(int *color) {

  if((TeamCodeRefWaypoint >=0)&&(WayPointList))
    {

      *color = 0; // black text
       _tcsncpy(Text,CALCULATED_INFO.OwnTeamCode,5);
       Text[5] = '\0';
    }
  else
    {
      // no waypoint selected
      *color = 0;
      Text[0] = '\0';
    }

  return(Text);
}


TCHAR *FormatterDiffTeamBearing::Render(int *color) {

  if((TeamCodeRefWaypoint >=0)&&(WayPointList))
    {
      Valid = true;
      
      Value = CALCULATED_INFO.TeammateBearing -  GPS_INFO.TrackBearing;
      
      if (Value < -180.0)
        Value += 360.0;
      else
        if (Value > 180.0)
          Value -= 360.0;
      
      if (Value > 1)
        _stprintf(Text, TEXT("%2.0f°»"), Value);
      else if (Value < -1)
        _stprintf(Text, TEXT("«%2.0f°"), -Value);
      else
        _tcscpy(Text, TEXT("«»"));
      
    } else {
    Valid = false;
    _tcscpy(Text, TEXT("---"));
  }
  
  *color = 0;
  return(Text);
}
