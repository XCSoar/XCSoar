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

#include "Process.h"
#include "Parser.h"
#include "Defines.h" // VENTA3
#include "externs.h"
#include "Math/FastMath.h"
#include "Device/device.h"
#include "Dialogs.h"
#include "Device/Port.h"
#include "Atmosphere.h"
#include "AATDistance.h"
#include "Battery.h"
#include "WayPoint.hpp"
#include "Registry.hpp"
#include "XCSoar.h"
#include "Utils.h"
#include "MapWindow.h"

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern AATDistance aatdistance;

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

void	TeamCodeProcessing(int UpDown)
{
	int tryCount = 0;
	int searchSlot = FindFlarmSlot(TeamFlarmIdTarget);
	int newFlarmSlot = -1;


	while (tryCount < FLARM_MAX_TRAFFIC)
	{
		if (UpDown == 1)
		{
			searchSlot++;
			if (searchSlot > FLARM_MAX_TRAFFIC - 1)
			{
				searchSlot = 0;
			}
		}
		else if (UpDown == -1)
		{
			searchSlot--;
			if (searchSlot < 0)
			{
				searchSlot = FLARM_MAX_TRAFFIC - 1;
			}
		}

		if (GPS_INFO.FLARM_Traffic[searchSlot].ID != 0)
		{
			newFlarmSlot = searchSlot;
			break; // a new flarmSlot with a valid flarm traffic record was found !
		}
		tryCount++;
	}

	if (newFlarmSlot != -1)
	{
		TeamFlarmIdTarget = GPS_INFO.FLARM_Traffic[newFlarmSlot].ID;

		if (_tcslen(GPS_INFO.FLARM_Traffic[newFlarmSlot].Name) != 0)
		{
			// copy the 3 first chars from the name to TeamFlarmCNTarget
			for (int z = 0; z < 3; z++)
			{
				if (GPS_INFO.FLARM_Traffic[newFlarmSlot].Name[z] != 0)
				{
					TeamFlarmCNTarget[z] = GPS_INFO.FLARM_Traffic[newFlarmSlot].Name[z];
				}
				else
				{
					TeamFlarmCNTarget[z] = 32; // add space char
				}
			}
			TeamFlarmCNTarget[3] = 0;
		}
		else
		{
			TeamFlarmCNTarget[0] = 0;
		}
	}
	else
	{
			// no flarm traffic to select!
			TeamFlarmIdTarget = 0;
			TeamFlarmCNTarget[0] = 0;
			return;
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

// VENTA3 QFE
void	QFEAltitudeProcessing(int UpDown)
{
	short step;
	if ( ( GPS_INFO.Altitude - QFEAltitudeOffset ) <10 ) step=1; else step=10;
	if(UpDown==1) {
	   QFEAltitudeOffset -= (step/ALTITUDEMODIFY);
	}	else if (UpDown==-1)
	  {
	    QFEAltitudeOffset += (step/ALTITUDEMODIFY);
/*
	    if(QFEAltitudeOffset < 0)
	      QFEAltitudeOffset = 0;
*/
	  } else if (UpDown==-2) {
	  DirectionProcessing(-1);
	} else if (UpDown==2) {
	  DirectionProcessing(1);
	}
	return;
}

// VENTA3 Alternates processing updown
void Alternate1Processing(int UpDown)
{
   if (UpDown==0) {
	if ( Alternate1 <0 ) return;
	LockTaskData(); SelectedWaypoint = Alternate1; PopupWaypointDetails(); UnlockTaskData();
  }
}
void Alternate2Processing(int UpDown)
{
   if (UpDown==0) {
	if ( Alternate2 <0 ) return;
	LockTaskData(); SelectedWaypoint = Alternate2; PopupWaypointDetails(); UnlockTaskData();
  }
}
void BestAlternateProcessing(int UpDown)
{
   if (UpDown==0) {
	if ( BestAlternate <0 ) return;
	LockTaskData(); SelectedWaypoint = BestAlternate; PopupWaypointDetails(); UnlockTaskData();
  }
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
          SetWindEstimate(CALCULATED_INFO.WindSpeed,
                          CALCULATED_INFO.WindBearing);
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
          SetWindEstimate(CALCULATED_INFO.WindSpeed,
                          CALCULATED_INFO.WindBearing);
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
	  // TODO bug: allow restart
	  // TODO bug: make this work only for manual
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
            // TODO bug: allow restart
            // TODO bug: make this work only for manual

            // TODO bug: This should trigger reset of flight stats, but
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
	//	TODO bug: not required? CALCULATED_INFO.TaskStartTime = 0;
      }
    }
    aatdistance.ResetEnterTrigger(ActiveWayPoint);
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
    minimum = 0.5*LIFTMODIFY*CALCULATED_INFO.MacCreadyRisk;
    break;
  case 21:
    minimum = 0.667*LIFTMODIFY*CALCULATED_INFO.MacCreadyRisk;
    break;
  default:
    break;
  }
}


void FormatterTime::SecsToDisplayTime(int d) {
  bool negative = (d<0);
  int dd = abs(d) % (3600*24);

  hours = (dd/3600);
  mins = (dd/60-hours*60);
  seconds = (dd-mins*60-hours*3600);
  hours = hours % 24;
  if (negative) {
    if (hours>0) {
      hours = -hours;
    } else if (mins>0) {
      mins = -mins;
    } else {
      seconds = -seconds;
    }
  }
  Valid = true;
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


int DetectStartTime(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // JMW added restart ability
  //
  // we want this to display landing time until next takeoff

  static int starttime = -1;
  static int lastflighttime = -1;

  if (Calculated->Flying) {
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
	lastflighttime = (int)Basic->Time-starttime;
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
  case 36:
    SecsToDisplayTime((int)CALCULATED_INFO.FlightTime);
    break;
  case 39:
    SecsToDisplayTime(DetectCurrentTime());
    break;
  case 40:
    SecsToDisplayTime((int)(GPS_INFO.Time));
    break;
  case 46:
    SecsToDisplayTime((int)(CALCULATED_INFO.LegTimeToGo+DetectCurrentTime()));
    Valid = ValidTaskPoint(ActiveWayPoint) &&
      (CALCULATED_INFO.LegTimeToGo< 0.9*ERROR_TIME);
    break;
  default:
    break;
  }
}


void FormatterAATTime::AssignValue(int i) {
  double dd;
  if (AATEnabled && ValidTaskPoint(ActiveWayPoint)) {
    dd = CALCULATED_INFO.TaskTimeToGo;
    if ((CALCULATED_INFO.TaskStartTime>0.0) && (CALCULATED_INFO.Flying)
        &&(ActiveWayPoint>0)) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    dd= max(0,min(24.0*3600.0,dd))-AATTaskLength*60;
    if (dd<0) {
      status = 1; // red
    } else {
      if (CALCULATED_INFO.TaskTimeToGoTurningNow > (AATTaskLength+5)*60) {
        status = 2; // blue
      } else {
        status = 0;  // black
      }
    }
  } else {
    dd = 0;
    status = 0; // black
  }

  switch (i) {
  case 27:
    SecsToDisplayTime((int)CALCULATED_INFO.AATTimeToGo);
    Valid = (ValidTaskPoint(ActiveWayPoint) && AATEnabled
	     && (CALCULATED_INFO.AATTimeToGo< 0.9*ERROR_TIME));
    break;
  case 41:
    SecsToDisplayTime((int)(CALCULATED_INFO.TaskTimeToGo));
    Valid = ValidTaskPoint(ActiveWayPoint)
      && (CALCULATED_INFO.TaskTimeToGo< 0.9*ERROR_TIME);
    break;
  case 42:
    SecsToDisplayTime((int)(CALCULATED_INFO.LegTimeToGo));
    Valid = ValidTaskPoint(ActiveWayPoint)
      && (CALCULATED_INFO.LegTimeToGo< 0.9*ERROR_TIME);
    break;
  case 45:
    SecsToDisplayTime((int)(CALCULATED_INFO.TaskTimeToGo+DetectCurrentTime()));
    Valid = ValidTaskPoint(ActiveWayPoint)
      && (CALCULATED_INFO.TaskTimeToGo< 0.9*ERROR_TIME);
    break;
  case 62:
    if (AATEnabled && ValidTaskPoint(ActiveWayPoint)) {
      SecsToDisplayTime((int)dd);
      Valid = (dd< 0.9*ERROR_TIME);
    } else {
      SecsToDisplayTime(0);
      Valid = false;
    }
    break;
  default:
    break;
  }
}

const TCHAR *FormatterTime::Render(int *color) {
  if (!Valid) {
    RenderInvalid(color);
    _stprintf(Text,_T("--:--"));
  } else {
    if ((hours<0) || (mins<0) || (seconds<0)) {
      // Time is negative
      *color = 1; // red!
      if (hours<0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else if (mins<0) { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      } else {
        _stprintf(Text,
                  _T("-00:%02d"),
                  abs(seconds));
        _tcscpy(CommentText, _T(""));
      }
    } else {
      // Time is positive
      *color = 0; // black
      if (hours>0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      }
    }
  }
  return(Text);
}


const TCHAR *FormatterAATTime::Render(int *color) {
  if (!Valid) {
    RenderInvalid(color);
    _stprintf(Text,_T("--:--"));
  } else {

    *color = status;

    if ((hours<0) || (mins<0) || (seconds<0)) {
      // Time is negative
      if (hours<0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else if (mins<0) { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      } else {
        _stprintf(Text,
                  _T("-00:%02d"),
                  abs(seconds));
        _tcscpy(CommentText, _T(""));
      }
    } else {
      // Time is positive
      if (hours>0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      }
    }
  }
  return(Text);
}

const TCHAR *FormatterDiffBearing::Render(int *color) {

  if (ValidTaskPoint(ActiveWayPoint)
      && CALCULATED_INFO.WaypointDistance > 10.0) {
    Valid = true;

    Value = CALCULATED_INFO.WaypointBearing -  GPS_INFO.TrackBearing;

    if (Value < -180.0)
      Value += 360.0;
    else
    if (Value > 180.0)
      Value -= 360.0;

#ifndef __MINGW32__
    if (Value > 1)
      _stprintf(Text, _T("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("«%2.0f°"), -Value);
    else
      _tcscpy(Text, _T("«»"));
#else
    if (Value > 1)
      _stprintf(Text, _T("%2.0fÂ°Â»"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("Â«%2.0fÂ°"), -Value);
    else
      _tcscpy(Text, _T("Â«Â»"));
#endif
    *color = 0;
  } else {
    Valid = false;
    RenderInvalid(color);
  }

  return(Text);
}

/*

if ((Calculated->FinalGlide) && (Calculated->Circling) && (Calculated->AverageThermal>0)) {
}
*/

const TCHAR *FormatterLowWarning::Render(int *color) {

  if (Valid) {
    _stprintf(Text,
              Format,
              Value );
    if (Value<minimum) {
      *color = 1; // red
    } else {
      *color = 0;
    }
  } else {
    RenderInvalid(color);
  }
  return(Text);
}
