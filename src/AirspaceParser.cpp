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

#include "Airspace/Airspaces.hpp"
#include "Interface.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "UtilsText.hpp"
#include "Util/StringUtil.hpp"
#include "LogFile.hpp"
#include "Math/Earth.hpp"
#include "IO/LineReader.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Compatibility/string.h"

#include <math.h>
#include <tchar.h>
#include <ctype.h>
#include <assert.h>

static int LineCount;

enum line_type {
  /** special value: end of file */
  ltEOF,
  ltClass,
  ltName,
  ltBase,
  ltTop,
  ltAttribute,
  ltDPoint,
  ltDArc,
  ltDSector,
  ltDCircle,
};

static const int k_nAreaCount = 12;
static const TCHAR* k_strAreaStart[k_nAreaCount] = {
  _T("R"),
  _T("Q"),
  _T("P"),
  _T("CTR"),
  _T("A"),
  _T("B"),
  _T("C"),
  _T("D"),
  _T("GP"),
  _T("W"),
  _T("E"),
  _T("F")
};

static const int k_nAreaType[k_nAreaCount] = {
  RESTRICT,
  DANGER,
  PROHIBITED,
  CTR,
  CLASSA,
  CLASSB,
  CLASSC,
  CLASSD,
  NOGLIDER,
  WAVE,
  CLASSE,
  CLASSF
};

// this can now be called multiple times to load several airspaces.

struct TempAirspaceType
{
  TempAirspaceType() {
    reset();
  }

  bool Waiting;

  // General
  tstring Name;
  AirspaceClass_t Type;
  AIRSPACE_ALT Base;
  AIRSPACE_ALT Top;

  // Polygon
  std::vector<GEOPOINT> points;

  // Circle or Arc
  GEOPOINT Center;
  fixed Radius;

  // Arc
  int Rotation;

  void
  reset()
  {
    Type = OTHER;
    points.clear();
    Center.Longitude = Angle();
    Center.Latitude = Angle();
    Rotation = 1;
    Radius = 0;
    Waiting = true;
  }

  void
  AddPolygon(Airspaces &airspace_database)
  {
    AbstractAirspace *as = new AirspacePolygon(points);
    as->set_properties(Name, Type, Base, Top);
    airspace_database.insert(as);
  }

  void
  AddCircle(Airspaces &airspace_database)
  {
    AbstractAirspace *as = new AirspaceCircle(Center, Radius);
    as->set_properties(Name, Type, Base, Top);
    airspace_database.insert(as);
  }
};

static bool
ShowParseWarning(int line, const TCHAR* str)
{
  TCHAR sTmp[MAX_PATH];
  _stprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
            gettext(TEXT("Parse Error at Line")), line, str,
            gettext(TEXT("Line skipped.")));
  return (MessageBoxX(sTmp, gettext(TEXT("Airspace")), MB_OKCANCEL) == IDOK);

}

// Returns index of line type found, or -1 if end of file reached
static enum line_type
GetNextLine(TLineReader &reader, TCHAR *&Text)
{
  TCHAR *Comment;
  int nSize;
  enum line_type nLineType = ltEOF;

  while ((Text = reader.read()) != NULL) {
    LineCount++;

    // Strip comments and newline chars from end of line
    Comment = _tcschr(Text, _T('*'));
    if (Comment != NULL)
      // Truncate line
      *Comment = _T('\0');

    // Ignore lines less than 3 characters
    nSize = _tcslen(Text);
    if (nSize < 3)
      continue;

    // Only return expected lines
    switch (Text[0]) {
    case _T('A'):
    case _T('a'):
      switch (Text[1]) {
      case _T('C'):
      case _T('c'):
        nLineType = ltClass;
        break;

      case _T('N'):
      case _T('n'):
        nLineType = ltName;
        break;

      case _T('L'):
      case _T('l'):
        nLineType = ltBase;
        break;

      case _T('H'):
      case _T('h'):
        nLineType = ltTop;
        break;

      case _T('T'):
      case _T('t'):
        // ToDo: adding airspace labels
        continue;

      default:
        if (!ShowParseWarning(LineCount, Text))
          return ltEOF;

        continue;
      }

      break;

    case _T('D'):
    case _T('d'):
      switch (Text[1]) {
      case _T('A'):
      case _T('a'):
        nLineType = ltDSector;
        break;

      case _T('B'):
      case _T('b'):
        nLineType = ltDArc;
        break;

      case _T('C'):
      case _T('c'):
        nLineType = ltDCircle;
        break;

      case _T('P'):
      case _T('p'):
        nLineType = ltDPoint;
        break;

        // todo DY airway segment
        // what about 'V T=' ?

      default:
        if (!ShowParseWarning(LineCount, Text))
          return ltEOF;

        continue;
      }

      break;

    case _T('V'):
    case _T('v'):
      nLineType = ltAttribute;
      break;

    case _T('S'):
    case _T('s'):
      // ignore the SB,SP ...
      if (Text[1] == _T('B') || Text[1] == _T('b'))
        continue;
      if (Text[1] == _T('P') || Text[1] == _T('p'))
        continue;

    default:
      if (!ShowParseWarning(LineCount, Text))
        return ltEOF;

      continue;
    }

    if (nLineType != ltEOF)
      break;
  }

  return nLineType;
}

static void
ReadAltitude(const TCHAR *Text_, AIRSPACE_ALT *Alt)
{
  TCHAR *Stop;
  TCHAR Text[128];
  TCHAR *pWClast = NULL;
  const TCHAR *pToken;
  bool fHasUnit = false;

  _tcsncpy(Text, Text_, sizeof(Text) / sizeof(Text[0]));
  Text[sizeof(Text) / sizeof(Text[0]) - 1] = '\0';

  _tcsupr(Text);

  pToken = _tcstok_r(Text, TEXT(" "), &pWClast);

  Alt->Altitude = 0;
  Alt->FL = 0;
  Alt->AGL = 0;
  Alt->Base = abUndef;

  while ((pToken != NULL) && (*pToken != '\0')) {
    if (isdigit(*pToken)) {
      double d = (double)_tcstod(pToken, &Stop);

      if (Alt->Base == abFL)
        Alt->FL = d;
      else if (Alt->Base == abAGL)
        Alt->AGL = d;
      else
        Alt->Altitude = d;

      if (*Stop != '\0') {
        pToken = Stop;
        continue;
      }
    } else if (_tcscmp(pToken, TEXT("GND")) == 0) {
      // JMW support XXXGND as valid, equivalent to XXXAGL
      Alt->Base = abAGL;
      if (Alt->Altitude > fixed_zero) {
        Alt->AGL = Alt->Altitude;
        Alt->Altitude = 0;
      } else {
        Alt->FL = 0;
        Alt->Altitude = 0;
        Alt->AGL = -1;
        fHasUnit = true;
      }
    } else if (_tcscmp(pToken, TEXT("SFC")) == 0) {
      Alt->Base = abAGL;
      Alt->FL = 0;
      Alt->Altitude = 0;
      Alt->AGL = -1;
      fHasUnit = true;
    } else if (_tcsstr(pToken, TEXT("FL")) == pToken) {
      // this parses "FL=150" and "FL150"
      Alt->Base = abFL;
      fHasUnit = true;
      if (pToken[2] != '\0') {// no separator between FL and number
        pToken = &pToken[2];
        continue;
      }
    } else if ((_tcscmp(pToken, TEXT("FT")) == 0) ||
               (_tcscmp(pToken, TEXT("F")) == 0)) {
      Alt->Altitude = Units::ToSysUnit(Alt->Altitude, unFeet);
      fHasUnit = true;
    } else if (_tcscmp(pToken, TEXT("MSL")) == 0) {
      Alt->Base = abMSL;
    } else if (_tcscmp(pToken, TEXT("M")) == 0) {
      // JMW must scan for MSL before scanning for M
      fHasUnit = true;
    } else if (_tcscmp(pToken, TEXT("AGL")) == 0) {
      Alt->Base = abAGL;
      Alt->AGL = Alt->Altitude;
      Alt->Altitude = 0;
    } else if (_tcscmp(pToken, TEXT("STD")) == 0) {
      if (Alt->Base != abUndef) {
        // warning! multiple base tags
      }
      Alt->Base = abFL;
      Alt->FL = Units::ToUserUnit(Alt->Altitude, unFlightLevel);
    } else if (_tcscmp(pToken, TEXT("UNL")) == 0) {
      // JMW added Unlimited (used by WGC2008)
      Alt->Base = abMSL;
      Alt->AGL = -1;
      Alt->Altitude = 50000;
    }

    pToken = _tcstok_r(NULL, TEXT(" \t"), &pWClast);
  }

  if (!fHasUnit && (Alt->Base != abFL)) {
    // ToDo warning! no unit defined use feet or user alt unit
    // Alt->Altitude = Units::ToSysAltitude(Alt->Altitude);
    Alt->Altitude = Units::ToSysUnit(Alt->Altitude, unFeet);
    Alt->AGL = Units::ToSysUnit(Alt->AGL, unFeet);
  }

  if (Alt->Base == abUndef)
    // ToDo warning! no base defined use MSL
    Alt->Base = abMSL;
}

static bool
ReadCoords(const TCHAR *Text, GEOPOINT &point)
{
  // Format: 53:20:41 N 010:24:41 E

  double deg = 0, min = 0, sec = 0;
  TCHAR *Stop;

  // ToDo, add more error checking and making it more tolerant/robust

  deg = (double)_tcstod(Text, &Stop);
  if ((Text == Stop) || (*Stop == '\0'))
    return false;

  Stop++;
  min = (double)_tcstod(Stop, &Stop);
  if (*Stop == '\0')
    return false;

  if (*Stop == ':') {
    Stop++;
    if (*Stop == '\0')
      return false;

    sec = (double)_tcstod(Stop, &Stop);
    if (sec < 0 || sec >= 60) {
      // ToDo
    }
  }

  point.Latitude = Angle::degrees(fixed(sec / 3600 + min / 60 + deg));

  if (*Stop == ' ')
    Stop++;

  if (*Stop == '\0')
    return false;

  if ((*Stop == 'S') || (*Stop == 's'))
    point.Latitude.flip();

  Stop++;
  if (*Stop == '\0')
    return false;

  deg = (double)_tcstod(Stop, &Stop);
  Stop++;
  min = (double)_tcstod(Stop, &Stop);
  if (*Stop == ':') {
    Stop++;
    if (*Stop == '\0')
      return false;

    sec = (double)_tcstod(Stop, &Stop);
  }

  point.Longitude = Angle::degrees(fixed(sec / 3600 + min / 60 + deg));

  if (*Stop == ' ')
    Stop++;

  if (*Stop == '\0')
    return false;

  if ((*Stop == 'W') || (*Stop == 'w'))
    point.Longitude.flip();
  point.Longitude = point.Longitude.as_bearing();
  return true;
}

static void
CalculateSector(const TCHAR *Text, TempAirspaceType &temp_area)
{
  fixed Radius;
  Angle StartBearing;
  Angle EndBearing;
  TCHAR *Stop;
  GEOPOINT TempPoint;
  static const fixed fixed_75 = fixed(7.5);
  static const fixed fixed_5 = fixed(5);

  Radius = Units::ToSysUnit(_tcstod(&Text[2], &Stop), unNauticalMiles);
  StartBearing = Angle::degrees(fixed(_tcstod(&Stop[1], &Stop)));
  EndBearing = Angle::degrees(fixed(_tcstod(&Stop[1], &Stop)));

  if (EndBearing < StartBearing)
    EndBearing += Angle::degrees(fixed_360);

  while ((EndBearing - StartBearing).magnitude_degrees() > fixed_75) {
    StartBearing = StartBearing.as_bearing();
    FindLatitudeLongitude(temp_area.Center, StartBearing, Radius, &TempPoint);
    temp_area.points.push_back(TempPoint);
    StartBearing += Angle::degrees(temp_area.Rotation * fixed_5);
  }

  FindLatitudeLongitude(temp_area.Center, EndBearing, Radius, &TempPoint);
  temp_area.points.push_back(TempPoint);
}

static void
CalculateArc(const TCHAR *Text, TempAirspaceType &temp_area)
{
  GEOPOINT Start;
  GEOPOINT End;
  Angle StartBearing;
  Angle EndBearing;
  fixed Radius;
  const TCHAR *Comma = NULL;
  GEOPOINT TempPoint;
  static const fixed fixed_75 = fixed(7.5);
  static const fixed fixed_5 = fixed(5);

  ReadCoords(&Text[3], Start);

  Comma = _tcschr(Text, ',');
  if (!Comma)
    return;

  ReadCoords(&Comma[1], End);

  DistanceBearing(temp_area.Center, Start, &Radius, &StartBearing);
  EndBearing = Bearing(temp_area.Center, End);
  TempPoint.Latitude = Start.Latitude;
  TempPoint.Longitude = Start.Longitude;
  temp_area.points.push_back(TempPoint);

  while ((EndBearing - StartBearing).magnitude_degrees() > fixed_75) {
    StartBearing += Angle::degrees(temp_area.Rotation * fixed_5);
    StartBearing = StartBearing.as_bearing();
    FindLatitudeLongitude(temp_area.Center, StartBearing, Radius, &TempPoint);
    temp_area.points.push_back(TempPoint);
  }

  TempPoint = End;
  temp_area.points.push_back(TempPoint);
}

static AirspaceClass_t
ParseType(const TCHAR* text)
{
  for (int i = 0; i < k_nAreaCount; i++)
    if (string_after_prefix(text, k_strAreaStart[i]))
      return (AirspaceClass_t)k_nAreaType[i];

  return OTHER;
}

static bool
ParseLine(Airspaces &airspace_database, enum line_type nLineType,
          const TCHAR *TempString, TempAirspaceType &temp_area)
{
  switch (nLineType) {
  case ltClass:
    if (!temp_area.Waiting)
      temp_area.AddPolygon(airspace_database);

    temp_area.reset();
    
    temp_area.Type = ParseType(&TempString[3]);
    temp_area.Waiting = false;
    break;

  case ltName:
    temp_area.Name = &TempString[3];
    break;

  case ltBase:
    ReadAltitude(&TempString[3], &temp_area.Base);
    break;

  case ltTop:
    ReadAltitude(&TempString[3],&temp_area.Top);
    break;

  case ltAttribute:
    // Need to set these while in count mode, or DB/DA will crash
    if (string_after_prefix_ci(&TempString[2], _T("X="))) {
      if (ReadCoords(&TempString[4],temp_area.Center))
        break;
    } else if (string_after_prefix_ci(&TempString[2], _T("D=-"))) {
      temp_area.Rotation = -1;
      break;
    } else if (string_after_prefix_ci(&TempString[2], _T("D=+"))) {
      temp_area.Rotation = +1;
      break;
    } else if (string_after_prefix_ci(&TempString[2], _T("Z"))) {
      // ToDo Display Zool Level
      break;
    } else if (string_after_prefix_ci(&TempString[2], _T("W"))) {
      // ToDo width of an airway
      break;
    } else if (string_after_prefix_ci(&TempString[2], _T("T"))) {
      // ----- JMW THIS IS REQUIRED FOR LEGACY FILES
      break;
    }
    return ShowParseWarning(LineCount, TempString);

  case ltDPoint:
  {
    GEOPOINT TempPoint;

    if (!ReadCoords(&TempString[3],TempPoint))
      return ShowParseWarning(LineCount, TempString);

    temp_area.points.push_back(TempPoint);
    break;
  }
  case ltDArc:
    CalculateArc(TempString, temp_area);
    break;

  case ltDSector:
    CalculateSector(TempString, temp_area);
    break;

  case ltDCircle:
    temp_area.Radius = Units::ToSysUnit(_tcstod(&TempString[2], NULL),
        unNauticalMiles);
    temp_area.AddCircle(airspace_database);
    temp_area.reset();
    break;
  }

  return true;
}

bool
ReadAirspace(Airspaces &airspace_database, TLineReader &reader)
{
  LogStartUp(TEXT("ReadAirspace"));
  enum line_type nLineType;

  LineCount = 0;

  XCSoarInterface::CreateProgressDialog(gettext(
      TEXT("Loading Airspace File...")));
  XCSoarInterface::SetProgressDialogMaxValue(1024);
  long file_size = reader.size();

  TempAirspaceType temp_area;

  TCHAR *line;
  while ((nLineType = GetNextLine(reader, line)) != ltEOF) {
    if (!ParseLine(airspace_database, nLineType, line, temp_area))
      return false;

    XCSoarInterface::SetProgressDialogValue(reader.tell() * 1024 / file_size);
  }

  // Process final area (if any). bFillMode is true.  JG 10-Nov-2005
  if (!temp_area.Waiting)
    temp_area.AddPolygon(airspace_database);

  return true;
}
