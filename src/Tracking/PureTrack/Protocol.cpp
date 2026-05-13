// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Protocol.hpp"
#include "Settings.hpp"
#include "Client.hpp"

#include "io/StringOutputStream.hxx"
#include "json/Serialize.hxx"
#include "lib/fmt/RuntimeError.hxx"

#include <boost/json.hpp>

using std::string_view_literals::operator""sv;

namespace PureTrack {

static unsigned
MapVehicleType(Settings::VehicleType type) noexcept
{
  switch (type) {
  case Settings::VehicleType::GLIDER:
    return 1;
  case Settings::VehicleType::PARAGLIDER:
    return 7;
  case Settings::VehicleType::POWERED_AIRCRAFT:
    return 8;
  case Settings::VehicleType::HOT_AIR_BALLOON:
    return 11;
  case Settings::VehicleType::HANGGLIDER_FLEX:
  case Settings::VehicleType::HANGGLIDER_RIGID:
    return 6;
  case Settings::VehicleType::COUNT:
    break;
  }

  return 1;
}

static boost::json::object
BuildPoint(const Sample &sample)
{
  boost::json::object point;
  point.emplace("ts",
                (std::int64_t)std::chrono::system_clock::to_time_t(
                  sample.timestamp));
  point.emplace("lat", sample.location.latitude.Degrees());
  point.emplace("lng", sample.location.longitude.Degrees());
  point.emplace("alt", sample.altitude);
  point.emplace("speed", sample.speed);
  point.emplace("course", sample.course);
  point.emplace("vspeed", sample.vertical_speed);

  return point;
}

std::string
BuildInsertRequestBody(const Settings &settings, const Sample &sample)
{
  boost::json::object request;
  request.emplace("key", std::string_view{settings.app_key});
  request.emplace("deviceID", std::string_view{settings.device_id});
  request.emplace("type", MapVehicleType(settings.vehicle_type));

  if (!settings.label.empty())
    request.emplace("label", std::string_view{settings.label});

  if (!settings.rego.empty())
    request.emplace("rego", std::string_view{settings.rego});

  boost::json::array points;
  points.emplace_back(BuildPoint(sample));
  request.emplace("points", std::move(points));

  StringOutputStream os;
  Json::Serialize(os, request);
  return std::move(os).GetValue();
}

static unsigned
GetUnsigned(const boost::json::object &o, std::string_view key) noexcept
{
  auto i = o.find(key);
  if (i == o.end() || !i->value().is_int64())
    return 0;

  const auto value = i->value().as_int64();
  return value >= 0 ? (unsigned)value : 0;
}

InsertResponse
ParseInsertResponse(unsigned status, const boost::json::value &body)
{
  InsertResponse response;
  response.http_code = status;

  if (!body.is_object())
    return response;

  const auto &o = body.as_object();
  if (auto i = o.find("success"sv); i != o.end() && i->value().is_bool())
    response.success = i->value().as_bool();

  response.http_code = GetUnsigned(o, "http_code"sv);
  response.trackers_received = GetUnsigned(o, "trackers_received"sv);
  response.total_points_received = GetUnsigned(o, "total_points_recieved"sv);
  response.total_points_inserted = GetUnsigned(o, "total_points_inserted"sv);
  return response;
}

std::runtime_error
ResponseToException(unsigned status, const boost::json::value &body)
{
  if (!body.is_object())
    return FmtRuntimeError("PureTrack status {}", status);

  const auto &o = body.as_object();

  if (auto i = o.find("message"sv); i != o.end() && i->value().is_string())
    return FmtRuntimeError("PureTrack status {}: {}",
                           status,
                           (std::string_view)i->value().as_string());

  if (auto i = o.find("error"sv); i != o.end() && i->value().is_string())
    return FmtRuntimeError("PureTrack status {}: {}",
                           status,
                           (std::string_view)i->value().as_string());

  return FmtRuntimeError("PureTrack status {}", status);
}

} // namespace PureTrack
