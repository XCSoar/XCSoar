// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RangeParser.hpp"
#include "FLARM/Range.hpp"
#include "util/IterableSplitString.hxx"
#include "util/NumberParser.hxx"
#include "util/StringSplit.hxx"

#include <cstdint>

/**
 * Parse a comma-separated list of sector values.  An empty or
 * malformed field becomes an empty value, so the sectors keep their
 * position.
 */
static void
ParseRangeValues(std::string_view src, FlarmRange::Values &dest)
{
  dest.clear();
  for (const std::string_view value : IterableSplitString{src, ','})
    dest.push_back(ParseInteger<unsigned>(value));
}

static std::optional<std::chrono::system_clock::time_point>
ParseUnixTime(std::string_view src) noexcept
{
  const auto seconds = ParseInteger<std::int64_t>(src);
  if (!seconds)
    return std::nullopt;

  using std::chrono::system_clock;
  return system_clock::time_point{std::chrono::seconds{*seconds}};
}

void
ParsePFLANRange(std::string_view fields, FlarmRange &range)
{
  using namespace std::string_view_literals;

  const auto [type, rest] = Split(fields, ',');

  if (type == "STATS"sv) {
    range.points = ParseInteger<unsigned>(rest);
    return;
  }

  if (type == "TIMESPAN"sv) {
    const auto [first, last] = Split(rest, ',');
    range.first = ParseUnixTime(first);
    range.last = ParseUnixTime(last);
    return;
  }

  const auto [channel_name, values] = Split(rest, ',');
  FlarmRange::Channel *channel;
  if (channel_name == "A"sv)
    channel = &range.channels[0];
  else if (channel_name == "B"sv)
    channel = &range.channels[1];
  else
    return;

  if (type == "RFTOP"sv)
    ParseRangeValues(values, channel->mean);
  else if (type == "RFCNT"sv)
    ParseRangeValues(values, channel->count);
  else if (type == "RFDEV"sv)
    ParseRangeValues(values, channel->deviation);
}
