/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Language/Language.hpp"
#include "Util/CharUtil.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StringParser.hxx"
#include "Util/Macros.hpp"
#include "Geo/Math.hpp"
#include "IO/LineReader.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Geo/GeoVector.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Util/StaticString.hxx"
#include "Util/StringCompare.hxx"

#include <tchar.h>

enum class AirspaceFileType {
  UNKNOWN,
  OPENAIR,
  TNP,
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

static constexpr AirspaceClassStringCouple airspace_class_strings[] = {
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
  { _T("RMZ"), RMZ },
  { _T("MATZ"), MATZ },
  { _T("GSEC"), WAVE },
};

static constexpr AirspaceClassCharCouple airspace_tnp_class_chars[] = {
  { _T('A'), CLASSA },
  { _T('B'), CLASSB },
  { _T('C'), CLASSC },
  { _T('D'), CLASSD },
  { _T('E'), CLASSE },
  { _T('F'), CLASSF },
  { _T('G'), CLASSG },
};

static constexpr AirspaceClassStringCouple airspace_tnp_type_strings[] = {
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
  { _T("MATZ"), MATZ },
  { _T("RMZ"), RMZ },
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
  double radius;

  // Arc
  int rotation;

  void
  Reset()
  {
    days_of_operation.SetAll();
    radio = _T("");
    type = OTHER;
    base = top = AirspaceAltitude();
    points.clear();
    center.longitude = Angle::Zero();
    center.latitude = Angle::Zero();
    rotation = 1;
    radius = 0;
  }

  void
  ResetTNP()
  {
    // Preserve type, radio and days_of_operation for next airspace blocks
    points.clear();
    center.longitude = Angle::Zero();
    center.latitude = Angle::Zero();
    rotation = 1;
    radius = 0;
  }

  void
  AddPolygon(Airspaces &airspace_database)
  {
    if (points.size() < 3)
      return;

    AbstractAirspace *as = new AirspacePolygon(points);
    as->SetProperties(std::move(name), type, base, top);
    as->SetRadio(radio);
    as->SetDays(days_of_operation);
    airspace_database.Add(as);
  }

  void
  AddCircle(Airspaces &airspace_database)
  {
    AbstractAirspace *as = new AirspaceCircle(center, radius);
    as->SetProperties(std::move(name), type, base, top);
    as->SetRadio(radio);
    as->SetDays(days_of_operation);
    airspace_database.Add(as);
  }

  static int
  ArcStepWidth(double radius)
  {
    if (radius > 50000)
      return 1;
    if (radius > 25000)
      return 2;
    if (radius > 10000)
      return 3;

    return 5;
  }

  void
  AppendArc(const GeoPoint start, const GeoPoint end)
  {

    // Determine start bearing and radius
    const GeoVector v = center.DistanceBearing(start);
    Angle start_bearing = v.bearing;
    const auto radius = v.distance;

    // 5 or -5, depending on direction
    const auto _step = ArcStepWidth(radius);
    const auto step = Angle::Degrees(rotation * _step);
    const auto threshold = _step * 1.5;

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
    const auto _step = ArcStepWidth(radius);
    const auto step = Angle::Degrees(rotation * _step);
    const auto threshold = _step * 1.5;

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
ShowParseWarning(int line, const TCHAR *str, OperationEnvironment &operation)
{
  StaticString<256> buffer;
  buffer.Format(_T("%s: %d\r\n\"%s\""),
                _("Parse Error at Line"), line, str);
  operation.SetErrorMessage(buffer.c_str());
  return false;
}

static void
ReadAltitude(StringParser<TCHAR> &input, AirspaceAltitude &altitude)
{
  auto unit = Unit::FEET;
  enum { MSL, AGL, SFC, FL, STD, UNLIMITED } type = MSL;
  double value = 0;

  while (true) {
    input.Strip();

    if (IsDigitASCII(input.front())) {
      input.ReadDouble(value);
    } else if (input.SkipMatchIgnoreCase(_T("GND"), 3) ||
               input.SkipMatchIgnoreCase(_T("AGL"), 3)) {
      type = AGL;
    } else if (input.SkipMatchIgnoreCase(_T("SFC"), 3)) {
      type = SFC;
    } else if (input.SkipMatchIgnoreCase(_T("FL"), 2)) {
      type = FL;
    } else if (input.SkipMatchIgnoreCase(_T("FT"), 2)) {
      unit = Unit::FEET;
    } else if (input.SkipMatchIgnoreCase(_T("MSL"), 3)) {
      type = MSL;
    } else if (input.front() == _T('M') || input.front() == _T('m')) {
      unit = Unit::METER;
      input.Skip();
    } else if (input.SkipMatchIgnoreCase(_T("STD"), 3)) {
      type = STD;
    } else if (input.SkipMatchIgnoreCase(_T("UNL"), 3)) {
      type = UNLIMITED;
    } else if (input.IsEmpty())
      break;
    else
      input.Skip();
  }

  switch (type) {
  case FL:
    altitude.reference = AltitudeReference::STD;
    altitude.flight_level = value;

    /* prepare fallback, just in case we have no terrain */
    altitude.altitude = Units::ToSysUnit(value, Unit::FLIGHT_LEVEL);
    return;

  case UNLIMITED:
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = 50000;
    return;

  case SFC:
    altitude.reference = AltitudeReference::AGL;
    altitude.altitude_above_terrain = -1;

    /* prepare fallback, just in case we have no terrain */
    altitude.altitude = 0;
    return;

  default:
    break;
  }

  // For MSL, AGL and STD we convert the altitude to meters
  value = Units::ToSysUnit(value, unit);
  switch (type) {
  case MSL:
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = value;
    return;

  case AGL:
    altitude.reference = AltitudeReference::AGL;
    altitude.altitude_above_terrain = value;

    /* prepare fallback, just in case we have no terrain */
    altitude.altitude = value;
    return;

  case STD:
    altitude.reference = AltitudeReference::STD;
    altitude.flight_level = Units::ToUserUnit(value, Unit::FLIGHT_LEVEL);

    /* prepare fallback, just in case we have no QNH */
    altitude.altitude = value;
    return;

  default:
    break;
  }
}

/**
 * @return the non-negative angle or a negative value on error
 */
static Angle
ReadNonNegativeAngle(StringParser<TCHAR> &input, double max_degrees)
{
  double degrees;
  if (!input.ReadDouble(degrees) || degrees < 0 || degrees > max_degrees)
    return Angle::Native(-1);

  if (input.SkipMatch(':')) {
    double minutes;
    if (!input.ReadDouble(minutes) || minutes < 0 || minutes > 60)
      return Angle::Native(-1);

    degrees += minutes / 60;

    if (input.SkipMatch(':')) {
      double seconds;
      if (!input.ReadDouble(seconds) || seconds < 0 || seconds > 60)
        return Angle::Native(-1);

      degrees += seconds / 3600;
    }
  }

  return Angle::Degrees(degrees);
}

static bool
ReadCoords(StringParser<TCHAR> &input, GeoPoint &point)
{
  // Format: 53:20:41 N 010:24:41 E
  // Alternative Format: 53:20.68 N 010:24.68 E

  auto angle = ReadNonNegativeAngle(input, 91);
  if (angle.IsNegative())
    return false;

  input.Strip();
  if (input.SkipMatch('S') || input.SkipMatch('s'))
    angle.Flip();
  else if (!input.SkipMatch('N') && !input.SkipMatch('n'))
    return false;

  point.latitude = angle;

  angle = ReadNonNegativeAngle(input, 181);
  if (angle.IsNegative())
    return false;

  input.Strip();
  if (input.SkipMatch('W') || input.SkipMatch('w'))
    angle.Flip();
  else if (!input.SkipMatch('E') && !input.SkipMatch('e'))
    return false;

  point.longitude = angle;

  point.Normalize(); // ensure longitude is within -180:180
  return true;
}

static bool
ParseBearingDegrees(StringParser<TCHAR> &input, Angle &value_r)
{
  double value;
  if (!input.ReadDouble(value) || value < 0 || value > 361)
    return false;

  value_r = Angle::Degrees(value).AsBearing();
  return true;
}

static bool
ParseArcBearings(StringParser<TCHAR> &input, TempAirspaceType &temp_area)
{
  // Determine radius and start/end bearing

  double radius;
  if (!input.ReadDouble(radius) || radius <= 0 || radius > 1000)
    return false;

  temp_area.radius = Units::ToSysUnit(radius, Unit::NAUTICAL_MILES);
  Angle start_bearing, end_bearing;
  if (!ParseBearingDegrees(input, start_bearing) ||
      !ParseBearingDegrees(input, end_bearing))
    return false;

  temp_area.AppendArc(start_bearing, end_bearing);
  return true;
}

static bool
ParseArcPoints(StringParser<TCHAR> &input, TempAirspaceType &temp_area)
{
  // Read start coordinates
  GeoPoint start;
  if (!ReadCoords(input, start))
    return false;

  // Skip comma character
  input.Strip();
  if (!input.SkipMatch(','))
    return false;

  // Read end coordinates
  GeoPoint end;
  if (!ReadCoords(input, end))
    return false;

  temp_area.AppendArc(start, end);
  return true;
}

static AirspaceClass
ParseType(const TCHAR *buffer)
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_class_strings); i++)
    if (StringIsEqualIgnoreCase(buffer, airspace_class_strings[i].string))
      return airspace_class_strings[i].type;

  return OTHER;
}

static bool
ParseLine(Airspaces &airspace_database, StringParser<TCHAR> &&input,
          TempAirspaceType &temp_area)
{
  double d;

  // Only return expected lines
  switch (input.pop_front()) {
  case _T('D'):
  case _T('d'):
    switch (input.pop_front()) {
    case _T('P'):
    case _T('p'):
      if (!input.SkipWhitespace())
        break;

    {
      GeoPoint temp_point;
      if (!ReadCoords(input, temp_point))
        return false;

      temp_area.points.push_back(temp_point);
      break;
    }

    case _T('C'):
    case _T('c'):
      if (!input.ReadDouble(d) || d < 0 || d > 1000)
        return false;

      temp_area.radius = Units::ToSysUnit(d, Unit::NAUTICAL_MILES);
      temp_area.AddCircle(airspace_database);
      temp_area.Reset();
      break;

    case _T('A'):
    case _T('a'):
      ParseArcBearings(input, temp_area);
      break;

    case _T('B'):
    case _T('b'):
      return ParseArcPoints(input, temp_area);

    default:
      return true;
    }
    break;

  case _T('V'):
  case _T('v'):
    input.Strip();
    if (input.SkipMatchIgnoreCase(_T("X="), 2)) {
      if (!ReadCoords(input, temp_area.center))
        return false;
    } else if (input.SkipMatchIgnoreCase(_T("D=-"), 3)) {
      temp_area.rotation = -1;
    } else if (input.SkipMatchIgnoreCase(_T("D=+"), 3)) {
      temp_area.rotation = +1;
    }
    break;

  case _T('A'):
  case _T('a'):
    switch (input.pop_front()) {
    case _T('C'):
    case _T('c'):
      if (!input.SkipWhitespace())
        break;

      temp_area.AddPolygon(airspace_database);
      temp_area.Reset();

      temp_area.type = ParseType(input.c_str());
      break;

    case _T('N'):
    case _T('n'):
      if (input.SkipWhitespace())
        temp_area.name = input.c_str();
      break;

    case _T('L'):
    case _T('l'):
      if (input.SkipWhitespace())
        ReadAltitude(input, temp_area.base);
      break;

    case _T('H'):
    case _T('h'):
      if (input.SkipWhitespace())
        ReadAltitude(input, temp_area.top);
      break;

    case _T('R'):
    case _T('r'):
      if (input.SkipWhitespace())
        temp_area.radio = input.c_str();
      break;

    default:
      return true;
    }

    break;

  }
  return true;
}

static bool
ParseLine(Airspaces &airspace_database, TCHAR *line,
          TempAirspaceType &temp_area)
{
  // Strip comments
  auto *comment = StringFind(line, _T('*'));
  if (comment != nullptr)
    *comment = _T('\0');

  return ParseLine(airspace_database, StringParser<TCHAR>(line), temp_area);
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
ReadNonNegativeAngleTNP(StringParser<TCHAR> &input, Angle &value_r,
                        unsigned max_degrees)
{
  unsigned deg, min, sec;
  if (!input.ReadUnsigned(sec))
    return false;

  deg = sec / 10000;
  min = (sec - deg * 10000) / 100;
  sec = sec - min * 100 - deg * 10000;

  if (deg > max_degrees || min >= 60 || sec >= 60)
    return false;

  value_r = Angle::DMS(deg, min, sec);
  return true;
}

static bool
ParseCoordsTNP(StringParser<TCHAR> &input, GeoPoint &point)
{
  // Format: N542500 E0105000
  bool negative = false;

  if (input.SkipMatch('S') || input.SkipMatch('s'))
    negative = true;
  else if (input.SkipMatch('N') || input.SkipMatch('n'))
    negative = false;
  else
    return false;

  if (!ReadNonNegativeAngleTNP(input, point.latitude, 91))
    return false;

  if (negative)
    point.latitude.Flip();

  input.Strip();

  if (input.SkipMatch('W') || input.SkipMatch('w'))
    negative = true;
  else if (input.SkipMatch('E') || input.SkipMatch('e'))
    negative = false;
  else
    return false;

  if (!ReadNonNegativeAngleTNP(input, point.longitude, 181))
    return false;

  if (negative)
    point.longitude.Flip();

  point.Normalize(); // ensure longitude is within -180:180

  return true;
}

static bool
ParseArcTNP(StringParser<TCHAR> &input, TempAirspaceType &temp_area)
{
  if (temp_area.points.empty())
    return false;

  // (ANTI-)CLOCKWISE RADIUS=34.95 CENTRE=N523333 E0131603 TO=N522052 E0122236

  GeoPoint from = temp_area.points.back();

  /* skip "RADIUS=... " */
  if (!input.SkipWord())
    return false;

  if (!input.SkipMatchIgnoreCase(_T("CENTRE="), 7))
    return false;

  if (!ParseCoordsTNP(input, temp_area.center))
    return false;

  if (!input.SkipMatchIgnoreCase(_T(" TO="), 4))
    return false;

  GeoPoint to;
  if (!ParseCoordsTNP(input, to))
    return false;

  temp_area.AppendArc(from, to);

  return true;
}

static bool
ParseCircleTNP(StringParser<TCHAR> &input, TempAirspaceType &temp_area)
{
  // CIRCLE RADIUS=17.00 CENTRE=N533813 E0095943

  if (!input.SkipMatchIgnoreCase(_T("RADIUS="), 7))
    return false;

  double radius;
  if (!input.ReadDouble(radius) || radius <= 0 || radius > 1000)
    return false;

  temp_area.radius = Units::ToSysUnit(radius, Unit::NAUTICAL_MILES);

  if (!input.SkipMatchIgnoreCase(_T(" CENTRE="), 8))
    return false;

  return ParseCoordsTNP(input, temp_area.center);
}

static bool
ParseLineTNP(Airspaces &airspace_database, StringParser<TCHAR> &input,
             TempAirspaceType &temp_area, bool &ignore)
{
  if (input.Match('#'))
    return true;

  if (input.SkipMatchIgnoreCase(_T("INCLUDE="), 8)) {
    if (input.MatchIgnoreCase(_T("YES"), 3))
      ignore = false;
    else if (input.MatchIgnoreCase(_T("NO"), 2))
      ignore = true;

    return true;
  }

  if (ignore)
    return true;

  if (input.SkipMatchIgnoreCase(_T("POINT="), 6)) {
    GeoPoint temp_point;
    if (!ParseCoordsTNP(input, temp_point))
      return false;

    temp_area.points.push_back(temp_point);
  } else if (input.SkipMatchIgnoreCase(_T("CIRCLE "), 7)) {
    if (!ParseCircleTNP(input, temp_area))
      return false;

    temp_area.AddCircle(airspace_database);
    temp_area.ResetTNP();
  } else if (input.SkipMatchIgnoreCase(_T("CLOCKWISE "), 10)) {
    temp_area.rotation = 1;
    if (!ParseArcTNP(input, temp_area))
      return false;
  } else if (input.SkipMatchIgnoreCase(_T("ANTI-CLOCKWISE "), 15)) {
    temp_area.rotation = -1;
    if (!ParseArcTNP(input, temp_area))
      return false;
  } else if (input.SkipMatchIgnoreCase(_T("TITLE="), 6)) {
    temp_area.AddPolygon(airspace_database);
    temp_area.ResetTNP();

    temp_area.name = input.c_str();
  } else if (input.SkipMatchIgnoreCase(_T("TYPE="), 5)) {
    temp_area.AddPolygon(airspace_database);
    temp_area.ResetTNP();

    temp_area.type = ParseTypeTNP(input.c_str());
  } else if (input.SkipMatchIgnoreCase(_T("CLASS="), 6)) {
    temp_area.type = ParseClassTNP(input.c_str());
  } else if (input.SkipMatchIgnoreCase(_T("TOPS="), 5)) {
    ReadAltitude(input, temp_area.top);
  } else if (input.SkipMatchIgnoreCase(_T("BASE="), 5)) {
    ReadAltitude(input, temp_area.base);
  } else if (input.SkipMatchIgnoreCase(_T("RADIO="), 6)) {
    temp_area.radio = input.c_str();
  } else if (input.SkipMatchIgnoreCase(_T("ACTIVE="), 7)) {
    if (input.MatchAllIgnoreCase(_T("WEEKEND")))
      temp_area.days_of_operation.SetWeekend();
    else if (input.MatchAllIgnoreCase(_T("WEEKDAY")))
      temp_area.days_of_operation.SetWeekdays();
    else if (input.MatchAllIgnoreCase(_T("EVERYDAY")))
      temp_area.days_of_operation.SetAll();
  }

  return true;
}

static AirspaceFileType
DetectFileType(const TCHAR *line)
{
  if (StringStartsWithIgnoreCase(line, _T("INCLUDE=")) ||
      StringStartsWithIgnoreCase(line, _T("TYPE=")) ||
      StringStartsWithIgnoreCase(line, _T("TITLE=")))
    return AirspaceFileType::TNP;

  const TCHAR *p = StringAfterPrefixCI(line, _T("AC"));
  if (p != nullptr && (StringIsEmpty(p) || *p == _T(' ')))
    return AirspaceFileType::OPENAIR;

  return AirspaceFileType::UNKNOWN;
}

bool
AirspaceParser::Parse(TLineReader &reader, OperationEnvironment &operation)
{
  bool ignore = false;

  // Create and init ProgressDialog
  operation.SetProgressRange(1024);

  const long file_size = reader.GetSize();

  TempAirspaceType temp_area;
  AirspaceFileType filetype = AirspaceFileType::UNKNOWN;

  TCHAR *line;

  // Iterate through the lines
  for (unsigned line_num = 1; (line = reader.ReadLine()) != nullptr; line_num++) {
    StripRight(line);

    // Skip empty line
    if (StringIsEmpty(line))
      continue;

    if (filetype == AirspaceFileType::UNKNOWN) {
      filetype = DetectFileType(line);
      if (filetype == AirspaceFileType::UNKNOWN)
        continue;
    }

    // Parse the line
    if (filetype == AirspaceFileType::OPENAIR)
      if (!ParseLine(airspaces, line, temp_area) &&
          !ShowParseWarning(line_num, line, operation))
        return false;

    if (filetype == AirspaceFileType::TNP) {
      StringParser<TCHAR> input(line);
      if (!ParseLineTNP(airspaces, input, temp_area, ignore) &&
          !ShowParseWarning(line_num, line, operation))
        return false;
    }

    // Update the ProgressDialog
    if ((line_num & 0xff) == 0)
      operation.SetProgressPosition(reader.Tell() * 1024 / file_size);
  }

  if (filetype == AirspaceFileType::UNKNOWN) {
    operation.SetErrorMessage(_("Unknown airspace filetype"));
    return false;
  }

  // Process final area (if any)
  temp_area.AddPolygon(airspaces);

  return true;
}
