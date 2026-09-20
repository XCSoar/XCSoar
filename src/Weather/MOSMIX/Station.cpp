// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Station.hpp"
#include "Geo/GeoVector.hpp"
#include "io/LineReader.hpp"
#include "io/FileLineReader.hpp"
#include "system/Path.hpp"
#include "util/DecimalParser.hxx"

#include <cmath>
#include <optional>
#include <string_view>

namespace {

/**
 * The catalogue writes coordinates as degrees and minutes packed into
 * one decimal number: 48.50 is 48 degrees 50 minutes, i.e. 48.833
 * degrees.  Reading it as a decimal fraction would put a station up
 * to half a degree -- about forty kilometres -- from where it stands.
 */
[[gnu::const]]
std::optional<double>
DegreesMinutesToDegrees(double value) noexcept
{
  const double magnitude = std::fabs(value);
  const double degrees = std::floor(magnitude);

  /* the field carries two decimals, so round to that: subtracting the
     degrees leaves a binary fraction that is a hair above or below
     the intended value, and 42.60 would otherwise measure 60.0000001
     minutes and be thrown out by the check below */
  const double minutes = std::round((magnitude - degrees) * 10000) / 100;

  if (minutes > 60)
    /* not a minutes field at all; refuse rather than invent a
       position.  Reading a decimal-degree file as degrees and
       minutes would displace a station by up to forty kilometres,
       and it would look perfectly plausible.

       The refusal is carried beside the number rather than in it:
       XCSoar compiles with -ffast-math, which lets the compiler
       assume no value is ever NaN and quietly folds std::isnan() to
       false (build/debug.mk restores that only for clang).  A NaN
       returned here would have sailed through every check on GCC
       builds. */
    return std::nullopt;

  /* exactly sixty is allowed: the catalogue carries one station
     (37531 GORI) written as 41.60, which is 42.00 un-normalised.  The
     position is unambiguous, so there is no reason to lose it. */
  const double result = degrees + minutes / 60;
  return value < 0 ? -result : result;
}

/**
 * Split off the last whitespace separated field.
 *
 * The catalogue's own separator line understates the column the
 * latitude starts in, and a fixed offset silently drops its last
 * digit.  The three numbers at the end carry no spaces, so taking
 * them from the right is both simpler and proof against that.
 */
bool
SplitLastField(std::string_view &line, std::string_view &field) noexcept
{
  while (!line.empty() && line.back() == ' ')
    line.remove_suffix(1);

  const auto space = line.find_last_of(' ');
  if (space == line.npos)
    return false;

  field = line.substr(space + 1);
  line = line.substr(0, space);
  return !field.empty();
}

} // anonymous namespace

MOSMIX::Station
MOSMIX::ParseStationLine(const char *line) noexcept
{
  Station station;
  station.id.clear();

  if (line == nullptr)
    return station;

  std::string_view rest{line};

  std::string_view elevation, longitude, latitude;
  if (!SplitLastField(rest, elevation) ||
      !SplitLastField(rest, longitude) ||
      !SplitLastField(rest, latitude))
    return station;

  /* ParseDecimal() rather than strtod(): it takes no "nan" and no
     "inf", which matters because -ffast-math lets the compiler assume
     neither can occur and fold any later check away */
  const auto lon = ParseDecimal(longitude);
  const auto lat = ParseDecimal(latitude);
  if (!lon || !lat)
    return station;

  const auto lon_degrees = DegreesMinutesToDegrees(*lon);
  const auto lat_degrees = DegreesMinutesToDegrees(*lat);
  if (!lon_degrees || !lat_degrees ||
      *lat_degrees < -90 || *lat_degrees > 90 ||
      *lon_degrees < -180 || *lon_degrees > 180)
    return station;

  /* the id is the first field, and the name between it and the
     numbers may contain spaces.  Some ids are indented, so the
     leading blanks go first -- taking the text up to the first space
     would otherwise hand back nothing at all. */
  while (!rest.empty() && rest.front() == ' ')
    rest.remove_prefix(1);

  const auto id = rest.substr(0, rest.find(' '));

  if (id.empty() || id.size() >= station.id.capacity())
    return station;

  station.id.SetASCII(id);
  station.location = GeoPoint(Angle::Degrees(*lon_degrees),
                              Angle::Degrees(*lat_degrees));
  return station;
}

std::vector<MOSMIX::Station>
MOSMIX::ReadStationCatalogue(NLineReader &reader)
{
  std::vector<Station> stations;

  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    const auto station = ParseStationLine(line);
    if (station.IsDefined())
      stations.push_back(station);
  }

  return stations;
}

std::vector<MOSMIX::Station>
MOSMIX::ReadStationCatalogue(Path path) noexcept
try {
  FileLineReaderA reader{path};
  return ReadStationCatalogue(reader);
} catch (...) {
  return {};
}

const MOSMIX::Station *
MOSMIX::FindNearestStation(std::span<const Station> stations,
                           const GeoPoint &location) noexcept
{
  if (!location.IsValid())
    return nullptr;

  const Station *best = nullptr;
  double best_distance = 0;

  for (const auto &station : stations) {
    const double distance = station.location.DistanceS(location);
    if (best == nullptr || distance < best_distance) {
      best = &station;
      best_distance = distance;
    }
  }

  return best;
}
