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

#include "Persist.hpp"
#include "XCSoar.h"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "SettingsComputer.hpp"
#include "OnLineContest.h"
#include "MacCready.h"
#include "Math/Pressure.h"
#include "Atmosphere.h"
#include "UtilsSystem.hpp"
#include "Logger.h"
#include "GlideComputer.hpp"
#include "Calculations.h"
#include "Protection.hpp"
#include "Components.hpp"
#include "Asset.hpp"

#ifndef _MSC_VER
  #include <algorithm>
  using std::min;
  using std::max;
#endif

static TCHAR szCalculationsPersistFileName[MAX_PATH]= TEXT("\0");
static TCHAR szCalculationsPersistDirectory[MAX_PATH]= TEXT("\0");

/**
 * Deletes the persistent memory file
 */
void
DeleteCalculationsPersist(void)
{
  DeleteFile(szCalculationsPersistFileName);
}

/**
 * Loads calculated values from the persistent memory file
 * @param Calculated DERIVED_INFO the values should be loaded into
 */
void
LoadCalculationsPersist(DERIVED_INFO *Calculated)
{
  // Get the persistent memory filename
  if (szCalculationsPersistFileName[0] == 0) {
    if (is_altair()) {
      LocalPath(szCalculationsPersistFileName, TEXT("persist/xcsoar-persist.log"));
      LocalPath(szCalculationsPersistDirectory, TEXT("persist"));
    } else {
      LocalPath(szCalculationsPersistFileName, TEXT("xcsoar-persist.log"));
      LocalPath(szCalculationsPersistDirectory);
    }
  }

  // Debug Log
  StartupStore(TEXT("LoadCalculationsPersist\n"));

  DWORD sizein;

  // Try to open the persistent memory file
  FILE *file = _tfopen(szCalculationsPersistFileName, _T("rb"));
  if (file == NULL) {
    StartupStore(TEXT("LoadCalculationsPersist file not found\n"));
    return;
  }

  fread(&sizein, sizeof(sizein), 1, file);
  if (sizein != sizeof(*Calculated)) {
    fclose(file);
    return;
  }

  // Read persistent memory into Calculated
  fread(Calculated, sizeof(*Calculated), 1, file);
  Calculated->Flying = false;
  Calculated->TimeInFlight = 0;
  Calculated->TimeOnGround = 60;

  fread(&sizein, sizeof(sizein), 1, file);
  if (sizein != sizeof(glide_computer.GetFlightStats())) {
    glide_computer.ResetFlight();
    fclose(file);
    return;
  }

  // Read persistent memory into FlightStats
  fread(&glide_computer.GetFlightStats(), sizeof(glide_computer.GetFlightStats()), 1, file);

  fread(&sizein, sizeof(sizein), 1, file);
  if (sizein != sizeof(glide_computer.GetOLC().data)) {
    glide_computer.GetOLC().ResetFlight();
    fclose(file);
    return;
  }

  // Read persistent memory into OLC.data
  fread(&glide_computer.GetOLC().data, sizeof(glide_computer.GetOLC().data), 1, file);

  fread(&sizein, sizeof(sizein), 1, file);
  if (sizein != 5 * sizeof(double)) {
    fclose(file);
    return;
  }

  double MACCREADY = GlidePolar::GetMacCready();
  double BUGS = GlidePolar::GetBugs();
  double BALLAST = GlidePolar::GetBallast();

  // Read persistent memory into McCready, QNH, bugs, ballast and temperature
  fread(&MACCREADY, sizeof(double), 1, file);
  fread(&QNH, sizeof(QNH), 1, file);
  fread(&BUGS, sizeof(double), 1, file);
  fread(&BALLAST, sizeof(double), 1, file);
  fread(&CuSonde::maxGroundTemperature,
      sizeof(CuSonde::maxGroundTemperature), 1, file);

  //    ReadFile(hFile,&CRUISE_EFFICIENCY,
  //             size,&dwBytesWritten,(OVERLAPPED*)NULL);

  QNH = min(1113.2, max(QNH, 913.2));
  MACCREADY = min(10.0, max(MACCREADY, 0.0));
  BUGS = min(1.0, max(BUGS, 0.0));
  BALLAST = min(1.0, max(BALLAST, 0.0));
  //   CRUISE_EFFICIENCY = min(1.5, max(CRUISE_EFFICIENCY,0.75));

  GlidePolar::SetMacCready(MACCREADY);
  GlidePolar::SetBugs(BUGS);
  GlidePolar::SetBallast(BALLAST);

  StartupStore(TEXT("LoadCalculationsPersist OK\n"));

  fclose(file);
}

/**
 * Saves the calculated values to the persistent memory file
 * @param gps_info The basic data
 * @param Calculated The calculated data
 */
void
SaveCalculationsPersist(const NMEA_INFO &gps_info,
    const DERIVED_INFO &Calculated)
{
  DWORD size;

  logger.LoggerClearFreeSpace(gps_info);

  if (FindFreeSpace(szCalculationsPersistDirectory) < MINFREESTORAGE) {
    if (!logger.LoggerClearFreeSpace(gps_info)) {
      StartupStore(TEXT("SaveCalculationsPersist insufficient storage\n"));
      return;
    } else {
      StartupStore(TEXT("SaveCalculationsPersist cleared logs to free storage\n"));
    }
  }

  StartupStore(TEXT("SaveCalculationsPersist\n"));

  FILE *file = _tfopen(szCalculationsPersistFileName, _T("wb"));

  if (file == NULL) {
    StartupStore(TEXT("SaveCalculationsPersist can't create file\n"));
    return;
  }

  size = sizeof(DERIVED_INFO);
  fwrite(&size, sizeof(size), 1, file);
  fwrite(&Calculated, size, 1, file);

  size = sizeof(FlightStatistics);
  fwrite(&size, sizeof(size), 1, file);
  fwrite(&glide_computer.GetFlightStats(), size, 1, file);

  size = sizeof(OLCData);
  fwrite(&size, sizeof(size), 1, file);
  fwrite(&glide_computer.GetOLC().data, size, 1, file);

  double MACCREADY = GlidePolar::GetMacCready();
  double BUGS = GlidePolar::GetBugs();
  double BALLAST = GlidePolar::GetBallast();

  size = sizeof(double)*5;
  fwrite(&size, sizeof(size), 1, file);
  fwrite(&MACCREADY, sizeof(MACCREADY), 1, file);
  fwrite(&QNH, sizeof(QNH), 1, file);
  fwrite(&BUGS, sizeof(BUGS), 1, file);
  fwrite(&BALLAST, sizeof(BALLAST), 1, file);
  fwrite(&CuSonde::maxGroundTemperature,
      sizeof(CuSonde::maxGroundTemperature), 1, file);

  //    WriteFile(hFile,&CRUISE_EFFICIENCY,
  //              size,&dwBytesWritten,(OVERLAPPED*)NULL);

  StartupStore(TEXT("SaveCalculationsPersist ok\n"));

  fclose(file);
}
