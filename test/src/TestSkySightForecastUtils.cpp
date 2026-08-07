// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/SkySight/ForecastUtils.hpp"
#include "TestUtil.hpp"

static void
TestForecastPreloadSelection()
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
  const auto selectable = SkySight::GetSelectableForecastTimes(layer);
  ok1(selectable.size() == 3);
  ok1(selectable[0] == 9 * DAY + 23 * 60 * 60);
  ok1(selectable[1] == 10 * DAY + 6 * 60 * 60);
  ok1(selectable[2] == 11 * DAY + 6 * 60 * 60);

  for (auto &datafile : layer.forecast_datafiles)
    datafile.link.clear();
  ok1(!SkySight::HasForecastCatalogLinks(layer));
}

static void
TestForecastBusyState()
{
  SkySight::Layer layer;

  ok1(!layer.HasPendingForecastMetadata());
  ok1(!layer.ShouldShowUpdating());
  layer.RequestForecastMetadata(SkySight::ForecastMetadataIntent::Refresh);
  ok1(layer.HasPendingForecastMetadata());
  ok1(layer.ShouldShowUpdating());
  layer.forecast_datafiles.emplace_back(1, "cached");
  ok1(!layer.ShouldShowUpdating());
  layer.RequestForecastMetadata(
    SkySight::ForecastMetadataIntent::ActiveDefault);
  ok1(layer.forecast_metadata_intent ==
      SkySight::ForecastMetadataIntent::ActiveDefault);
  layer.RequestForecastMetadata(SkySight::ForecastMetadataIntent::Refresh);
  ok1(layer.forecast_metadata_intent ==
      SkySight::ForecastMetadataIntent::ActiveDefault);
  layer.RequestForecastMetadata(SkySight::ForecastMetadataIntent::PreloadAll);
  ok1(layer.forecast_metadata_intent ==
      SkySight::ForecastMetadataIntent::PreloadAll);
  layer.ClearForecastMetadataRequest();
  ok1(!layer.HasPendingForecastMetadata());
  layer.decoding = true;
  ok1(layer.ShouldShowUpdating());
  layer.decoding = false;
  layer.pending_downloads = 1;
  ok1(layer.ShouldShowUpdating());
}

static void
TestCachedForecastMerge()
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

  layer.forecast_time = 100;
  SkySight::MergeCachedForecastTimes(layer, cached_times, 250);
  ok1(layer.forecast_time == 200);

  layer.forecast_time_mode = SkySight::ForecastTimeMode::Fixed;
  layer.forecast_time = 100;
  SkySight::MergeCachedForecastTimes(layer, cached_times, 250);
  ok1(layer.forecast_time == 100);
}

static void
TestAutomaticForecastSelection()
{
  constexpr time_t DAY = 24 * 60 * 60;
  constexpr time_t NOW = 10 * DAY + 7 * 60 * 60;

  SkySight::Layer layer;
  layer.forecast_datafiles = {
    {9 * DAY + 12 * 60 * 60, "yesterday"},
    {10 * DAY + 12 * 60 * 60, "today"},
    {11 * DAY + 12 * 60 * 60, "tomorrow"},
  };

  ok1(SkySight::ChooseAutomaticForecastTime(layer, NOW) ==
      9 * DAY + 12 * 60 * 60);

  layer.id = "pfdtot";
  ok1(SkySight::ChooseAutomaticForecastTime(layer, NOW) ==
      10 * DAY + 12 * 60 * 60);

  layer.forecast_datafiles.emplace_back(10 * DAY + 6 * 60 * 60,
                                        "today-earlier");
  ok1(SkySight::ChooseAutomaticForecastTime(layer, NOW) ==
      10 * DAY + 6 * 60 * 60);

  layer.forecast_datafiles.erase(
    std::remove_if(layer.forecast_datafiles.begin(),
                   layer.forecast_datafiles.end(),
                   [](const auto &datafile) {
                     return SkySight::GetForecastDayBucket(datafile.time) == 10;
                   }),
    layer.forecast_datafiles.end());
  ok1(SkySight::ChooseAutomaticForecastTime(layer, NOW) ==
      9 * DAY + 12 * 60 * 60);
}

int
main()
{
  plan_tests(44);
  TestForecastPreloadSelection();
  TestForecastBusyState();
  TestCachedForecastMerge();
  TestAutomaticForecastSelection();
  return exit_status();
}
