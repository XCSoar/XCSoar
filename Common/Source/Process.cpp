/*
XCSoar Glide Computer
Copyright (C) 2000 - 2004  M Roberts

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
*/
#include "stdafx.h"
#include <windows.h>

#include "Process.h"
#include "externs.h"
#include "Utils.h"
#include "device.h"

// JMW added key codes,
// so -1 down
//     1 up
//     0 enter

void	AltitudeProcessing(int UpDown)
{
	#ifdef _SIM_
		if(UpDown==1)
			GPS_INFO.Altitude += (100/ALTITUDEMODIFY);
		else if (UpDown==-1)
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

void	McReadyProcessing(int UpDown)
{

	if(UpDown==1) {
		MACREADY += (double)0.2;

		if (MACREADY>10.0) { // JMW added sensible limit
			MACREADY=10.0;
		}
	}
	else if(UpDown==-1)
	{
		MACREADY -= (double)0.2;
		if(MACREADY < 0)
		{
			MACREADY = 0;
		}
	} else if (UpDown==0)
	{
		CALCULATED_INFO.AutoMcReady = !CALCULATED_INFO.AutoMcReady; // JMW toggle automacready
	}

  devPutMcReady(devA(), MACREADY);
  devPutMcReady(devB(), MACREADY);

	return;
}

extern void PopupWaypointDetails();

void NextUpDown(int UpDown)
{
	if(UpDown==1)
	{
		if(ActiveWayPoint < MAXTASKPOINTS)
		{
			if(Task[ActiveWayPoint+1].Index >= 0)
			{
				if(ActiveWayPoint == 0)
				{
					CALCULATED_INFO.TaskStartTime = GPS_INFO.Time ;
				}
				ActiveWayPoint ++;
				CALCULATED_INFO.LegStartTime = GPS_INFO.Time ;
			}
		}
	}
	else if (UpDown==-1)
	{
		if(ActiveWayPoint >0)
		{
			ActiveWayPoint --;
		}
	} else if (UpDown==0) {
		SelectedWaypoint = Task[ActiveWayPoint].Index;
		UnlockFlightData();
		PopupWaypointDetails();
		LockFlightData();
	}
}


void NoProcessing(int UpDown)
{
	return;
}




/////////////////////////////////////////////

void FormatterLowWarning::AssignValue(int i) {
  InfoBoxFormatter::AssignValue(i);
  switch (i) {
  case 1:
    minimum = ALTITUDEMODIFY*SAFETYALTITUDETERRAIN;
  default:
    break;
  }

}


void FormatterTime::SecsToDisplayTime(int d) {
  hours = (d/3600);
  mins = (d/60-hours*60);
  seconds = (d-mins*60+hours*3600);
}



int DetectStartTime() {
  static int starttime = -1;
  if (starttime == -1) {
    if (GPS_INFO.Speed > 5) {
      starttime = (int)GPS_INFO.Time;
    } else {
      return 0;
    }
  }
  return (int)GPS_INFO.Time-starttime;
}



void FormatterTime::AssignValue(int i) {
  switch (i) {
  case 27:
    SecsToDisplayTime(CALCULATED_INFO.AATTimeToGo);
    break;
  case 36:
    SecsToDisplayTime(DetectStartTime());
    break;
  default:
    break;
  }

}



void InfoBoxFormatter::AssignValue(int i) {
  switch (i) {
  case 0:
    Value = ALTITUDEMODIFY*GPS_INFO.Altitude;
    break;
  case 1:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.AltitudeAGL  ;
    break;
  case 2:
    Value = LIFTMODIFY*CALCULATED_INFO.Average30s;
    break;
  case 3:
    Value = CALCULATED_INFO.WaypointBearing;
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
  case 9:
    Value = CALCULATED_INFO.LastThermalTime;
    break;
  case 10:
    Value = MACREADY;
    break;
  case 11:
    Value = DISTANCEMODIFY*CALCULATED_INFO.WaypointDistance;
    break;
  case 12:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeDifference;
    break;
  case 13:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeRequired; 
    break;
  case 14:
    Value = 0; // Next Waypoint Text
    break;
  case 15:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeDifference;
    break;
  case 16:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeRequired;
    break;
  case 17:
    Value = SPEEDMODIFY*CALCULATED_INFO.TaskSpeed;
    break;
  case 18:
    Value = DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo; 
    break;
  case 19:
    if (CALCULATED_INFO.LDFinish== 999) {
      Valid = false;
    } else {
      Valid = true;
      Value = CALCULATED_INFO.LDFinish; 
    }
    break;
  case 20:
    Value = ALTITUDEMODIFY*CALCULATED_INFO.TerrainAlt ;
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
    break;
  case 29:
    Value = DISTANCEMODIFY*CALCULATED_INFO.AATMinDistance ; 
    break;
  case 30:
    Value = SPEEDMODIFY*CALCULATED_INFO.AATMaxSpeed;
    break;
  case 31:
    Value = SPEEDMODIFY*CALCULATED_INFO.AATMinSpeed;
    break;
  case 32:
    if (GPS_INFO.AirspeedAvailable) {
      Value = SPEEDMODIFY*GPS_INFO.Airspeed;
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 33:
    if (GPS_INFO.BaroAltitudeAvailable) {
      Value = ALTITUDEMODIFY*GPS_INFO.BaroAltitude;
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 34:
    Value = SPEEDMODIFY*CALCULATED_INFO.VMcReady; 
    break;
  case 35:
    Value = CALCULATED_INFO.PercentCircling;
    break;
  case 37:
    if (GPS_INFO.AccelerationAvailable) {
      Value = GPS_INFO.Gload;
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  default:
    break;

  };
}


void InfoBoxFormatter::Render(HWND hWnd) {
  if (Valid) {
    _stprintf(Text,
              Format, 
              Value );
  } else {
    _stprintf(Text,TEXT("---"));
  }
  SetWindowLong(hWnd, GWL_USERDATA, 0);	  
  SetWindowText(hWnd,Text);

}


void FormatterLowWarning::Render(HWND hWnd) {
  
  _stprintf(Text,
            Format, 
            Value );
  if (Value<minimum) {
    SetWindowLong(hWnd, GWL_USERDATA, 2); // red text
  } else {
    SetWindowLong(hWnd, GWL_USERDATA, 0);	  
  }
  SetWindowText(hWnd,Text);

}



void FormatterTime::Render(HWND hWnd) {
  if (hours<1) {
    _stprintf(Text,
              TEXT("%02d:%02d"), 
              mins, seconds );
  } else {
    _stprintf(Text,
              TEXT("%02d:%02d"), 
              hours, mins );

  }
  SetWindowLong(hWnd, GWL_USERDATA, 0);	  
  SetWindowText(hWnd,Text);
}


void FormatterWaypoint::Render(HWND hWnd) {

  if(ActiveWayPoint >=0)
    {
      if ( DisplayTextType == DISPLAYFIRSTTHREE)
        {
          _tcsncpy(Text,WayPointList[ Task[ActiveWayPoint].Index ].Name,3);
          Text[3] = '\0';
        }
      else if( DisplayTextType == DISPLAYNUMBER)
        {
          _stprintf(Text,TEXT("%d"),WayPointList[ Task[ActiveWayPoint].Index ].Number );
        }
      else
        {
          _tcsncpy(Text,WayPointList[ Task[ActiveWayPoint].Index ].Name,5);
          Text[5] = '\0';
        }
    }
  else
    {
      Text[0] = '\0';
    }

  SetWindowLong(hWnd, GWL_USERDATA, 0);	  
  SetWindowText(hWnd, Text);

}
