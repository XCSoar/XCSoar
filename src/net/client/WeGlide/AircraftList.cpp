// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AircraftList.hpp"
#include "AircraftCache.hpp"
#include "Settings.hpp"
#include "LogFile.hpp"
#include "util/StringCompare.hxx"

#ifdef HAVE_HTTP
#include "Error.hpp"
#include "json/ParserOutputStream.hxx"
#include "net/http/Progress.hpp"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/fmt/ToBuffer.hxx"
#endif

#include <boost/json.hpp>

#include <algorithm>
#include <exception>
#include <string_view>

using std::string_view_literals::operator""sv;

namespace WeGlide {

static std::vector<AircraftType>
ParseAircraftList(const boost::json::value &value)
{
  std::vector<AircraftType> list;
  if (!value.is_array())
    return list;

  list.reserve(value.as_array().size());
  for (const auto &entry : value.as_array()) {
    if (!entry.is_object())
      continue;

    const auto &obj = entry.as_object();
    auto *id = obj.if_contains("id"sv);
    auto *name = obj.if_contains("name"sv);
    if (id == nullptr || name == nullptr ||
        !id->is_number() || !name->is_string())
      continue;

    AircraftType item;
    item.id = id->to_number<uint32_t>();
    item.name = name->as_string().c_str();
    list.push_back(item);
  }

  std::sort(list.begin(), list.end(),
            [](const auto &a, const auto &b){
              return StringCollate(a.name, b.name) < 0;
            });
  return list;
}

std::vector<AircraftType>
LoadAircraftListCache()
{
  boost::json::value value;
  if (!LoadAircraftListCacheValue(value))
    return {};

  return ParseAircraftList(value);
}

bool
LookupAircraftTypeName(unsigned aircraft_id,
                       StaticString<96> &name)
{
  const auto list = LoadAircraftListCache();
  auto i = std::find_if(list.begin(), list.end(),
                        [aircraft_id](const auto &item){
                          return item.id == aircraft_id;
                        });
  if (i == list.end())
    return false;

  name = i->name;
  return true;
}

#ifdef HAVE_HTTP

Co::Task<std::vector<AircraftType>>
DownloadAircraftList(::CurlGlobal &curl, const WeGlideSettings &settings,
                     ProgressListener &progress)
{
  const auto url = FmtBuffer<256>("{}/aircraft", settings.default_url);
  CurlEasy easy{url};

  Curl::Setup(easy);
  easy.SetTimeout(25);
  const Net::ProgressAdapter progress_adapter{easy, progress};

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  auto body = parser.Finish();

  if (response.status != 200)
    throw ResponseToException(response.status, body);

  auto parsed = ParseAircraftList(body);
  if (!parsed.empty()) {
    try {
      StoreAircraftListCacheValue(body);
    } catch (const std::exception &e) {
      LogFmt("WeGlide: failed to update aircraft cache: {}", e.what());
    }
  }

  co_return parsed;
}

#endif

} // namespace WeGlide
