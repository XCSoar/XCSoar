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

#include "Persist.hpp"
#include "XCSoar.h"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "SettingsComputer.hpp"
#include "OnLineContest.h"
#include "McReady.h"
#include "Math/Pressure.h"
#include "Atmosphere.h"
#include "UtilsSystem.hpp"
#include "Logger.h"

#include "Calculations.h"


static TCHAR szCalculationsPersistFileName[MAX_PATH]= TEXT("\0");
static TCHAR szCalculationsPersistDirectory[MAX_PATH]= TEXT("\0");

extern OLCOptimizer olc;

void DeleteCalculationsPersist(void) {
  DeleteFile(szCalculationsPersistFileName);
}

void LoadCalculationsPersist(DERIVED_INFO *Calculated) {
  if (szCalculationsPersistFileName[0]==0) {
#ifdef GNAV
    LocalPath(szCalculationsPersistFileName,
              TEXT("persist/xcsoar-persist.log"));
    LocalPath(szCalculationsPersistDirectory,
              TEXT("persist"));
#else
    LocalPath(szCalculationsPersistFileName,
              TEXT("xcsoar-persist.log"));
    LocalPath(szCalculationsPersistDirectory);
#endif
  }

  StartupStore(TEXT("LoadCalculationsPersist\n"));

  DWORD sizein;

  FILE *file = _tfopen(szCalculationsPersistFileName, _T("rb"));
  if (file != NULL) {
    fread(&sizein, sizeof(sizein), 1, file);
    if (sizein != sizeof(*Calculated)) {
      fclose(file);
      return;
    }

    fread(Calculated, sizeof(*Calculated), 1, file);

    fread(&sizein, sizeof(sizein), 1, file);
    if (sizein != sizeof(flightstats)) {
      flightstats.Reset();
      fclose(file);
      return;
    }

    fread(&flightstats, sizeof(flightstats), 1, file);

    fread(&sizein, sizeof(sizein), 1, file);
    if (sizein != sizeof(olc.data)) {
      olc.ResetFlight();
      fclose(file);
      return;
    }

    fread(&olc.data, sizeof(olc.data), 1, file);

    fread(&sizein, sizeof(sizein), 1, file);
    if (sizein != 5 * sizeof(double)) {
      fclose(file);
      return;
    }

    fread(&MACCREADY, sizeof(MACCREADY), 1, file);
    fread(&QNH, sizeof(QNH), 1, file);
    fread(&BUGS, sizeof(BUGS), 1, file);
    fread(&BALLAST, sizeof(BALLAST), 1, file);
    fread(&CuSonde::maxGroundTemperature,
          sizeof(CuSonde::maxGroundTemperature), 1, file);

    //    ReadFile(hFile,&CRUISE_EFFICIENCY,
    //             size,&dwBytesWritten,(OVERLAPPED*)NULL);

    MACCREADY = min(10.0,max(MACCREADY,0));
    QNH = min(1113.2, max(QNH,913.2));
    BUGS = min(1.0, max(BUGS,0.0));
    BALLAST = min(1.0, max(BALLAST,0.0));
    //   CRUISE_EFFICIENCY = min(1.5, max(CRUISE_EFFICIENCY,0.75));

    StartupStore(TEXT("LoadCalculationsPersist OK\n"));

    fclose(file);
  } else {
    StartupStore(TEXT("LoadCalculationsPersist file not found\n"));
  }
}


void SaveCalculationsPersist(DERIVED_INFO *Calculated) {
  DWORD size;

  LoggerClearFreeSpace();

  if (FindFreeSpace(szCalculationsPersistDirectory)<MINFREESTORAGE) {
    if (!LoggerClearFreeSpace()) {
      StartupStore(TEXT("SaveCalculationsPersist insufficient storage\n"));
      return;
    } else {
      StartupStore(TEXT("SaveCalculationsPersist cleared logs to free storage\n"));
    }
  }

  StartupStore(TEXT("SaveCalculationsPersist\n"));

  FILE *file = _tfopen(szCalculationsPersistFileName, _T("wb"));
  if (file != NULL) {
    size = sizeof(DERIVED_INFO);
    fwrite(&size, sizeof(size), 1, file);
    fwrite(Calculated, sizeof(*Calculated), 1, file);

    size = sizeof(Statistics);
    fwrite(&size, sizeof(size), 1, file);
    fwrite(&flightstats, sizeof(flightstats), 1, file);

    size = sizeof(OLCData);
    fwrite(&size, sizeof(size), 1, file);
    fwrite(&olc.data, sizeof(olc.data), 1, file);

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
  } else {
    StartupStore(TEXT("SaveCalculationsPersist can't create file\n"));
  }

}

