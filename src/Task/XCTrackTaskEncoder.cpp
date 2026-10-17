// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCTrackTaskEncoder.hpp"
#include "PolylineDecoder.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "json/Serialize.hxx"
#include "io/StringOutputStream.hxx"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

#include <cmath>
#include <string>

static int_fast32_t
DegreesE5(const Angle angle) noexcept
{
  return static_cast<int_fast32_t>(std::lround(angle.Degrees() * 1e5));
}

static std::string
EncodeZone(const GeoPoint &location, int altitude, int radius)
{
  std::string z;
  AppendPolylineInt(z, DegreesE5(location.longitude));
  AppendPolylineInt(z, DegreesE5(location.latitude));
  AppendPolylineInt(z, altitude);
  AppendPolylineInt(z, radius);
  return z;
}

std::optional<std::string>
EncodeXCTrackQRText(const OrderedTask &task)
{
  const unsigned n = task.TaskSize();
  if (n < 2)
    return std::nullopt;

  boost::json::array points;
  points.reserve(n);

  for (unsigned i = 0; i < n; ++i) {
    const auto &point = task.GetPoint(i);
    const auto &zone = point.GetObservationZone();
    const auto shape = zone.GetShape();
    if (shape != ObservationZone::Shape::CYLINDER &&
        shape != ObservationZone::Shape::MAT_CYLINDER)
      return std::nullopt;

    const auto &cylinder = static_cast<const CylinderZone &>(zone);
    const auto radius = std::lround(cylinder.GetRadius());
    if (radius <= 0 || radius > 500000)
      return std::nullopt;

    const auto &waypoint = point.GetWaypoint();
    if (waypoint.name.length() == 0)
      return std::nullopt;

    const auto altitude = std::lround(waypoint.GetElevationOrZero());
    /* DecodeXCTrackTask() rejects altitudes outside this range. */
    if (altitude < -1000 || altitude > 9000)
      return std::nullopt;

    boost::json::object turnpoint;
    turnpoint["n"] = waypoint.name.c_str();
    turnpoint["z"] = EncodeZone(waypoint.location,
                                static_cast<int>(altitude),
                                static_cast<int>(radius));
    points.push_back(std::move(turnpoint));
  }

  boost::json::object root;
  root["taskType"] = "CLASSIC";
  root["version"] = 2;
  root["t"] = std::move(points);

  StringOutputStream json;
  Json::Serialize(json, boost::json::value(root));

  std::string text = "XCTSK:";
  text += json.GetValue();
  return text;
}
