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

#include "AirfieldDetails.h"
#include "UtilsText.hpp"
#include "SettingsTask.hpp"
#include "Language.hpp"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "StringUtil.hpp"

#include <zzip/lib.h>
#include "wcecompat/ts_string.h"
#include <stdlib.h>
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointSorter.hpp"
#include "Components.hpp"

ZZIP_FILE* zAirfieldDetails = NULL;

static TCHAR  szAirfieldDetailsFile[MAX_PATH] = TEXT("\0");

/**
 * Opens the airfield details file handle
 */
static void
OpenAirfieldDetails()
{
  char zfilename[MAX_PATH];

  zAirfieldDetails = NULL;

  GetRegistryString(szRegistryAirfieldFile, szAirfieldDetailsFile, MAX_PATH);

  if (!string_is_empty(szAirfieldDetailsFile)) {
    ExpandLocalPath(szAirfieldDetailsFile);
    unicode2ascii(szAirfieldDetailsFile, zfilename, MAX_PATH);
    SetRegistryString(szRegistryAirfieldFile, TEXT("\0"));
  } else {
    static TCHAR szFile[MAX_PATH];
    GetRegistryString(szRegistryMapFile, szFile, MAX_PATH);
    if (!string_is_empty(szFile)) {
      ExpandLocalPath(szFile);
      _tcscat(szFile,TEXT("/airfields.txt"));
      unicode2ascii(szFile, zfilename, MAX_PATH);
    } else {
      zfilename[0]= 0;
    }
  }
  if (strlen(zfilename)>0) {
    zAirfieldDetails = zzip_fopen(zfilename,"rb");
  }
}

/**
 * Closes the airfield details file handle
 */
static void
CloseAirfieldDetails()
{
  if (zAirfieldDetails == NULL) {
    return;
  }
  // file was OK, so save the registry
  ContractLocalPath(szAirfieldDetailsFile);
  SetRegistryString(szRegistryAirfieldFile, szAirfieldDetailsFile);

  zzip_fclose(zAirfieldDetails);
  zAirfieldDetails = NULL;
}


static bool
check_name(const Waypoint &waypoint, const TCHAR *Name)
{
  // TODO: detect and warn on multiple matches!
  
  TCHAR UName[100];
  TCHAR NameA[100];
  TCHAR NameB[100];
  TCHAR NameC[100];
  TCHAR NameD[100];
  TCHAR TmpName[100];
  
  _tcscpy(UName, waypoint.Name.c_str());
  
  CharUpper(UName); // WP name
  // VENTA3 fix: If airfields name
  // was not uppercase it was not recon
  
  _stprintf(NameA, TEXT("%s A/F"), Name);
  _stprintf(NameB, TEXT("%s AF"), Name);
  _stprintf(NameC, TEXT("%s A/D"), Name);
  _stprintf(NameD, TEXT("%s AD"), Name);
  
  bool isHome = false;
  
  _stprintf(TmpName, TEXT("%s=HOME"), UName);
  if ((_tcscmp(Name, TmpName) == 0))
    isHome = true;
  
  if (isHome == true) {
    XCSoarInterface::SetSettingsComputer().HomeWaypoint = waypoint.id;
  }
  
  if ((_tcscmp(UName, Name) == 0)
      || (_tcscmp(UName, NameA) == 0)
      || (_tcscmp(UName, NameB) == 0)
      || (_tcscmp(UName, NameC) == 0)
      || (_tcscmp(UName, NameD) == 0) || isHome) {
    // found
    return true;
  } else {
    return false;
  }
}

static void
LookupAirfieldDetail(WaypointSelectInfoVector &airports,
                     TCHAR *Name, const tstring &Details)
{
  CharUpper(Name); // AIR name

  for (WaypointSelectInfoVector::const_iterator it =
         airports.begin(); it!= airports.end(); ++it) {
    const Waypoint &wp = *it->way_point;

    if (check_name(wp, Name)) {
      way_points.set_details(wp, Details);

      airports.erase(v); // this one no longer needs searching, remove from list
      return;
    }
  }
}

/**
 * Parses the data provided by the airfield details file handle
 */
static void
ParseAirfieldDetails()
{
  /*
   * VENTA3 fix: if empty lines, do not set details for the waypoint
   *        fix: remove CR from text appearing as a spurious char in waypoint details
   */

  if (zAirfieldDetails == NULL)
    return;

  TCHAR TempString[READLINE_LENGTH + 1];
  TCHAR CleanString[READLINE_LENGTH + 1];
  tstring Details;
  TCHAR Name[201];

  Name[0] = 0;
  TempString[0] = 0;
  CleanString[0] = 0;

  bool inDetails = false;
  bool hasDetails = false; // VENTA3
  int i, n;
  unsigned j;
  int k = 0;

  WaypointSorter waypoints_filter(way_points, XCSoarInterface::Basic().Location, 1);
  WaypointSelectInfoVector airports = waypoints_filter.get_list();
  waypoints_filter.filter_airport(airports);

  while (ReadString(zAirfieldDetails, READLINE_LENGTH, TempString)) {
    if (TempString[0] == '[') { // Look for start
      if (inDetails) {
        LookupAirfieldDetail(airports, Name, Details);
        Details.clear();
        Name[0] = 0;
        hasDetails = false;
      }

      // extract name
      for (i = 1; i < 200; i++) {
        if (TempString[i] == ']') {
          break;
        }
        Name[i - 1] = TempString[i];
      }
      Name[i - 1] = 0;

      inDetails = true;

      if (k % 20 == 0) {
        XCSoarInterface::StepProgressDialog();
      }
      k++;
    } else {
      // VENTA3: append text to details string
      for (j = 0; j < _tcslen(TempString); j++) {
        if (TempString[j] > 0x20) {
          hasDetails = true;
          break;
        }
      }

      // first hasDetails set true for rest of details
      if (hasDetails == true) {
        // Remove carriage returns
        for (j = 0, n = 0; j < _tcslen(TempString); j++) {
          if (TempString[j] == 0x0d)
            continue;
          CleanString[n++] = TempString[j];
        }
        CleanString[n] = '\0';

        Details += CleanString;
        Details += _T("\r\n");
      }
    }
  }

  if (inDetails) {
    LookupAirfieldDetail(airports, Name, Details);
    Details.clear();
  }
}

/**
 * Opens the airfield details file and parses it
 */
void
ReadAirfieldFile()
{
  StartupStore(TEXT("ReadAirfieldFile\n"));
  XCSoarInterface::CreateProgressDialog(
      gettext(TEXT("Loading Airfield Details File...")));

  OpenAirfieldDetails();
  ParseAirfieldDetails();
  CloseAirfieldDetails();
}
