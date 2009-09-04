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

#include "Calculations.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Components.hpp"
#include "Units.hpp"
#include "Message.h"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Calibration.hpp"
#include "McReady.h"
#include "Logger.h"
#include "LocalTime.hpp"
#include "ReplayLogger.hpp"
#include "Math/FastMath.h"
#include <math.h>
#include "InputEvents.h"
#include "Message.h"
#include "TeamCodeCalculation.h"
#include <tchar.h>
#include "GlideComputer.hpp"
#include "Math/NavFunctions.hpp" // used for team code
#include "Calculations2.h"
#ifdef NEWCLIMBAV
#include "ClimbAverageCalculator.h" // JMW new
#endif
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Math/Pressure.h"
#include "WayPoint.hpp"
#include "LogFile.hpp"
#include "BestAlternate.hpp"
#include "Persist.hpp"
#include "Airspace.h"
#include "CalculationsAirspace.hpp"
#include "ConditionMonitor.hpp"
#include "MapWindowProjection.hpp"

bool EnableNavBaroAltitude=false;
int EnableExternalTriggerCruise=false;
bool ForceFinalGlide= false;
bool AutoForceFinalGlide= false;
bool EnableFAIFinishHeight = false;

int FinishLine=1;
DWORD FinishRadius=1000;


void
DoAutoQNH(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
  static int done_autoqnh = 0;

  // Reject if already done
  if (done_autoqnh==10) return;

  // Reject if in IGC logger mode
  if (ReplayLogger::IsEnabled()) return;

  // Reject if no valid GPS fix
  if (Basic->NAVWarning) return;

  // Reject if no baro altitude
  if (!Basic->BaroAltitudeAvailable) return;

  // Reject if terrain height is invalid
  if (!Calculated->TerrainValid) return;

  if (Basic->Speed<TAKEOFFSPEEDTHRESHOLD) {
    done_autoqnh++;
  } else {
    done_autoqnh= 0; // restart...
  }

  if (done_autoqnh==10) {
    double fixaltitude = Calculated->TerrainAlt;

    QNH = FindQNH(Basic->BaroAltitude, fixaltitude);
    AirspaceQnhChangeNotify(QNH);
  }
}

///////////////////////////////////////////////

void RefreshTaskStatistics(void) {
  /* JMW incomplete
  const double mc = GlidePolar::GetMacCready();
  const double ce = GlidePolar::GetCruiseEfficiency();

  mutexGlideComputer.Lock();
  mutexFlightData.Lock();
  mutexTaskData.Lock();
  TaskStatistics(&GPS_INFO, &CALCULATED_INFO, mc, ce);
  AATStats(&GPS_INFO, &CALCULATED_INFO);
  TaskSpeed(&GPS_INFO, &CALCULATED_INFO, mc, ce);
  IterateEffectiveMacCready(&GPS_INFO, &CALCULATED_INFO);
  mutexTaskData.Unlock();
  mutexFlightData.Unlock();
  mutexGlideComputer.Unlock();
  */
}


