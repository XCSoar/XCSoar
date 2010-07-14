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

#include "Protection.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Device/Port.hpp"
#include "SettingsComputer.hpp"
#include "Math/FastMath.h"
#include "Dialogs.h"
#include "Message.hpp"
#include "Atmosphere.h"
#include "Hardware/Battery.h"
#include "MapWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "DeviceBlackboard.hpp"
#include "TaskClientUI.hpp"
#include "Simulator.hpp"
#include "PopupMessage.hpp"
#include "MainWindow.hpp"

#include <stdlib.h>
#include "FlarmCalculations.h"

#include <algorithm>

using std::max;

// JMW added key codes,
// so -1 down
//     1 up
//     0 enter
//
// TODO: make a proper class

void
ActionInterface::on_key_Airspeed(int UpDown)
{
  if (UpDown == 0) {
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
  const FLARM_STATE &flarm = Basic().flarm;
  const FLARM_TRAFFIC *traffic = SettingsComputer().TeamFlarmIdTarget.defined()
    ? flarm.FindTraffic(SettingsComputer().TeamFlarmIdTarget)
    : NULL;

  if (UpDown == 1) {
    if (traffic != NULL)
      traffic = flarm.NextTraffic(traffic);
    if (traffic == NULL)
      traffic = flarm.FirstTraffic();
  } else {
    if (traffic != NULL)
      traffic = flarm.PreviousTraffic(traffic);
    if (traffic == NULL)
      traffic = flarm.LastTraffic();
  }

  if (traffic != NULL) {
    SetSettingsComputer().TeamFlarmIdTarget = traffic->ID;

    if (traffic->HasName()) {
      // copy the 3 first chars from the name to TeamFlarmCNTarget
      for (int z = 0; z < 3; z++) {
        if (traffic->Name[z] != 0)
          SetSettingsComputer().TeamFlarmCNTarget[z] = traffic->Name[z];
        else
          SetSettingsComputer().TeamFlarmCNTarget[z] = 32; // add space char
      }
      SetSettingsComputer().TeamFlarmCNTarget[3] = 0;
    } else {
      SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
    }
  } else {
    // no flarm traffic to select!
    SetSettingsComputer().TeamFlarmIdTarget.clear();
    SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
    return;
  }
}

void
ActionInterface::on_key_Altitude(int UpDown)
{
  if (!is_simulator())
    return;

  fixed fixed_step = (fixed)Units::ToSysUnit(100, Units::AltitudeUnit);

  if (UpDown == 1)
    device_blackboard.SetAltitude(Basic().GPSAltitude + fixed_step);
  else if (UpDown == -1)
    device_blackboard.SetAltitude(max(fixed_zero,
                                      Basic().GPSAltitude - fixed_step));
  else if (UpDown == -2)
    on_key_Direction(-1);
  else if (UpDown == 2)
    on_key_Direction(1);
}

void
ActionInterface::on_key_Alternate1(int UpDown)
{
#ifdef OLD_TASK // alternates
  if (UpDown == 0) {
    if (SettingsComputer().Alternate1 < 0)
      return;

    task.setSelected(SettingsComputer().Alternate1);
    dlgWayPointDetailsShowModal(main_window, way_point);
  }
#endif
}

void
ActionInterface::on_key_Alternate2(int UpDown)
{
#ifdef OLD_TASK // alternates
  if (UpDown == 0) {
    if (SettingsComputer().Alternate2 < 0)
      return;

    task.setSelected(SettingsComputer().Alternate2);
    dlgWayPointDetailsShowModal( way_point);
  }
#endif
}

void
ActionInterface::on_key_BestAlternate(int UpDown)
{
#ifdef OLD_TASK // alternates
  if (UpDown == 0) {
    if (Calculated().BestAlternate < 0)
      return;

    task.setSelected(Calculated().BestAlternate);
    dlgWayPointDetailsShowModal( way_point);
  }
#endif
}

void
ActionInterface::on_key_Speed(int UpDown)
{
  if (!is_simulator())
    return;

  fixed fixed_step = (fixed)Units::ToSysUnit(10, Units::SpeedUnit);

  if (UpDown == 1)
    device_blackboard.SetSpeed(Basic().GroundSpeed + fixed_step);
  else if (UpDown == -1)
    device_blackboard.SetSpeed(max(fixed_zero,
                                   Basic().GroundSpeed - fixed_step));
  else if (UpDown == -2)
    on_key_Direction(-1);
  else if (UpDown == 2)
    on_key_Direction(1);
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
    Profile::Set(szProfileAccelerometerZero,Temp);
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
    Profile::SetWind();
  }
  return;
*/
}

void	ActionInterface::on_key_WindSpeed(int UpDown)
{
/* JMW ILLEGAL/incomplete
	if(UpDown==1)
		Calculated().WindSpeed += Units::ToSysUnit(1, Units::SpeedUnit);
	else if (UpDown== -1)
	{
		Calculated().WindSpeed -= Units::ToSysUnit(1, Units::SpeedUnit);
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
	  Profile::SetWind();
	}
*/
	return;
}

void
ActionInterface::on_key_Direction(int UpDown)
{
  static const Angle a5 = Angle::degrees(fixed(5));

  if (is_simulator()) {
    if (UpDown == 1)
      device_blackboard.SetTrackBearing(Basic().TrackBearing + a5);
    else if (UpDown == -1)
      device_blackboard.SetTrackBearing(Basic().TrackBearing - a5);
  }
}

void
ActionInterface::on_key_MacCready(int UpDown)
{
  GlidePolar polar = task_ui.get_glide_polar();
  double MACCREADY = polar.get_mc();
  if (UpDown == 1) {
    MACCREADY += (double)0.1;
    if (MACCREADY > 5.0) {
      MACCREADY = 5.0;
    }
    polar.set_mc(fixed(MACCREADY));
    task_ui.set_glide_polar(polar);
  } else if (UpDown == -1) {
    MACCREADY -= (double)0.1;
    if (MACCREADY < 0) {
      MACCREADY = 0;
    }
    polar.set_mc(fixed(MACCREADY));
    task_ui.set_glide_polar(polar);
  } else if (UpDown == 0) {
    SetSettingsComputer().auto_mc = !SettingsComputer().auto_mc;
  } else if (UpDown == -2) {
    SetSettingsComputer().auto_mc = false;
  } else if (UpDown == +2) {
    SetSettingsComputer().auto_mc = true;
  }

  device_blackboard.SetMC(fixed(MACCREADY));
}

void
ActionInterface::on_key_ForecastTemperature(int UpDown)
{
  if (UpDown == 1)
    CuSonde::adjustForecastTemperature(0.5);
  else if (UpDown == -1)
    CuSonde::adjustForecastTemperature(-0.5);
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
  if (UpDown > 0) {
    task_ui.incrementActiveTaskPoint(1);
  } else if (UpDown < 0) {
    task_ui.incrementActiveTaskPoint(-1);
  } else if (UpDown == 0) {
    const Waypoint *wp = task_ui.getActiveWaypoint();
    if (wp)
      dlgWayPointDetailsShowModal(main_window, *wp);
  }
}

void
ActionInterface::on_key_None(int UpDown)
{
  (void)UpDown;
  return;
}
