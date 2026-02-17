// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ListTasks.hpp"
#include "Error.hpp"
#include "Settings.hpp"
#include "json/ParserOutputStream.hxx"
#include "net/http/Progress.hpp"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "co/Task.hxx"

#include <boost/json.hpp>

using std::string_view_literals::operator""sv;

namespace WeGlide {

const char *
ToString(TaskKind kind) noexcept
{
  switch (kind) {
  case TaskKind::UNKNOWN:
    return "";

  case TaskKind::FREE:
    return "Free";

  case TaskKind::FREE4:
    return "Free (4 TP)";

  case TaskKind::TRIANGLE:
    return "Triangle";
  }

  return "";
}

TaskKind
ParseTaskKind(std::string_view kind) noexcept
{
  if (kind == "FR"sv)
    return TaskKind::FREE;
  else if (kind == "FR4"sv)
    return TaskKind::FREE4;
  else if (kind == "TR"sv)
    return TaskKind::TRIANGLE;
  else
    return TaskKind::UNKNOWN;
}

static std::vector<TurnpointInfo>
ParseTurnpoints(const boost::json::value &jv) noexcept
{
  std::vector<TurnpointInfo> turnpoints;

  if (!jv.is_array())
    return turnpoints;

  for (const auto &feature : jv.as_array()) {
    if (!feature.is_object())
      continue;

    const auto &obj = feature.as_object();

    TurnpointInfo tp;

    if (auto p = obj.if_contains("properties"sv); p && p->is_object()) {
      const auto &props = p->as_object();

      if (auto n = props.if_contains("name"sv); n && n->is_string())
        tp.name = n->as_string();

      if (auto e = props.if_contains("elevation"sv); e && e->is_number())
        tp.elevation = e->to_number<int>();
      else
        tp.elevation = 0;

      if (auto r = props.if_contains("radius"sv); r && r->is_number())
        tp.radius = r->to_number<double>() * 1000;
      else
        tp.radius = -1;
    }

    if (auto g = obj.if_contains("geometry"sv); g && g->is_object()) {
      const auto &geom = g->as_object();

      if (auto c = geom.if_contains("coordinates"sv); c && c->is_array()) {
        const auto &coords = c->as_array();
        if (coords.size() >= 2) {
          tp.longitude = coords[0].to_number<double>();
          tp.latitude = coords[1].to_number<double>();
        }
      }
    }

    turnpoints.push_back(std::move(tp));
  }

  return turnpoints;
}

static auto
tag_invoke(boost::json::value_to_tag<TaskInfo>,
           const boost::json::value &jv)
{
  const auto &json = jv.as_object();

  TaskInfo info;
  info.id = json.at("id").to_number<uint_least64_t>();
  info.name = json.at("name").as_string();
  info.user_name = json.at("user").at("name").as_string();

  // convert km to m
  info.distance = json.at("distance").to_number<double>() * 1000;

  if (auto k = json.if_contains("kind"sv); k && k->is_string())
    info.kind = ParseTaskKind(k->as_string());

  if (auto r = json.if_contains("ruleset"sv); r && r->is_string())
    info.ruleset = r->as_string();

  if (auto t = json.if_contains("token"sv); t && t->is_string())
    info.token = t->as_string();

  if (auto pf = json.if_contains("point_features"sv))
    info.turnpoints = ParseTurnpoints(*pf);

  return info;
}

Co::Task<std::vector<TaskInfo>>
ListTasksByUser(CurlGlobal &curl, const WeGlideSettings &settings,
                uint_least64_t user_id,
                ProgressListener &progress)
{
  const auto url = FmtBuffer<256>("{}/task?user_id_in={}",
                                  settings.default_url, user_id);

  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  auto body = parser.Finish();

  if (response.status != 200)
    throw ResponseToException(response.status, body);

  co_return boost::json::value_to<std::vector<TaskInfo>>(body);
}

Co::Task<std::vector<TaskInfo>>
ListDeclaredTasks(CurlGlobal &curl, const WeGlideSettings &settings,
                  ProgressListener &progress)
{
  const auto url = FmtBuffer<256>("{}/task/declaration",
                                  settings.default_url);

  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  auto body = parser.Finish();

  if (response.status != 200)
    throw ResponseToException(response.status, body);

  co_return boost::json::value_to<std::vector<TaskInfo>>(body);
}

Co::Task<std::vector<TaskInfo>>
ListDailyCompetitions(CurlGlobal &curl, const WeGlideSettings &settings,
                      ProgressListener &progress)
{
  const auto url = FmtBuffer<256>("{}/task/competitions/today",
                                  settings.default_url);

  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  auto body = parser.Finish();

  if (response.status != 200)
    throw ResponseToException(response.status, body);

  co_return boost::json::value_to<std::vector<TaskInfo>>(body);
}

} // namespace WeGlide
