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
#include "Dialogs.h"
#include "Port.h"

// JMW added key codes,
// so -1 down
//     1 up
//     0 enter

void	AirspeedProcessing(int UpDown)
{
  if (UpDown==0) {
    EnableCalibration = !EnableCalibration;
    if (EnableCalibration)
      DoStatusMessage(TEXT("Calibrate ON"));
    else
      DoStatusMessage(TEXT("Calibrate OFF"));
  }

}

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


extern TCHAR szRegistryAccelerometerZero[];


void	AccelerometerProcessing(int UpDown)
{
  DWORD Temp;
  if (UpDown==0) {
    AccelerometerZero*= GPS_INFO.Gload;
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

void	McCreadyProcessing(int UpDown)
{

  if(UpDown==1) {

    MCCREADY += (double)0.2;

    if (MCCREADY>10.0) { // JMW added sensible limit
      MCCREADY=10.0;
    }
  }
  else if(UpDown==-1)
    {
      MCCREADY -= (double)0.2;
      if(MCCREADY < 0)
	{
	  MCCREADY = 0;
	}
    } else if (UpDown==0)
    {
      CALCULATED_INFO.AutoMcCready = !CALCULATED_INFO.AutoMcCready; // JMW toggle automacready

    }

  devPutMcCready(devA(), MCCREADY); // NOTE THIS IS IN USER UNITS
  devPutMcCready(devB(), MCCREADY);

  // JMW testing only
  if (Port2Available && GPS_INFO.VarioAvailable) {
    if (UpDown==1) {
      Port2WriteNMEA(TEXT("PDVTM,2"));
    }
    if (UpDown==-1) {
	Port2WriteNMEA(TEXT("PDVTM,0"));
    }
    if (UpDown== 0) {
	Port2WriteNMEA(TEXT("PDAPL,52"));
    }
    //    Port2WriteString(TEXT("$PDVTM,1*46\r\n"));
    //    Port2WriteString(TEXT("$PDAPL,52*62\r\n"));
    //    Port2WriteString(TEXT("$PDVAD,10,-10,0*76\r\n"));
    //    Port2WriteString(TEXT("$PDVGP,-123,777,-1543,-9,0*59\r\n"));
    //    Port2WriteString(TEXT("$PDVMC,20,300,0,220,1013*52\r\n"));
    //    Port2WriteString(TEXT("$PDVAL,90,750,300,100,0*6A\r\n"));
  }

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
  hours = hours % 24;
  if (d<0) {
    Valid = FALSE;
  } else {
    Valid = TRUE;
  }
}


int DetectCurrentTime() {
  TIME_ZONE_INFORMATION TimeZoneInformation;
  GetTimeZoneInformation(&TimeZoneInformation);
  int localtime = ((int)GPS_INFO.Time-TimeZoneInformation.Bias*60);
  return localtime;
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
  case 9:
    SecsToDisplayTime((int)CALCULATED_INFO.LastThermalTime);
    break;
  case 27:
    SecsToDisplayTime((int)CALCULATED_INFO.AATTimeToGo);
    break;
  case 36:
    SecsToDisplayTime(DetectStartTime());
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
    Valid = CALCULATED_INFO.TerrainValid;
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
  case 10:
    Value = MCCREADY;
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
    Value = SPEEDMODIFY*CALCULATED_INFO.TaskSpeed;
    if (ActiveWayPoint>=0) {
      Valid = true;
    } else {
      Valid = false;
    }
    break;
  case 18:
    Value = DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo;
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
      Value = CALCULATED_INFO.LDFinish;
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
    Value = SPEEDMODIFY*CALCULATED_INFO.AATMaxSpeed;
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
      Valid = false;
    }
    break;
  case 31:
    Value = SPEEDMODIFY*CALCULATED_INFO.AATMinSpeed;
    if (ActiveWayPoint>=0) {
      Valid = AATEnabled;
    } else {
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
    Value = SPEEDMODIFY*CALCULATED_INFO.VMcCready;
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

  if (Valid) {
    _stprintf(Text,
              Format,
              Value );
  } else {
    _stprintf(Text,TEXT("---"));
  }
  if (Value<minimum) {
    SetWindowLong(hWnd, GWL_USERDATA, 2); // red text
  } else {
    SetWindowLong(hWnd, GWL_USERDATA, 0);
  }
  SetWindowText(hWnd,Text);

}



void FormatterTime::Render(HWND hWnd) {
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
  SetWindowLong(hWnd, GWL_USERDATA, 0);
  SetWindowText(hWnd,Text);
}


void FormatterWaypoint::Render(HWND hWnd) {

  if(ActiveWayPoint >=0)
    {

      if (WayPointList[Task[ActiveWayPoint].Index].Reachable) {
        SetWindowLong(hWnd, GWL_USERDATA, 3); // blue text
      } else {
        SetWindowLong(hWnd, GWL_USERDATA, 0); // black text
      }

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
      // no waypoint selected
      SetWindowLong(hWnd, GWL_USERDATA, 0);
      Text[0] = '\0';
    }

  SetWindowText(hWnd, Text);

}
