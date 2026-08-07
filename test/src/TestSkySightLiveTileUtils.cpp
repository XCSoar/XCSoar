// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/SkySight/Layers.hpp"
#include "Weather/SkySight/LiveTileUtils.hpp"
#include "TestUtil.hpp"

static void
TestKnownLiveTimestamp()
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
TestLiveTileResolution()
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
  plan_tests(26);
  TestKnownLiveTimestamp();
  TestLiveTileResolution();
  return exit_status();
}
