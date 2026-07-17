// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/WeatherUIState.hpp"
#include "Weather/SkySight/Layers.hpp"
#include "Weather/SkySight/ForecastUtils.hpp"
#include "Weather/SkySight/LiveTileUtils.hpp"
#include "Weather/SkySight/SkySightLimits.hpp"
#include "Weather/SkySight/SkySightRequestPolicy.hpp"
#include "TestUtil.hpp"

static void
TestOverlaySession()
{
  OverlaySession session;
  session.Clear();

  ok1(!session.page_entered);
  ok1(!session.suspended_for_pan);
  ok1(!session.cursor_initialized);
  ok1(!session.HasPageOwnership());

  ok1(session.EnterPage());
  ok1(!session.EnterPage());
  ok1(session.page_entered);
  ok1(session.HasPageOwnership());

  session.SuspendForPan();
  ok1(session.IsSuspendedForPan());
  ok1(session.HasPageOwnership());

  session.LeavePage();
  ok1(!session.page_entered);
  ok1(session.HasPageOwnership());

  session.ResumeAfterPan();
  ok1(!session.IsSuspendedForPan());
  ok1(!session.HasPageOwnership());

  session.SuspendForPan();
  ok1(!session.IsSuspendedForPan());
}

static void
TestWeatherUiStateRaspReset()
{
  WeatherUIState weather;
  weather.Clear();

  ok1(weather.map == -1);
  ok1(weather.time_auto_advance);
  ok1(!weather.time.IsPlausible());
  ok1(!weather.rasp.cursor_initialized);

  weather.time_auto_advance = false;
  weather.time = BrokenTime(12, 30);
  weather.ResetRaspForDedicatedPage();
  ok1(weather.time == BrokenTime(12, 30));

  weather.time_auto_advance = true;
  weather.ResetRaspForDedicatedPage();
  ok1(!weather.time.IsPlausible());
}

static void
TestWeatherUiStateXcthermCursor()
{
  WeatherUIState weather;
  weather.Clear();

  ok1(!weather.xctherm.cursor_initialized);
  ok1(weather.xctherm_cursor.layer == 0);
  ok1(weather.xctherm_cursor.forecast_utc_hour == 12);
  ok1(!weather.xctherm_cursor.altitude_manual_override);
  ok1(!weather.xctherm_cursor.time_manual_override);

  weather.xctherm.EnterPage();
  weather.xctherm.SuspendForPan();
  weather.xctherm.cursor_initialized = true;
  weather.xctherm_cursor.layer = 7;
  weather.xctherm_cursor.forecast_utc_hour = 19;
  weather.xctherm_cursor.altitude_manual_override = true;
  weather.xctherm_cursor.time_manual_override = true;

  weather.Clear();
  ok1(!weather.xctherm.HasPageOwnership());
  ok1(!weather.xctherm.cursor_initialized);
  ok1(weather.xctherm_cursor.layer == 0);
  ok1(weather.xctherm_cursor.forecast_utc_hour == 12);
  ok1(!weather.xctherm_cursor.altitude_manual_override);
  ok1(!weather.xctherm_cursor.time_manual_override);
}

static void
TestSkySightKnownLiveTimestamp()
{
  constexpr time_t NOW = 10'000;
  SkySight::Layer layer{"satellite", "Satellite", "", true, true, true};
  ok1(!layer.HasKnownLiveTimestamp());
  ok1(layer.IsLiveMetadataPollDue(NOW, 30, 300));

  layer.last_update = 1234;
  ok1(layer.HasKnownLiveTimestamp());
  layer.live_timestamp_from_probe = true;
  ok1(layer.live_timestamp_from_probe);
  layer.last_update_request = NOW;
  ok1(!layer.IsLiveMetadataPollDue(NOW + 299, 30, 300));
  ok1(layer.IsLiveMetadataPollDue(NOW + 300, 30, 300));

  SkySight::Layer rain{"rain", "Rain", "", true, true, true};
  ok1(rain.IsLiveMetadataPollDue(NOW + 1, 30, 300));
  rain.last_update_request = NOW;
  ok1(!rain.IsLiveMetadataPollDue(NOW + 29, 30, 300));
  ok1(rain.IsLiveMetadataPollDue(NOW + 30, 30, 300));
  rain.live_metadata_support = SkySight::LiveMetadataSupport::Unsupported;
  ok1(!rain.IsLiveMetadataPollDue(NOW + 300, 30, 300));

  layer.live_layer = false;
  ok1(!layer.HasKnownLiveTimestamp());
}

static void
TestSkySightForecastPreloadSelection()
{
  constexpr time_t DAY = 24 * 60 * 60;
  constexpr time_t NOW = 10 * DAY + 7 * 60 * 60;

  SkySight::Layer layer;
  layer.forecast_datafiles = {
    {9 * DAY + 23 * 60 * 60, "previous"},
    {10 * DAY + 12 * 60 * 60, "later"},
    {10 * DAY + 6 * 60 * 60, "earlier"},
    {11 * DAY + 6 * 60 * 60, "tomorrow"},
    {10 * DAY + 8 * 60 * 60, ""},
  };

  auto selected = SkySight::GetForecastPreloadDatafiles(layer, NOW);
  ok1(selected.size() == 3);
  ok1(selected[0]->time == 10 * DAY + 6 * 60 * 60);
  ok1(selected[1]->time == 10 * DAY + 12 * 60 * 60);
  ok1(selected[2]->time == 11 * DAY + 6 * 60 * 60);

  layer.id = "pfdtot";
  selected = SkySight::GetForecastPreloadDatafiles(layer, NOW);
  ok1(selected.size() == 2);
  ok1(selected[0]->time == 10 * DAY + 6 * 60 * 60);
  ok1(selected[1]->time == 11 * DAY + 6 * 60 * 60);

  ok1(SkySight::GetForecastCatalogStart(NOW) == NOW);
  ok1(SkySight::GetForecastCatalogStart(DAY / 2) == DAY / 2);

  ok1(SkySight::HasForecastCatalogLinks(layer));
  for (auto &datafile : layer.forecast_datafiles)
    datafile.link.clear();
  ok1(!SkySight::HasForecastCatalogLinks(layer));
}

static void
TestSkySightRequestFailurePolicy()
{
  constexpr time_t NOW = 1000;
  SkySight::RequestFailurePolicy policy;

  ok1(SkySight::ParseRetryAfterSeconds("123", NOW) == NOW + 123);
  ok1(policy.CanQueue("missing-tile", NOW));
  auto decision = policy.OnHttpFailure("missing-tile", 404, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);
  ok1(!policy.CanQueue("missing-tile", NOW + 24 * 60 * 60));

  decision = policy.OnHttpFailure("bad-tile", 400, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);
  decision = policy.OnHttpFailure("forbidden-tile", 403, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);

  decision = policy.OnHttpFailure("unauthorized", 401, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Reauthenticate);
  ok1(policy.CanQueue("unauthorized", NOW));
  decision = policy.OnHttpFailure("unauthorized", 401, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);
  ok1(!policy.CanQueue("unauthorized", NOW + 24 * 60 * 60));

  decision = policy.OnHttpFailure("throttled", 429, NOW, NOW + 123);
  ok1(decision.action == SkySight::RequestFailureAction::Retry);
  ok1(decision.ready_at == NOW + 123);
  ok1(!policy.CanQueue("throttled", NOW + 122));
  ok1(policy.CanQueue("throttled", NOW + 123));
  decision = policy.OnHttpFailure("throttled-fallback", 429, NOW);
  ok1(decision.ready_at == NOW + 30);

  decision = policy.OnHttpFailure("server-error", 503, NOW);
  ok1(decision.ready_at == NOW + 10);
  decision = policy.OnHttpFailure("server-error", 503, NOW + 10);
  ok1(decision.ready_at == NOW + 30);
  for (unsigned i = 0; i < 10; ++i)
    decision = policy.OnTransportFailure("server-error", decision.ready_at);
  ok1(decision.ready_at <= NOW + 30 + 10 * 60 * 10);

  policy.Clear();
  ok1(policy.CanQueue("missing-tile", NOW));

  SkySight::AuthenticationFailurePolicy authentication;
  ok1(authentication.CanAttempt(NOW));
  auto login_decision = authentication.OnHttpFailure(401, NOW);
  ok1(login_decision.action ==
      SkySight::AuthenticationFailureAction::Rejected);
  ok1(!authentication.CanAttempt(NOW + 24 * 60 * 60));
  authentication.Reset();
  ok1(authentication.CanAttempt(NOW));

  login_decision = authentication.OnHttpFailure(429, NOW, NOW + 123);
  ok1(login_decision.action ==
      SkySight::AuthenticationFailureAction::Throttle);
  ok1(login_decision.ready_at == NOW + 123);
  ok1(!authentication.CanAttempt(NOW + 122));
  ok1(authentication.CanAttempt(NOW + 123));

  authentication.Reset();
  login_decision = authentication.OnHttpFailure(429, NOW);
  ok1(login_decision.ready_at == NOW + 30);
  ok1(!authentication.CanAttempt(NOW + 29));

  authentication.Reset();
  login_decision = authentication.OnHttpFailure(503, NOW);
  ok1(login_decision.action == SkySight::AuthenticationFailureAction::Retry);
  ok1(login_decision.ready_at == NOW + 30);
  login_decision = authentication.OnTransportFailure(NOW + 30);
  ok1(login_decision.ready_at == NOW + 90);

  SkySight::ThrottleFallbackPolicy throttle;
  ok1(throttle.OnThrottle(NOW) == NOW + 30);
  ok1(throttle.OnThrottle(NOW + 30) == NOW + 60);
  ok1(throttle.OnThrottle(NOW + 60) == NOW + 90);
  ok1(throttle.OnThrottle(NOW + 90) == NOW + 120);
  ok1(throttle.OnThrottle(NOW + 120, NOW + 323) == NOW + 323);
  ok1(throttle.OnThrottle(NOW + 323) == NOW + 353);

  SkySight::LiveTileRequestPacer pacer;
  ok1(pacer.CanStart(NOW));
  pacer.OnStarted(NOW);
  ok1(pacer.CanStart(NOW + 1));
  pacer.OnStarted(NOW + 1);
  ok1(pacer.CanStart(NOW + 2));
  pacer.OnStarted(NOW + 2);
  ok1(pacer.CanStart(NOW + 3));
  pacer.OnStarted(NOW + 3);
  ok1(!pacer.CanStart(NOW + 10));
  ok1(pacer.CanStart(NOW + 11));

  SkySight::LiveTileRequestPacer rearmed_pacer;
  for (unsigned i = 0; i < 4; ++i)
    rearmed_pacer.OnStarted(NOW + i);
  rearmed_pacer.OnQueueState(NOW + 4, false);
  rearmed_pacer.OnQueueState(NOW + 34, false);
  rearmed_pacer.OnStarted(NOW + 34);
  ok1(rearmed_pacer.CanStart(NOW + 34));

  SkySight::LiveTileRequestPacer throttled_pacer;
  throttled_pacer.OnStarted(NOW);
  throttled_pacer.OnThrottle();
  ok1(!throttled_pacer.CanStart(NOW + 1));
  ok1(throttled_pacer.CanStart(NOW + 8));

  SkySight::InteractiveRequestPacer interactive_pacer;
  ok1(interactive_pacer.CanStart(NOW));
  interactive_pacer.OnStarted(NOW);
  ok1(!interactive_pacer.CanStart(NOW));
  ok1(interactive_pacer.CanStart(NOW + 1));
}

static void
TestSkySightForecastBusyState()
{
  SkySight::Layer layer;

  ok1(!layer.ShouldShowUpdating());

  layer.datafiles_pending = true;
  ok1(layer.ShouldShowUpdating());

  layer.forecast_datafiles.emplace_back(1, "cached");
  ok1(!layer.ShouldShowUpdating());

  layer.decoding = true;
  ok1(layer.ShouldShowUpdating());

  layer.decoding = false;
  layer.pending_downloads = 1;
  ok1(layer.ShouldShowUpdating());
}

static void
TestSkySightCachedForecastMerge()
{
  SkySight::Layer layer;
  layer.forecast_datafiles = {
    {300, "three-hundred"},
    {100, "one-hundred"},
  };
  layer.forecast_time = 200;

  const std::vector<time_t> cached_times{400, 300, 200};
  SkySight::MergeCachedForecastTimes(layer, cached_times, 250);

  ok1(layer.forecast_datafiles.size() == 4);
  ok1(layer.forecast_datafiles[0].time == 400);
  ok1(layer.forecast_datafiles[1].time == 300);
  ok1(layer.forecast_datafiles[2].time == 200);
  ok1(layer.forecast_datafiles[3].time == 100);
  ok1(layer.forecast_datafiles[1].link == "three-hundred");
  ok1(layer.forecast_datafiles[2].link.empty());
  ok1(layer.forecast_time == 200);
  ok1(layer.from == 100);
  ok1(layer.to == 400);

  layer.forecast_time = 999;
  SkySight::MergeCachedForecastTimes(layer, cached_times, 250);
  ok1(layer.forecast_time == 200);
  ok1(layer.forecast_datafiles.size() == 4);
}

static void
TestSkySightResourcePolicies()
{
  ok1(SkySight::IsSafeId("EAST_US"));
  ok1(SkySight::IsSafeId("wstar_bsratio"));
  ok1(!SkySight::IsSafeId(""));
  ok1(!SkySight::IsSafeId(".."));
  ok1(!SkySight::IsSafeId("EUROPE/../x"));
  ok1(!SkySight::IsSafeId("rain.jpg"));
  ok1(SkySight::IsNetCdfGridSizeAllowed(1024, 1024));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(1, 1024));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(
    SkySight::MAX_NETCDF_GRID_AXIS + 1, 2));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(4096, 4096));
}

static void
TestSkySightLiveTileResolution()
{
  const GeoBitmap::TileData tile{8, 103, 207};
  const auto parent = SkySight::GetTileAncestor(tile, 6);
  ok1(parent.zoom == 6);
  ok1(parent.x == 25);
  ok1(parent.y == 51);

  ok1(SkySight::SelectLiveTileZoom(8, 1) == 7);
  ok1(SkySight::SelectLiveTileZoom(9, 1) == 8);
  ok1(SkySight::SelectLiveTileZoom(1, 1) == 1);
  ok1(SkySight::GetLiveTileMapZoomMaximum(8) == 9);
  ok1(SkySight::GetLiveTileMapZoomMaximum(GeoBitmap::MAX_TILE_ZOOM) ==
      GeoBitmap::MAX_TILE_ZOOM);
  ok1(SkySight::IsRecentLiveTileTimestamp(10'000, 10'000));
  ok1(SkySight::IsRecentLiveTileTimestamp(8'800, 10'000));
  ok1(!SkySight::IsRecentLiveTileTimestamp(8'200, 10'000));
  ok1(!SkySight::IsRecentLiveTileTimestamp(10'600, 10'000));

  constexpr std::array<unsigned, 3> newest_tie{2, 2, 1};
  ok1(SkySight::SelectCoherentLiveTileTimestamp(10'000, newest_tie) ==
      10'000);
  constexpr std::array<unsigned, 3> older_complete{1, 4, 2};
  ok1(SkySight::SelectCoherentLiveTileTimestamp(10'000, older_complete) ==
      9'400);
  constexpr std::array<unsigned, 3> empty_coverage{};
  ok1(SkySight::SelectCoherentLiveTileTimestamp(10'000, empty_coverage) == 0);
}

int
main()
{
  plan_tests(32 + 10 + 10 + 5 + 12 + 4 + 30 + 41 + 2 + 1);

  TestOverlaySession();
  TestWeatherUiStateRaspReset();
  TestWeatherUiStateXcthermCursor();
  TestSkySightKnownLiveTimestamp();
  TestSkySightForecastPreloadSelection();
  TestSkySightRequestFailurePolicy();
  TestSkySightForecastBusyState();
  TestSkySightCachedForecastMerge();
  TestSkySightResourcePolicies();
  TestSkySightLiveTileResolution();

  return exit_status();
}
