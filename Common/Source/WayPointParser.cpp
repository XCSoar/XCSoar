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

#include "WayPointParser.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "DeviceBlackboard.hpp"
#include "SettingsComputer.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "Registry.hpp"
#include "Profile.hpp"
#include "LocalPath.hpp"
#include "UtilsProfile.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "MapWindowProjection.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "Waypoint/Waypoints.hpp"

#include <tchar.h>
#include <stdio.h>

#ifdef HAVE_POSIX
#include <errno.h>
#endif

#include "wcecompat/ts_string.h"

int WaypointsOutOfRange = 1; // include all

static int globalFileNum = 0;

TCHAR *strtok_r(const TCHAR *s, TCHAR *delim, TCHAR **lasts);

//static void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber);

static bool ParseWayPointString(Waypoint &way_point, const TCHAR *input,
    const RasterTerrain *terrain);
static bool ParseAngle(const TCHAR *input, fixed *value_r, TCHAR **endptr_r);
static void ParseFlags(const TCHAR *input, const TCHAR **endptr_r, Waypoint& waypoint);
static bool ParseAltitude(const TCHAR *input, fixed *altitude_r, TCHAR **endptr_r);

static TCHAR TempString[READLINE_LENGTH];

static int WaypointOutOfTerrainRangeDontAskAgain = -1;

static void
CloseWaypoints(Waypoints &way_points)
{
  way_points.clear();
  WaypointOutOfTerrainRangeDontAskAgain = WaypointsOutOfRange;
}

static bool
WaypointInTerrainRange(const Waypoint &way_point, const RasterTerrain &terrain)
{
  TCHAR sTmp[250];
  int res;

  if (WaypointOutOfTerrainRangeDontAskAgain == 1)
    return true;

  if (!terrain.isTerrainLoaded())
    return true;

  if (terrain.WaypointIsInTerrainRange(way_point.Location)) {
    return true;
  } 

  if (WaypointOutOfTerrainRangeDontAskAgain == 2)
    return false;
  if (WaypointOutOfTerrainRangeDontAskAgain == 1)
    return true;

  if (WaypointOutOfTerrainRangeDontAskAgain != 0)
    return false;

  _stprintf(sTmp,gettext(
      TEXT("Waypoint #%d \"%s\" \r\nout of Terrain bounds\r\n\r\nLoad anyway?")),
      way_point.id, way_point.Name.c_str());

  res = dlgWaypointOutOfTerrain(sTmp);

  switch (res) {
  case wpTerrainBoundsYes:
    return true;

  case wpTerrainBoundsNo:
    return false;

  case wpTerrainBoundsYesAll:
    WaypointOutOfTerrainRangeDontAskAgain = 1;
    WaypointsOutOfRange = 1;
    SetToRegistry(szRegistryWaypointsOutOfRange, WaypointsOutOfRange);
    Profile::StoreRegistry();
    return true;

  case mrCancel:
  case wpTerrainBoundsNoAll:
    WaypointOutOfTerrainRangeDontAskAgain = 2;
    WaypointsOutOfRange = 2;
    SetToRegistry(szRegistryWaypointsOutOfRange, WaypointsOutOfRange);
    Profile::StoreRegistry();
    return false;
  }

  return false;
}

static int
ParseWayPointError(int LineNumber, const TCHAR *FileName, const TCHAR *String)
{
  TCHAR szTemp[250];

  if (!string_is_empty(FileName)) {
    _stprintf(szTemp, TEXT("%s\r\n%s %s %s %d\r\n%s"),
              gettext(TEXT("Waypointfile Parse Error")),
              gettext(TEXT("File")), FileName,
              gettext(TEXT("Line")), LineNumber, String);
  } else {
    _stprintf(szTemp, TEXT("%s\r\n%s %s %d\r\n%s"),
              gettext(TEXT("Waypointfile Parse Error")),
              gettext(TEXT("(Map file)")),
              gettext(TEXT("Line")), LineNumber, String);
  }

  MessageBoxX(szTemp, gettext(TEXT("Error")), MB_OK | MB_ICONWARNING);

  return 1;
}

static bool
FeedWayPointLine(Waypoints &way_points, const RasterTerrain *terrain,
    const TCHAR *line)
{
  if (TempString[0] == '\0' || TempString[0] == 0x1a || // dos end of file
      _tcsstr(TempString, TEXT("**")) == TempString || // Look For Comment
      _tcsstr(TempString, TEXT("*")) == TempString) // Look For SeeYou Comment
    /* nothing was parsed, return without error condition */
    return true;

  Waypoint new_waypoint = way_points.create(GEOPOINT(0,0));
  if (!ParseWayPointString(new_waypoint, TempString, terrain)) {
    return false;
  }

  if (terrain != NULL && !WaypointInTerrainRange(new_waypoint, *terrain)) {
    return true;
  }

  way_points.append(new_waypoint);
  return true;
}

static void
ReadWayPointFile(FILE *fp, const TCHAR *CurrentWpFileName,
    Waypoints &way_points, const RasterTerrain *terrain)
{
  //  TCHAR szTemp[100];
  int nTrigger = 10;
  DWORD fSize, fPos = 0;
  int nLineNumber = 0;

  XCSoarInterface::CreateProgressDialog(
      gettext(TEXT("Loading Waypoints File...")));

  fseek(fp, 0, SEEK_END);
  fSize = ftell(fp);
  fseek(fp, 0, SEEK_SET); /* no rewind() on PPC */

  if (fSize == 0)
    return;

  // SetFilePointer(hFile,0,NULL,FILE_BEGIN);
  fPos = 0;
  nTrigger = (fSize / 10);

  while (ReadStringX(fp, READLINE_LENGTH, TempString)) {
    nLineNumber++;
    fPos += _tcslen(TempString);

    if (nTrigger < (int)fPos) {
      nTrigger += (fSize / 10);
      XCSoarInterface::StepProgressDialog();
    }

    if (!FeedWayPointLine(way_points, terrain, TempString)
        && ParseWayPointError(nLineNumber, CurrentWpFileName, TempString) != 1)
      break;
  }
}

static void
ReadWayPointFile(ZZIP_FILE *fp, const TCHAR *CurrentWpFileName,
    Waypoints &way_points, const RasterTerrain *terrain)
{
  //  TCHAR szTemp[100];
  int nTrigger = 10;
  DWORD fSize, fPos = 0;
  int nLineNumber = 0;

  XCSoarInterface::CreateProgressDialog(
      gettext(TEXT("Loading Waypoints File...")));

  fSize = zzip_file_size(fp);

  if (fSize == 0)
    return;

  // SetFilePointer(hFile,0,NULL,FILE_BEGIN);
  fPos = 0;
  nTrigger = (fSize / 10);

  while (ReadString(fp, READLINE_LENGTH, TempString)) {
    nLineNumber++;
    fPos += _tcslen(TempString);

    if (nTrigger < (int)fPos) {
      nTrigger += (fSize / 10);
      XCSoarInterface::StepProgressDialog();
    }

    if (!FeedWayPointLine(way_points, terrain, TempString)
        && ParseWayPointError(nLineNumber, CurrentWpFileName, TempString) != 1)
      break;
  }
}

void
WaypointAltitudeFromTerrain(Waypoint &way_point, const RasterTerrain &terrain)
{
  double myalt = -1;

  if (terrain.GetMap()) {
    RasterRounding rounding(*terrain.GetMap(), 0, 0);
    myalt = terrain.GetTerrainHeight(way_point.Location, rounding);
  }

  if (myalt > 0) {
    way_point.Altitude = myalt;
  } else {
    // error, can't find altitude for waypoint!
  }
}

static bool
ParseWayPointString(Waypoint &way_point, const TCHAR *input,
    const RasterTerrain *terrain)
{
  TCHAR *endptr;

  way_point.FileNum = globalFileNum;

  unsigned ignore_id = _tcstol(input, &endptr, 10);
  (void)ignore_id;

  if (endptr == input || *endptr != _T(','))
    return false;
  //  way_point.id;
  // JMW note: we use internal ids now, so ignore file ids

  input = endptr + 1;

  if (!ParseAngle(input, &way_point.Location.Latitude, &endptr)
      || way_point.Location.Latitude > 90
      || way_point.Location.Latitude < -90
      || *endptr != _T(','))
    return false;

  input = endptr + 1;

  ParseAngle(input, &way_point.Location.Longitude, &endptr);
  if (!ParseAngle(input, &way_point.Location.Longitude, &endptr)
      || way_point.Location.Longitude > 180
      || way_point.Location.Longitude < -180
      || *endptr != _T(','))
    return false;

  input = endptr + 1;

  if (!ParseAltitude(input, &way_point.Altitude, &endptr)
      || *endptr != _T(','))
    return false;

  input = endptr + 1;

  ParseFlags(input, &input, way_point);
  if (*input != _T(','))
    return false;

  ++input;

  {
    tstring text = input;
    size_t end = text.find_first_of(TEXT(",")); 
    if (!end) {
      return false;
    }
    way_point.Name = trim(text.substr(0, end));
  }

  endptr = _tcschr(input, _T(','));
  if (endptr != NULL) {
    input = endptr + 1;
    endptr = _tcschr(input, '*');
    if (endptr != NULL) {
      // if it is a home waypoint raise zoom level
      way_point.Zoom = _tcstol(endptr + 1, NULL, 10);
    } else {
      way_point.Zoom = 0;
    }
    if (endptr>input) {
      tstring text = input;
      way_point.Comment = trim(text.substr(0,endptr-input));
    } else {
      way_point.Comment = trim(input);
    }
  } else {
    way_point.Comment.clear();
    way_point.Zoom = 0;
  }

  if (way_point.Altitude <= 0 && terrain != NULL)
    WaypointAltitudeFromTerrain(way_point, *terrain);

  return true;
}


static bool
ParseAngle(const TCHAR *input, fixed *value_r, TCHAR **endptr_r)
{
  TCHAR *endptr;
  double Degrees, Mins;

  Degrees = (double)_tcstol(input, &endptr, 10);
  if (endptr == input || *endptr != ':')
    return false;

  input = endptr + 1;
  Mins = (double)_tcstod(input, &endptr);
  if (endptr == input)
    return false;

  if (endptr[-1] == 'E')
    // this is a hack: strtod() stops after the "E" (east, or exponent);
    // get it back
    --endptr;

  input = endptr;

  if (*input == ':') {
    ++input;
    Mins += ((double)_tcstol(input, &endptr, 10) / 60.0);

    if (endptr == input)
      return false;
  }

  Degrees += (Mins / 60);

  if ((*endptr == 'N') || (*endptr == 'E'))
    ; // nothing
  else if ((*endptr == 'S') || (*endptr == 'W'))
    Degrees *= -1;
  else
    return false;

  *value_r = Degrees;
  *endptr_r = endptr + 1;

  return true;
}

static void
ParseFlags(const TCHAR *input, const TCHAR **endptr_r, Waypoint &waypoint)
{
  waypoint.Flags.Airport = 0;
  waypoint.Flags.TurnPoint = 0;
  waypoint.Flags.LandPoint = 0;
  waypoint.Flags.Home = 0;
  waypoint.Flags.StartPoint = 0;
  waypoint.Flags.FinishPoint = 0;
  waypoint.Flags.Restricted = 0;
  waypoint.Flags.WaypointFlag = 0;

  while (_istalpha(*input)) {
    switch (*input++) {
    case 'A':
      waypoint.Flags.Airport = true;
      break;

    case 'T':
      waypoint.Flags.TurnPoint = true;
      break;

    case 'L':
      waypoint.Flags.LandPoint = true;
      break;

    case 'H':
      waypoint.Flags.Home = true;
      break;

    case 'S':
      waypoint.Flags.StartPoint = true;
      break;

    case 'F':
      waypoint.Flags.FinishPoint = true;
      break;

    case 'R':
      waypoint.Flags.Restricted = true;
      break;

    case 'W':
      waypoint.Flags.WaypointFlag = true;
      break;
    }
  }

  *endptr_r = input;
}

static bool
ParseAltitude(const TCHAR *input, fixed *altitude_r, TCHAR **endptr_r)
{
  TCHAR *endptr;
  double altitude;

  altitude = _tcstod(input, &endptr);
  if (endptr == input)
    return false;

  switch (*endptr) {
  case 'M': // meter's nothing to do
  case 'm':
    ++endptr;
    break;

  case 'F': // feet, convert to meter
  case 'f':
    altitude /= TOFEET;
    ++endptr;
    break;
  }

  *altitude_r = altitude;
  *endptr_r = endptr;

  return true;
}

bool
ReadWayPointZipFile(const TCHAR *path, Waypoints &way_points,
    const RasterTerrain *terrain)
{
  char path_ascii[MAX_PATH];
  ZZIP_FILE *fp;

  unicode2ascii(path, path_ascii, sizeof(path_ascii));
  fp = zzip_fopen(path_ascii, "rt");
  if (fp == NULL)
    return false;

  ReadWayPointFile(fp, path, way_points, terrain);
  zzip_fclose(fp);
  return true;
}

bool
ReadWayPointFile(const TCHAR *path, Waypoints &way_points,
                 const RasterTerrain *terrain)
{
  char path_ascii[MAX_PATH];
  FILE *fp;

  unicode2ascii(path, path_ascii, sizeof(path_ascii));
  fp = fopen(path_ascii, "rt");
  if (fp == NULL)
    /* fall back to ReadWayPointZipFile() if the file was not found */
    return
    #ifdef HAVE_POSIX
    errno == ENOTDIR &&
    #endif
    ReadWayPointZipFile(path, way_points, terrain);

  ReadWayPointFile(fp, path, way_points, terrain);
  fclose(fp);

  return true;
}

void
ReadWaypoints(Waypoints &way_points, const RasterTerrain *terrain)
{
  StartupStore(TEXT("ReadWaypoints\n"));

  TCHAR szFile[MAX_PATH];
  bool file_embedded = false;

  // JMW TODO protect with mutex (whole waypoint list class)

  CloseWaypoints(way_points);

  GetRegistryString(szRegistryWayPointFile, szFile, MAX_PATH);
  SetRegistryString(szRegistryWayPointFile, TEXT("\0"));

  if (!string_is_empty(szFile)) {
    ExpandLocalPath(szFile);
  } else {
    file_embedded = true;
    GetRegistryString(szRegistryMapFile, szFile, MAX_PATH);
    ExpandLocalPath(szFile);
    _tcscat(szFile, TEXT("/"));
    _tcscat(szFile, TEXT("waypoints.xcw"));
  }

  globalFileNum = 0;
  if (ReadWayPointFile(szFile, way_points, terrain)) {
    // read OK, so set the registry to the actual file name
    if (!file_embedded) {
      printf("save\n");
      ContractLocalPath(szFile);
      SetRegistryString(szRegistryWayPointFile, szFile);
    }

    // inform Waypoints whether it is ok to write to this file
    way_points.set_file0_writable(!file_embedded);
  } else {
    StartupStore(TEXT("No waypoint file 1\n"));
  }

  // read additional waypoint file

  GetRegistryString(szRegistryAdditionalWayPointFile, szFile, MAX_PATH);

  SetRegistryString(szRegistryAdditionalWayPointFile, TEXT("\0"));

  if (!string_is_empty(szFile)) {
    ExpandLocalPath(szFile);

    globalFileNum = 1;
    if (ReadWayPointFile(szFile, way_points, terrain)) {
      // read OK, so set the registry to the actual file name
      ContractLocalPath(szFile);
      SetRegistryString(szRegistryAdditionalWayPointFile, szFile);
    } else {
      StartupStore(TEXT("No waypoint file 2\n"));
    }
  }
  way_points.optimise();
}

void
SetHome(const Waypoints &way_points, const RasterTerrain *terrain,
        SETTINGS_COMPUTER &settings,
        const bool reset, const bool set_location)
{
  StartupStore(TEXT("SetHome\n"));

  // check invalid home waypoint or forced reset due to file change
  // VENTA3
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.HomeWaypoint)) {
    settings.HomeWaypoint = -1;
  }

  // VENTA3 -- reset Alternates
  if (reset
      || !way_points.lookup_id(settings.Alternate1)
      || !way_points.lookup_id(settings.Alternate2)) {
    settings.Alternate1= -1;
    settings.Alternate2= -1;
  }

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || !way_points.lookup_id(settings.TeamCodeRefWaypoint)) 
    settings.TeamCodeRefWaypoint = -1;

  if (!way_points.lookup_id(settings.HomeWaypoint)) {
    // search for home in waypoint list, if we don't have a home
    settings.HomeWaypoint = -1;

    if (settings.HomeWaypoint == -1) {
      const Waypoint* wp = way_points.find_home();
      if (wp) {
        settings.HomeWaypoint = wp->id;
      }
    }
  }

  // set team code reference waypoint if we don't have one
  if (settings.TeamCodeRefWaypoint == -1)
    settings.TeamCodeRefWaypoint = settings.HomeWaypoint;


  if (set_location) {
    if (const Waypoint *wp = way_points.lookup_id(settings.HomeWaypoint)) {
      // OK, passed all checks now
      StartupStore(TEXT("Start at home waypoint\n"));
      device_blackboard.SetStartupLocation(wp->Location, wp->Altitude);
    } else if (terrain != NULL) {
      // no home at all, so set it from center of terrain if available
      GEOPOINT loc;
      if (terrain->GetTerrainCenter(&loc)) {
        StartupStore(TEXT("Start at terrain center\n"));
        device_blackboard.SetStartupLocation(loc, 0);
      }
    }
  }

  //
  // Save the home waypoint number in the resgistry
  //
  // VENTA3> this is probably useless, since HomeWayPoint &c were currently
  //         just loaded from registry.
  SetToRegistry(szRegistryHomeWaypoint,settings.HomeWaypoint);
  SetToRegistry(szRegistryAlternate1,settings.Alternate1);
  SetToRegistry(szRegistryAlternate2,settings.Alternate2);
  SetToRegistry(szRegistryTeamcodeRefWaypoint,settings.TeamCodeRefWaypoint);
}

// Number,Latitude,Longitude,Altitude,Flags,Name,Comment(,Zoom))
// Number starts at 1
// Lat/long expressed as D:M:S[N/S/E/W]
// Altitude as XXXM
// Flags: T,H,A,L

static void
WaypointFlagsToString(const Waypoint& waypoint, TCHAR *Flags)
{
  if (waypoint.Flags.Airport) {
    _tcscat(Flags,TEXT("A"));
  }
  if (waypoint.Flags.TurnPoint) {
    _tcscat(Flags,TEXT("T"));
  }
  if (waypoint.Flags.LandPoint) {
    _tcscat(Flags,TEXT("L"));
  }
  if (waypoint.Flags.Home) {
    _tcscat(Flags,TEXT("H"));
  }
  if (waypoint.Flags.StartPoint) {
    _tcscat(Flags,TEXT("S"));
  }
  if (waypoint.Flags.FinishPoint) {
    _tcscat(Flags,TEXT("F"));
  }
  if (waypoint.Flags.Restricted) {
    _tcscat(Flags,TEXT("R"));
  }
  if (waypoint.Flags.WaypointFlag) {
    _tcscat(Flags,TEXT("W"));
  }

  // set as turnpoint by default if nothing else
  if (string_is_empty(Flags))
    _tcscat(Flags, TEXT("T"));
}

static void
WaypointLongitudeToString(double Longitude, TCHAR *Buffer)
{
  TCHAR EW[] = TEXT("WE");
  int dd, mm, ss;

  int sign = Longitude < 0 ? 0 : 1;
  Longitude = fabs(Longitude);

  dd = (int)Longitude;
  Longitude = (Longitude - dd) * 60.0;
  mm = (int)(Longitude);
  Longitude = (Longitude - mm) * 60.0;
  ss = (int)(Longitude + 0.5);

  if (ss >= 60) {
    mm++;
    ss -= 60;
  }

  if (mm >= 60) {
    dd++;
    mm -= 60;
  }

  _stprintf(Buffer, TEXT("%03d:%02d:%02d%c"), dd, mm, ss, EW[sign]);
}

static void
WaypointLatitudeToString(double Latitude, TCHAR *Buffer)
{
  TCHAR EW[] = TEXT("SN");
  int dd, mm, ss;

  int sign = Latitude < 0 ? 0 : 1;
  Latitude = fabs(Latitude);

  dd = (int)Latitude;
  Latitude = (Latitude - dd) * 60.0;
  mm = (int)(Latitude);
  Latitude = (Latitude - mm) * 60.0;
  ss = (int)(Latitude + 0.5);

  if (ss >= 60) {
    mm++;
    ss -= 60;
  }

  if (mm >= 60) {
    dd++;
    mm -= 60;
  }
  _stprintf(Buffer, TEXT("%02d:%02d:%02d%c"), dd, mm, ss, EW[sign]);
}

static void
WriteWayPointFileWayPoint(FILE *fp, const Waypoint &way_point)
{
  TCHAR Flags[MAX_PATH];
  TCHAR Latitude[MAX_PATH];
  TCHAR Longitude[MAX_PATH];

  Flags[0] = 0;

  WaypointLatitudeToString(way_point.Location.Latitude, Latitude);
  WaypointLongitudeToString(way_point.Location.Longitude, Longitude);
  WaypointFlagsToString(way_point, Flags);

  fprintf(fp,"%d,%S,%S,%dM,%S,%S,%S\r\n",
          way_point.id,
          Latitude,
          Longitude,
          iround(way_point.Altitude),
          Flags,
          way_point.Name.c_str(),
          way_point.Comment.c_str());
}

static void
WriteWayPointFile(Waypoints &way_points, FILE *fp,
                  const SETTINGS_COMPUTER &settings_computer)
{
  for (Waypoints::WaypointTree::const_iterator it = way_points.begin();
       it != way_points.end(); it++) {
    const Waypoint& wp = it->get_waypoint();
    if (wp.FileNum == globalFileNum) {
      WriteWayPointFileWayPoint(fp, wp);
    }
  }
}

void
WaypointWriteFiles(Waypoints &way_points,
                   const SETTINGS_COMPUTER &settings_computer)
{
  // JMW TODO protect with mutex (whole waypoint list class)

  TCHAR szFile[MAX_PATH];

  FILE *fp = NULL;

  way_points.set_home(settings_computer.HomeWaypoint);

  GetRegistryString(szRegistryWayPointFile, szFile, MAX_PATH);
  ExpandLocalPath(szFile);

  if (!string_is_empty(szFile)) {
    fp = _tfopen(szFile, TEXT("wb"));
  } else {
    LocalPath(szFile);
    _tcscat(szFile, TEXT("\\waypoints1.dat"));
    fp = _tfopen(szFile, TEXT("wb"));
  }

  if (fp != NULL) {
    globalFileNum = 0;
    WriteWayPointFile(way_points, fp, settings_computer);
    fprintf(fp, "\r\n");
    fclose(fp);
    fp = NULL;
  }

  GetRegistryString(szRegistryAdditionalWayPointFile, szFile, MAX_PATH);
  ExpandLocalPath(szFile);

  if (!string_is_empty(szFile)) {
    fp = _tfopen(szFile, TEXT("wb"));
  } else {
    LocalPath(szFile);
    _tcscat(szFile, TEXT("\\waypoints2.dat"));
    fp = _tfopen(szFile, TEXT("wb"));
  }

  if (fp != NULL) {
    globalFileNum = 1;
    WriteWayPointFile(way_points, fp, settings_computer);
    fprintf(fp, "\r\n");
    fclose(fp);
    fp = NULL;
  }
}
