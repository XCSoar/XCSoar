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
#include "Language.hpp"
#include "Profile.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "StringUtil.hpp"
#include "UtilsText.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ZipLineReader.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

extern Waypoints way_points;

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
SetAirfieldDetails(TCHAR *Name, const tstring &Details)
{
  CharUpper(Name); // AIR name

  for (Waypoints::WaypointTree::const_iterator it = way_points.begin();
      it != way_points.end(); ++it) {
    if (check_name(it->get_waypoint(), Name)) {
      way_points.set_details(it->get_waypoint(), Details);
      return;
    }
  }
}

/**
 * Parses the data provided by the airfield details file handle
 */
static void
ParseAirfieldDetails(TLineReader &reader)
{
  TCHAR CleanString[READLINE_LENGTH + 1];
  tstring Details;
  TCHAR Name[201];

  Name[0] = 0;
  CleanString[0] = 0;

  bool inDetails = false;
  int i;

  double filesize = std::max(reader.size(), 1l);
  XCSoarInterface::SetProgressDialogMaxValue(100);

  TCHAR *TempString;
  while ((TempString = reader.read()) != NULL) {
    if (TempString[0] == '[') { // Look for start
      if (inDetails)
        SetAirfieldDetails(Name, Details);

      Details.clear();

      // extract name
      for (i = 1; i <= 201; i++) {
        if (TempString[i] == ']')
          break;

        Name[i - 1] = TempString[i];
      }
      Name[i - 1] = 0;

      inDetails = true;

      unsigned status = reader.tell() * 100 / filesize;
      XCSoarInterface::SetProgressDialogValue(status);

      TCHAR status_text[100];
      _stprintf(status_text,
                TEXT("Loading Airfield Details File... %u %%"),
                status);
      XCSoarInterface::SetProgressDialogText(status_text);
    } else {
      // append text to details string
      if (!string_is_empty(TempString)) {
        // Remove carriage returns
        TrimRight(TempString);

        Details += TempString;
        Details += _T("\r\n");
      }
    }
  }

  if (inDetails) {
    SetAirfieldDetails(Name, Details);
    Details.clear();
  }
}

/**
 * Opens the airfield details file and parses it
 */
void
ReadAirfieldFile()
{
  LogStartUp(TEXT("ReadAirfieldFile"));
  XCSoarInterface::CreateProgressDialog(
      gettext(TEXT("Loading Airfield Details File...")));

  TCHAR path[MAX_PATH];
  Profile::Get(szProfileAirfieldFile, path, MAX_PATH);

  if (!string_is_empty(path)) {
    ExpandLocalPath(path);

    FileLineReader reader(path);
    if (reader.error())
      return;

    ParseAirfieldDetails(reader);
  } else {
    Profile::Get(szProfileMapFile, path, MAX_PATH);
    if (string_is_empty(path))
      return;

    ExpandLocalPath(path);
    _tcscat(path, _T("/airfields.txt"));

    ZipLineReader reader(path);
    if (reader.error())
      return;

    ParseAirfieldDetails(reader);
  }
}
