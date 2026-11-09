// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "net/client/WeGlide/Tiles.hpp"
#include "TestUtil.hpp"

static constexpr char kToken[] =
  R"({"query":"token=HS256&expires=1&g=1&token_path=%2Fapi%2Fgrid%2F",)"
  R"("expires":1792415012,)"
  R"("base_url":"https://copilot-tiles.weglide.org/api"})";

static constexpr char kCatalog[] =
  R"([{"id":"thermal","name":"Cloudbase","units":"m",)"
  R"("description":"Height reached",)"
  R"("legend":{"colors":[{"value":350.0,"color":[0,0,143]},)"
  R"({"value":500,"color":[1,2,3]}]}},)"
  R"({"id":"vario","name":"Netto","units":"m/s",)"
  R"("legend":{"colors":[{"value":-0.7,"color":[0,0,143]}]}}])";

static constexpr char kLatest[] =
  R"([{"layer_id":"thermal","time":1792407840,"interval":60,"max_zoom":7},)"
  R"({"layer_id":"vario","time":1792407840,"interval":20,"max_zoom":7},)"
  R"({"layer_id":"thermal","time":0,"interval":60,"max_zoom":7}])";

static void
TestParse()
{
  const auto token = WeGlide::ParseTileQuery(kToken);
  ok1(token.has_value());
  ok1(token->base_url == "https://copilot-tiles.weglide.org/api");
  ok1(token->expires == 1792415012u);
  const char encoded_path[] = "%2Fapi%2Fgrid%2F";
  const bool kept_encoding =
    token->query.find(encoded_path) != std::string::npos;
  ok1(kept_encoding);
  ok1(!WeGlide::ParseTileQuery("{"));
  ok1(!WeGlide::ParseTileQuery(R"({"base_url":"https://x"})"));

  const auto catalog = WeGlide::ParseCatalog(kCatalog);
  ok1(catalog.has_value());
  ok1(catalog->size() == 2);
  ok1((*catalog)[0].id == "thermal");
  ok1((*catalog)[0].name == "Cloudbase");
  ok1((*catalog)[0].units == "m");
  ok1((*catalog)[0].legend.size() == 2);
  ok1((*catalog)[0].legend[0].value == 350.0);
  ok1((*catalog)[0].legend[0].color.b == 143);
  ok1((*catalog)[1].id == "vario");
  ok1((*catalog)[1].legend[0].value < -0.6);
  ok1(!WeGlide::ParseCatalog(R"({"id":"thermal"})"));

  const auto latest = WeGlide::ParseLatest(kLatest);
  ok1(latest.has_value());
  ok1(latest->size() == 2);
  ok1((*latest)[0].layer_id == "thermal");
  ok1((*latest)[0].time == 1792407840u);
  ok1((*latest)[0].interval_seconds == 60);
  ok1((*latest)[0].max_zoom == 7);
  ok1((*latest)[1].interval_seconds == 20);
  ok1(!WeGlide::ParseLatest("null"));
}

static void
TestUrlAndPolicy()
{
  WeGlide::TileQuery token;
  token.base_url = "https://copilot-tiles.weglide.org/api/";
  token.query = "token=HS256&expires=1&token_path=%2Fapi%2Fgrid%2F";
  token.expires = 10'000;

  const auto catalog_url = WeGlide::GridUrl(token, "catalog");
  const char catalog_expected[] =
    "https://copilot-tiles.weglide.org/api/grid/catalog?"
    "token=HS256&expires=1&token_path=%2Fapi%2Fgrid%2F";
  ok1(catalog_url == catalog_expected);
  ok1(catalog_url.find("lon=") == std::string::npos);
  ok1(catalog_url.find("lat=") == std::string::npos);

  const auto path = WeGlide::TileRelativePath("thermal", 1792407840, 7, 66, 44);
  ok1(path == "thermal/1792407840/7/66/44.png");
  ok1(WeGlide::TileRelativePath("../x", 1, 1, 1, 1).empty());
  ok1(WeGlide::TileRelativePath("thermal", 0, 1, 1, 1).empty());
  ok1(WeGlide::GridUrl({}, "catalog").empty());

  constexpr uint32_t now = 1'000;
  ok1(WeGlide::NextTileQuery(now, now + 31u * 60u, 0) ==
      WeGlide::TileQueryAction::USE);
  ok1(WeGlide::NextTileQuery(now, now + 29u * 60u, 0) ==
      WeGlide::TileQueryAction::RENEW);
  ok1(WeGlide::NextTileQuery(now, now, 0) ==
      WeGlide::TileQueryAction::RENEW);
  ok1(WeGlide::NextTileQuery(now, 0, 0) ==
      WeGlide::TileQueryAction::RENEW);
  ok1(WeGlide::NextTileQuery(now, now + 31u * 60u, now + 10) ==
      WeGlide::TileQueryAction::WAIT);
  ok1(WeGlide::AllowTileRetry(0));
  ok1(!WeGlide::AllowTileRetry(1));
  ok1(WeGlide::SubscriptionWaitUntil(now) ==
      now + WeGlide::kSubscriptionWaitSeconds);
  ok1(WeGlide::SubscriptionWaitUntil(0xffffffffu) == 0xffffffffu);

  WeGlide::LayerTick thermal;
  thermal.layer_id = "thermal";
  thermal.time = 50;
  thermal.interval_seconds = 60;
  thermal.max_zoom = 7;
  WeGlide::LayerTick vario = thermal;
  vario.layer_id = "vario";
  vario.interval_seconds = 20;
  WeGlide::LayerTick broken = thermal;
  broken.time = 0;
  ok1(WeGlide::IsUsableTick(thermal));
  ok1(!WeGlide::IsUsableTick(broken));
  const WeGlide::LayerTick ticks[] = {thermal, vario, broken};
  ok1(WeGlide::SmallestInterval(ticks) == 20);
  ok1(WeGlide::SmallestInterval({}) == 0);

  WeGlide::LayerTick series = thermal;
  series.time = 1792407840;
  series.interval_seconds = 60;
  ok1(WeGlide::CurrentTickTime(series, 1792407840) == 1792407840);
  ok1(WeGlide::CurrentTickTime(series, 1792407839) == 1792407840);
  ok1(WeGlide::CurrentTickTime(series, 1792416689) == 1792416660);
  series.interval_seconds = 0;
  ok1(WeGlide::CurrentTickTime(series, 1792416689) == 1792407840);

  series.time = 1000;
  series.interval_seconds = 60;
  ok1(WeGlide::AlignTickTime(series, 1100, 1300) == 1060);
  ok1(WeGlide::AlignTickTime(series, 99999, 1300) == 1300);
  ok1(WeGlide::AlignTickTime(series, 1, 1300) == 1000);
  series.time = 1792407840;
  series.interval_seconds = 60;
  ok1(WeGlide::StepTickTime(series, 1792416660, -1, 1792416689) ==
      1792415760);
  ok1(WeGlide::StepTickTime(series, 1792415760, 1, 1792416689) ==
      1792416660);
  ok1(WeGlide::StepTickTime(series, 1792416660, 1, 1792416689) ==
      1792416660);
  ok1(WeGlide::StepTickTime(series, 1792407840, -1, 1792416689) ==
      1792407840);
  series.interval_seconds = 20;
  ok1(WeGlide::StepTickTime(series, 0, -1, 1792416689) == 1792415780);
}

static void
TestCacheDrop()
{
  WeGlide::LayerTick thermal;
  thermal.layer_id = "thermal";
  thermal.time = 200;
  thermal.interval_seconds = 60;
  thermal.max_zoom = 7;

  WeGlide::StoredTile old_time{"thermal", 100, 10, 1};
  WeGlide::StoredTile current{"thermal", 200, 40, 5};
  WeGlide::StoredTile older_bytes{"vario", 200, 30, 1};
  WeGlide::StoredTile newer_bytes{"vario", 200, 30, 3};
  const WeGlide::StoredTile tiles[] = {
    old_time, current, older_bytes, newer_bytes,
  };
  const WeGlide::LayerTick latest[] = {thermal};

  const auto times_only = WeGlide::TilesToDrop(tiles, latest, 250, 0);
  ok1(times_only.size() == 1);
  ok1(times_only[0] == 0);

  const auto capped = WeGlide::TilesToDrop(tiles, latest, 250, 50);
  ok1(capped.size() == 3);
  ok1(capped[0] == 0);
  ok1(capped[1] == 2);
  ok1(capped[2] == 3);

  thermal.time = 1000;
  thermal.interval_seconds = 60;
  const WeGlide::LayerTick series[] = {thermal};
  const WeGlide::StoredTile history[] = {
    {"thermal", 1000, 10, 1},
    {"thermal", 1120, 10, 2},
    {"thermal", 1130, 10, 3},
    {"thermal", 1360, 10, 4},
  };
  const auto history_drop =
    WeGlide::TilesToDrop(history, series, 1300, 0);
  ok1(history_drop.size() == 2);
  ok1(history_drop[0] == 2);
  ok1(history_drop[1] == 3);
}

int
main()
{
  plan_tests(25 + 32 + 9);
  TestParse();
  TestUrlAndPolicy();
  TestCacheDrop();
  return exit_status();
}
