/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$
}
*/
#include "StdAfx.h"
#include <windows.h>

#include "Defines.h" // VENTA3
#include "compatibility.h"
#ifdef OLDPPC
#include "XCSoarProcess.h"
#else
#include "Process.h"
#endif
#include "externs.h"
#include "Math/FastMath.h"
#include "device.h"
#include "Dialogs.h"
#include "Port.h"
#include "Atmosphere.h"
#include "AATDistance.h"
#include "Battery.h"
#include "Registry.hpp"
#include "XCSoar.h"

#include <stdlib.h>

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
  Valid = TRUE;
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

BOOL InfoBoxFormatter::isValid(void) {
  return Valid;
}

void InfoBoxFormatter::RenderInvalid(int *color) {
  _tcscpy(CommentText, TEXT(""));
  _tcscpy(Text, TEXT("---"));
  *color = -1;
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


const TCHAR *FormatterTime::Render(int *color) {
  if (!Valid) {
    RenderInvalid(color);
    _stprintf(Text,TEXT("--:--"));
  } else {
    if ((hours<0) || (mins<0) || (seconds<0)) {
      // Time is negative
      *color = 1; // red!
      if (hours<0) { // hh:mm, ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  TEXT("%02d"),
                  seconds);
      } else if (mins<0) { // mm:ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, TEXT(""));
      } else {
        _stprintf(Text,
                  TEXT("-00:%02d"),
                  abs(seconds));
        _tcscpy(CommentText, TEXT(""));
      }
    } else {
      // Time is positive
      *color = 0; // black
      if (hours>0) { // hh:mm, ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  TEXT("%02d"),
                  seconds);
      } else { // mm:ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, TEXT(""));
      }
    }
  }
  return(Text);
}


const TCHAR *FormatterAATTime::Render(int *color) {
  if (!Valid) {
    RenderInvalid(color);
    _stprintf(Text,TEXT("--:--"));
  } else {

    *color = status;

    if ((hours<0) || (mins<0) || (seconds<0)) {
      // Time is negative
      if (hours<0) { // hh:mm, ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  TEXT("%02d"),
                  seconds);
      } else if (mins<0) { // mm:ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, TEXT(""));
      } else {
        _stprintf(Text,
                  TEXT("-00:%02d"),
                  abs(seconds));
        _tcscpy(CommentText, TEXT(""));
      }
    } else {
      // Time is positive
      if (hours>0) { // hh:mm, ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  TEXT("%02d"),
                  seconds);
      } else { // mm:ss
        _stprintf(Text,
                  TEXT("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, TEXT(""));
      }
    }
  }
  return(Text);
}


const TCHAR *FormatterWaypoint::Render(int *color) {
  int thewaypoint = ActiveWayPoint;
  LockTaskData();
  if(ValidTaskPoint(thewaypoint))
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
      Valid = false;
      RenderInvalid(color);
    }
  UnlockTaskData();

  return(Text);
}

// VENTA3 Alternate destinations
const TCHAR *FormatterAlternate::RenderTitle(int *color) {

  LockTaskData();
  if(ValidWayPoint(ActiveAlternate))
    {
      if ( DisplayTextType == DISPLAYFIRSTTHREE)
        {
          _tcsncpy(Text,WayPointList[ActiveAlternate].Name,3);
          Text[3] = '\0';
        }
      else if( DisplayTextType == DISPLAYNUMBER)
        {
          _stprintf(Text,TEXT("%d"),
		    WayPointList[ActiveAlternate].Number );
        }
      else
        {
          _tcsncpy(Text,WayPointList[ActiveAlternate].Name,
                   (sizeof(Text)/sizeof(TCHAR))-1);
          Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
        }
    }
  else
    {
      Valid = false;
      RenderInvalid(color);
    }
  UnlockTaskData();

  return(Text);
}

/*
 * Currently even if set for FIVV, colors are not used.
 */
const TCHAR *
FormatterAlternate::Render(int *color)
{
  LockTaskData();
  if(Valid && ValidWayPoint(ActiveAlternate)) {
	switch (WayPointCalc[ActiveAlternate].VGR ) {
		case 0:
			// impossible, give a magenta debug color;
			*color = 5;
			break;
		case 1:
#ifdef FIVV
			*color = 0; // green
#else
			*color = 0; // blue
#endif
			break;
		case 2:
#ifdef FIVV
			*color = 0; // yellow 4
#else
			*color = 0; // normale white
#endif
			break;
		case 3:
			*color = 1; // red
			break;
		default:
			// even more impossible, give debug color magenta
			*color = 5;
			break;
	}

	Value=WayPointCalc[ActiveAlternate].GR;

	_stprintf(Text,Format,Value);
  } else {
	Valid = false;
	RenderInvalid(color);
  }
   UnlockTaskData();
   return(Text);
}


void FormatterAlternate::AssignValue(int i) {
  LockTaskData();
   switch (i) {
	case 67:
		if (OnAlternate1 == false ) { // first run, activate calculations
			OnAlternate1 = true;
        		Value=INVALID_GR;
		} else {
			if ( ValidWayPoint(Alternate1) ) Value=WayPointCalc[Alternate1].GR;
			else Value=INVALID_GR;
		}
		break;
/*
		if ( ValidWayPoint(Alternate1) ) Value=WayPointCalc[Alternate1].GR;
		else Value=INVALID_GR;
		break;
*/
	case 68:
		if (OnAlternate2 == false ) { // first run, activate calculations
			OnAlternate2 = true;
        		Value=INVALID_GR;
		} else {
			if ( ValidWayPoint(Alternate2) ) Value=WayPointCalc[Alternate2].GR;
			else Value=INVALID_GR;
		}
		break;
	case 69:
		if (OnBestAlternate == false ) { // first run, waiting for slowcalculation loop
			OnBestAlternate = true;		// activate it
        		Value=INVALID_GR;
		} else {
			if ( ValidWayPoint(BestAlternate)) Value=WayPointCalc[BestAlternate].GR;
			else Value=INVALID_GR;
		}
		break;
	default:
		Value=66.6; // something evil to notice..
		break;
   }

   Valid=false;
   if (Value < INVALID_GR) {
    	Valid = true;
	if (Value >= 100 )
	  {
	    _tcscpy(Format, _T("%1.0f"));
	  }
	else
	  {
	    _tcscpy(Format, _T("%1.1f"));
	  }
   }

   UnlockTaskData();
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
      _stprintf(Text, TEXT("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("«%2.0f°"), -Value);
    else
      _tcscpy(Text, TEXT("«»"));
#else
    if (Value > 1)
      _stprintf(Text, TEXT("%2.0fÂ°Â»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("Â«%2.0fÂ°"), -Value);
    else
      _tcscpy(Text, TEXT("Â«Â»"));
#endif
    *color = 0;
  } else {
    Valid = false;
    RenderInvalid(color);
  }

  return(Text);
}




const TCHAR *FormatterTeamCode::Render(int *color) {

  if(ValidWayPoint(TeamCodeRefWaypoint))
    {
      *color = 0; // black text
       _tcsncpy(Text,CALCULATED_INFO.OwnTeamCode,5);
       Text[5] = '\0';
    }
  else
    {
      RenderInvalid(color);
    }

  return(Text);
}


const TCHAR *FormatterDiffTeamBearing::Render(int *color) {

  if(ValidWayPoint(TeamCodeRefWaypoint) && TeammateCodeValid) {
    Valid = true;

    Value = CALCULATED_INFO.TeammateBearing -  GPS_INFO.TrackBearing;

    if (Value < -180.0)
      Value += 360.0;
    else
      if (Value > 180.0)
        Value -= 360.0;

#ifndef __MINGW32__
    if (Value > 1)
      _stprintf(Text, TEXT("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("«%2.0f°"), -Value);
    else
      _tcscpy(Text, TEXT("«»"));
#else
    if (Value > 1)
      _stprintf(Text, TEXT("%2.0fÂ°Â»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("Â«%2.0fÂ°"), -Value);
    else
      _tcscpy(Text, TEXT("Â«Â»"));
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



InfoBoxFormatter::InfoBoxFormatter(const TCHAR *theformat) {
  _tcscpy(Format, theformat);
  Valid = TRUE;
  Value = 0.0;
  Text[0] = 0;
  CommentText[0] = 0;
}
