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
#include "ProgressGlue.hpp"
#include "Units.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
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

enum asFileType {
  ftUnknown,
  ftOpenAir,
  ftTNP
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
  _stprintf(sTmp, _T("%s: %d\r\n\"%s\"\r\n%s."),
            _("Parse Error at Line"), line, str,
            _("Line skipped."));
  return (MessageBoxX(sTmp, _("Airspace"), MB_OKCANCEL) == IDOK);

}

static void
ReadAltitude(const TCHAR *Text_, AIRSPACE_ALT *Alt)
{
  TCHAR Text[128];
  bool fHasUnit = false;

  _tcsncpy(Text, Text_, sizeof(Text) / sizeof(Text[0]));
  Text[sizeof(Text) / sizeof(Text[0]) - 1] = '\0';

  _tcsupr(Text);

  Alt->Altitude = 0;
  Alt->FL = 0;
  Alt->AGL = 0;
  Alt->Base = abUndef;

  const TCHAR *p = Text;
  while (true) {
    while (*p == _T(' '))
      ++p;

    if (_istdigit(*p)) {
      TCHAR *endptr;
      double d = _tcstod(p, &endptr);

      if (Alt->Base == abFL)
        Alt->FL = d;
      else if (Alt->Base == abAGL)
        Alt->AGL = d;
      else
        Alt->Altitude = d;

      p = endptr;
    } else if (_tcsncmp(p, _T("GND"), 3) == 0) {
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

      p += 3;
    } else if (_tcsncmp(p, _T("SFC"), 3) == 0) {
      Alt->Base = abAGL;
      Alt->FL = 0;
      Alt->Altitude = 0;
      Alt->AGL = -1;
      fHasUnit = true;

      p += 3;
    } else if (_tcsncmp(p, _T("FL"), 2) == 0) {
      // this parses "FL=150" and "FL150"
      Alt->Base = abFL;
      fHasUnit = true;

      p += 2;
    } else if (*p == _T('F')) {
      Alt->Altitude = Units::ToSysUnit(Alt->Altitude, unFeet);
      fHasUnit = true;

      ++p;
      if (*p == _T('T'))
        ++p;
    } else if (_tcsncmp(p, _T("MSL"), 3) == 0) {
      Alt->Base = abMSL;

      p += 3;
    } else if (*p == _T('M')) {
      // JMW must scan for MSL before scanning for M
      fHasUnit = true;

      ++p;
    } else if (_tcsncmp(p, _T("AGL"), 3) == 0) {
      Alt->Base = abAGL;
      Alt->AGL = Alt->Altitude;
      Alt->Altitude = 0;

      p += 3;
    } else if (_tcsncmp(p, _T("STD"), 3) == 0) {
      if (Alt->Base != abUndef) {
        // warning! multiple base tags
      }
      Alt->Base = abFL;
      Alt->FL = Units::ToUserUnit(Alt->Altitude, unFlightLevel);

      p += 3;
    } else if (_tcsncmp(p, _T("UNL"), 3) == 0) {
      // JMW added Unlimited (used by WGC2008)
      Alt->Base = abMSL;
      Alt->AGL = -1;
      Alt->Altitude = 50000;

      p += 3;
    } else if (*p == _T('\0'))
      break;
    else
      ++p;
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

  point.Latitude = Angle::dms(fixed(deg), fixed(min), fixed(sec));

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

  point.Longitude = Angle::dms(fixed(deg), fixed(min), fixed(sec));

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
  const Angle BearingStep = Angle::degrees(temp_area.Rotation * fixed(5));

  Radius = Units::ToSysUnit(_tcstod(&Text[2], &Stop), unNauticalMiles);
  StartBearing = Angle::degrees(fixed(_tcstod(&Stop[1], &Stop)));
  EndBearing = Angle::degrees(fixed(_tcstod(&Stop[1], &Stop)));

  if (EndBearing < StartBearing)
    EndBearing += Angle::degrees(fixed_360);

  while ((EndBearing - StartBearing).magnitude_degrees() > fixed_75) {
    StartBearing = StartBearing.as_bearing();
    FindLatitudeLongitude(temp_area.Center, StartBearing, Radius, &TempPoint);
    temp_area.points.push_back(TempPoint);
    StartBearing += BearingStep;
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
  const Angle BearingStep = Angle::degrees(temp_area.Rotation * fixed(5));

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
    StartBearing += BearingStep;
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
ParseLine(Airspaces &airspace_database, const TCHAR *line,
          TempAirspaceType &temp_area)
{
  // Ignore lines less than 3 characters
  int nSize = _tcslen(line);
  if (nSize < 3)
    return true;

  // Only return expected lines
  switch (line[0]) {
  case _T('A'):
  case _T('a'):
    switch (line[1]) {
    case _T('C'):
    case _T('c'):
      if (!temp_area.Waiting)
        temp_area.AddPolygon(airspace_database);

      temp_area.reset();

      temp_area.Type = ParseType(&line[3]);
      temp_area.Waiting = false;
      break;

    case _T('N'):
    case _T('n'):
      temp_area.Name = &line[3];
      break;

    case _T('L'):
    case _T('l'):
      ReadAltitude(&line[3], &temp_area.Base);
      break;

    case _T('H'):
    case _T('h'):
      ReadAltitude(&line[3],&temp_area.Top);
      break;

    default:
      return true;
    }

    break;

  case _T('D'):
  case _T('d'):
    switch (line[1]) {
    case _T('A'):
    case _T('a'):
      CalculateSector(line, temp_area);
      break;

    case _T('B'):
    case _T('b'):
      CalculateArc(line, temp_area);
      break;

    case _T('C'):
    case _T('c'):
      temp_area.Radius = Units::ToSysUnit(_tcstod(&line[2], NULL),
                                          unNauticalMiles);
      temp_area.AddCircle(airspace_database);
      temp_area.reset();
      break;

    case _T('P'):
    case _T('p'):
    {
      GEOPOINT TempPoint;

      if (!ReadCoords(&line[3],TempPoint))
        return false;

      temp_area.points.push_back(TempPoint);
      break;
    }
    default:
      return true;
    }

    break;

  case _T('V'):
  case _T('v'):
    // Need to set these while in count mode, or DB/DA will crash
    if (string_after_prefix_ci(&line[2], _T("X="))) {
      if (!ReadCoords(&line[4],temp_area.Center))
        return false;
    } else if (string_after_prefix_ci(&line[2], _T("D=-"))) {
      temp_area.Rotation = -1;
    } else if (string_after_prefix_ci(&line[2], _T("D=+"))) {
      temp_area.Rotation = +1;
    }
  }

  return true;
}

static AirspaceClass_t
ParseClassTNP(const TCHAR* text)
{
  if (text[0] == _T('A'))
    return CLASSA;

  if (text[0] == _T('B'))
    return CLASSB;

  if (text[0] == _T('C'))
    return CLASSC;

  if (text[0] == _T('D'))
    return CLASSD;

  if (text[0] == _T('E'))
    return CLASSE;

  if (text[0] == _T('F'))
    return CLASSF;

  return OTHER;
}

static AirspaceClass_t
ParseTypeTNP(const TCHAR* text)
{
  if (_tcsicmp(text, _T("C")) == 0 ||
      _tcsicmp(text, _T("CTA")) == 0 ||
      _tcsicmp(text, _T("CTA")) == 0 ||
      _tcsicmp(text, _T("CTA/CTR")) == 0)
    return CTR;

  if (_tcsicmp(text, _T("R")) == 0 ||
      _tcsicmp(text, _T("RESTRICTED")) == 0)
    return RESTRICT;

  if (_tcsicmp(text, _T("P")) == 0 ||
      _tcsicmp(text, _T("PROHIBITED")) == 0)
    return RESTRICT;

  if (_tcsicmp(text, _T("D")) == 0 ||
      _tcsicmp(text, _T("DANGER")) == 0)
    return RESTRICT;

  if (_tcsicmp(text, _T("G")) == 0 ||
      _tcsicmp(text, _T("GSEC")) == 0)
    return WAVE;

  return OTHER;
}

static bool
ParseCoordsTNP(const TCHAR *Text, GEOPOINT &point)
{
  // Format: N542500 E0105000
  bool negative = false;
  double deg = 0, min = 0, sec = 0;
  TCHAR *ptr;

  if (Text[0] == _T('S') || Text[0] == _T('s'))
    negative = true;

  sec = (double)_tcstod(&Text[1], &ptr);
  deg = abs(sec / 10000);
  min = abs((sec - deg * 10000) / 100);
  sec = sec - min * 100 - deg * 10000;

  point.Latitude = Angle::dms(fixed(deg), fixed(min), fixed(sec));
  if (negative)
    point.Latitude.flip();

  negative = false;

  if (ptr[0] == _T(' '))
    ptr++;

  if (ptr[0] == _T('W') || ptr[0] == _T('w'))
    negative = true;

  sec = (double)_tcstod(&ptr[1], &ptr);
  deg = abs(sec / 10000);
  min = abs((sec - deg * 10000) / 100);
  sec = sec - min * 100 - deg * 10000;

  point.Longitude = Angle::dms(fixed(deg), fixed(min), fixed(sec));
  if (negative)
    point.Longitude.flip();

  point.Longitude = point.Longitude.as_bearing();

  return true;
}

static bool
ParseArcTNP(const TCHAR *Text, TempAirspaceType &temp_area)
{
  if (temp_area.points.empty())
    return false;

  // (ANTI-)CLOCKWISE RADIUS=34.95 CENTRE=N523333 E0131603 TO=N522052 E0122236

  GEOPOINT from = temp_area.points.back();

  const TCHAR* parameter;
  if ((parameter = _tcsstr(Text, _T(" "))) == NULL)
    return false;
  if ((parameter = string_after_prefix_ci(parameter, _T(" CENTRE="))) == NULL)
    return false;
  ParseCoordsTNP(parameter, temp_area.Center);

  GEOPOINT to;
  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  parameter++;
  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  if ((parameter = string_after_prefix_ci(parameter, _T(" TO="))) == NULL)
    return false;
  ParseCoordsTNP(parameter, to);

  Angle bearing_from;
  Angle bearing_to;
  fixed radius;

  static const fixed fixed_75 = fixed(7.5);
  const Angle BearingStep = Angle::degrees(temp_area.Rotation * fixed(5));

  DistanceBearing(temp_area.Center, from, &radius, &bearing_from);
  bearing_to = Bearing(temp_area.Center, to);

  GEOPOINT TempPoint;
  while ((bearing_to - bearing_from).magnitude_degrees() > fixed_75) {
    bearing_from += BearingStep;
    bearing_from = bearing_from.as_bearing();
    FindLatitudeLongitude(temp_area.Center, bearing_from, radius, &TempPoint);
    temp_area.points.push_back(TempPoint);
  }

  return true;
}

static bool
ParseCircleTNP(const TCHAR *Text, TempAirspaceType &temp_area)
{
  // CIRCLE RADIUS=17.00 CENTRE=N533813 E0095943

  const TCHAR* parameter;
  if ((parameter = string_after_prefix_ci(Text, _T("RADIUS="))) == NULL)
    return false;
  temp_area.Radius = Units::ToSysUnit(_tcstod(parameter, NULL),
                                      unNauticalMiles);

  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  if ((parameter = string_after_prefix_ci(parameter, _T(" CENTRE="))) == NULL)
    return false;
  ParseCoordsTNP(parameter, temp_area.Center);

  return true;
}

static bool
ParseLineTNP(Airspaces &airspace_database, const TCHAR *line,
             TempAirspaceType &temp_area, bool &ignore)
{
  const TCHAR* parameter;
  if ((parameter = string_after_prefix_ci(line, _T("INCLUDE="))) != NULL) {
    if (_tcsicmp(parameter, _T("YES")) == 0)
      ignore = false;
    else if (_tcsicmp(parameter, _T("NO")) == 0)
      ignore = true;

    return true;
  }

  if (ignore)
    return true;

  if ((parameter = string_after_prefix_ci(line, _T("TITLE="))) != NULL) {
    temp_area.Name = parameter;
  } else if ((parameter = string_after_prefix_ci(line, _T("TYPE="))) != NULL) {
    if (!temp_area.Waiting)
      temp_area.AddPolygon(airspace_database);

    temp_area.reset();

    temp_area.Type = ParseTypeTNP(parameter);
    temp_area.Waiting = false;
  } else if ((parameter = string_after_prefix_ci(line, _T("CLASS="))) != NULL) {
    if (temp_area.Type == OTHER)
      temp_area.Type = ParseClassTNP(parameter);
  } else if ((parameter = string_after_prefix_ci(line, _T("TOPS="))) != NULL) {
    ReadAltitude(parameter, &temp_area.Top);
  } else if ((parameter = string_after_prefix_ci(line, _T("BASE="))) != NULL) {
    ReadAltitude(parameter, &temp_area.Base);
  } else if ((parameter = string_after_prefix_ci(line, _T("POINT="))) != NULL) {
    GEOPOINT TempPoint;

    if (!ParseCoordsTNP(parameter, TempPoint))
      return false;

    temp_area.points.push_back(TempPoint);
  } else if ((parameter =
      string_after_prefix_ci(line, _T("CIRCLE "))) != NULL) {
    if (!ParseCircleTNP(parameter, temp_area))
      return false;

    temp_area.AddCircle(airspace_database);
  } else if ((parameter =
      string_after_prefix_ci(line, _T("CLOCKWISE "))) != NULL) {
    temp_area.Rotation = 1;
    if (!ParseArcTNP(parameter, temp_area))
      return false;
  } else if ((parameter =
      string_after_prefix_ci(line, _T("ANTI-CLOCKWISE "))) != NULL) {
    temp_area.Rotation = -1;
    if (!ParseArcTNP(parameter, temp_area))
      return false;
  }

  return true;
}

static asFileType
DetectFileType(const TCHAR *line)
{
  if (string_after_prefix_ci(line, _T("INCLUDE=")) ||
      string_after_prefix_ci(line, _T("TYPE=")))
    return ftTNP;

  if (string_after_prefix_ci(line, _T("AC ")))
    return ftOpenAir;

  return ftUnknown;
}

bool
ReadAirspace(Airspaces &airspace_database, TLineReader &reader)
{
  LogStartUp(_T("ReadAirspace"));

  int LineCount = 0;
  bool ignore = false;

  // Create and init ProgressDialog
  ProgressGlue::Create(_("Loading Airspace File..."));
  ProgressGlue::SetRange(1024);

  long file_size = reader.size();

  TempAirspaceType temp_area;
  asFileType filetype = ftUnknown;

  TCHAR *line;
  TCHAR *comment;
  // Iterate through the lines
  while ((line = reader.read()) != NULL) {
    // Increase line counter
    LineCount++;

    // Strip comments
    comment = _tcschr(line, _T('*'));
    if (comment != NULL)
      *comment = _T('\0');

    // Skip empty line
    unsigned linelength = _tcslen(line);
    if (linelength < 1)
      continue;

    if (filetype == ftUnknown) {
      filetype = DetectFileType(line);
      if (filetype == ftUnknown)
        continue;
    }

    // Parse the line
    if (filetype == ftOpenAir)
      if (!ParseLine(airspace_database, line, temp_area) &&
          !ShowParseWarning(LineCount, line))
        return false;

    if (filetype == ftTNP)
      if (!ParseLineTNP(airspace_database, line, temp_area, ignore) &&
          !ShowParseWarning(LineCount, line))
        return false;

    // Update the ProgressDialog
    ProgressGlue::SetValue(reader.tell() * 1024 / file_size);
  }

  if (filetype == ftUnknown) {
    MessageBoxX(_T("Unknown Filetype."), _("Airspace"), MB_OK);
    return false;
  }

  // Process final area (if any)
  if (!temp_area.Waiting)
    temp_area.AddPolygon(airspace_database);

  return true;
}
