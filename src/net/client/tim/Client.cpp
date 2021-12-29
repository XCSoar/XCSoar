/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Client.hpp"
#include "Thermal.hpp"
#include "net/http/CoStreamRequest.hxx"
#include "net/http/Easy.hxx"
#include "net/http/Setup.hxx"
#include "json/ParserOutputStream.hxx"

#include <stdexcept>

static auto
tag_invoke(boost::json::value_to_tag<GeoPoint>,
           const boost::json::value &jv)
{
  const auto &json = jv.as_object();
  GeoPoint gp{
              Angle::Degrees(json.at("lon").as_double()),
              Angle::Degrees(json.at("lat").as_double()),
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
  thermal.time = std::chrono::system_clock::from_time_t(json.at("time").as_int64());
  thermal.location = boost::json::value_to<GeoPoint>(jv);
  thermal.climb_rate = json.at("climbRate").as_double();
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
