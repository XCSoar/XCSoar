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

#include "InfoBoxEvents.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Device/Parser.h"
#include "externs.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Math/FastMath.h"
#include "Device/device.h"
#include "Dialogs.h"
#include "Device/Port.h"
#include "AATDistance.h"
#include "Atmosphere.h"
#include "Battery.h"
#include "WayPoint.hpp"
#include "Registry.hpp"
#include "Utils.h"
#include "MapWindow.h"
#include "McReady.h"

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
