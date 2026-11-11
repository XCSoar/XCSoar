// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * The implementation follows Naviter's "Extended OpenAir file format specification"
 * Version 2.1.2
 * as of 2026-05-21
 * https://github.com/naviter/seeyou_file_formats/blob/main/OpenAir_File_Format_Support.md#aa-airspace-activation-times
 *
 * A weekly AA line from the OpenAir 3.0 draft is read
 * (`AA MON-FRI 07:00Z/15:00Z`). A date range, AA EXCEPT, or a sun
 * event is not: that line keeps the airspace active.
 */

#include "AirspaceParser.hpp"
#include "Airspace/Airspaces.hpp"
#include "Operation/ProgressListener.hpp"
#include "Radio/TransponderCode.hpp"
#include "Units/System.hpp"
#include "Language/Language.hpp"
#include "util/CharUtil.hxx"
#include "util/StringAPI.hxx"
#include "util/StringParser.hxx"
#include "util/Macros.hpp"
#include "Geo/Math.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Geo/GeoVector.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "io/BufferedReader.hxx"
#include "io/StringConverter.hpp"
#include "util/StaticString.hxx"
#include "util/StringCompare.hxx"
#include "util/StringSplit.hxx"
#include "time/BrokenDateTime.hpp"
#include "util/TruncateString.hpp"
#include "LogFile.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using std::string_view_literals::operator""sv;

enum class AirspaceActivationDecode {
  NEVER_ACTIVE,
  INTERVAL,
  WEEKDAY,
  UNDETECTABLE_CONTENT,
};

struct ParsedActivation {
  AirspaceActivationInterval interval;
  AirspaceWeekdayPeriod weekday;
};

enum class AirspaceFileType {
  UNKNOWN,
  OPENAIR,
  TNP,
};

struct AirspaceClassCharCouple
{
  const char character;
  AirspaceClass asclass;
};

struct AirspaceClassStringCouple
{
  const char *string;
  AirspaceClass asclass;
};

static constexpr AirspaceClassStringCouple airspace_class_strings[] = {
  { "R", RESTRICTED },
  { "Q", DANGER },
  { "P", PROHIBITED },
  { "CTR", CTR },
  { "A", CLASSA },
  { "B", CLASSB },
  { "C", CLASSC },
  { "D", CLASSD },
  { "GP", NOGLIDER },
  { "W", WAVE },
  { "E", CLASSE },
  { "F", CLASSF },
  { "TMZ", TMZ },
  { "G", CLASSG },
  { "RMZ", RMZ },
  { "MATZ", MATZ },
  { "GSEC", GLIDING_SECTOR },
  { "UNC", UNCLASSIFIED },
  { "RESTRICTED", RESTRICTED },
  { "TMA", TMA },
  { "TRA", TRA },
  { "TSA", TSA },
  { "FIR", FIR },
  { "UIR", UIR },
  { "ADIZ", ADIZ },
  { "ATZ", ATZ },
  { "AWY", AWY },
  { "MTR", MTR },
  { "ALERT", ALERT },
  { "WARNING", WARNING },
  { "DANGER", DANGER },
  { "PROHIBITED", PROHIBITED },
  { "PROTECTED", PROTECTED },
  { "HTZ", HTZ },
  { "TRP", TRP },
  { "TIZ", TIZ },
  { "TIA", TIA },
  { "MTA", MTA },
  { "CTA", CTA },
  { "ACCSEC", ACC_SECTOR },
  { "AERIAL_SPORTING_RECREATIONAL", AERIAL_SPORTING_RECREATIONAL },
  { "ASRA", AERIAL_SPORTING_RECREATIONAL },
  { "OFR", OVERFLIGHT_RESTRICTION },
  { "MRT", MRT },
  { "TFR", TFR },
  { "VFRSEC", VFR_SECTOR },
  { "FIS", FIS_SECTOR },
  { "LTA", LTA },
  { "UTA", UTA },
  { "AIRSPACECLASSCOUNT", AIRSPACECLASSCOUNT }
};

static constexpr AirspaceClassCharCouple airspace_tnp_class_chars[] = {
  { 'A', CLASSA },
  { 'B', CLASSB },
  { 'C', CLASSC },
  { 'D', CLASSD },
  { 'E', CLASSE },
  { 'F', CLASSF },
  { 'G', CLASSG },
};

static constexpr AirspaceClassStringCouple airspace_tnp_type_strings[] = {
  { "C", CTR },
  { "CTA", CTR },
  { "CTR", CTR },
  { "CTA/CTR", CTR },
  { "CTR/CTA", CTR },
  { "R", RESTRICTED },
  { "RESTRICTED", RESTRICTED },
  { "P", PROHIBITED },
  { "PROHIBITED", PROHIBITED },
  { "D", DANGER },
  { "DANGER", DANGER },
  { "G", WAVE },
  { "GSEC", WAVE },
  { "T", TMZ },
  { "TMZ", TMZ },
  { "CYR", RESTRICTED },
  { "CYD", DANGER },
  { "CYA", CLASSF },
  { "MATZ", MATZ },
  { "RMZ", RMZ },
};

static constexpr AirspaceClass airspace_ICAO_and_Unclassified[] = {
  AirspaceClass::CLASSA, AirspaceClass::CLASSB, AirspaceClass::CLASSC, AirspaceClass::CLASSD,
  AirspaceClass::CLASSE, AirspaceClass::CLASSF, AirspaceClass::CLASSG, AirspaceClass::UNCLASSIFIED};

// this can now be called multiple times to load several airspaces.

struct TempAirspace
{
  /**
   * This exception class gets thrown when Commit() fails; in that
   * case, the error messages doesn't show the current line (which
   * begins a new airspace) but the first line of the airspace that is
   * being committed.
   */
  struct CommitError {
    const char *msg;

    explicit constexpr CommitError(const char *_msg) noexcept
      :msg(_msg) {}
  };

  TempAirspace() noexcept {
    points.reserve(256);
    Reset(0);
  }

  // General
  std::string name;
  std::string station_name;
  RadioFrequency radio_frequency;
  TransponderCode transponder_code;
  AirspaceClass asclass;
  AirspaceClass astype;
  AirspaceOrigin origin = AirspaceOrigin::FILE;
  std::optional<AirspaceAltitude> base;
  std::optional<AirspaceAltitude> top;
  AirspaceActivity days_of_operation;
  bool has_activation_time;
  bool never_active;
  bool activation_unreadable = false;
  /** shortcut: 64 AA lines; more marks the airspace active. */
  static constexpr std::size_t MAX_ACTIVATION_INTERVALS = 64;
  std::vector<AirspaceActivationInterval> activation_intervals;
  std::vector<AirspaceWeekdayPeriod> activation_weekdays;

  // Polygon
  std::vector<GeoPoint> points;

  // Circle or Arc
  GeoPoint center;
  double radius;

  // Arc
  int rotation;

  /**
   * The line number where the current airspace began.
   */
  unsigned first_line_number;

  void
  Reset(unsigned line_number) noexcept
  {
    days_of_operation.SetAll();
    has_activation_time = false;
    never_active = false;
    activation_unreadable = false;
    activation_intervals.clear();
    activation_weekdays.clear();
    name.clear();
    radio_frequency = RadioFrequency::Null();
    transponder_code = TransponderCode::Null();
    station_name.clear();
    asclass = OTHER;
    astype = OTHER; // the default if no AY tag parsed (i.e. AC tag is not a ICAO or not UNCLASSIFIED)
    base.reset();
    top.reset();
    points.clear();
    center = GeoPoint::Invalid();
    radius = -1;
    rotation = 1;
    first_line_number = line_number;
  }

  void
  ResetTNP(unsigned line_number) noexcept
  {
    // Preserve asclass, radio and days_of_operation for next airspace blocks
    name.clear();
    points.clear();
    center = GeoPoint::Invalid();
    radius = -1;
    rotation = 1;
    first_line_number = line_number;
  }

  void
  CopyPropertiesTo(AbstractAirspace &airspace) noexcept
  {
    airspace.SetProperties(std::move(name),
                           has_activation_time, never_active,
                           std::move(station_name),
                           std::move(transponder_code),
                           asclass, astype, *base, *top);
    if (has_activation_time)
      airspace.SetActivationSchedule(std::move(activation_intervals),
                                     std::move(activation_weekdays),
                                     activation_unreadable);

    airspace.SetRadioFrequency(radio_frequency);
    airspace.SetDays(days_of_operation);
    airspace.SetOrigin(origin);
  }

  /**
   * If there is an airspace, add it to the #Airspaces and return
   * true.  Returns false if no airspace was being constructed.
   * Throws if the airspace is bad.
   */
  bool Commit(Airspaces &airspace_database) {
    if (!points.empty()) {
      AddPolygon(airspace_database);
      return true;
    } else
      return false;
  }

  /**
   * Perform common checks before an airspace is committed to
   * #Airspaces.  Throws on error.
   */
  void Check() {
    if (asclass == OTHER && name.empty())
      throw CommitError{"Airspace has no name"};
  }

  void
  AddPolygon(Airspaces &airspace_database)
  {
    Check();

    if (points.size() < 3)
      throw CommitError{"Not enough polygon points"};

    if (!base)
      throw CommitError{"No base altitude"};

    if (!top)
      throw CommitError{"No top altitude"};

    auto as = std::make_shared<AirspacePolygon>(points);
    CopyPropertiesTo(*as);
    airspace_database.Add(std::move(as));
  }

  GeoPoint RequireCenter() {
    if (!center.IsValid())
      throw CommitError("No center");
    return center;
  }

  double RequireRadius() {
    if (radius < 0)
      throw CommitError("No radius");
    return radius;
  }

  void
  AddCircle(Airspaces &airspace_database)
  {
    Check();

    if (!points.empty())
      throw CommitError{"Airspace is a mix of polygon and circle"};

    if (!base)
      throw CommitError{"No base altitude"};

    if (!top)
      throw CommitError{"No top altitude"};

    auto as = std::make_shared<AirspaceCircle>(RequireCenter(),
                                               RequireRadius());
    CopyPropertiesTo(*as);
    airspace_database.Add(std::move(as));
  }

  static constexpr int
  ArcStepWidth(double radius) noexcept
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
    const auto center = RequireCenter();

    // Determine start bearing and radius
    const GeoVector v = center.DistanceBearing(start);
    Angle start_bearing = v.bearing;
    const auto radius = v.distance;

    // 5 or -5, depending on direction
    const auto _step = ArcStepWidth(radius);
    const auto step = Angle::Degrees(rotation * _step);
    const auto threshold = Angle::Degrees(_step * 1.5);

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
    while ((end_bearing - start_bearing).Absolute() > threshold) {
      start_bearing += step;
      points.push_back(FindLatitudeLongitude(center, start_bearing, radius));
    }

    // Add last polygon point
    points.push_back(end);
  }

  void
  AppendArc(Angle start, Angle end)
  {
    const auto center = RequireCenter();

    // 5 or -5, depending on direction
    const auto _step = ArcStepWidth(radius);
    const auto step = Angle::Degrees(rotation * _step);
    const auto threshold = Angle::Degrees(_step * 1.5);

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
    while ((end - start).Absolute() > threshold) {
      start += step;
      points.push_back(FindLatitudeLongitude(center, start, radius));
    }

    // Add last polygon point
    points.push_back(FindLatitudeLongitude(center, end, radius));
  }
};

[[nodiscard]]
static AirspaceAltitude
ReadAltitude(StringParser<> &input)
{
  ParseAirspaceAltitudeOptions options;
  options.strict_unknown_tokens = false;
  options.accept_amsl = false;
  options.unlimited_ceiling_m = 50000;

  const auto altitude = ParseAirspaceAltitude(input, options);
  assert(altitude.has_value());
  return *altitude;
}

/**
 * Throws on error.
 */
static Angle
ReadNonNegativeAngle(StringParser<> &input, double max_degrees)
{
  double degrees;

  if (auto x = input.ReadDouble(); x && *x >= 0 && *x <= max_degrees)
    degrees = *x;
  else
    throw std::runtime_error("Bad angle");

  if (input.SkipMatch(':')) {
    if (auto minutes = input.ReadDouble();
        minutes && *minutes >= 0 && *minutes <= 60)
      degrees += *minutes / 60;
    else
      throw std::runtime_error("Bad angle");

    if (input.SkipMatch(':')) {
      if (auto seconds = input.ReadDouble();
          seconds && *seconds >= 0 && *seconds <= 60)
        degrees += *seconds / 3600;
      else
        throw std::runtime_error("Bad angle");
    }
  }

  return Angle::Degrees(degrees);
}

/**
 * Throws on error.
 */
static GeoPoint
ReadCoords(StringParser<> &input)
{
  // Format: 53:20:41 N 010:24:41 E
  // Alternative Format: 53:20.68 N 010:24.68 E

  GeoPoint point;
  point.latitude = ReadNonNegativeAngle(input, 91);

  input.Strip();
  if (input.SkipMatch('S') || input.SkipMatch('s'))
    point.latitude.Flip();
  else if (!input.SkipMatch('N') && !input.SkipMatch('n'))
    throw std::runtime_error("N or S expected");

  point.longitude = ReadNonNegativeAngle(input, 181);

  input.Strip();
  if (input.SkipMatch('W') || input.SkipMatch('w'))
    point.longitude.Flip();
  else if (!input.SkipMatch('E') && !input.SkipMatch('e'))
    throw std::runtime_error("W or E expected");

  point.Normalize(); // ensure longitude is within -180:180
  return point;
}

/**
 * Throws on error.
 */
static Angle
ParseBearingDegrees(StringParser<> &input)
{
  if (auto value = input.ReadDouble(); value && *value >= 0 && *value <= 361)
    return Angle::Degrees(*value).AsBearing();
  else
    throw std::runtime_error("Bad angle");
}

static double
ParseRadiusNM(StringParser<> &input)
{
  if (auto radius = input.ReadDouble();
      radius && *radius > 0 && *radius <= 1000)
    return Units::ToSysUnit(*radius, Unit::NAUTICAL_MILES);
  else
    throw std::runtime_error("Bad radius");
}

/**
 * Throws on error.
 */
static void
ParseArcBearings(StringParser<> &input, TempAirspace &temp_area)
{
  // Determine radius and start/end bearing

  temp_area.radius = ParseRadiusNM(input);

  input.Strip();
  if (!input.SkipMatch(','))
    throw std::runtime_error("',' expected");

  Angle start_bearing = ParseBearingDegrees(input);

  input.Strip();
  if (!input.SkipMatch(','))
    throw std::runtime_error("',' expected");

  Angle end_bearing = ParseBearingDegrees(input);

  temp_area.AppendArc(start_bearing, end_bearing);
}

/**
 * Throws on error.
 */
static void
ParseArcPoints(StringParser<> &input, TempAirspace &temp_area)
{
  // Read start coordinates
  GeoPoint start = ReadCoords(input);

  // Skip comma character
  input.Strip();
  if (!input.SkipMatch(','))
    throw std::runtime_error("',' expected");

  // Read end coordinates
  GeoPoint end = ReadCoords(input);

  temp_area.AppendArc(start, end);
}

[[gnu::pure]]
static AirspaceClass
ParseClass(const char *buffer) noexcept
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_class_strings); i++)
    if (StringIsEqualIgnoreCase(buffer, airspace_class_strings[i].string))
      return airspace_class_strings[i].asclass;

  return OTHER;
}

[[gnu::pure]]
static AirspaceClass
ParseType(const char *buffer) noexcept
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_class_strings); i++)
    if (StringIsEqualIgnoreCase(buffer, airspace_class_strings[i].string)) {
      if (StringIsEqualIgnoreCase(buffer, "UNCLASSIFIED")) {
        return OTHER;
      } else {
        return airspace_class_strings[i].asclass;
      }
    }

  return OTHER;
}

/*
 * ISO 8601 T24:00:00 is the end of the day, the next midnight.
 * BrokenTime rejects hour 24.
 */
static bool
AcceptISO8601(BrokenDateTime &dt) noexcept
{
  if (dt.hour == 24 && dt.minute == 0 && dt.second == 0 &&
      dt.IsDatePlausible()) {
    dt.hour = 0;
    dt.IncrementDay();
  }

  return dt.IsPlausible();
}

/*
 * read ISO8601 formatted date and time UTC
 * example: 2026-05-10T14:00:00Z
 *      or: 2026-05-10T14:00Z
 *      or: 2026-05-10T24:00Z (end of that UTC day)
 */
static bool
ReadISO8601(const char *ts, BrokenDateTime &dt) noexcept
{
  unsigned year, month, day, hour, minute, second;
  char suffix = '\0';
  int result = sscanf(ts, "%04u-%02u-%02uT%02u:%02u:%02u%c",
                      &year, &month, &day, &hour, &minute, &second, &suffix);
  if (result == 7 && suffix == 'Z') {
    dt.year = year;
    dt.month = month;
    dt.day = day;
    dt.hour = hour;
    dt.minute = minute;
    dt.second = second;

    if (AcceptISO8601(dt)) return true;
  }

  // try w/o second
  suffix = '\0';
  result = sscanf(ts, "%04u-%02u-%02uT%02u:%02u%c",
                      &year, &month, &day, &hour, &minute, &suffix);
  if (result == 6 && suffix == 'Z') {
    dt.year = year;
    dt.month = month;
    dt.day = day;
    dt.hour = hour;
    dt.minute = minute;
    dt.second = 0;

    if (AcceptISO8601(dt)) return true;
  }
  return false;
}

static AirspaceActivationDecode
ParseInterval(const char *in, AirspaceActivationInterval &interval) noexcept
{
  char active_time_pair[80];

  interval.start = BrokenDateTime::Invalid();
  interval.end = BrokenDateTime::Invalid();

  CopyTruncateString(active_time_pair, sizeof(active_time_pair), in);

  char *slash_loc = std::strchr(active_time_pair, '/');
  if (slash_loc == nullptr)
    return AirspaceActivationDecode::UNDETECTABLE_CONTENT;

  *slash_loc = 0;
  /* "NONE" leaves the start implausible, which Contains() treats as open. */
  if (!StringIsEqualIgnoreCase(active_time_pair, "NONE") &&
      !ReadISO8601(active_time_pair, interval.start))
    return AirspaceActivationDecode::UNDETECTABLE_CONTENT;

  if (StringIsEqualIgnoreCase(slash_loc + 1, "NONE"))
    return AirspaceActivationDecode::INTERVAL;

  if (!ReadISO8601(slash_loc + 1, interval.end))
    return AirspaceActivationDecode::UNDETECTABLE_CONTENT;

  return AirspaceActivationDecode::INTERVAL;
}

/** Sunday is 0, matching BrokenDate::day_of_week. */
static int
DayIndex(std::string_view name) noexcept
{
  static constexpr const char *names[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
  };

  if (name.size() != 3)
    return -1;

  for (unsigned i = 0; i < ARRAY_SIZE(names); ++i)
    if (StringIsEqualIgnoreCase(name, names[i]))
      return i;

  return -1;
}

/** `MON-FRI`, `SAT,SUN` or `FRI-MON`. */
static bool
ParseWeekdayMask(std::string_view text, uint8_t &days) noexcept
{
  days = 0;
  while (!text.empty()) {
    const auto comma = text.find(',');
    const auto part = text.substr(0, comma);
    const auto dash = part.find('-');
    const int first = DayIndex(dash == std::string_view::npos
                                   ? part
                                   : part.substr(0, dash));
    const int last = dash == std::string_view::npos
                         ? first
                         : DayIndex(part.substr(dash + 1));
    if (first < 0 || last < 0 ||
        (dash != std::string_view::npos && first == last))
      return false;

    for (int day = first;; day = (day + 1) % 7) {
      days |= uint8_t(1u << day);
      if (day == last)
        break;
    }

    if (comma == std::string_view::npos)
      break;
    text.remove_prefix(comma + 1);
  }

  return days != 0;
}

/** `hh:mmZ`. 24:00Z is allowed only as an end. Seconds are not read. */
static bool
ReadClock(const char *text, unsigned &minute, bool allow_24) noexcept
{
  unsigned hour, min;
  char suffix = 0;
  if (std::strlen(text) != 6 ||
      std::sscanf(text, "%2u:%2u%c", &hour, &min, &suffix) != 3 ||
      suffix != 'Z' || min > 59)
    return false;

  if (hour == 24 && min == 0 && allow_24) {
    minute = 24 * 60;
    return true;
  }

  if (hour > 23)
    return false;

  minute = hour * 60 + min;
  return true;
}

static bool
ParseWeekday(const char *in, AirspaceWeekdayPeriod &out) noexcept
{
  char buf[80];
  CopyTruncateString(buf, sizeof(buf), in);

  char *clock = buf;
  while (*clock != 0 && (*clock < '0' || *clock > '9'))
    ++clock;
  if (*clock == 0)
    return false;

  const char *days = nullptr;
  if (clock != buf) {
    if (clock[-1] != ' ')
      return false;
    clock[-1] = 0;
    days = buf;
  }

  char *slash = std::strchr(clock, '/');
  if (slash == nullptr || std::strchr(slash + 1, ' ') != nullptr)
    return false;

  *slash = 0;
  unsigned start_minute, end_minute;
  out = {};
  if (days != nullptr && !ParseWeekdayMask(days, out.days))
    return false;
  if (!ReadClock(clock, start_minute, false) ||
      !ReadClock(slash + 1, end_minute, true))
    return false;

  out.start_minute = start_minute;
  out.end_minute = end_minute;
  return true;
}

static bool
LooksLikeInterval(std::string_view text) noexcept
{
  /* NONE/end is an interval. A weekday name such as SAT also
     contains a T, so the ISO mark has to follow a digit. */
  if (text.size() >= 5 &&
      StringIsEqualIgnoreCase(text.substr(0, 5), "NONE/"sv))
    return true;

  for (std::size_t i = 1; i < text.size(); ++i)
    if ((text[i] == 'T' || text[i] == 't') &&
        text[i - 1] >= '0' && text[i - 1] <= '9')
      return true;

  return false;
}

static AirspaceActivationDecode
ParseActivationTime(const char *in, ParsedActivation &parsed) noexcept
{
  std::string_view text(in);
  while (!text.empty() && text.front() == ' ')
    text.remove_prefix(1);

  /* NONE means no activation time. The airspace is kept, inactive. */
  if (StringIsEqualIgnoreCase(text, "NONE"sv))
    return AirspaceActivationDecode::NEVER_ACTIVE;

  /* EXCEPT, a date range and sun events fail below and stay active. */
  if (LooksLikeInterval(text))
    return ParseInterval(in, parsed.interval);

  return ParseWeekday(in, parsed.weekday)
             ? AirspaceActivationDecode::WEEKDAY
             : AirspaceActivationDecode::UNDETECTABLE_CONTENT;
}

[[gnu::pure]]
static bool
IsICAOClassOrUnclassified(AirspaceClass asclass) noexcept
{
  auto it = std::find(std::begin(airspace_ICAO_and_Unclassified),
                      std::end(airspace_ICAO_and_Unclassified), asclass);
  return  it != std::end(airspace_ICAO_and_Unclassified);
}

[[gnu::pure]]
static std::string_view
ReadRadioFrequency(const std::string_view line) noexcept
{
  const auto [frq, _] = Split(line, ' ');
  return frq;
}

/**
 * Throws on error.
 */
static void
ParseLine(Airspaces &airspace_database, unsigned line_number,
          StringParser<> &&input,
          StringConverter &string_converter,
          TempAirspace &temp_area)
{
  // Only return expected lines
  switch (input.pop_front()) {
  case 'D':
  case 'd':
    switch (input.pop_front()) {
    case 'P':
    case 'p':
      if (!input.SkipWhitespace())
        break;

      temp_area.points.push_back(ReadCoords(input));
      break;

    case 'C':
    case 'c':
      temp_area.radius = ParseRadiusNM(input);
      temp_area.AddCircle(airspace_database);
      temp_area.Reset(line_number);
      break;

    case 'A':
    case 'a':
      ParseArcBearings(input, temp_area);
      break;

    case 'B':
    case 'b':
      ParseArcPoints(input, temp_area);
      break;
    }
    break;

  case 'V':
  case 'v':
    input.Strip();
    if (input.SkipMatchIgnoreCase("X="sv)) {
      temp_area.center = ReadCoords(input);
    } else if (input.SkipMatchIgnoreCase("D=-"sv)) {
      temp_area.rotation = -1;
    } else if (input.SkipMatchIgnoreCase("D=+"sv)) {
      temp_area.rotation = +1;
    }
    break;

  case 'A':
  case 'a':
    switch (input.pop_front()) {
    case 'A':
    case 'a':
      if (input.SkipWhitespace()) {
        temp_area.has_activation_time = true;
        ParsedActivation parsed;
        const auto decoded = ParseActivationTime(input.c_str(), parsed);
        switch (decoded) {
        case AirspaceActivationDecode::INTERVAL:
        case AirspaceActivationDecode::WEEKDAY:
          if (temp_area.never_active)
            break;
          if (temp_area.activation_intervals.size() +
                  temp_area.activation_weekdays.size() >=
              TempAirspace::MAX_ACTIVATION_INTERVALS) {
            /* shortcut: extra lines must not hide the airspace. */
            temp_area.activation_unreadable = true;
            LogFormat("Airspace %s: too many activation times",
                      temp_area.name.c_str());
            break;
          }
          if (decoded == AirspaceActivationDecode::INTERVAL)
            temp_area.activation_intervals.push_back(parsed.interval);
          else
            temp_area.activation_weekdays.push_back(parsed.weekday);
          break;

        case AirspaceActivationDecode::NEVER_ACTIVE:
          temp_area.never_active = true;
          temp_area.activation_unreadable = false;
          temp_area.activation_intervals.clear();
          temp_area.activation_weekdays.clear();
          break;

        case AirspaceActivationDecode::UNDETECTABLE_CONTENT:
          if (temp_area.never_active)
            break;
          /* An unreadable time must not silence the airspace. */
          temp_area.activation_unreadable = true;
          LogFormat("Airspace %s: Activation Time(s) not plausible : %s",
                    temp_area.name.c_str(), input.c_str());
          break;
        }
      }
      break;

    case 'C':
    case 'c':
      if (!input.SkipWhitespace())
        break;

      if (temp_area.Commit(airspace_database))
        temp_area.Reset(line_number);

      temp_area.asclass = ParseClass(input.c_str());
      break;

    case 'N':
    case 'n':
      if (input.SkipWhitespace())
        temp_area.name = string_converter.Convert(input.c_str());
      break;

    case 'L':
    case 'l':
      if (input.SkipWhitespace())
        temp_area.base = ReadAltitude(input);
      break;

    case 'H':
    case 'h':
      if (input.SkipWhitespace())
        temp_area.top = ReadAltitude(input);
      break;

    case 'Y':
    case 'y':
      if (input.SkipWhitespace())
        if (IsICAOClassOrUnclassified(temp_area.asclass))
          temp_area.astype = ParseType(input.c_str());
      break;

    /** 'AR 999.999 or 'AF 999.999' in accordance with the Naviter change proposed in 2018 - (Find 'Additional OpenAir fields' here) http://www.winpilot.com/UsersGuide/UserAirspace.asp **/
    case 'R':
    case 'r':
    case 'F':
    case 'f':
      if (input.SkipWhitespace())
        temp_area.radio_frequency = RadioFrequency::Parse(ReadRadioFrequency(input.c_str()));
      break;

    case 'G':
    case 'g':
      if (input.SkipWhitespace())
        temp_area.station_name = string_converter.Convert(input.c_str());
      break;

    case 'X':
    case 'x':
      if (input.SkipWhitespace()) {
        std::string tempString = std::string(
            string_converter.Convert(input.c_str())); // Convert to std::string
        temp_area.transponder_code =
            TransponderCode::Parse(tempString.c_str());
      }
      break;
    }

    break;
  }
}

/**
 * Throws on error.
 */
static void
ParseLine(Airspaces &airspace_database, unsigned line_number, char *line,
          StringConverter &string_converter,
          TempAirspace &temp_area)
{
  // Strip comments
  auto *comment = StringFind(line, '*');
  if (comment != nullptr)
    *comment = '\0';

  ParseLine(airspace_database, line_number, StringParser<>{line},
            string_converter,
            temp_area);
}

[[gnu::pure]]
static AirspaceClass
ParseClassTNP(const char *buffer) noexcept
{
  for (unsigned i = 0; i < ARRAY_SIZE(airspace_tnp_class_chars); i++)
    if (buffer[0] == airspace_tnp_class_chars[i].character)
      return airspace_tnp_class_chars[i].asclass;

  return OTHER;
}

[[gnu::pure]]
static AirspaceClass
ParseTypeTNP(const char *buffer) noexcept
{
  // Handle e.g. "CLASS=CLASS C" properly
  const char *asclass = StringAfterPrefixIgnoreCase(buffer, "CLASS "sv);
  if (asclass) {
    AirspaceClass _class = ParseClassTNP(asclass);
    if (_class != OTHER)
      return _class;
  } else {
    asclass = buffer;
  }

  for (unsigned i = 0; i < ARRAY_SIZE(airspace_tnp_type_strings); i++)
    if (StringIsEqualIgnoreCase(asclass, airspace_tnp_type_strings[i].string))
      return airspace_tnp_type_strings[i].asclass;

  return OTHER;
}

/**
 * Throws on error.
 */
static Angle
ReadNonNegativeAngleTNP(StringParser<> &input, unsigned max_degrees)
{
  unsigned deg, min, sec;

  if (auto _sec = input.ReadUnsigned())
    sec = *_sec;
  else
    throw std::runtime_error("Bad angle");

  deg = sec / 10000;
  min = (sec - deg * 10000) / 100;
  sec = sec - min * 100 - deg * 10000;

  if (deg > max_degrees || min >= 60 || sec >= 60)
    throw std::runtime_error("Bad angle");

  return Angle::DMS(deg, min, sec);
}

/**
 * Throws on error.
 */
static GeoPoint
ParseCoordsTNP(StringParser<> &input)
{
  GeoPoint point;
  // Format: N542500 E0105000
  bool negative = false;

  if (input.SkipMatch('S') || input.SkipMatch('s'))
    negative = true;
  else if (input.SkipMatch('N') || input.SkipMatch('n'))
    negative = false;
  else
    throw std::runtime_error("N or S expected");

  point.latitude = ReadNonNegativeAngleTNP(input, 91);
  if (negative)
    point.latitude.Flip();

  input.Strip();

  if (input.SkipMatch('W') || input.SkipMatch('w'))
    negative = true;
  else if (input.SkipMatch('E') || input.SkipMatch('e'))
    negative = false;
  else
    throw std::runtime_error("W or E expected");

  point.longitude = ReadNonNegativeAngleTNP(input, 181);
  if (negative)
    point.longitude.Flip();

  point.Normalize(); // ensure longitude is within -180:180
  return point;
}

/**
 * Throws on error.
 */
static void
ParseArcTNP(StringParser<> &input, TempAirspace &temp_area)
{
  if (temp_area.points.empty())
    throw std::runtime_error("Arc on empty airspace");

  // (ANTI-)CLOCKWISE RADIUS=34.95 CENTRE=N523333 E0131603 TO=N522052 E0122236

  GeoPoint from = temp_area.points.back();

  /* skip "RADIUS=... " */
  if (!input.SkipWord())
    throw std::runtime_error("Arc syntax error");

  if (!input.SkipMatchIgnoreCase("CENTRE="sv))
    throw std::runtime_error("CENTRE=... expected");

  temp_area.center = ParseCoordsTNP(input);

  if (!input.SkipMatchIgnoreCase(" TO="sv))
    throw std::runtime_error("TO=... expected");

  GeoPoint to = ParseCoordsTNP(input);

  temp_area.AppendArc(from, to);
}

/**
 * Throws on error.
 */
static void
ParseCircleTNP(StringParser<> &input, TempAirspace &temp_area)
{
  // CIRCLE RADIUS=17.00 CENTRE=N533813 E0095943

  if (!input.SkipMatchIgnoreCase("RADIUS="sv))
    throw std::runtime_error("RADIUS=... expected");

  temp_area.radius = ParseRadiusNM(input);

  if (!input.SkipMatchIgnoreCase(" CENTRE="sv))
    throw std::runtime_error("CENTRE=... expected");

  temp_area.center = ParseCoordsTNP(input);
}

/**
 * Throws on error.
 */
static void
ParseLineTNP(Airspaces &airspace_database, unsigned line_number,
             StringParser<> &input,
             StringConverter &string_converter,
             TempAirspace &temp_area, bool &ignore)
{
  if (input.Match('#'))
    return;

  if (input.SkipMatchIgnoreCase("INCLUDE="sv)) {
    if (input.MatchIgnoreCase("YES"sv))
      ignore = false;
    else if (input.MatchIgnoreCase("NO"sv))
      ignore = true;

    return;
  }

  if (ignore)
    return;

  if (input.SkipMatchIgnoreCase("POINT="sv)) {
    temp_area.points.push_back(ParseCoordsTNP(input));
  } else if (input.SkipMatchIgnoreCase("CIRCLE "sv)) {
    ParseCircleTNP(input, temp_area);

    temp_area.AddCircle(airspace_database);
    temp_area.ResetTNP(line_number);
  } else if (input.SkipMatchIgnoreCase("CLOCKWISE "sv)) {
    temp_area.rotation = 1;
    ParseArcTNP(input, temp_area);
  } else if (input.SkipMatchIgnoreCase("ANTI-CLOCKWISE "sv)) {
    temp_area.rotation = -1;
    ParseArcTNP(input, temp_area);
  } else if (input.SkipMatchIgnoreCase("TITLE="sv)) {
    if (temp_area.Commit(airspace_database))
      temp_area.ResetTNP(line_number);

    temp_area.name = string_converter.Convert(input.c_str());
  } else if (input.SkipMatchIgnoreCase("TYPE="sv)) {
    if (temp_area.Commit(airspace_database))
      temp_area.ResetTNP(line_number);

    temp_area.asclass = ParseTypeTNP(input.c_str());
  } else if (input.SkipMatchIgnoreCase("CLASS="sv)) {
    temp_area.asclass = ParseClassTNP(input.c_str());
  } else if (input.SkipMatchIgnoreCase("TOPS="sv)) {
    temp_area.top = ReadAltitude(input);
  } else if (input.SkipMatchIgnoreCase("BASE="sv)) {
    temp_area.base = ReadAltitude(input);
  } else if (input.SkipMatchIgnoreCase("RADIO="sv)) {
    temp_area.radio_frequency = RadioFrequency::Parse(ReadRadioFrequency(input.c_str()));
  } else if (input.SkipMatchIgnoreCase("ACTIVE="sv)) {
    if (input.MatchAllIgnoreCase("WEEKEND"))
      temp_area.days_of_operation.SetWeekend();
    else if (input.MatchAllIgnoreCase("WEEKDAY"))
      temp_area.days_of_operation.SetWeekdays();
    else if (input.MatchAllIgnoreCase("EVERYDAY"))
      temp_area.days_of_operation.SetAll();
  }
}

static AirspaceFileType
DetectFileType(const char *line) noexcept
{
  if (StringStartsWithIgnoreCase(line, "INCLUDE=") ||
      StringStartsWithIgnoreCase(line, "TYPE=") ||
      StringStartsWithIgnoreCase(line, "TITLE="))
    return AirspaceFileType::TNP;

  const char *p = StringAfterPrefixIgnoreCase(line, "AC"sv);
  if (p != nullptr && (StringIsEmpty(p) || *p == ' '))
    return AirspaceFileType::OPENAIR;

  return AirspaceFileType::UNKNOWN;
}

void
ParseAirspaceFile(Airspaces &airspaces,
                  BufferedReader &reader,
                  AirspaceOrigin origin)
{
  StringConverter string_converter;

  bool ignore = false;

  TempAirspace temp_area;
  temp_area.origin = origin;
  AirspaceFileType filetype = AirspaceFileType::UNKNOWN;

  char *line;

  // Iterate through the lines
  while ((line = reader.ReadLine()) != nullptr) {
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
    try {
      if (filetype == AirspaceFileType::OPENAIR)
        ParseLine(airspaces, reader.GetLineNumber(), line,
                  string_converter, temp_area);
      if (filetype == AirspaceFileType::TNP) {
        StringParser<> input(line);
        ParseLineTNP(airspaces, reader.GetLineNumber(), input, string_converter,
                     temp_area, ignore);
      }
    } catch (const TempAirspace::CommitError &e) {
      throw FmtRuntimeError("Error in airspace at line {}: {}",
                            temp_area.first_line_number, e.msg);
    } catch (...) {
      // TODO translate this?
      std::throw_with_nested(FmtRuntimeError("Error in line {} ('{}')",
                                             reader.GetLineNumber(),
                                             line));
    }
  }

  if (filetype == AirspaceFileType::UNKNOWN)
    throw std::runtime_error(_("Unknown airspace filetype"));

  // Process final area (if any)
  temp_area.Commit(airspaces);
}
