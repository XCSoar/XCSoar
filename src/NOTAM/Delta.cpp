// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Delta.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <unordered_set>

namespace NOTAMDelta {

bool
IsApiResponseValid(const boost::json::value &value) noexcept
{
  if (!value.is_object())
    return false;

  const auto &obj = value.as_object();
  const auto it = obj.find("items");
  return it != obj.end() && it->value().is_array();
}

NOTAMClient::KnownMap
BuildKnownMap(const std::vector<struct NOTAM> &notams)
{
  NOTAMClient::KnownMap known;
  known.reserve(notams.size());
  for (const auto &notam : notams) {
    if (!notam.id.empty() && !notam.last_updated.empty())
      known.emplace(notam.id, notam.last_updated);
  }
  return known;
}

bool
CanUseDelta(const NOTAMClient::KnownMap &known,
            const boost::json::value & /* cached_api */,
            const bool cached_api_valid,
            const GeoPoint known_location,
            const unsigned known_radius_km,
            const GeoPoint location,
            const unsigned radius_km) noexcept
{
  if (!location.IsValid())
    return false;

  if (!cached_api_valid)
    return false;

  if (known.empty() || !known_location.IsValid())
    return false;

  if (known_radius_km != radius_km)
    return false;

  const double radius_m = radius_km * 1000.0;
  const double distance_m = location.Distance(known_location);
  return distance_m <= (radius_m * 0.5);
}

void
ApplyDeltaUpdates(std::vector<struct NOTAM> &current,
                  const std::vector<struct NOTAM> &updates,
                  const std::vector<std::string> &removed_ids)
{
  std::unordered_set<std::string> removed;
  if (!removed_ids.empty())
    removed.insert(removed_ids.begin(), removed_ids.end());

  if (!updates.empty()) {
    removed.reserve(removed.size() + updates.size());
    for (const auto &update : updates) {
      if (!update.id.empty())
        removed.emplace(update.id);
    }
  }

  if (!removed.empty()) {
    current.erase(std::remove_if(current.begin(), current.end(),
                                 [&removed](const struct NOTAM &notam) {
                                   return !notam.id.empty() &&
                                     removed.contains(notam.id);
                                 }),
                  current.end());
  }

  if (!updates.empty()) {
    current.reserve(current.size() + updates.size());
    for (const auto &update : updates) {
      if (!update.id.empty())
        current.push_back(update);
    }
  }
}

static bool
GetNotamIdFromItem(const boost::json::value &item, std::string &id)
{
  if (!item.is_object())
    return false;

  const auto &obj = item.as_object();
  const auto it_props = obj.find("properties");
  if (it_props == obj.end() || !it_props->value().is_object())
    return false;

  const auto &props = it_props->value().as_object();
  const auto it_core = props.find("coreNOTAMData");
  if (it_core == props.end() || !it_core->value().is_object())
    return false;

  const auto &core = it_core->value().as_object();
  const auto it_notam = core.find("notam");
  if (it_notam == core.end() || !it_notam->value().is_object())
    return false;

  const auto &notam = it_notam->value().as_object();
  const auto it_id = notam.find("id");
  if (it_id == notam.end() || !it_id->value().is_string())
    return false;

  id = it_id->value().as_string().c_str();
  return !id.empty();
}

bool
ApplyDeltaToApi(boost::json::value &api_response,
                const boost::json::value &delta_response,
                const std::vector<std::string> &removed_ids)
{
  if (!api_response.is_object() || !delta_response.is_object())
    return false;

  auto &api_obj = api_response.as_object();
  const auto it_api_items = api_obj.find("items");
  if (it_api_items == api_obj.end() || !it_api_items->value().is_array())
    return false;

  auto &api_items = it_api_items->value().as_array();

  std::unordered_set<std::string> removed;
  if (!removed_ids.empty())
    removed.insert(removed_ids.begin(), removed_ids.end());

  std::unordered_set<std::string> updated;
  const auto &delta_obj = delta_response.as_object();
  const auto it_delta_items = delta_obj.find("items");
  if (it_delta_items != delta_obj.end() &&
      it_delta_items->value().is_array()) {
    const auto &delta_items = it_delta_items->value().as_array();
    updated.reserve(delta_items.size());
    for (const auto &item : delta_items) {
      std::string id;
      if (GetNotamIdFromItem(item, id))
        updated.emplace(std::move(id));
    }
  }

  if (!removed.empty() || !updated.empty()) {
    api_items.erase(std::remove_if(api_items.begin(), api_items.end(),
                                   [&removed, &updated](
                                     const boost::json::value &item) {
                                     std::string id;
                                     return GetNotamIdFromItem(item, id) &&
                                       (removed.contains(id) ||
                                        updated.contains(id));
                                   }),
                    api_items.end());
  }

  if (it_delta_items != delta_obj.end() &&
      it_delta_items->value().is_array()) {
    const auto &delta_items = it_delta_items->value().as_array();
    api_items.reserve(api_items.size() + delta_items.size());
    for (const auto &item : delta_items)
      api_items.emplace_back(item);
  }

  if (api_obj.find("totalCount") != api_obj.end()) {
    api_obj["totalCount"] =
      static_cast<std::uint64_t>(api_items.size());
  }

  return true;
}

} // namespace NOTAMDelta
