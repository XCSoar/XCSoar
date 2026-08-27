// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Renderer/AirspaceLabelList.hpp"
#include "Renderer/AirspaceLabelPlacement.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Engine/Airspace/AirspaceWarningConfig.hpp"
#include "TestUtil.hpp"

#include <array>

static constexpr PixelRect map_rect{0, 0, 1000, 1000};
static constexpr PixelSize label_size{100, 40};
static constexpr unsigned clearance = 4;

[[gnu::pure]]
static bool
Equals(const PixelRect a, const PixelRect b) noexcept
{
  return a.left == b.left && a.top == b.top &&
    a.right == b.right && a.bottom == b.bottom;
}

[[gnu::pure]]
static PixelRect
GetCollisionRect(const AirspaceLabelPlacement &placement) noexcept
{
  return placement.visual_rect.WithMargin(int(clearance));
}

static void
TestPreferredPlacement()
{
  LabelBlock block;
  const auto placement =
    PlaceAirspaceLabel({500, 500}, label_size, clearance, map_rect, &block);

  ok1(placement.has_value());
  ok1(placement && placement->candidate_index == 0);
  ok1(placement &&
      Equals(placement->visual_rect, {450, 500, 550, 540}));
  ok1(placement && map_rect.Contains(GetCollisionRect(*placement)));
}

static void
TestFallbackPlacement()
{
  LabelBlock block;
  ok1(block.check({446, 496, 554, 544}));

  const auto placement =
    PlaceAirspaceLabel({500, 500}, label_size, clearance, map_rect, &block);
  ok1(placement.has_value());
  ok1(placement && placement->candidate_index == 1);
  ok1(placement &&
      Equals(placement->visual_rect, {450, 451, 550, 491}));
}

static void
TestSharedAnchorPlacement()
{
  LabelBlock block;
  std::array<PixelRect, 8> occupied{};
  bool all_placed = true;
  bool candidates_are_ordered = true;

  for (unsigned i = 0; i < occupied.size(); ++i) {
    const auto placement = PlaceAirspaceLabel({500, 500}, label_size,
                                               clearance, map_rect, &block);
    if (!placement) {
      all_placed = false;
      break;
    }

    candidates_are_ordered &= placement->candidate_index == i;
    occupied[i] = GetCollisionRect(*placement);
  }

  bool no_overlaps = all_placed;
  for (unsigned i = 0; i < occupied.size(); ++i)
    for (unsigned j = 0; j < i; ++j)
      no_overlaps &= !occupied[i].OverlapsWith(occupied[j]);

  ok1(all_placed);
  ok1(candidates_are_ordered);
  ok1(no_overlaps);
}

static void
TestNearbyAnchors()
{
  LabelBlock block;
  const auto first =
    PlaceAirspaceLabel({500, 500}, label_size, clearance, map_rect, &block);
  const auto second =
    PlaceAirspaceLabel({510, 505}, label_size, clearance, map_rect, &block);

  ok1(first.has_value());
  ok1(second.has_value());
  ok1(first && first->candidate_index == 0 && second &&
      second->candidate_index != 0);
  ok1(first && second &&
      !GetCollisionRect(*first).OverlapsWith(GetCollisionRect(*second)));
}

static void
TestMapEdges()
{
  LabelBlock top_block;
  const auto top =
    PlaceAirspaceLabel({500, 0}, label_size, clearance, map_rect, &top_block);
  ok1(top && top->candidate_index == 4);

  LabelBlock bottom_block;
  const auto bottom = PlaceAirspaceLabel({500, 999}, label_size, clearance,
                                          map_rect, &bottom_block);
  ok1(bottom && bottom->candidate_index == 1);

  LabelBlock left_block;
  const auto left =
    PlaceAirspaceLabel({0, 500}, label_size, clearance, map_rect, &left_block);
  ok1(left && left->candidate_index == 2);

  LabelBlock right_block;
  const auto right = PlaceAirspaceLabel({999, 500}, label_size, clearance,
                                         map_rect, &right_block);
  ok1(right && right->candidate_index == 3);
}

static void
TestAllCandidatesBlocked()
{
  LabelBlock block;
  bool filled = true;
  for (unsigned i = 0; i < 8; ++i)
    filled &= PlaceAirspaceLabel({500, 500}, label_size, clearance, map_rect,
                                 &block)
      .has_value();

  const auto placement =
    PlaceAirspaceLabel({500, 500}, label_size, clearance, map_rect, &block);
  ok1(filled);
  ok1(!placement.has_value());
}

static void
TestScaledLabelGeometry()
{
  LabelBlock small_block;
  const auto small = PlaceAirspaceLabel({500, 500}, {100, 40}, 4, map_rect,
                                         &small_block);

  LabelBlock large_block;
  const auto large_first = PlaceAirspaceLabel({500, 500}, {200, 80}, 8,
                                               map_rect, &large_block);
  const auto large_second = PlaceAirspaceLabel({500, 500}, {200, 80}, 8,
                                                map_rect, &large_block);

  ok1(small && large_first && large_second);
  ok1(small && large_first &&
      large_first->visual_rect.GetWidth() > small->visual_rect.GetWidth() &&
      large_first->visual_rect.GetHeight() > small->visual_rect.GetHeight());
  ok1(large_first && large_second &&
      !large_first->visual_rect.WithMargin(8).OverlapsWith(
        large_second->visual_rect.WithMargin(8)));
  ok1(large_first && large_second && large_first->candidate_index == 0 &&
      large_second->candidate_index == 1);
}

static void
TestRectangleEdgeSemantics()
{
  const PixelRect first{0, 0, 10, 10};
  ok1(first.OverlapsWith({10, 0, 20, 10}));
  ok1(!first.OverlapsWith({11, 0, 20, 10}));
}

static void
TestPlacementIsStable()
{
  LabelBlock first_block;
  const auto first = PlaceAirspaceLabel({500, 500}, label_size, clearance,
                                         map_rect, &first_block);
  LabelBlock second_block;
  const auto second = PlaceAirspaceLabel({500, 500}, label_size, clearance,
                                          map_rect, &second_block);

  ok1(first && second);
  ok1(first && second && first->candidate_index == second->candidate_index);
  ok1(first && second &&
      Equals(first->visual_rect, second->visual_rect));
}

[[gnu::const]]
static AirspaceAltitude
MakeAltitude(const double altitude) noexcept
{
  return {altitude, 0, 0, AltitudeReference::MSL};
}

static void
TestPlacementPriority()
{
  AirspaceWarningConfig config;
  config.SetDefaults();
  config.class_warnings[CLASSC] = false;
  config.class_warnings[CLASSD] = true;

  AirspaceLabelList warning_labels;
  warning_labels.Add(GeoPoint::Zero(), CLASSC, CTR, MakeAltitude(2000),
                     MakeAltitude(3000));
  warning_labels.Add(GeoPoint::Zero(), CLASSD, MakeAltitude(1000),
                     MakeAltitude(3000));
  warning_labels.Sort(config);
  ok1(warning_labels[0].cls == CLASSD);
  ok1(warning_labels[1].border_class == CTR);

  LabelBlock warning_block;
  const auto warning_winner = PlaceAirspaceLabel({500, 500}, label_size,
                                                  clearance, map_rect,
                                                  &warning_block);
  const auto warning_loser = PlaceAirspaceLabel({500, 500}, label_size,
                                                 clearance, map_rect,
                                                 &warning_block);
  ok1(warning_winner && warning_winner->candidate_index == 0);
  ok1(warning_loser && warning_loser->candidate_index != 0);

  AirspaceLabelList altitude_labels;
  altitude_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(1000),
                      MakeAltitude(3000));
  altitude_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(2000),
                      MakeAltitude(3000));
  altitude_labels.Sort(config);
  ok1(altitude_labels[0].base.altitude == 2000);

  AirspaceLabelList equal_labels;
  equal_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(1000),
                   MakeAltitude(3000));
  equal_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(1000),
                   MakeAltitude(3000));
  equal_labels.Sort(config);
  ok1(equal_labels[0].ordinal == 0 && equal_labels[1].ordinal == 1);
}

int
main()
{
  plan_tests(36);

  TestPreferredPlacement();
  TestFallbackPlacement();
  TestSharedAnchorPlacement();
  TestNearbyAnchors();
  TestMapEdges();
  TestAllCandidatesBlocked();
  TestScaledLabelGeometry();
  TestRectangleEdgeSemantics();
  TestPlacementIsStable();
  TestPlacementPriority();

  return exit_status();
}
