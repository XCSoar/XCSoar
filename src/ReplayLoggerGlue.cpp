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

#include "ReplayLoggerGlue.hpp"
#include "Logger/Logger.hpp"
#include "Protection.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Device/Port.hpp"
#include "Math/Earth.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "StringUtil.hpp"
#include "UtilsText.hpp"
#include "LocalPath.hpp"
#include "Device/device.hpp"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "PeriodClock.hpp"

#include <algorithm>

bool
ReplayLoggerGlue::ScanBuffer(const TCHAR *buffer, double *Time,
                             double *Latitude, double *Longitude, 
                             double *Altitude)
{

  if (ReplayLogger::ScanBuffer(buffer, Time, Latitude, Longitude, Altitude)) {
    return true;
  }

  int found=0;
  TCHAR event[200];
  TCHAR misc[200];

  found = _stscanf(buffer, TEXT("LPLT event=%[^ ] %[A-Za-z0-9 \\/().,]"),
      event, misc);

  if (found > 0) {
    pt2Event fevent = InputEvents::findEvent(event);
    if (fevent) {
      if (found == 2) {
        TCHAR *mmisc = StringMallocParse(misc);
        fevent(mmisc);
        free(mmisc);
      } else {
        fevent(TEXT("\0"));
      }
    }
  }
  return false;
}


double
ReplayLoggerGlue::get_time(const bool reset,
                           const double mintime)
{
  static PeriodClock clock;
  static double t_simulation;
  
  if (reset) {
    clock.reset();
    t_simulation = 0;
  } else {
    t_simulation += TimeScale * max(clock.elapsed(), 0) / 1000.0;
    clock.update();
  }
  
  t_simulation = std::max(mintime, t_simulation);
  return t_simulation;
}

void
ReplayLoggerGlue::on_advance(const GEOPOINT &loc,
                             const double speed, const double bearing,
                             const double alt, const double baroalt, const double t)
{
  device_blackboard.SetLocation(loc, speed, bearing, alt, baroalt, t);
  TriggerGPSUpdate();
}

void
ReplayLoggerGlue::on_stop()
{
  device_blackboard.StopReplay();
  logger.clearBuffer();
}

void
ReplayLoggerGlue::on_bad_file()
{
  MessageBoxX(gettext(TEXT("Could not open IGC file!")),
              gettext(TEXT("Flight replay")), MB_OK | MB_ICONINFORMATION);
}

void
ReplayLoggerGlue::on_reset()
{
  logger.clearBuffer();
}

