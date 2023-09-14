// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"
#include "Thermal.hpp"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "json/ParserOutputStream.hxx"

#include <stdexcept>

static auto
tag_invoke(boost::json::value_to_tag<GeoPoint>,
           const boost::json::value &jv)
{
  const auto &json = jv.as_object();
  GeoPoint gp{
    Angle::Degrees(json.at("lon").to_number<double>()),
    Angle::Degrees(json.at("lat").to_number<double>()),
  };
  if (!gp.Check())
    throw std::runtime_error("Invalid location");
  return gp;
}

namespace TIM {

static auto
tag_invoke(boost::json::value_to_tag<Thermal>,
           const boost::json::value &jv)
{
  const auto &json = jv.as_object();
  Thermal thermal;
  thermal.time = std::chrono::system_clock::from_time_t(json.at("time").to_number<uint64_t>());
  thermal.location = boost::json::value_to<GeoPoint>(jv);
  thermal.climb_rate = json.at("climbRate").to_number<double>();
  return thermal;
}

Co::Task<std::vector<Thermal>>
GetThermals(CurlGlobal &curl, std::chrono::hours max_age,
            GeoPoint location, unsigned radius)
{
  assert(location.IsValid());
  assert(radius > 0);

  char url[256];
  snprintf(url, sizeof(url),
           "https://www.thermalmap.info/api/live/thermals/?maxAge=%u&lat=%f&lon=%f&radius=%u",
           (unsigned)max_age.count(),
           location.latitude.Degrees(),
           location.longitude.Degrees(),
           radius);

  CurlEasy easy{url};
  Curl::Setup(easy);
  easy.SetFailOnError();

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);

  co_return boost::json::value_to<std::vector<Thermal>>(parser.Finish());
}

} // namespace TIM
