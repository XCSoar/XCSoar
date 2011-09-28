/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "AirspaceParser.hpp"
#include "Airspace/Airspaces.hpp"
#include "Operation.hpp"
#include "Units/Units.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"
#include "Math/Earth.hpp"
#include "IO/LineReader.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Engine/Navigation/Geometry/GeoVector.hpp"
#include "Compatibility/string.h"
#include "Engine/Airspace/AirspaceClass.hpp"

#include <math.h>
#include <tchar.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <windef.h> /* for MAX_PATH */

#define fixed_7_5 fixed(7.5)

enum AirspaceFileType {
  AFT_UNKNOWN,
  AFT_OPENAIR,
  AFT_TNP
};

struct AirspaceClassCharCouple
{
  const TCHAR character;
  AirspaceClass type;
};

struct AirspaceClassStringCouple
{
  const TCHAR *string;
  AirspaceClass type;
};

static const AirspaceClassStringCouple airspace_class_strings[] = {
  { _T("R"), RESTRICT },
  { _T("Q"), DANGER },
  { _T("P"), PROHIBITED },
  { _T("CTR"), CTR },
  { _T("A"), CLASSA },
  { _T("B"), CLASSB },
  { _T("C"), CLASSC },
  { _T("D"), CLASSD },
  { _T("GP"), NOGLIDER },
  { _T("W"), WAVE },
  { _T("E"), CLASSE },
  { _T("F"), CLASSF },
  { _T("TMZ"), TMZ },
  { _T("G"), CLASSG },
};

static const AirspaceClassCharCouple airspace_tnp_class_chars[] = {
  { _T('A'), CLASSA },
  { _T('B'), CLASSB },
  { _T('C'), CLASSC },
  { _T('D'), CLASSD },
  { _T('E'), CLASSE },
  { _T('F'), CLASSF },
  { _T('G'), CLASSG },
};

static const AirspaceClassStringCouple airspace_tnp_type_strings[] = {
  { _T("C"), CTR },
  { _T("CTA"), CTR },
  { _T("CTR"), CTR },
  { _T("CTA/CTR"), CTR },
  { _T("CTR/CTA"), CTR },
  { _T("R"), RESTRICT },
  { _T("RESTRICTED"), RESTRICT },
  { _T("P"), PROHIBITED },
  { _T("PROHIBITED"), PROHIBITED },
  { _T("D"), DANGER },
  { _T("DANGER"), DANGER },
  { _T("G"), WAVE },
  { _T("GSEC"), WAVE },
  { _T("T"), TMZ },
  { _T("TMZ"), TMZ },
  { _T("CYR"), RESTRICT },
  { _T("CYD"), DANGER },
  { _T("CYA"), CLASSF },
};

// this can now be called multiple times to load several airspaces.

struct TempAirspaceType
{
  TempAirspaceType() {
    points.reserve(256);
    reset();
  }

  // General
  tstring Name;
  tstring Radio;
  AirspaceClass Type;
  AirspaceAltitude Base;
  AirspaceAltitude Top;
  AirspaceActivity days_of_operation;

  // Polygon
  std::vector<GeoPoint> points;

  // Circle or Arc
  GeoPoint Center;
  fixed Radius;

  // Arc
  int Rotation;

  void
  reset()
  {
    days_of_operation.set_all();
    Radio = _T("");
    Type = OTHER;
    points.clear();
    Center.longitude = Angle::Zero();
    Center.latitude = Angle::Zero();
    Rotation = 1;
    Radius = fixed_zero;
  }

  void
  AddPolygon(Airspaces &airspace_database)
  {
    AbstractAirspace *as = new AirspacePolygon(points);
    as->SetProperties(Name, Type, Base, Top);
    as->SetRadio(Radio);
    as->SetDays(days_of_operation);
    airspace_database.insert(as);
  }

  void
  AddCircle(Airspaces &airspace_database)
  {
    AbstractAirspace *as = new AirspaceCircle(Center, Radius);
    as->SetProperties(Name, Type, Base, Top);
    as->SetRadio(Radio);
    as->SetDays(days_of_operation);
    airspace_database.insert(as);
  }

  void
  AppendArc(const GeoPoint Start, const GeoPoint End)
  {
    // 5 or -5, depending on direction
    const Angle BearingStep = Angle::Degrees(Rotation * fixed(5));

    // Determine start bearing and radius
    const GeoVector v = Center.DistanceBearing(Start);
    Angle StartBearing = v.bearing;
    const fixed Radius = v.distance;

    // Determine end bearing
    Angle EndBearing = Center.Bearing(End);

    // Add first polygon point
    points.push_back(Start);

    // Add intermediate polygon points
    while ((EndBearing - StartBearing).AbsoluteDegrees() > fixed_7_5) {
      StartBearing = (StartBearing + BearingStep).AsBearing();
      points.push_back(FindLatitudeLongitude(Center, StartBearing, Radius));
    }

    // Add last polygon point
    points.push_back(End);
  }

  void
  AppendArc(Angle Start, Angle End)
  {
    // 5 or -5, depending on direction
    const Angle BearingStep = Angle::Degrees(Rotation * fixed(5));

    // Add first polygon point
    points.push_back(FindLatitudeLongitude(Center, Start, Radius));

    // Add intermediate polygon points
    while ((End - Start).AbsoluteDegrees() > fixed_7_5) {
      Start = (Start + BearingStep).AsBearing();
      points.push_back(FindLatitudeLongitude(Center, Start, Radius));
    }

    // Add last polygon point
    points.push_back(FindLatitudeLongitude(Center, End, Radius));
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
ReadAltitude(const TCHAR *Text, AirspaceAltitude &Alt)
{
  Units_t unit = unFeet;
  enum { MSL, AGL, SFC, FL, STD, UNLIMITED } type = MSL;
  fixed altitude = fixed_zero;

  const TCHAR *p = Text;
  while (true) {
    while (*p == _T(' '))
      ++p;

    if (_istdigit(*p)) {
      TCHAR *endptr;
      altitude = fixed(_tcstod(p, &endptr));
      p = endptr;
    } else if (_tcsnicmp(p, _T("GND"), 3) == 0 ||
               _tcsnicmp(p, _T("AGL"), 3) == 0) {
      type = AGL;
      p += 3;
    } else if (_tcsnicmp(p, _T("SFC"), 3) == 0) {
      type = SFC;
      p += 3;
    } else if (_tcsnicmp(p, _T("FL"), 2) == 0) {
      type = FL;
      p += 2;
    } else if (*p == _T('F') || *p == _T('f')) {
      unit = unFeet;
      ++p;

      if (*p == _T('T') || *p == _T('t'))
        ++p;
    } else if (_tcsnicmp(p, _T("MSL"), 3) == 0) {
      type = MSL;
      p += 3;
    } else if (*p == _T('M') || *p == _T('m')) {
      unit = unMeter;
      ++p;
    } else if (_tcsnicmp(p, _T("STD"), 3) == 0) {
      type = STD;
      p += 3;
    } else if (_tcsnicmp(p, _T("UNL"), 3) == 0) {
      type = UNLIMITED;
      p += 3;
    } else if (*p == _T('\0'))
      break;
    else
      ++p;
  }

  if (type == FL) {
    Alt.type = AirspaceAltitude::FL;
    Alt.flight_level = altitude;
    return;
  }

  if (type == UNLIMITED) {
    Alt.type = AirspaceAltitude::MSL;
    Alt.altitude = fixed(50000);
    return;
  }

  if (type == SFC) {
    Alt.type = AirspaceAltitude::AGL;
    Alt.altitude_above_terrain = fixed_minus_one;
    return;
  }

  // For MSL, AGL and STD we convert the altitude to meters
  altitude = Units::ToSysUnit(altitude, unit);
  if (type == MSL) {
    Alt.type = AirspaceAltitude::MSL;
    Alt.altitude = altitude;
    return;
  }

  if (type == AGL) {
    Alt.type = AirspaceAltitude::AGL;
    Alt.altitude_above_terrain = altitude;
    return;
  }

  if (type == STD) {
    Alt.type = AirspaceAltitude::FL;
    Alt.flight_level = Units::ToUserUnit(altitude, unFlightLevel);
    return;
  }
}

static bool
ReadCoords(const TCHAR *Text, GeoPoint &point)
{
  // Format: 53:20:41 N 010:24:41 E
  // Alternative Format: 53:20.68 N 010:24.68 E

  TCHAR *Stop;

  // ToDo, add more error checking and making it more tolerant/robust

  double deg = _tcstod(Text, &Stop);
  if ((Text == Stop) || (*Stop == '\0'))
    return false;

  if (*Stop == ':') {
    Stop++;

    double min = _tcstod(Stop, &Stop);
    if (*Stop == '\0')
      return false;

    deg += min / 60;

    if (*Stop == ':') {
      Stop++;

      double sec = _tcstod(Stop, &Stop);
      if (*Stop == '\0')
        return false;

      deg += sec / 3600;
    }
  }

  point.latitude = Angle::Degrees(fixed(deg));

  if (*Stop == ' ')
    Stop++;

  if (*Stop == '\0')
    return false;

  if ((*Stop == 'S') || (*Stop == 's'))
    point.latitude.Flip();

  Stop++;
  if (*Stop == '\0')
    return false;

  deg = _tcstod(Stop, &Stop);
  if ((Text == Stop) || (*Stop == '\0'))
    return false;

  if (*Stop == ':') {
    Stop++;

    double min = _tcstod(Stop, &Stop);
    if (*Stop == '\0')
      return false;

    deg += min / 60;

    if (*Stop == ':') {
      Stop++;

      double sec = _tcstod(Stop, &Stop);
      if (*Stop == '\0')
        return false;

      deg += sec / 3600;
    }
  }

  point.longitude = Angle::Degrees(fixed(deg));

  if (*Stop == ' ')
    Stop++;

  if (*Stop == '\0')
    return false;

  if ((*Stop == 'W') || (*Stop == 'w'))
    point.longitude.Flip();

  point.Normalize(); // ensure longitude is within -180:180
  return true;
}

static void
ParseArcBearings(const TCHAR *Text, TempAirspaceType &temp_area)
{
  // Determine radius and start/end bearing
  TCHAR *Stop;
  temp_area.Radius = Units::ToSysUnit(fixed(_tcstod(&Text[2], &Stop)), unNauticalMiles);
  Angle StartBearing = Angle::Degrees(fixed(_tcstod(&Stop[1], &Stop))).AsBearing();
  Angle EndBearing = Angle::Degrees(fixed(_tcstod(&Stop[1], &Stop))).AsBearing();

  temp_area.AppendArc(StartBearing, EndBearing);
}

static bool
ParseArcPoints(const TCHAR *Text, TempAirspaceType &temp_area)
{
  // Read start coordinates
  GeoPoint Start;
  if (!ReadCoords(&Text[3], Start))
    return false;

  // Skip comma character
  const TCHAR* Comma = _tcschr(Text, ',');
  if (!Comma)
    return false;

  // Read end coordinates
  GeoPoint End;
  if (!ReadCoords(&Comma[1], End))
    return false;

  temp_area.AppendArc(Start, End);
  return true;
}

static AirspaceClass
ParseType(const TCHAR* text)
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_class_strings); i++)
    if (string_after_prefix(text, airspace_class_strings[i].string))
      return airspace_class_strings[i].type;

  return OTHER;
}

/**
 * Returns the value of the specified line, after a space character
 * which is skipped.  If the input is empty (without a leading space),
 * an empty string is returned, as a special case to work around
 * broken input files.
 *
 * @return the first character of the value, or NULL if the input is
 * malformed
 */
static const TCHAR *
value_after_space(const TCHAR *p)
{
  if (string_is_empty(p))
    return p;

  if (*p != _T(' '))
    /* not a space: must be a malformed line */
    return NULL;

  /* skip the space */
  return p + 1;
}

static const TCHAR *
skip_spaces(const TCHAR *p)
{
  while (*p == _T(' '))
    p++;

  return p;
}

static bool
ParseLine(Airspaces &airspace_database, TCHAR *line,
          TempAirspaceType &temp_area)
{
  const TCHAR *value;

  // Strip comments
  TCHAR *comment = _tcschr(line, _T('*'));
  if (comment != NULL)
    *comment = _T('\0');

  // Only return expected lines
  switch (line[0]) {
  case _T('D'):
  case _T('d'):
    switch (line[1]) {
    case _T('P'):
    case _T('p'):
      value = value_after_space(line + 2);
      if (value == NULL)
        break;

    {
      GeoPoint TempPoint;
      if (!ReadCoords(value, TempPoint))
        return false;

      temp_area.points.push_back(TempPoint);
      break;
    }

    case _T('C'):
    case _T('c'):
      temp_area.Radius = Units::ToSysUnit(fixed(_tcstod(&line[2], NULL)),
                                          unNauticalMiles);
      temp_area.AddCircle(airspace_database);
      temp_area.reset();
      break;

    case _T('A'):
    case _T('a'):
      ParseArcBearings(line, temp_area);
      break;

    case _T('B'):
    case _T('b'):
      return ParseArcPoints(line, temp_area);

    default:
      return true;
    }
    break;

  case _T('V'):
  case _T('v'):
    // Need to set these while in count mode, or DB/DA will crash
    if ((value = string_after_prefix_ci(skip_spaces(line + 1), _T("X="))) != NULL) {
      if (!ReadCoords(value, temp_area.Center))
        return false;
    } else if (string_after_prefix_ci(skip_spaces(line + 1), _T("D=-"))) {
      temp_area.Rotation = -1;
    } else if (string_after_prefix_ci(skip_spaces(line + 1), _T("D=+"))) {
      temp_area.Rotation = +1;
    }
    break;

  case _T('A'):
  case _T('a'):
    switch (line[1]) {
    case _T('C'):
    case _T('c'):
      value = value_after_space(line + 2);
      if (value == NULL)
        break;

      if (!temp_area.points.empty())
        temp_area.AddPolygon(airspace_database);

      temp_area.reset();

      temp_area.Type = ParseType(value);
      break;

    case _T('N'):
    case _T('n'):
      value = value_after_space(line + 2);
      if (value != NULL)
        temp_area.Name = value;
      break;

    case _T('L'):
    case _T('l'):
      value = value_after_space(line + 2);
      if (value != NULL)
        ReadAltitude(value, temp_area.Base);
      break;

    case _T('H'):
    case _T('h'):
      value = value_after_space(line + 2);
      if (value != NULL)
        ReadAltitude(value, temp_area.Top);
      break;

    case _T('R'):
    case _T('r'):
      value = value_after_space(line + 2);
      if (value != NULL)
        temp_area.Radio = value;
      break;

    default:
      return true;
    }

    break;

  }
  return true;
}

static AirspaceClass
ParseClassTNP(const TCHAR* text)
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_tnp_class_chars); i++)
    if (text[0] == airspace_tnp_class_chars[i].character)
      return airspace_tnp_class_chars[i].type;

  return OTHER;
}

static AirspaceClass
ParseTypeTNP(const TCHAR* text)
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_tnp_type_strings); i++)
    if (_tcsicmp(text, airspace_tnp_type_strings[i].string) == 0)
      return airspace_tnp_type_strings[i].type;

  return OTHER;
}

static bool
ParseCoordsTNP(const TCHAR *Text, GeoPoint &point)
{
  // Format: N542500 E0105000
  bool negative = false;
  long deg = 0, min = 0, sec = 0;
  TCHAR *ptr;

  if (Text[0] == _T('S') || Text[0] == _T('s'))
    negative = true;

  sec = _tcstol(&Text[1], &ptr, 10);
  deg = labs(sec / 10000);
  min = labs((sec - deg * 10000) / 100);
  sec = sec - min * 100 - deg * 10000;

  point.latitude = Angle::DMS(fixed(deg), fixed(min), fixed(sec));
  if (negative)
    point.latitude.Flip();

  negative = false;

  if (ptr[0] == _T(' '))
    ptr++;

  if (ptr[0] == _T('W') || ptr[0] == _T('w'))
    negative = true;

  sec = _tcstol(&ptr[1], &ptr, 10);
  deg = labs(sec / 10000);
  min = labs((sec - deg * 10000) / 100);
  sec = sec - min * 100 - deg * 10000;

  point.longitude = Angle::DMS(fixed(deg), fixed(min), fixed(sec));
  if (negative)
    point.longitude.Flip();

  point.Normalize(); // ensure longitude is within -180:180

  return true;
}

static bool
ParseArcTNP(const TCHAR *Text, TempAirspaceType &temp_area)
{
  if (temp_area.points.empty())
    return false;

  // (ANTI-)CLOCKWISE RADIUS=34.95 CENTRE=N523333 E0131603 TO=N522052 E0122236

  GeoPoint from = temp_area.points.back();

  const TCHAR* parameter;
  if ((parameter = _tcsstr(Text, _T(" "))) == NULL)
    return false;
  if ((parameter = string_after_prefix_ci(parameter, _T(" CENTRE="))) == NULL)
    return false;

  if (!ParseCoordsTNP(parameter, temp_area.Center))
    return false;

  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  parameter++;
  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  if ((parameter = string_after_prefix_ci(parameter, _T(" TO="))) == NULL)
    return false;

  GeoPoint to;
  if (!ParseCoordsTNP(parameter, to))
    return false;

  temp_area.AppendArc(from, to);

  return true;
}

static bool
ParseCircleTNP(const TCHAR *Text, TempAirspaceType &temp_area)
{
  // CIRCLE RADIUS=17.00 CENTRE=N533813 E0095943

  const TCHAR* parameter;
  if ((parameter = string_after_prefix_ci(Text, _T("RADIUS="))) == NULL)
    return false;
  temp_area.Radius = Units::ToSysUnit(fixed(_tcstod(parameter, NULL)),
                                      unNauticalMiles);

  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  if ((parameter = string_after_prefix_ci(parameter, _T(" CENTRE="))) == NULL)
    return false;
  ParseCoordsTNP(parameter, temp_area.Center);

  return true;
}

static bool
ParseLineTNP(Airspaces &airspace_database, TCHAR *line,
             TempAirspaceType &temp_area, bool &ignore)
{
  // Strip comments
  TCHAR *comment = _tcschr(line, _T('*'));
  if (comment != NULL)
    *comment = _T('\0');

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

  if ((parameter = string_after_prefix_ci(line, _T("POINT="))) != NULL) {
    GeoPoint TempPoint;
    if (!ParseCoordsTNP(parameter, TempPoint))
      return false;

    temp_area.points.push_back(TempPoint);
  } else if ((parameter =
      string_after_prefix_ci(line, _T("CIRCLE "))) != NULL) {
    if (!ParseCircleTNP(parameter, temp_area))
      return false;

    temp_area.AddCircle(airspace_database);
    temp_area.reset();
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
  } else if ((parameter = string_after_prefix_ci(line, _T("TITLE="))) != NULL) {
    temp_area.Name = parameter;
  } else if ((parameter = string_after_prefix_ci(line, _T("TYPE="))) != NULL) {
    if (!temp_area.points.empty())
      temp_area.AddPolygon(airspace_database);

    temp_area.reset();

    temp_area.Type = ParseTypeTNP(parameter);
  } else if ((parameter = string_after_prefix_ci(line, _T("CLASS="))) != NULL) {
    if (temp_area.Type == OTHER)
      temp_area.Type = ParseClassTNP(parameter);
  } else if ((parameter = string_after_prefix_ci(line, _T("TOPS="))) != NULL) {
    ReadAltitude(parameter, temp_area.Top);
  } else if ((parameter = string_after_prefix_ci(line, _T("BASE="))) != NULL) {
    ReadAltitude(parameter, temp_area.Base);
  } else if ((parameter = string_after_prefix_ci(line, _T("RADIO="))) != NULL) {
    temp_area.Radio = parameter;
  } else if ((parameter = string_after_prefix_ci(line, _T("ACTIVE="))) != NULL) {
    if (_tcsicmp(parameter, _T("WEEKEND")) == 0)
      temp_area.days_of_operation.set_weekend();
    else if (_tcsicmp(parameter, _T("WEEKDAY")) == 0)
      temp_area.days_of_operation.set_weekdays();
    else if (_tcsicmp(parameter, _T("EVERYDAY")) == 0)
      temp_area.days_of_operation.set_all();
  }

  return true;
}

static AirspaceFileType
DetectFileType(const TCHAR *line)
{
  if (string_after_prefix_ci(line, _T("INCLUDE=")) ||
      string_after_prefix_ci(line, _T("TYPE=")) ||
      string_after_prefix_ci(line, _T("TITLE=")))
    return AFT_TNP;

  const TCHAR *p = string_after_prefix_ci(line, _T("AC"));
  if (p != NULL && (string_is_empty(p) || *p == _T(' ')))
    return AFT_OPENAIR;

  return AFT_UNKNOWN;
}

bool
AirspaceParser::Parse(TLineReader &reader, OperationEnvironment &operation)
{
  bool ignore = false;

  // Create and init ProgressDialog
  operation.SetProgressRange(1024);

  long file_size = reader.size();

  TempAirspaceType temp_area;
  AirspaceFileType filetype = AFT_UNKNOWN;

  TCHAR *line;

  // Iterate through the lines
  for (unsigned LineCount = 1; (line = reader.read()) != NULL; LineCount++) {
    // Skip empty line
    if (string_is_empty(line))
      continue;

    if (filetype == AFT_UNKNOWN) {
      filetype = DetectFileType(line);
      if (filetype == AFT_UNKNOWN)
        continue;
    }

    // Parse the line
    if (filetype == AFT_OPENAIR)
      if (!ParseLine(airspaces, line, temp_area) &&
          !ShowParseWarning(LineCount, line))
        return false;

    if (filetype == AFT_TNP)
      if (!ParseLineTNP(airspaces, line, temp_area, ignore) &&
          !ShowParseWarning(LineCount, line))
        return false;

    // Update the ProgressDialog
    if ((LineCount & 0xff) == 0)
      operation.SetProgressPosition(reader.tell() * 1024 / file_size);
  }

  if (filetype == AFT_UNKNOWN) {
    operation.SetErrorMessage(_("Unknown airspace filetype"));
    return false;
  }

  // Process final area (if any)
  if (!temp_area.points.empty())
    temp_area.AddPolygon(airspaces);

  return true;
}
