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

static bool
check_name(const Waypoint &waypoint, const TCHAR *Name)
{
  // TODO: detect and warn on multiple matches!
  
  TCHAR UName[100];
  _tcscpy(UName, waypoint.Name.c_str());
  CharUpper(UName); // WP name

  if (_tcscmp(UName, Name) == 0)
    return true;

  TCHAR tmp[100];

  _stprintf(tmp, TEXT("%s A/F"), Name);
  if (_tcscmp(UName, tmp) == 0)
    return true;
  
  _stprintf(tmp, TEXT("%s AF"), Name);
  if (_tcscmp(UName, tmp) == 0)
    return true;
  
  _stprintf(tmp, TEXT("%s A/D"), Name);
  if (_tcscmp(UName, tmp) == 0)
    return true;
  
  _stprintf(tmp, TEXT("%s AD"), Name);
  if (_tcscmp(UName, tmp) == 0)
    return true;
  
  _stprintf(tmp, TEXT("%s=HOME"), UName);
  if (_tcscmp(Name, tmp) == 0) {
    XCSoarInterface::SetSettingsComputer().HomeWaypoint = waypoint.id;
    return true;
  }

  return false;
}

static void
SetAirfieldDetails(Waypoints &way_points, TCHAR *Name, const tstring &Details)
{
  CharUpper(Name); // AIR name

  for (Waypoints::WaypointTree::const_iterator it = way_points.begin();
      it != way_points.end(); ++it) {
    if (it->get_waypoint().is_landable()
        && check_name(it->get_waypoint(), Name)) {
      way_points.set_details(it->get_waypoint(), Details);
      return;
    }
  }
}

/**
 * Parses the data provided by the airfield details file handle
 */
static void
ParseAirfieldDetails(Waypoints &way_points, TLineReader &reader)
{
  tstring Details;
  TCHAR Name[201];

  Name[0] = 0;

  bool inDetails = false;
  int i;

  long filesize = std::max(reader.size(), 1l);
  XCSoarInterface::SetProgressDialogMaxValue(100);

  TCHAR *TempString;
  while ((TempString = reader.read()) != NULL) {
    if (TempString[0] == '[') { // Look for start
      if (inDetails)
        SetAirfieldDetails(way_points, Name, Details);

      Details.clear();

      // extract name
      for (i = 1; i <= 201; i++) {
        if (TempString[i] == ']')
          break;

        Name[i - 1] = TempString[i];
      }
      Name[i - 1] = 0;

      inDetails = true;

      XCSoarInterface::SetProgressDialogValue(reader.tell() * 100 / filesize);
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
    SetAirfieldDetails(way_points, Name, Details);
    Details.clear();
  }
}

/**
 * Opens the airfield details file and parses it
 */
void
ReadAirfieldFile(Waypoints &way_points)
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

    ParseAirfieldDetails(way_points, reader);
  } else {
    Profile::Get(szProfileMapFile, path, MAX_PATH);
    if (string_is_empty(path))
      return;

    ExpandLocalPath(path);
    _tcscat(path, _T("/airfields.txt"));

    ZipLineReader reader(path);
    if (reader.error())
      return;

    ParseAirfieldDetails(way_points, reader);
  }
}
