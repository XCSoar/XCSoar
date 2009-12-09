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

#include "Waypointparser.h"
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
#include "MapWindowProjection.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "WayPointList.hpp"

#include <tchar.h>
#include <stdio.h>

#ifdef HAVE_POSIX
#include <errno.h>
#endif

#include "wcecompat/ts_string.h"

static int globalFileNum = 0;

TCHAR *strtok_r(const TCHAR *s, TCHAR *delim, TCHAR **lasts);

//static void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber);
static bool ParseWayPointString(WAYPOINT &way_point, const TCHAR *input,
    const RasterTerrain *terrain);

static bool ParseAngle(const TCHAR *input, double *value_r, TCHAR **endptr_r);
static int ParseFlags(const TCHAR *input, const TCHAR **endptr_r);
static bool ParseAltitude(const TCHAR *input, double *altitude_r, TCHAR **endptr_r);

static TCHAR TempString[READLINE_LENGTH];

static int WaypointOutOfTerrainRangeDontAskAgain = -1;

static void
CloseWayPoints(WayPointList &way_points)
{
  way_points.clear();
  WaypointOutOfTerrainRangeDontAskAgain = WaypointsOutOfRange;
}

static bool
WaypointInTerrainRange(const WAYPOINT &way_point, const RasterTerrain &terrain)
{
  if (WaypointOutOfTerrainRangeDontAskAgain == 1)
    return true;

  if (!terrain.isTerrainLoaded())
    return true;

  if (terrain.WaypointIsInTerrainRange(way_point.Location))
    return true;
  else {
    if (WaypointOutOfTerrainRangeDontAskAgain == 0) {
      TCHAR sTmp[250];
      int res;

      _stprintf(
          sTmp,
          gettext(
              TEXT("Waypoint #%d \"%s\" \r\nout of Terrain bounds\r\n\r\nLoad anyway?")),
          way_point.Number, way_point.Name);

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
    } else {
      if (WaypointOutOfTerrainRangeDontAskAgain == 2)
        return (false);
      if (WaypointOutOfTerrainRangeDontAskAgain == 1)
        return (true);
    }

    return false;
  }
}

static int
ParseWayPointError(int LineNumber, const TCHAR *FileName, const TCHAR *String)
{
  TCHAR szTemp[250];

  if (_tcslen(FileName) > 0) {
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
FeedWayPointLine(WayPointList &way_points, const RasterTerrain *terrain,
    const TCHAR *line)
{
  if (TempString[0] == '\0' || TempString[0] == 0x1a || // dos end of file
      _tcsstr(TempString, TEXT("**")) == TempString || // Look For Comment
      _tcsstr(TempString, TEXT("*")) == TempString) // Look For SeeYou Comment
    /* nothing was parsed, return without error condition */
    return true;

  WAYPOINT *new_waypoint = way_points.append();
  if (new_waypoint == NULL)
    return false; // failed to allocate

  new_waypoint->Details = NULL;

  if (!ParseWayPointString(*new_waypoint, TempString, terrain)) {
    way_points.pop();
    return false;
  }

  if (terrain != NULL && !WaypointInTerrainRange(*new_waypoint, *terrain)) {
    way_points.pop();
    return true;
  }

  return true;
}

static void
ReadWayPointFile(FILE *fp, const TCHAR *CurrentWpFileName,
    WayPointList &way_points, const RasterTerrain *terrain)
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
    WayPointList &way_points, const RasterTerrain *terrain)
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
WaypointAltitudeFromTerrain(WAYPOINT &way_point, const RasterTerrain &terrain)
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
ParseWayPointString(WAYPOINT &way_point, const TCHAR *input,
    const RasterTerrain *terrain)
{
  TCHAR *endptr;
  size_t length;

  way_point.FileNum = globalFileNum;

  way_point.Number = _tcstol(input, &endptr, 10);
  if (endptr == input || *endptr != _T(','))
    return false;

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

  way_point.Flags = ParseFlags(input, &input);
  if (*input != _T(','))
    return false;

  ++input;

  endptr = _tcschr(input, _T(','));
  if (endptr != NULL)
    length = endptr - input;
  else
    length = _tcslen(input);

  if (length >= sizeof(way_point.Name))
    length = sizeof(way_point.Name) - 1;

  while (length > 0 && input[length - 1] == 0)
    --length;

  memcpy(way_point.Name, input, length * sizeof(input[0]));
  way_point.Name[length] = 0;

  if (endptr != NULL) {
    input = endptr + 1;

    endptr = _tcschr(input, '*');
    if (endptr != NULL) {
      length = endptr - input;

      // if it is a home waypoint raise zoom level
      way_point.Zoom = _tcstol(endptr + 2, NULL, 10);
    } else {
      length = _tcslen(input);
      way_point.Zoom = 0;
    }

    if (length >= sizeof(way_point.Comment))
      length = sizeof(way_point.Comment) - 1;

    while (length > 0 && input[length - 1] == 0)
      --length;

    memcpy(way_point.Comment, input, length * sizeof(input[0]));
    way_point.Comment[length] = 0;
  } else {
    way_point.Comment[0] = 0;
    way_point.Zoom = 0;
  }

  if (way_point.Altitude <= 0 && terrain != NULL)
    WaypointAltitudeFromTerrain(way_point, *terrain);

  if (way_point.Details) {
    free(way_point.Details);
  }

  return true;
}

  /*
void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber)
{
  int index = 0;
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength        = 0;

  StringLength = _tcslen(Source);

  while( (CurrentFieldNumber < DesiredFieldNumber) && (index < StringLength) )
    {
      if ( Source[ index ] == ',' )
        {
          CurrentFieldNumber++;
        }
      index++;
    }

  if ( CurrentFieldNumber == DesiredFieldNumber )
    {
      while( (index < StringLength)    &&
             (Source[ index ] != ',') &&
             (Source[ index ] != 0x00) )
        {
          Destination[dest_index] = Source[ index ];
          index++; dest_index++;
        }
      Destination[dest_index] = '\0';
    }
  // strip trailing spaces
  for (int i=dest_index-1; i>0; i--) {
    if (Destination[i]==' ') {
      Destination[i]= '\0';
    } else return;
  }
}
*/

static bool
ParseAngle(const TCHAR *input, double *value_r, TCHAR **endptr_r)
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

static int
ParseFlags(const TCHAR *input, const TCHAR **endptr_r)
{
  int Flags = 0;

  while (_istalpha(*input)) {
    switch (*input++) {
    case 'A':
      Flags |= AIRPORT;
      break;

    case 'T':
      Flags |= TURNPOINT;
      break;

    case 'L':
      Flags |= LANDPOINT;
      break;

    case 'H':
      Flags |= HOME;
      break;

    case 'S':
      Flags |= START;
      break;

    case 'F':
      Flags |= FINISH;
      break;

    case 'R':
      Flags |= RESTRICTED;
      break;

    case 'W':
      Flags |= WAYPOINTFLAG;
      break;
    }
  }

  *endptr_r = input;
  return Flags;
}

static bool
ParseAltitude(const TCHAR *input, double *altitude_r, TCHAR **endptr_r)
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
ReadWayPointZipFile(const TCHAR *path, WayPointList &way_points,
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
ReadWayPointFile(const TCHAR *path, WayPointList &way_points,
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
ReadWayPoints(WayPointList &way_points, const RasterTerrain *terrain)
{
  StartupStore(TEXT("ReadWayPoints\n"));

  TCHAR szFile1[MAX_PATH] = TEXT("\0");
  TCHAR szFile2[MAX_PATH] = TEXT("\0");
  bool file_embedded = false;

  // JMW TODO protect with mutex (whole waypoint list class)

  CloseWayPoints(way_points);

  GetRegistryString(szRegistryWayPointFile, szFile1, MAX_PATH);
  SetRegistryString(szRegistryWayPointFile, TEXT("\0"));

  if (_tcslen(szFile1) > 0) {
    ExpandLocalPath(szFile1);
  } else {
    file_embedded = true;
    GetRegistryString(szRegistryMapFile, szFile1, MAX_PATH);
    ExpandLocalPath(szFile1);
    _tcscat(szFile1, TEXT("/"));
    _tcscat(szFile1, TEXT("waypoints.xcw"));
  }

  globalFileNum = 0;
  if (ReadWayPointFile(szFile1, way_points, terrain)) {
    // read OK, so set the registry to the actual file name
    if (!file_embedded) {
      printf("save\n");
      ContractLocalPath(szFile1);
      SetRegistryString(szRegistryWayPointFile, szFile1);
    }
  } else {
    StartupStore(TEXT("No waypoint file 1\n"));
  }

  // read additional waypoint file

  GetRegistryString(szRegistryAdditionalWayPointFile, szFile2, MAX_PATH);

  SetRegistryString(szRegistryAdditionalWayPointFile, TEXT("\0"));

  if (_tcslen(szFile2) > 0) {
    ExpandLocalPath(szFile2);

    globalFileNum = 1;
    if (ReadWayPointFile(szFile2, way_points, terrain)) {
      // read OK, so set the registry to the actual file name
      ContractLocalPath(szFile2);
      SetRegistryString(szRegistryAdditionalWayPointFile, szFile2);
    } else {
      StartupStore(TEXT("No waypoint file 2\n"));
    }
  }
}

void
SetHome(const WayPointList &way_points, const RasterTerrain *terrain,
    SETTINGS_COMPUTER &settings, const bool reset, const bool set_location)
{
  StartupStore(TEXT("SetHome\n"));

  // check invalid home waypoint or forced reset due to file change
  // VENTA3
  if (reset
      || !way_points.verify_index(0)
      || !way_points.verify_index(settings.HomeWaypoint))
    settings.HomeWaypoint = -1;

  // VENTA3 -- reset Alternates
  if (reset
      || !way_points.verify_index(settings.Alternate1)
      || !way_points.verify_index(settings.Alternate2)) {
    settings.Alternate1 = -1;
    settings.Alternate2 = -1;
  }

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || !way_points.verify_index(settings.TeamCodeRefWaypoint))
    settings.TeamCodeRefWaypoint = -1;

  if (!way_points.verify_index(settings.HomeWaypoint)) {
    // search for home in waypoint list, if we don't have a home
    settings.HomeWaypoint = -1;

    for (unsigned i = 0; way_points.verify_index(i); ++i) {
      if ((way_points.get(i).Flags & HOME) == HOME) {
        if (settings.HomeWaypoint == -1) {
          settings.HomeWaypoint = i;
          break; // only search for one
        }
      }
    }
  }

  // set team code reference waypoint if we don't have one
  if (settings.TeamCodeRefWaypoint == -1)
    settings.TeamCodeRefWaypoint = settings.HomeWaypoint;


  if (set_location) {
    if (way_points.verify_index(settings.HomeWaypoint)) {
      // OK, passed all checks now
      StartupStore(TEXT("Start at home waypoint\n"));
      const WAYPOINT &home = way_points.get(settings.HomeWaypoint);
      device_blackboard.SetStartupLocation(home.Location, home.Altitude);
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

int
FindNearestWayPoint(const WayPointList &way_points,
    MapWindowProjection &map_projection, const GEOPOINT &loc, double MaxRange,
    bool exhaustive)
{
  int NearestIndex = -1;
  double NearestDistance, Dist;

  NearestDistance = MaxRange;
  for (unsigned i = 0; way_points.verify_index(i); ++i) {
    if (way_points.get_calc(i).Visible) {
      if (map_projection.WaypointInScaleFilter(way_points.get(i))) {
        // only look for visible waypoints
        // feature added by Samuel Gisiger
        Dist = Distance(loc, way_points.get(i).Location);
        if (Dist < NearestDistance) {
          NearestIndex = i;
          NearestDistance = Dist;
        }
      }
    }
  }

  // JMW allow exhaustive check for when looking up in status dialog
  if (exhaustive && (NearestIndex == -1)) {
    for (unsigned i = 0; way_points.verify_index(i); ++i) {
      Dist = Distance(loc, way_points.get(i).Location);
      if (Dist < NearestDistance) {
        NearestIndex = i;
        NearestDistance = Dist;
      }
    }
  }

  if (NearestDistance < MaxRange)
    return NearestIndex;
  else
    return -1;
}

// Number,Latitude,Longitude,Altitude,Flags,Name,Comment(,Zoom))
// Number starts at 1
// Lat/long expressed as D:M:S[N/S/E/W]
// Altitude as XXXM
// Flags: T,H,A,L

static void
WaypointFlagsToString(int FlagsNum, TCHAR *Flags)
{
  if ((FlagsNum & AIRPORT) == AIRPORT) {
    _tcscat(Flags, TEXT("A"));
  }
  if ((FlagsNum & TURNPOINT) == TURNPOINT) {
    _tcscat(Flags, TEXT("T"));
  }
  if ((FlagsNum & LANDPOINT) == LANDPOINT) {
    _tcscat(Flags, TEXT("L"));
  }
  if ((FlagsNum & HOME) == HOME) {
    _tcscat(Flags, TEXT("H"));
  }
  if ((FlagsNum & START) == START) {
    _tcscat(Flags, TEXT("S"));
  }
  if ((FlagsNum & FINISH) == FINISH) {
    _tcscat(Flags, TEXT("F"));
  }
  if ((FlagsNum & RESTRICTED) == RESTRICTED) {
    _tcscat(Flags, TEXT("R"));
  }
  if ((FlagsNum & WAYPOINTFLAG) == WAYPOINTFLAG) {
    _tcscat(Flags, TEXT("W"));
  }
  if (_tcslen(Flags) == 0) {
    _tcscat(Flags, TEXT("T"));
  }
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
WriteWayPointFileWayPoint(FILE *fp, const WAYPOINT &way_point)
{
  TCHAR Flags[MAX_PATH];
  TCHAR Latitude[MAX_PATH];
  TCHAR Longitude[MAX_PATH];
  TCHAR Comment[MAX_PATH];

  Flags[0] = 0;

  WaypointLatitudeToString(way_point.Location.Latitude, Latitude);
  WaypointLongitudeToString(way_point.Location.Longitude, Longitude);
  WaypointFlagsToString(way_point.Flags, Flags);

  _stprintf(Comment, way_point.Comment);

  for (int j = 0; j < (int)_tcslen(Comment); j++) {
    if (Comment[j] == _T('\r')) {
      Comment[j] = 0;
    }
    if (Comment[j] == _T('\n')) {
      Comment[j] = 0;
    }
  }

  fprintf(fp, "%d,%S,%S,%dM,%S,%S,%S\r\n", way_point.Number, Latitude,
      Longitude, iround(way_point.Altitude), Flags, way_point.Name,
      way_point.Comment);
}

static void
WriteWayPointFile(WayPointList &way_points, FILE *fp,
    const SETTINGS_COMPUTER &settings_computer)
{
  // remove previous home if it exists in this file
  for (unsigned i = 0; way_points.verify_index(i); ++i) {
    WAYPOINT &way_point = way_points.set(i);

    if (way_point.FileNum == globalFileNum) {
      if ((way_point.Flags & HOME) == HOME) {
        way_point.Flags -= HOME;
      }
    }
  }

  for (unsigned i = 0; way_points.verify_index(i); ++i) {
    WAYPOINT &way_point = way_points.set(i);

    if (way_point.FileNum == globalFileNum) {
      // set home flag if it's the home
      if ((int)i == settings_computer.HomeWaypoint) {
        if ((way_point.Flags & HOME) != HOME) {
          way_point.Flags += HOME;
        }
      }

      WriteWayPointFileWayPoint(fp, way_point);
    }
  }
}

void
WaypointWriteFiles(WayPointList &way_points,
    const SETTINGS_COMPUTER &settings_computer)
{
  // JMW TODO protect with mutex (whole waypoint list class)

  TCHAR szFile1[MAX_PATH] = TEXT("\0");
  TCHAR szFile2[MAX_PATH] = TEXT("\0");

  FILE *fp = NULL;

  GetRegistryString(szRegistryWayPointFile, szFile1, MAX_PATH);
  ExpandLocalPath(szFile1);

  if (_tcslen(szFile1) > 0) {
    fp = _tfopen(szFile1, TEXT("wb"));
  } else {
    LocalPath(szFile1);
    _tcscat(szFile1, TEXT("\\waypoints1.dat"));
    fp = _tfopen(szFile1, TEXT("wb"));
  }

  if (fp != NULL) {
    globalFileNum = 0;
    WriteWayPointFile(way_points, fp, settings_computer);
    fprintf(fp, "\r\n");
    fclose(fp);
    fp = NULL;
  }

  GetRegistryString(szRegistryAdditionalWayPointFile, szFile2, MAX_PATH);
  ExpandLocalPath(szFile2);

  if (_tcslen(szFile2) > 0) {
    fp = _tfopen(szFile2, TEXT("wb"));
  } else {
    LocalPath(szFile2);
    _tcscat(szFile2, TEXT("\\waypoints2.dat"));
    fp = _tfopen(szFile2, TEXT("wb"));
  }

  if (fp != NULL) {
    globalFileNum = 1;
    WriteWayPointFile(way_points, fp, settings_computer);
    fprintf(fp, "\r\n");
    fclose(fp);
    fp = NULL;
  }
}
