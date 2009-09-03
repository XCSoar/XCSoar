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

#include "XCSoar.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "Device/device.h"
#include "McReady.h"
#include <math.h>

bool BallastTimerActive = false;
int BallastSecsToEmpty = 120;

void
BallastDump(const NMEA_INFO *Basic)
{
  static double BallastTimeLast = -1;

  if (BallastTimerActive) {
    // JMW only update every 5 seconds to stop flooding the devices
    if (Basic->Time > BallastTimeLast+5) {
      double BALLAST = GlidePolar::GetBallast();
      double BALLAST_last = BALLAST;
      double dt = Basic->Time - BallastTimeLast;
      double percent_per_second = 1.0/max(10.0, BallastSecsToEmpty);
      BALLAST -= dt*percent_per_second;
      if (BALLAST<0) {
	BallastTimerActive = false;
	BALLAST = 0.0;
      }
      GlidePolar::SetBallast(BALLAST);
      if (fabs(BALLAST-BALLAST_last)>0.05) { // JMW update on 5 percent!
	GlidePolar::UpdatePolar(true);
      }
      BallastTimeLast = Basic->Time;
    }
  } else {
    BallastTimeLast = Basic->Time;
  }
}

