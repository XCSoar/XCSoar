/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Operation/Operation.hpp"
#include "Units/System.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Util/CharUtil.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"
#include "Geo/Math.hpp"
#include "IO/LineReader.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Geo/GeoVector.hpp"
#include "Compatibility/string.h"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Util/StaticString.hpp"

#include <math.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <windef.h> /* for MAX_PATH */

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
    Reset();
  }

  // General
  tstring name;
  tstring radio;
  AirspaceClass type;
  AirspaceAltitude base;
  AirspaceAltitude top;
  AirspaceActivity days_of_operation;

  // Polygon
  std::vector<GeoPoint> points;

  // Circle or Arc
  GeoPoint center;
  fixed radius;

  // Arc
  int rotation;

  void
  Reset()
  {
    days_of_operation.SetAll();
    radio = _T("");
    type = OTHER;
    points.clear();
    center.longitude = Angle::Zero();
    center.latitude = Angle::Zero();
    rotation = 1;
    radius = fixed_zero;
  }

  void
  ResetTNP()
  {
    // Preserve type, radio and days_of_operation for next airspace blocks
    points.clear();
    center.longitude = Angle::Zero();
    center.latitude = Angle::Zero();
    rotation = 1;
    radius = fixed_zero;
  }

  void
  AddPolygon(Airspaces &airspace_database)
  {
    AbstractAirspace *as = new AirspacePolygon(points);
    as->SetProperties(name, type, base, top);
    as->SetRadio(radio);
    as->SetDays(days_of_operation);
    airspace_database.Add(as);
  }

  void
  AddCircle(Airspaces &airspace_database)
  {
    AbstractAirspace *as = new AirspaceCircle(center, radius);
    as->SetProperties(name, type, base, top);
    as->SetRadio(radio);
    as->SetDays(days_of_operation);
    airspace_database.Add(as);
  }

  static fixed
  ArcStepWidth(fixed radius)
  {
    if (radius > fixed_int_constant(50000))
      return fixed_int_constant(1);
    if (radius > fixed_int_constant(25000))
      return fixed_int_constant(2);
    if (radius > fixed_int_constant(10000))
      return fixed_int_constant(3);

    return fixed_int_constant(5);
  }

  void
  AppendArc(const GeoPoint start, const GeoPoint end)
  {

    // Determine start bearing and radius
    const GeoVector v = center.DistanceBearing(start);
    Angle start_bearing = v.bearing;
    const fixed radius = v.distance;

    // 5 or -5, depending on direction
    const fixed _step = ArcStepWidth(radius);
    const Angle step = Angle::Degrees(rotation * _step);
    const fixed threshold = _step * fixed(1.5);

    // Determine end bearing
    Angle end_bearing = center.Bearing(end);

    if (rotation > 0) {
      while (end_bearing < start_bearing)
        end_bearing += Angle::FullCircle();
    } else if (rotation < 0) {
      while (end_bearing > start_bearing)
        end_bearing -= Angle::FullCircle();
    }

    // Add first polygon point
    points.push_back(start);

    // Add intermediate polygon points
    while ((end_bearing - start_bearing).AbsoluteDegrees() > threshold) {
      start_bearing += step;
      points.push_back(FindLatitudeLongitude(center, start_bearing, radius));
    }

    // Add last polygon point
    points.push_back(end);
  }

  void
  AppendArc(Angle start, Angle end)
  {
    // 5 or -5, depending on direction
    const fixed _step = ArcStepWidth(radius);
    const Angle step = Angle::Degrees(rotation * _step);
    const fixed threshold = _step * fixed(1.5);

    if (rotation > 0) {
      while (end < start)
        end += Angle::FullCircle();
    } else if (rotation < 0) {
      while (end > start)
        end -= Angle::FullCircle();
    }

    // Add first polygon point
    points.push_back(FindLatitudeLongitude(center, start, radius));

    // Add intermediate polygon points
    while ((end - start).AbsoluteDegrees() > threshold) {
      start += step;
      points.push_back(FindLatitudeLongitude(center, start, radius));
    }

    // Add last polygon point
    points.push_back(FindLatitudeLongitude(center, end, radius));
  }
};

static bool
ShowParseWarning(int line, const TCHAR* str)
{
  StaticString<256> buffer;
  buffer.Format(_T("%s: %d\r\n\"%s\"\r\n%s."),
                _("Parse Error at Line"), line, str, _("Line skipped."));
  return (ShowMessageBox(buffer, _("Airspace"), MB_OKCANCEL) == IDOK);

}

static void
ReadAltitude(const TCHAR *buffer, AirspaceAltitude &altitude)
{
  Unit unit = Unit::FEET;
  enum { MSL, AGL, SFC, FL, STD, UNLIMITED } type = MSL;
  fixed value = fixed_zero;

  const TCHAR *p = buffer;
  while (true) {
    while (*p == _T(' '))
      ++p;

    if (IsDigitASCII(*p)) {
      TCHAR *endptr;
      value = fixed(_tcstod(p, &endptr));
      p = endptr;
    } else if (StringIsEqualIgnoreCase(p, _T("GND"), 3) ||
               StringIsEqualIgnoreCase(p, _T("AGL"), 3)) {
      type = AGL;
      p += 3;
    } else if (StringIsEqualIgnoreCase(p, _T("SFC"), 3)) {
      type = SFC;
      p += 3;
    } else if (StringIsEqualIgnoreCase(p, _T("FL"), 2)) {
      type = FL;
      p += 2;
    } else if (*p == _T('F') || *p == _T('f')) {
      unit = Unit::FEET;
      ++p;

      if (*p == _T('T') || *p == _T('t'))
        ++p;
    } else if (StringIsEqualIgnoreCase(p, _T("MSL"), 3)) {
      type = MSL;
      p += 3;
    } else if (*p == _T('M') || *p == _T('m')) {
      unit = Unit::METER;
      ++p;
    } else if (StringIsEqualIgnoreCase(p, _T("STD"), 3)) {
      type = STD;
      p += 3;
    } else if (StringIsEqualIgnoreCase(p, _T("UNL"), 3)) {
      type = UNLIMITED;
      p += 3;
    } else if (*p == _T('\0'))
      break;
    else
      ++p;
  }

  switch (type) {
  case FL:
    altitude.type = AirspaceAltitude::Type::FL;
    altitude.flight_level = value;
    return;

  case UNLIMITED:
    altitude.type = AirspaceAltitude::Type::MSL;
    altitude.altitude = fixed(50000);
    return;

  case SFC:
    altitude.type = AirspaceAltitude::Type::AGL;
    altitude.altitude_above_terrain = fixed_minus_one;
    return;

  default:
    break;
  }

  // For MSL, AGL and STD we convert the altitude to meters
  value = Units::ToSysUnit(value, unit);
  switch (type) {
  case MSL:
    altitude.type = AirspaceAltitude::Type::MSL;
    altitude.altitude = value;
    return;

  case AGL:
    altitude.type = AirspaceAltitude::Type::AGL;
    altitude.altitude_above_terrain = value;
    return;

  case STD:
    altitude.type = AirspaceAltitude::Type::FL;
    altitude.flight_level = Units::ToUserUnit(value, Unit::FLIGHT_LEVEL);
    return;

  default:
    break;
  }
}

static bool
ReadCoords(const TCHAR *buffer, GeoPoint &point)
{
  // Format: 53:20:41 N 010:24:41 E
  // Alternative Format: 53:20.68 N 010:24.68 E

  TCHAR *endptr;

  // ToDo, add more error checking and making it more tolerant/robust

  double deg = _tcstod(buffer, &endptr);
  if ((buffer == endptr) || (*endptr == '\0'))
    return false;

  if (*endptr == ':') {
    endptr++;

    double min = _tcstod(endptr, &endptr);
    if (*endptr == '\0')
      return false;

    deg += min / 60;

    if (*endptr == ':') {
      endptr++;

      double sec = _tcstod(endptr, &endptr);
      if (*endptr == '\0')
        return false;

      deg += sec / 3600;
    }
  }

  point.latitude = Angle::Degrees(fixed(deg));

  if (*endptr == ' ')
    endptr++;

  if (*endptr == '\0')
    return false;

  if ((*endptr == 'S') || (*endptr == 's'))
    point.latitude.Flip();

  endptr++;
  if (*endptr == '\0')
    return false;

  deg = _tcstod(endptr, &endptr);
  if ((buffer == endptr) || (*endptr == '\0'))
    return false;

  if (*endptr == ':') {
    endptr++;

    double min = _tcstod(endptr, &endptr);
    if (*endptr == '\0')
      return false;

    deg += min / 60;

    if (*endptr == ':') {
      endptr++;

      double sec = _tcstod(endptr, &endptr);
      if (*endptr == '\0')
        return false;

      deg += sec / 3600;
    }
  }

  point.longitude = Angle::Degrees(fixed(deg));

  if (*endptr == ' ')
    endptr++;

  if (*endptr == '\0')
    return false;

  if ((*endptr == 'W') || (*endptr == 'w'))
    point.longitude.Flip();

  point.Normalize(); // ensure longitude is within -180:180
  return true;
}

static void
ParseArcBearings(const TCHAR *buffer, TempAirspaceType &temp_area)
{
  // Determine radius and start/end bearing
  TCHAR *endptr;
  temp_area.radius = Units::ToSysUnit(fixed(_tcstod(&buffer[2], &endptr)), Unit::NAUTICAL_MILES);
  Angle start_bearing = Angle::Degrees(fixed(_tcstod(&endptr[1], &endptr))).AsBearing();
  Angle end_bearing = Angle::Degrees(fixed(_tcstod(&endptr[1], &endptr))).AsBearing();

  temp_area.AppendArc(start_bearing, end_bearing);
}

static bool
ParseArcPoints(const TCHAR *buffer, TempAirspaceType &temp_area)
{
  // Read start coordinates
  GeoPoint start;
  if (!ReadCoords(&buffer[3], start))
    return false;

  // Skip comma character
  const TCHAR* comma = _tcschr(buffer, ',');
  if (!comma)
    return false;

  // Read end coordinates
  GeoPoint end;
  if (!ReadCoords(&comma[1], end))
    return false;

  temp_area.AppendArc(start, end);
  return true;
}

static AirspaceClass
ParseType(const TCHAR *buffer)
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_class_strings); i++)
    if (StringAfterPrefix(buffer, airspace_class_strings[i].string))
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
ValueAfterSpace(const TCHAR *p)
{
  if (StringIsEmpty(p))
    return p;

  if (*p != _T(' '))
    /* not a space: must be a malformed line */
    return NULL;

  /* skip the space */
  return p + 1;
}

static const TCHAR *
SkipSpaces(const TCHAR *p)
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
      value = ValueAfterSpace(line + 2);
      if (value == NULL)
        break;

    {
      GeoPoint temp_point;
      if (!ReadCoords(value, temp_point))
        return false;

      temp_area.points.push_back(temp_point);
      break;
    }

    case _T('C'):
    case _T('c'):
      temp_area.radius = Units::ToSysUnit(fixed(_tcstod(&line[2], NULL)),
                                          Unit::NAUTICAL_MILES);
      temp_area.AddCircle(airspace_database);
      temp_area.Reset();
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
    if ((value = StringAfterPrefixCI(SkipSpaces(line + 1), _T("X="))) != NULL) {
      if (!ReadCoords(value, temp_area.center))
        return false;
    } else if (StringAfterPrefixCI(SkipSpaces(line + 1), _T("D=-"))) {
      temp_area.rotation = -1;
    } else if (StringAfterPrefixCI(SkipSpaces(line + 1), _T("D=+"))) {
      temp_area.rotation = +1;
    }
    break;

  case _T('A'):
  case _T('a'):
    switch (line[1]) {
    case _T('C'):
    case _T('c'):
      value = ValueAfterSpace(line + 2);
      if (value == NULL)
        break;

      if (!temp_area.points.empty())
        temp_area.AddPolygon(airspace_database);

      temp_area.Reset();

      temp_area.type = ParseType(value);
      break;

    case _T('N'):
    case _T('n'):
      value = ValueAfterSpace(line + 2);
      if (value != NULL)
        temp_area.name = value;
      break;

    case _T('L'):
    case _T('l'):
      value = ValueAfterSpace(line + 2);
      if (value != NULL)
        ReadAltitude(value, temp_area.base);
      break;

    case _T('H'):
    case _T('h'):
      value = ValueAfterSpace(line + 2);
      if (value != NULL)
        ReadAltitude(value, temp_area.top);
      break;

    case _T('R'):
    case _T('r'):
      value = ValueAfterSpace(line + 2);
      if (value != NULL)
        temp_area.radio = value;
      break;

    default:
      return true;
    }

    break;

  }
  return true;
}

static AirspaceClass
ParseClassTNP(const TCHAR *buffer)
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_tnp_class_chars); i++)
    if (buffer[0] == airspace_tnp_class_chars[i].character)
      return airspace_tnp_class_chars[i].type;

  return OTHER;
}

static AirspaceClass
ParseTypeTNP(const TCHAR *buffer)
{
  // Handle e.g. "TYPE=CLASS C" properly
  const TCHAR *type = StringAfterPrefixCI(buffer, _T("CLASS "));
  if (type) {
    AirspaceClass _class = ParseClassTNP(type);
    if (_class != OTHER)
      return _class;
  } else {
    type = buffer;
  }

  for (unsigned i = 0; i < ARRAY_SIZE(airspace_tnp_type_strings); i++)
    if (StringIsEqualIgnoreCase(type, airspace_tnp_type_strings[i].string))
      return airspace_tnp_type_strings[i].type;

  return OTHER;
}

static bool
ParseCoordsTNP(const TCHAR *buffer, GeoPoint &point)
{
  // Format: N542500 E0105000
  bool negative = false;
  long deg = 0, min = 0, sec = 0;
  TCHAR *ptr;

  if (buffer[0] == _T('S') || buffer[0] == _T('s'))
    negative = true;

  sec = _tcstol(&buffer[1], &ptr, 10);
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
ParseArcTNP(const TCHAR *buffer, TempAirspaceType &temp_area)
{
  if (temp_area.points.empty())
    return false;

  // (ANTI-)CLOCKWISE RADIUS=34.95 CENTRE=N523333 E0131603 TO=N522052 E0122236

  GeoPoint from = temp_area.points.back();

  const TCHAR* parameter;
  if ((parameter = _tcsstr(buffer, _T(" "))) == NULL)
    return false;
  if ((parameter = StringAfterPrefixCI(parameter, _T(" CENTRE="))) == NULL)
    return false;

  if (!ParseCoordsTNP(parameter, temp_area.center))
    return false;

  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  parameter++;
  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  if ((parameter = StringAfterPrefixCI(parameter, _T(" TO="))) == NULL)
    return false;

  GeoPoint to;
  if (!ParseCoordsTNP(parameter, to))
    return false;

  temp_area.AppendArc(from, to);

  return true;
}

static bool
ParseCircleTNP(const TCHAR *buffer, TempAirspaceType &temp_area)
{
  // CIRCLE RADIUS=17.00 CENTRE=N533813 E0095943

  const TCHAR* parameter;
  if ((parameter = StringAfterPrefixCI(buffer, _T("RADIUS="))) == NULL)
    return false;
  temp_area.radius = Units::ToSysUnit(fixed(_tcstod(parameter, NULL)),
                                      Unit::NAUTICAL_MILES);

  if ((parameter = _tcsstr(parameter, _T(" "))) == NULL)
    return false;
  if ((parameter = StringAfterPrefixCI(parameter, _T(" CENTRE="))) == NULL)
    return false;
  ParseCoordsTNP(parameter, temp_area.center);

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
  if ((parameter = StringAfterPrefixCI(line, _T("INCLUDE="))) != NULL) {
    if (StringIsEqualIgnoreCase(parameter, _T("YES")))
      ignore = false;
    else if (StringIsEqualIgnoreCase(parameter, _T("NO")))
      ignore = true;

    return true;
  }

  if (ignore)
    return true;

  if ((parameter = StringAfterPrefixCI(line, _T("POINT="))) != NULL) {
    GeoPoint temp_point;
    if (!ParseCoordsTNP(parameter, temp_point))
      return false;

    temp_area.points.push_back(temp_point);
  } else if ((parameter =
      StringAfterPrefixCI(line, _T("CIRCLE "))) != NULL) {
    if (!ParseCircleTNP(parameter, temp_area))
      return false;

    temp_area.AddCircle(airspace_database);
    temp_area.ResetTNP();
  } else if ((parameter =
      StringAfterPrefixCI(line, _T("CLOCKWISE "))) != NULL) {
    temp_area.rotation = 1;
    if (!ParseArcTNP(parameter, temp_area))
      return false;
  } else if ((parameter =
      StringAfterPrefixCI(line, _T("ANTI-CLOCKWISE "))) != NULL) {
    temp_area.rotation = -1;
    if (!ParseArcTNP(parameter, temp_area))
      return false;
  } else if ((parameter = StringAfterPrefixCI(line, _T("TITLE="))) != NULL) {
    if (!temp_area.points.empty())
      temp_area.AddPolygon(airspace_database);

    temp_area.ResetTNP();

    temp_area.name = parameter;
  } else if ((parameter = StringAfterPrefixCI(line, _T("TYPE="))) != NULL) {
    if (!temp_area.points.empty())
      temp_area.AddPolygon(airspace_database);

    temp_area.ResetTNP();

    temp_area.type = ParseTypeTNP(parameter);
  } else if ((parameter = StringAfterPrefixCI(line, _T("CLASS="))) != NULL) {
    temp_area.type = ParseClassTNP(parameter);
  } else if ((parameter = StringAfterPrefixCI(line, _T("TOPS="))) != NULL) {
    ReadAltitude(parameter, temp_area.top);
  } else if ((parameter = StringAfterPrefixCI(line, _T("BASE="))) != NULL) {
    ReadAltitude(parameter, temp_area.base);
  } else if ((parameter = StringAfterPrefixCI(line, _T("RADIO="))) != NULL) {
    temp_area.radio = parameter;
  } else if ((parameter = StringAfterPrefixCI(line, _T("ACTIVE="))) != NULL) {
    if (StringIsEqualIgnoreCase(parameter, _T("WEEKEND")))
      temp_area.days_of_operation.SetWeekend();
    else if (StringIsEqualIgnoreCase(parameter, _T("WEEKDAY")))
      temp_area.days_of_operation.SetWeekdays();
    else if (StringIsEqualIgnoreCase(parameter, _T("EVERYDAY")))
      temp_area.days_of_operation.SetAll();
  }

  return true;
}

static AirspaceFileType
DetectFileType(const TCHAR *line)
{
  if (StringAfterPrefixCI(line, _T("INCLUDE=")) ||
      StringAfterPrefixCI(line, _T("TYPE=")) ||
      StringAfterPrefixCI(line, _T("TITLE=")))
    return AFT_TNP;

  const TCHAR *p = StringAfterPrefixCI(line, _T("AC"));
  if (p != NULL && (StringIsEmpty(p) || *p == _T(' ')))
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
  for (unsigned line_num = 1; (line = reader.read()) != NULL; line_num++) {
    // Skip empty line
    if (StringIsEmpty(line))
      continue;

    if (filetype == AFT_UNKNOWN) {
      filetype = DetectFileType(line);
      if (filetype == AFT_UNKNOWN)
        continue;
    }

    // Parse the line
    if (filetype == AFT_OPENAIR)
      if (!ParseLine(airspaces, line, temp_area) &&
          !ShowParseWarning(line_num, line))
        return false;

    if (filetype == AFT_TNP)
      if (!ParseLineTNP(airspaces, line, temp_area, ignore) &&
          !ShowParseWarning(line_num, line))
        return false;

    // Update the ProgressDialog
    if ((line_num & 0xff) == 0)
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
