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
#include "FLARM/FlarmCalculations.h"

#include <algorithm>

using std::max;

void
ActionInterface::on_key_Direction(InfoBoxKeyCodes UpDown)
{
  static const Angle a5 = Angle::degrees(fixed(5));

  if (is_simulator()) {
    if (UpDown == ibkUp)
      device_blackboard.SetTrackBearing(Basic().TrackBearing + a5);
    else if (UpDown == ibkDown)
      device_blackboard.SetTrackBearing(Basic().TrackBearing - a5);
  }
}

void
ActionInterface::on_key_MacCready(InfoBoxKeyCodes UpDown)
{
  GlidePolar polar = task_ui.get_glide_polar();
  double MACCREADY = polar.get_mc();
  if (UpDown == ibkUp) {
    MACCREADY += (double)0.1;
    if (MACCREADY > 5.0) {
      MACCREADY = 5.0;
    }
    polar.set_mc(fixed(MACCREADY));
    task_ui.set_glide_polar(polar);
  } else if (UpDown == ibkDown) {
    MACCREADY -= (double)0.1;
    if (MACCREADY < 0) {
      MACCREADY = 0;
    }
    polar.set_mc(fixed(MACCREADY));
    task_ui.set_glide_polar(polar);
  } else if (UpDown == ibkEnter) {
    SetSettingsComputer().auto_mc = !SettingsComputer().auto_mc;
  } else if (UpDown == ibkLeft) {
    SetSettingsComputer().auto_mc = false;
  } else if (UpDown == +2) {
    SetSettingsComputer().auto_mc = true;
  }

  device_blackboard.SetMC(fixed(MACCREADY));
}

/*
	1	Next waypoint
	0	Show waypoint details
	-1	Previous waypoint
	2	Next waypoint with wrap around
	-2	Previous waypoint with wrap around
*/
void
ActionInterface::on_key_Waypoint(InfoBoxKeyCodes UpDown)
{
  if (UpDown > 0) {
    task_ui.incrementActiveTaskPoint(1);
  } else if (UpDown < 0) {
    task_ui.incrementActiveTaskPoint(-1);
  } else if (UpDown == ibkEnter) {
    const Waypoint *wp = task_ui.getActiveWaypoint();
    if (wp)
      dlgWayPointDetailsShowModal(main_window, *wp);
  }
}
