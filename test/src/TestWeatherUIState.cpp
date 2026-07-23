// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/WeatherUIState.hpp"
#include "Weather/SkySight/ForecastUtils.hpp"
#include "Weather/SkySight/SkySightLimits.hpp"
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
  ok1(SkySight::GetTileRetryDelay(1) == 10);
  ok1(SkySight::GetTileRetryDelay(2) == 30);
  ok1(SkySight::GetTileRetryDelay(3) == 60);
  ok1(!SkySight::ShouldSuppressTile(2));
  ok1(SkySight::ShouldSuppressTile(3));

  ok1(SkySight::IsNetCdfGridSizeAllowed(1024, 1024));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(1, 1024));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(
    SkySight::MAX_NETCDF_GRID_AXIS + 1, 2));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(4096, 4096));

  ok1(SkySight::IsSafeId("EUROPE"));
  ok1(SkySight::IsSafeId("wind_925"));
  ok1(SkySight::IsSafeId("west-us.v1"));
  ok1(!SkySight::IsSafeId(""));
  ok1(!SkySight::IsSafeId("."));
  ok1(!SkySight::IsSafeId(".."));
  ok1(!SkySight::IsSafeId("../etc"));
  ok1(!SkySight::IsSafeId("a/b"));
  ok1(!SkySight::IsSafeId("a\\b"));
}

int
main()
{
  plan_tests(32 + 7 + 5 + 12 + 9 + 9);

  TestOverlaySession();
  TestWeatherUiStateRaspReset();
  TestWeatherUiStateXcthermCursor();
  TestSkySightForecastPreloadSelection();
  TestSkySightForecastBusyState();
  TestSkySightCachedForecastMerge();
  TestSkySightResourcePolicies();

  return exit_status();
}
