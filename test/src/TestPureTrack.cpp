// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Tracking/PureTrack/Client.hpp"
#include "Tracking/PureTrack/Protocol.hpp"
#include "Tracking/PureTrack/Settings.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

#include <boost/json.hpp>

#include <chrono>
#include <string_view>

static long long
JsonInt(const boost::json::value &value) noexcept
{
  if (value.is_int64())
    return value.as_int64();
  if (value.is_uint64())
    return (long long)value.as_uint64();
  return (long long)value.as_double();
}

static boost::json::object
InsertBody(PureTrack::Settings &settings)
{
  PureTrack::Sample sample;
  sample.timestamp = std::chrono::system_clock::from_time_t(1713563621);
  sample.location = GeoPoint(Angle::Degrees(174.348365),
                             Angle::Degrees(-41.2334745));
  sample.altitude = -5;
  sample.speed = 25;
  sample.course = 270;
  sample.vertical_speed = 1.5;

  return boost::json::parse(PureTrack::BuildInsertRequestBody(settings,
                                                              sample))
    .as_object();
}

int
main()
{
  plan_tests(27);

  PureTrack::Settings settings;
  settings.SetDefaults();
  settings.app_key = "app-key";
  settings.device_id = "d7ry390";
  settings.label = "Nimbus";
  settings.vehicle_type = PureTrack::Settings::VehicleType::PARAGLIDER;

  const auto body = InsertBody(settings);
  ok1(std::string_view{body.at("deviceID").as_string()} == "d7ry390");
  ok1(std::string_view{body.at("key").as_string()} == "app-key");
  ok1(std::string_view{body.at("label").as_string()} == "Nimbus");
  ok1(body.find("rego") == body.end());

  const auto &point = body.at("points").as_array().at(0).as_object();
  ok1(JsonInt(point.at("ts")) == 1713563621);
  ok1(equals(point.at("lat").as_double(), -41.2334745));
  ok1(equals(point.at("lng").as_double(), 174.348365));
  ok1(equals(point.at("alt").as_double(), -5));
  ok1(equals(point.at("speed").as_double(), 25));
  ok1(equals(point.at("course").as_double(), 270));
  ok1(equals(point.at("vspeed").as_double(), 1.5));

  settings.label.clear();
  const auto unlabeled = InsertBody(settings);
  ok1(unlabeled.find("label") == unlabeled.end());

  const PureTrack::Settings::VehicleType types[] = {
    PureTrack::Settings::VehicleType::GLIDER,
    PureTrack::Settings::VehicleType::PARAGLIDER,
    PureTrack::Settings::VehicleType::POWERED_AIRCRAFT,
    PureTrack::Settings::VehicleType::HOT_AIR_BALLOON,
    PureTrack::Settings::VehicleType::HANGGLIDER_FLEX,
    PureTrack::Settings::VehicleType::HANGGLIDER_RIGID,
    PureTrack::Settings::VehicleType::COUNT,
  };
  const unsigned ids[] = { 1, 7, 8, 11, 6, 6, 1 };
  static_assert(ARRAY_SIZE(types) == ARRAY_SIZE(ids));
  for (unsigned i = 0; i < ARRAY_SIZE(types); ++i) {
    settings.vehicle_type = types[i];
    ok1(JsonInt(InsertBody(settings).at("type")) == ids[i]);
  }

  const char *response =
    R"({"success":true,"http_code":200,"trackers_received":1,)"
    R"("total_points_recieved":2,"total_points_inserted":2})";
  const auto parsed =
    PureTrack::ParseInsertResponse(200, boost::json::parse(response));
  ok1(parsed.success);
  ok1(parsed.http_code == 200);
  ok1(parsed.trackers_received == 1);
  ok1(parsed.total_points_received == 2);
  ok1(parsed.total_points_inserted == 2);

  const auto empty = PureTrack::ParseInsertResponse(500,
                                                    boost::json::parse("[]"));
  ok1(!empty.success);
  ok1(empty.http_code == 500);

  const auto error = PureTrack::ResponseToException(
    403, boost::json::parse(R"({"message":"nope"})"));
  ok1(StringFind(error.what(), "nope") != nullptr);

  return exit_status();
}
