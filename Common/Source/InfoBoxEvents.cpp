/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "InfoBoxManager.h"
#include "Protection.hpp"
#include "Device/Parser.h"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Math/FastMath.h"
#include "Device/device.h"
#include "Dialogs.h"
#include "Message.h"
#include "Device/Port.h"
#include "Atmosphere.h"
#include "Battery.h"
#include "WayPoint.hpp"
#include "Registry.hpp"
#include "MapWindow.h"
#include "McReady.h"
#include "Interface.hpp"
#include "Components.hpp"
#include <stdlib.h>
#include "FlarmCalculations.h"
#include "Math/Pressure.h"
#include "Asset.hpp"

#define m_max(a,b)	(((a)>(b))?(a):(b))
// JMW added key codes,
// so -1 down
//     1 up
//     0 enter
//
// TODO: make a proper class

void
ActionInterface::on_key_Airspeed(int UpDown)
{
  if (UpDown==0) {
    SetSettingsComputer().EnableCalibration =
      !SettingsComputer().EnableCalibration;

    if (SettingsComputer().EnableCalibration)
      Message::AddMessage(TEXT("Calibrate ON"));
    else
      Message::AddMessage(TEXT("Calibrate OFF"));
  }
}

void
ActionInterface::on_key_TeamCode(int UpDown)
{
  int tryCount = 0;
  int searchSlot = FindFlarmSlot(Basic(), SettingsComputer().TeamFlarmIdTarget);
  int newFlarmSlot = -1;

  while (tryCount < FLARM_MAX_TRAFFIC) {
    if (UpDown == 1) {
      searchSlot++;
      if (searchSlot > FLARM_MAX_TRAFFIC - 1) {
	searchSlot = 0;
      }
    } else if (UpDown == -1) {
      searchSlot--;
      if (searchSlot < 0) {
	searchSlot = FLARM_MAX_TRAFFIC - 1;
      }
    }

    if (Basic().FLARM_Traffic[searchSlot].ID != 0) {
      newFlarmSlot = searchSlot;
      break; // a new flarmSlot with a valid flarm traffic record was found !
    }
    tryCount++;
  }

  if (newFlarmSlot != -1) {
    SetSettingsComputer().TeamFlarmIdTarget = Basic().FLARM_Traffic[newFlarmSlot].ID;

    if (_tcslen(Basic().FLARM_Traffic[newFlarmSlot].Name) != 0) {
      // copy the 3 first chars from the name to TeamFlarmCNTarget
      for (int z = 0; z < 3; z++) {
	if (Basic().FLARM_Traffic[newFlarmSlot].Name[z] != 0) {
	  SetSettingsComputer().TeamFlarmCNTarget[z] =
	    Basic().FLARM_Traffic[newFlarmSlot].Name[z];
	} else {
	  SetSettingsComputer().TeamFlarmCNTarget[z] = 32; // add space char
	}
      }
      SetSettingsComputer().TeamFlarmCNTarget[3] = 0;
    } else {
      SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
    }
  } else {
    // no flarm traffic to select!
    SetSettingsComputer().TeamFlarmIdTarget = 0;
    SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
    return;
  }
}

#include "DeviceBlackboard.hpp"

void
ActionInterface::on_key_Altitude(int UpDown)
{
  if (is_simulator()) {
    if(UpDown==1) {
      device_blackboard.SetAltitude(Basic().Altitude+100/ALTITUDEMODIFY);
    } else if (UpDown==-1) {
      device_blackboard.SetAltitude(m_max(0,Basic().Altitude-100/ALTITUDEMODIFY));
    } else if (UpDown==-2) {
      on_key_Direction(-1);
    } else if (UpDown==2) {
      on_key_Direction(1);
    }
  }
  return;
}

// VENTA3 QFE
void
ActionInterface::on_key_QFEAltitude(int UpDown)
{
  short step;
  if ( ( Basic().Altitude - QFEAltitudeOffset ) <10 ) step=1; else step=10;
  if(UpDown==1) {
    QFEAltitudeOffset -= (step/ALTITUDEMODIFY);
  } else if (UpDown==-1) {
    QFEAltitudeOffset += (step/ALTITUDEMODIFY);
  } else if (UpDown==-2) {
    on_key_Direction(-1);
  } else if (UpDown==2) {
    on_key_Direction(1);
  }
  return;
}

// VENTA3 Alternates processing updown
void
ActionInterface::on_key_Alternate1(int UpDown)
{
   if (UpDown==0) {
     if ( SettingsComputer().Alternate1 <0 ) return;
     task.setSelected(SettingsComputer().Alternate1);

     ScopePopupBlock block(main_window.popup);
     PopupWaypointDetails();
  }
}

void
ActionInterface::on_key_Alternate2(int UpDown)
{
   if (UpDown==0) {
     if ( SettingsComputer().Alternate2 <0 )
       return;
     task.setSelected(SettingsComputer().Alternate2);

     ScopePopupBlock block(main_window.popup);
     PopupWaypointDetails();
  }
}

void
ActionInterface::on_key_BestAlternate(int UpDown)
{
   if (UpDown==0) {
     if ( Calculated().BestAlternate <0 )
       return;
     task.setSelected(Calculated().BestAlternate);

     ScopePopupBlock block(main_window.popup);
     PopupWaypointDetails();
  }
}

void
ActionInterface::on_key_Speed(int UpDown)
{
  if (is_simulator()) {
    if(UpDown==1)
      device_blackboard.SetSpeed(Basic().Speed+10/SPEEDMODIFY);
    else if (UpDown==-1) {
      device_blackboard.SetSpeed(m_max(0,Basic().Speed-10/SPEEDMODIFY));
    } else if (UpDown==-2) {
      on_key_Direction(-1);
    } else if (UpDown==2) {
      on_key_Direction(1);
    }
  }
  return;
}


void
ActionInterface::on_key_Accelerometer(int UpDown)
{
  if (UpDown==0) {
    /* JMW broken
    DWORD Temp;
    AccelerometerZero*= Basic().Gload;
    if (AccelerometerZero<1) {
      AccelerometerZero = 100;
    }
    Temp = (int)AccelerometerZero;
    SetToRegistry(szRegistryAccelerometerZero,Temp);
    */
  }
}

void
ActionInterface::on_key_WindDirection(int UpDown)
{
/* JMW ILLEGAL/incomplete
  if(UpDown==1)
    {
      Calculated().WindBearing  += 5;
      while (Calculated().WindBearing  >= 360)
	{
	  Calculated().WindBearing  -= 360;
	}
    }
  else if (UpDown==-1)
    {
      Calculated().WindBearing  -= 5;
      while (Calculated().WindBearing  < 0)
	{
	  Calculated().WindBearing  += 360;
	}
    } else if (UpDown == 0) {
    glide_computer.SetWindEstimate(Calculated().WindSpeed,
				   Calculated().WindBearing);
    SaveWindToRegistry();
  }
  return;
*/
}

void	ActionInterface::on_key_WindSpeed(int UpDown)
{
/* JMW ILLEGAL/incomplete
	if(UpDown==1)
		Calculated().WindSpeed += (1/SPEEDMODIFY);
	else if (UpDown== -1)
	{
		Calculated().WindSpeed -= (1/SPEEDMODIFY);
		if(Calculated().WindSpeed < 0)
			Calculated().WindSpeed = 0;
	}
	// JMW added faster way of changing wind direction
	else if (UpDown== -2) {
		on_key_WindDirection(-1);
	} else if (UpDown== 2) {
		on_key_WindDirection(1);
	} else if (UpDown == 0) {
          glide_computer.SetWindEstimate(Calculated().WindSpeed,
					 Calculated().WindBearing);
	  SaveWindToRegistry();
	}
*/
	return;
}

void
ActionInterface::on_key_Direction(int UpDown)
{
  if (is_simulator()) {
    if(UpDown==1) {
      device_blackboard.SetTrackBearing(Basic().TrackBearing+5);
    } else if (UpDown==-1) {
      device_blackboard.SetTrackBearing(Basic().TrackBearing-5);
    }
  }
  return;
}


void
ActionInterface::on_key_MacCready(int UpDown)
{
  double MACCREADY = GlidePolar::GetMacCready();
  if(UpDown==1) {
    MACCREADY += (double)0.1;
    if (MACCREADY>5.0) { // JMW added sensible limit
      MACCREADY=5.0;
    }
    GlidePolar::SetMacCready(MACCREADY);
  }
  else if(UpDown==-1) {
    MACCREADY -= (double)0.1;
    if(MACCREADY < 0) {
      MACCREADY = 0;
    }
    GlidePolar::SetMacCready(MACCREADY);
  }
 else if (UpDown==0)
    {
      SetSettingsComputer().AutoMacCready
	= !SettingsComputer().AutoMacCready;
    }
  else if (UpDown==-2)
    {
      SetSettingsComputer().AutoMacCready = false;  // SDP on auto maccready

    }
  else if (UpDown==+2)
    {
      SetSettingsComputer().AutoMacCready = true;	// SDP off auto maccready
    }

  // JMW TODO check scope
  AllDevicesPutMcCready(MACCREADY);
}


void
ActionInterface::on_key_ForecastTemperature(int UpDown)
{
  if (UpDown==1) {
    CuSonde::adjustForecastTemperature(0.5);
  }
  if (UpDown== -1) {
    CuSonde::adjustForecastTemperature(-0.5);
  }
}


/*
	1	Next waypoint
	0	Show waypoint details
	-1	Previous waypoint
	2	Next waypoint with wrap around
	-2	Previous waypoint with wrap around
*/
void
ActionInterface::on_key_Waypoint(int UpDown)
{

  if (UpDown>0) {
    task.advanceTaskPoint(SettingsComputer());
  } else if ((UpDown == 2) && (task.ValidTaskPoint(0))) {
      // No more, try first

      /* ****DISABLED****
         if(task.getActiveIndex() == 0)	{
         // TODO bug: allow restart
         // TODO bug: make this work only for manual

         // TODO bug: This should trigger reset of flight stats, but
         // should ask first...
         if (Calculated().TaskStartTime==0) {
         Calculated().TaskStartTime = Basic().Time ;
         }
         }
      AdvanceArmed = false;
      task.getActiveIndex() = 0;
      */
      /* JMW illegal
         Calculated().LegStartTime = Basic().Time ;
      */
  } else if (UpDown<0){
    task.retreatTaskPoint(SettingsComputer());
  } else if (UpDown==0) {
    task.setSelected();

    ScopePopupBlock block(main_window.popup);
    PopupWaypointDetails();
  }
}


void
ActionInterface::on_key_None(int UpDown)
{
  (void)UpDown;
  return;
}
