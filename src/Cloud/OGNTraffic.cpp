// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNTraffic.hpp"
#include "FLARM/Traffic.hpp"
#include "Geo/Boost/RangeBox.hpp"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"
#include "util/tstring.hpp"

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <tchar.h>

OGNTrafficContainer::OGNTrafficContainer() = default;

OGNTrafficContainer::~OGNTrafficContainer() { clear(); }

void
OGNTrafficContainer::clear()
{
  while (!list.empty()) {
    auto &traffic = list.back();
    list.pop_back();
    id_set.erase(id_set.iterator_to(traffic));
    rtree.remove(traffic.shared_from_this());
    device_map.erase(traffic.device_id);
  }
}

OGNTraffic *
OGNTrafficContainer::Find(const tstring &device_id)
{
  auto i = device_map.find(device_id);
  return i != device_map.end() ? i->second.get() : nullptr;
}

OGNTraffic *
OGNTrafficContainer::FindById(unsigned id)
{
  auto i = id_set.find(id, OGNTraffic::IdCompare());
  return i != id_set.end() ? const_cast<OGNTraffic *>(&*i) : nullptr;
}

OGNTraffic &
OGNTrafficContainer::Make(const tstring &device_id, const GeoPoint &location,
                          int altitude, unsigned track, unsigned speed,
                          int climb_rate, double turn_rate,
                          FlarmTraffic::AircraftType aircraft_type)
{
  auto i = device_map.find(device_id);
  if (i != device_map.end()) {
    auto &traffic = *i->second;

    // Update existing
    if (location != traffic.location) {
      auto ptr = traffic.shared_from_this();
      rtree.remove(ptr);
      traffic.Update(location, altitude, track, speed, climb_rate, turn_rate,
                     aircraft_type);
      rtree.insert(ptr);
    } else {
      traffic.Update(location, altitude, track, speed, climb_rate, turn_rate,
                     aircraft_type);
    }

    // Move to front of list
    list.erase(list.iterator_to(traffic));
    list.push_front(traffic);

    return traffic;
  }

  // Extract FlarmId from OGN device ID (for FLARM database matching)
  // OGN uses prefixes: FLR, ICA, PAW, DD, OGN followed by hex digits
  // Examples: "FLR123456", "ICA40697B", "PAW407B1B", "DD1234", "OGN1234"
  unsigned flarm_id = 0;
  const TCHAR *id_str = device_id.c_str();

  // Check for known prefixes and extract hex part
  if (StringStartsWith(id_str, _T("FLR"))) {
    // FLARM device: "FLR123456" -> parse "123456" as hex
#ifdef _UNICODE
    wchar_t *endptr = nullptr;
    const wchar_t *hex_start = id_str + 3;
#else
    char *endptr = nullptr;
    const char *hex_start = id_str + 3;
#endif
    unsigned parsed = ParseUnsigned(hex_start, &endptr, 16);
    if (endptr != nullptr && endptr > hex_start && parsed != 0)
      flarm_id = parsed;
  } else if (StringStartsWith(id_str, _T("ICA"))) {
    // ICAO/ADS-B: "ICA40697B" -> parse "40697B" as hex
#ifdef _UNICODE
    wchar_t *endptr = nullptr;
    const wchar_t *hex_start = id_str + 3;
#else
    char *endptr = nullptr;
    const char *hex_start = id_str + 3;
#endif
    unsigned parsed = ParseUnsigned(hex_start, &endptr, 16);
    if (endptr != nullptr && endptr > hex_start && parsed != 0)
      flarm_id = parsed;
  } else if (StringStartsWith(id_str, _T("PAW"))) {
    // PilotAware device: "PAW407B1B" -> parse "407B1B" as hex (ICAO 24-bit
    // address)
#ifdef _UNICODE
    wchar_t *endptr = nullptr;
    const wchar_t *hex_start = id_str + 3;
#else
    char *endptr = nullptr;
    const char *hex_start = id_str + 3;
#endif
    unsigned parsed = ParseUnsigned(hex_start, &endptr, 16);
    if (endptr != nullptr && endptr > hex_start && parsed != 0)
      flarm_id = parsed;
  } else if (StringStartsWith(id_str, _T("DD"))) {
    // OGN DD device: "DD1234" -> parse "1234" as hex
#ifdef _UNICODE
    wchar_t *endptr = nullptr;
    const wchar_t *hex_start = id_str + 2;
#else
    char *endptr = nullptr;
    const char *hex_start = id_str + 2;
#endif
    unsigned parsed = ParseUnsigned(hex_start, &endptr, 16);
    if (endptr != nullptr && endptr > hex_start && parsed != 0)
      flarm_id = parsed;
  } else if (StringStartsWith(id_str, _T("OGN"))) {
    // OGN device: "OGN1234" -> parse "1234" as hex
#ifdef _UNICODE
    wchar_t *endptr = nullptr;
    const wchar_t *hex_start = id_str + 3;
#else
    char *endptr = nullptr;
    const char *hex_start = id_str + 3;
#endif
    unsigned parsed = ParseUnsigned(hex_start, &endptr, 16);
    if (endptr != nullptr && endptr > hex_start && parsed != 0)
      flarm_id = parsed;
  }

  // Always generate a unique ID for pilot_id (separate from FlarmId)
  unsigned unique_id = next_id++;

  // Create new
  auto traffic = std::make_shared<OGNTraffic>(unique_id, device_id, flarm_id);
  traffic->Update(location, altitude, track, speed, climb_rate, turn_rate,
                  aircraft_type);

  list.push_front(*traffic);
  id_set.insert(*traffic);
  rtree.insert(traffic);
  device_map[device_id] = traffic;

  return *traffic;
}

void
OGNTrafficContainer::Expire(std::chrono::steady_clock::time_point before)
{
  while (!list.empty() && list.back().stamp < before) {
    auto &traffic = list.back();
    auto ptr = traffic.shared_from_this();

    list.pop_back();
    id_set.erase(id_set.iterator_to(traffic));
    rtree.remove(ptr);
    device_map.erase(traffic.device_id);
  }
}

OGNTrafficContainer::query_iterator_range
OGNTrafficContainer::QueryWithinRange(GeoPoint location, double range) const
{
  const auto q =
      boost::geometry::index::intersects(BoostRangeBox(location, range));
  return {rtree.qbegin(q), rtree.qend()};
}
