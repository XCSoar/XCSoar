// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Forecast.hpp"
#include "io/Reader.hxx"
#include "io/MemoryReader.hxx"
#include "lib/zlib/GunzipReader.hxx"
#include "time/BrokenDate.hpp"
#include "util/DecimalParser.hxx"

#include <fmt/format.h>

#include <cstdint>
#include <string>

namespace {

/**
 * Enough for the timesteps and the first few forecast elements of a
 * MOSMIX document, which is all this reads.  A document that has not
 * shown its maximum temperature by then is not one we understand, and
 * carrying on would only trade memory for nothing.
 */
constexpr std::size_t READ_LIMIT = 256 * 1024;

constexpr std::size_t CHUNK = 16 * 1024;

/**
 * A document read on demand: it pulls from the stream only when a
 * search runs past what it already holds, so a marker found early
 * leaves the rest of the file unread.
 */
class LazyDocument {
  Reader &reader;
  std::string data;
  bool eof = false;

public:
  explicit LazyDocument(Reader &_reader) noexcept:reader(_reader) {}

  const std::string &Data() const noexcept {
    return data;
  }

  /**
   * Read one more chunk.
   *
   * @return false at end of stream or once #READ_LIMIT is reached
   */
  bool Grow() {
    if (eof || data.size() >= READ_LIMIT)
      return false;

    const std::size_t before = data.size();
    data.resize(before + CHUNK);

    std::size_t n;
    try {
      n = reader.Read(std::as_writable_bytes(std::span{data}.subspan(before)));
    } catch (...) {
      /* A stream that stops in the middle is expected here: the
         forecast may be fetched as a byte range, which leaves the
         deflate stream without its end.  Everything read so far
         stands, and if the maximum was not among it the parse below
         simply finds nothing. */
      data.resize(before);
      eof = true;
      return false;
    }

    data.resize(before + n);

    if (n == 0) {
      eof = true;
      return false;
    }

    return true;
  }

  /**
   * Find @p needle at or after @p from, reading more of the stream
   * while it is not there yet.
   */
  std::size_t Find(std::string_view needle, std::size_t from) {
    while (true) {
      const auto i = data.find(needle, from);
      if (i != data.npos)
        return i;

      /* a marker may straddle the end of what we hold, so the next
         search has to start a little before it */
      from = data.size() >= needle.size()
        ? data.size() - needle.size() + 1
        : 0;

      if (!Grow())
        return data.npos;
    }
  }

  /** Make sure at least @p n bytes are held, if the stream has them. */
  bool FillTo(std::size_t n) {
    while (data.size() < n)
      if (!Grow())
        return data.size() >= n;

    return true;
  }
};

/**
 * The timestep the DWD stamps on the daytime maximum, e.g.
 * "2026-09-20T18:00:00.000Z".
 */
/* not noexcept: fmt::format() allocates, so it can throw
   std::bad_alloc, and terminating on that would be worse than letting
   the caller's error handling see it */
std::string
MakeTimeStep(const BrokenDate &date)
{
  return fmt::format("{:04}-{:02}-{:02}T{:02}:00:00.000Z",
                     date.year, date.month, date.day,
                     MOSMIX::MAXIMUM_TEMPERATURE_HOUR);
}

} // anonymous namespace

std::optional<Temperature>
MOSMIX::ReadForecastMaximum(Reader &reader, const BrokenDate &date)
{
  if (!date.IsPlausible())
    return std::nullopt;

  LazyDocument doc{reader};

  /* which timestep carries the daytime maximum?  The values below are
     a bare list in this order, so its position is all we need. */
  const auto wanted = MakeTimeStep(date);

  static constexpr std::string_view STEP_OPEN = "<dwd:TimeStep>";
  std::size_t index = 0;
  std::size_t pos = 0;
  bool found = false;

  while (true) {
    const auto open = doc.Find(STEP_OPEN, pos);
    if (open == std::string::npos)
      break;

    const auto value = open + STEP_OPEN.size();
    if (!doc.FillTo(value + wanted.size()))
      break;

    if (doc.Data().compare(value, wanted.size(), wanted) == 0) {
      found = true;
      break;
    }

    ++index;
    pos = value;
  }

  if (!found)
    /* the run is too old to reach this day, or too new to still cover
       it */
    return std::nullopt;

  /* now the element itself */
  const auto element =
    fmt::format("dwd:elementName=\"{}\"", MAXIMUM_TEMPERATURE_ELEMENT);
  const auto at = doc.Find(element, 0);
  if (at == std::string::npos)
    return std::nullopt;

  static constexpr std::string_view VALUE_OPEN = "<dwd:value>";
  const auto value_open = doc.Find(VALUE_OPEN, at);
  if (value_open == std::string::npos)
    return std::nullopt;

  /* skip to the value at the timestep's position; the DWD writes "-"
     where it states nothing, which is most of them */
  std::size_t i = value_open + VALUE_OPEN.size();
  for (std::size_t n = 0;; ++n) {
    while (true) {
      if (!doc.FillTo(i + 1))
        return std::nullopt;
      const char ch = doc.Data()[i];
      if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
        break;
      ++i;
    }

    const auto &data = doc.Data();
    if (data[i] == '<')
      /* the list ended before the timestep we want */
      return std::nullopt;

    std::size_t end = i;
    while (true) {
      if (!doc.FillTo(end + 1))
        return std::nullopt;
      const char ch = doc.Data()[end];
      if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '<')
        break;
      ++end;
    }

    if (n == index) {
      const auto token = std::string_view{doc.Data()}.substr(i, end - i);
      if (token == "-")
        /* the element is stated only twice a day; this is one of the
           timesteps in between */
        return std::nullopt;

      /* ParseDecimal() rather than strtod(): it takes no "nan" and
         no "inf".  That matters here, because -ffast-math lets the
         compiler assume neither can occur, so a check on the parsed
         value would not have caught them. */
      const auto kelvin = ParseDecimal(token);
      if (!kelvin || *kelvin <= 0)
        return std::nullopt;

      return Temperature::FromKelvin(*kelvin);
    }

    i = end;
  }
}

std::optional<Temperature>
MOSMIX::ReadForecastMaximumFromKmz(std::span<const std::byte> kmz,
                                   const BrokenDate &date) noexcept
try {
  /* A ZIP local file header: the signature, the compression method at
     offset 8, and at offsets 26 and 28 the lengths of the name and
     extra fields, after which the entry's stream begins.  The archive
     holds one member, so this is all that is needed to find it -- the
     central directory at the end of the file says the same thing
     about the same single entry. */
  static constexpr std::size_t LOCAL_HEADER = 30;
  if (kmz.size() <= LOCAL_HEADER)
    return std::nullopt;

  const auto *p = reinterpret_cast<const uint8_t *>(kmz.data());
  if (p[0] != 'P' || p[1] != 'K' || p[2] != 3 || p[3] != 4)
    return std::nullopt;

  if (unsigned(p[8] | (p[9] << 8)) != 8)
    /* stored, or something we do not inflate */
    return std::nullopt;

  const std::size_t offset = LOCAL_HEADER +
    std::size_t(p[26] | (p[27] << 8)) + std::size_t(p[28] | (p[29] << 8));
  if (offset >= kmz.size())
    return std::nullopt;

  MemoryReader memory{kmz.subspan(offset)};
  GunzipReader inflate{memory, true};
  return ReadForecastMaximum(inflate, date);
} catch (...) {
  return std::nullopt;
}

std::string
MOSMIX::MakeForecastURL(std::string_view station_id)
{
  return fmt::format("https://opendata.dwd.de/weather/local_forecasts/mos"
                     "/MOSMIX_L/single_stations/{}/kml"
                     "/MOSMIX_L_LATEST_{}.kmz",
                     station_id, station_id);
}
