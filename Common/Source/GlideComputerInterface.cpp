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

#include "GlideComputer.hpp"
#include "Units.hpp"
#include "InputEvents.h"
#include "Task.h"
#include "Message.h"
#include "SettingsTask.hpp"
#include "LocalTime.hpp"

void GlideComputer::AnnounceWayPointSwitch(bool do_advance) {
  if (task.getActiveIndex() == 0) {
//    InputEvents::processGlideComputer(GCE_TASK_START);
// JMW why commented out?
    TCHAR TempTime[40];
    TCHAR TempAlt[40];
    TCHAR TempSpeed[40];
    Units::TimeToText(TempTime, (int)TimeLocal((int)Calculated().TaskStartTime));
    _stprintf(TempAlt, TEXT("%.0f %s"),
              Calculated().TaskStartAltitude*ALTITUDEMODIFY,
              Units::GetAltitudeName());
    _stprintf(TempSpeed, TEXT("%.0f %s"),
	      Calculated().TaskStartSpeed*TASKSPEEDMODIFY,
             Units::GetTaskSpeedName());

    TCHAR TempAll[120];
    _stprintf(TempAll, TEXT("\r\nAltitude: %s\r\nSpeed:%s\r\nTime: %s"), 
          TempAlt, TempSpeed, TempTime);
    Message::AddMessage(TEXT("Task Start"), TempAll);

  } else if (Calculated().ValidFinish && task.ActiveIsFinalWaypoint()) {
    InputEvents::processGlideComputer(GCE_TASK_FINISH);
  } else {
    InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
  }

  // JMW this should not happen here!
  // JMW Task....
  if (do_advance) {
    task.advanceTaskPoint(SettingsComputer());
    SetLegStart();
  }
  // set waypoint detail to active task WP

  GlideComputerStats::SetFastLogging();
}

