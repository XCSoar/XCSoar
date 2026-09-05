// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Renderer/AirspaceLabelList.hpp"
#include "Renderer/AirspaceLabelPlacement.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Engine/Airspace/AirspaceWarningConfig.hpp"
#include "TestUtil.hpp"

#include <array>
#include <optional>

static constexpr PixelRect MAP_RECT{0, 0, 1000, 1000};
static constexpr PixelSize LABEL_SIZE{100, 40};
static constexpr unsigned CLEARANCE = 4;

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
  return placement.visual_rect.WithMargin(int(CLEARANCE));
}

static void
TestPreferredPlacement()
{
  LabelBlock block;
  const auto placement =
    PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block);

  ok1(placement.has_value());
  ok1(placement && placement->candidate_index == AirspaceLabelCandidate::BELOW);
  ok1(placement &&
      Equals(placement->visual_rect, {450, 500, 550, 540}));
  ok1(placement && MAP_RECT.Contains(GetCollisionRect(*placement)));
}

static void
TestFallbackPlacement()
{
  LabelBlock block;
  ok1(block.check({446, 496, 554, 544}));

  const auto placement =
    PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block);
  ok1(placement.has_value());
  ok1(placement && placement->candidate_index == AirspaceLabelCandidate::ABOVE);
  ok1(placement &&
      Equals(placement->visual_rect, {450, 451, 550, 491}));
}

static void
TestPreferredCandidate()
{
  LabelBlock block;
  const auto placement =
    PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block,
                       std::optional<AirspaceLabelCandidate>{
                         AirspaceLabelCandidate::LOWER_RIGHT});

  ok1(placement.has_value());
  ok1(placement &&
      placement->candidate_index == AirspaceLabelCandidate::LOWER_RIGHT);
  ok1(placement &&
      Equals(placement->visual_rect, {559, 549, 659, 589}));
}

static void
TestCachedCandidateReuse()
{
  LabelBlock first_block;
  const auto first = PlaceAirspaceLabel(
    {500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &first_block,
    AirspaceLabelCandidate::LOWER_RIGHT);
  LabelBlock panned_block;
  const auto panned = PlaceAirspaceLabel(
    {520, 510}, LABEL_SIZE, CLEARANCE, MAP_RECT, &panned_block,
    first ? std::optional{first->candidate_index} : std::nullopt);

  ok1(first && panned);
  ok1(first && panned && first->candidate_index == panned->candidate_index);
  ok1(first && panned &&
      panned->visual_rect.left == first->visual_rect.left + 20 &&
      panned->visual_rect.top == first->visual_rect.top + 10);

  ok1(panned && !panned_block.check(GetCollisionRect(*panned)));
}

static void
TestBlockedPreferredCandidate()
{
  bool preferred_placed = true;
  bool fallback_placed = true;
  bool fixed_order = true;
  bool no_overlaps = true;
  bool fallback_retained = true;

  for (unsigned i = 0;
       i < static_cast<unsigned>(AirspaceLabelCandidate::COUNT); ++i) {
    const auto candidate = static_cast<AirspaceLabelCandidate>(i);
    LabelBlock blocked_block;
    const auto blocker = PlaceAirspaceLabel(
      {500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &blocked_block, candidate);
    const auto fallback = PlaceAirspaceLabel(
      {500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &blocked_block, candidate);

    preferred_placed &= blocker && blocker->candidate_index == candidate;
    fallback_placed &= fallback.has_value();
    fixed_order &= fallback && fallback->candidate_index ==
      (candidate == AirspaceLabelCandidate::BELOW
       ? AirspaceLabelCandidate::ABOVE : AirspaceLabelCandidate::BELOW);
    no_overlaps &= blocker && fallback &&
      !GetCollisionRect(*blocker).OverlapsWith(GetCollisionRect(*fallback));

    // Once the blocker disappears, keep the last successful fallback slot.
    LabelBlock next_frame;
    const auto next = PlaceAirspaceLabel(
      {500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &next_frame,
      fallback ? std::optional{fallback->candidate_index} : std::nullopt);
    fallback_retained &= fallback && next &&
      next->candidate_index == fallback->candidate_index;
  }

  ok1(preferred_placed);
  ok1(fallback_placed);
  ok1(fixed_order);
  ok1(no_overlaps);
  ok1(fallback_retained);
}

static void
TestPreferredCandidateGeometry()
{
  // A format or scale change must use the new geometry immediately.
  LabelBlock block;
  ok1(block.check({590, 496, 610, 544}));
  const auto small = PlaceAirspaceLabel(
    {500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block,
    AirspaceLabelCandidate::BELOW);
  ok1(small && small->candidate_index == AirspaceLabelCandidate::BELOW);

  block.reset();
  ok1(block.check({590, 496, 610, 544}));
  const auto large = PlaceAirspaceLabel(
    {500, 500}, {200, 80}, 8, MAP_RECT, &block,
    small ? std::optional{small->candidate_index} : std::nullopt);
  ok1(large && large->candidate_index == AirspaceLabelCandidate::ABOVE &&
      large->visual_rect.GetSize() == PixelSize(200u, 80u));

  // A preferred slot moved beyond the map edge also falls back immediately.
  const auto edge = PlaceAirspaceLabel(
    {999, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, nullptr,
    AirspaceLabelCandidate::LOWER_RIGHT);
  ok1(edge && edge->candidate_index == AirspaceLabelCandidate::LEFT &&
      MAP_RECT.Contains(GetCollisionRect(*edge)));
}

static void
TestSharedAnchorPlacement()
{
  LabelBlock block;
  std::array<PixelRect, static_cast<unsigned>(AirspaceLabelCandidate::COUNT)>
    occupied{};
  bool all_placed = true;
  bool candidates_are_ordered = true;

  for (unsigned i = 0; i < occupied.size(); ++i) {
    const auto placement =
      PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block);
    if (!placement) {
      all_placed = false;
      break;
    }

    candidates_are_ordered &=
      placement->candidate_index == static_cast<AirspaceLabelCandidate>(i);
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
    PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block);
  const auto second =
    PlaceAirspaceLabel({510, 505}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block);

  ok1(first.has_value());
  ok1(second.has_value());
  ok1(first && first->candidate_index == AirspaceLabelCandidate::BELOW &&
      second && second->candidate_index != AirspaceLabelCandidate::BELOW);
  ok1(first && second &&
      !GetCollisionRect(*first).OverlapsWith(GetCollisionRect(*second)));
}

static void
TestMapEdges()
{
  LabelBlock top_block;
  const auto top =
    PlaceAirspaceLabel({500, 0}, LABEL_SIZE, CLEARANCE, MAP_RECT, &top_block);
  ok1(top && top->candidate_index == AirspaceLabelCandidate::LOWER_RIGHT);

  LabelBlock bottom_block;
  const auto bottom = PlaceAirspaceLabel({500, 999}, LABEL_SIZE, CLEARANCE,
                                          MAP_RECT, &bottom_block);
  ok1(bottom && bottom->candidate_index == AirspaceLabelCandidate::ABOVE);

  LabelBlock left_block;
  const auto left =
    PlaceAirspaceLabel({0, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &left_block);
  ok1(left && left->candidate_index == AirspaceLabelCandidate::RIGHT);

  LabelBlock right_block;
  const auto right = PlaceAirspaceLabel({999, 500}, LABEL_SIZE, CLEARANCE,
                                         MAP_RECT, &right_block);
  ok1(right && right->candidate_index == AirspaceLabelCandidate::LEFT);
}

static void
TestAllCandidatesBlocked()
{
  LabelBlock block;
  bool filled = true;
  for (unsigned i = 0;
       i < static_cast<unsigned>(AirspaceLabelCandidate::COUNT); ++i)
    filled &= PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT,
                                 &block)
      .has_value();

  const auto placement =
    PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE, MAP_RECT, &block);
  ok1(filled);
  ok1(!placement.has_value());
}

static void
TestScaledLabelGeometry()
{
  LabelBlock small_block;
  const auto small = PlaceAirspaceLabel({500, 500}, {100, 40}, 4, MAP_RECT,
                                         &small_block);

  LabelBlock large_block;
  const auto large_first = PlaceAirspaceLabel({500, 500}, {200, 80}, 8,
                                               MAP_RECT, &large_block);
  const auto large_second = PlaceAirspaceLabel({500, 500}, {200, 80}, 8,
                                                MAP_RECT, &large_block);

  ok1(small && large_first && large_second);
  ok1(small && large_first &&
      large_first->visual_rect.GetWidth() > small->visual_rect.GetWidth() &&
      large_first->visual_rect.GetHeight() > small->visual_rect.GetHeight());
  ok1(large_first && large_second &&
      !large_first->visual_rect.WithMargin(8).OverlapsWith(
        large_second->visual_rect.WithMargin(8)));
  ok1(large_first && large_second &&
      large_first->candidate_index == AirspaceLabelCandidate::BELOW &&
      large_second->candidate_index == AirspaceLabelCandidate::ABOVE);
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
  const auto first = PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE,
                                         MAP_RECT, &first_block);
  LabelBlock second_block;
  const auto second = PlaceAirspaceLabel({500, 500}, LABEL_SIZE, CLEARANCE,
                                          MAP_RECT, &second_block);

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
                     MakeAltitude(3000), 1);
  warning_labels.Add(GeoPoint::Zero(), CLASSD, MakeAltitude(1000),
                     MakeAltitude(3000), 2);
  warning_labels.Sort(config);
  ok1(warning_labels[0].cls == CLASSD);
  ok1(warning_labels[1].border_class == CTR);

  LabelBlock warning_block;
  const auto warning_winner = PlaceAirspaceLabel({500, 500}, LABEL_SIZE,
                                                  CLEARANCE, MAP_RECT,
                                                  &warning_block);
  const auto warning_loser = PlaceAirspaceLabel({500, 500}, LABEL_SIZE,
                                                 CLEARANCE, MAP_RECT,
                                                 &warning_block);
  ok1(warning_winner &&
      warning_winner->candidate_index == AirspaceLabelCandidate::BELOW);
  ok1(warning_loser &&
      warning_loser->candidate_index != AirspaceLabelCandidate::BELOW);

  AirspaceLabelList altitude_labels;
  altitude_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(1000),
                      MakeAltitude(3000), 3);
  altitude_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(2000),
                      MakeAltitude(3000), 4);
  altitude_labels.Sort(config);
  ok1(altitude_labels[0].base.altitude == 2000);

  AirspaceLabelList equal_labels;
  equal_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(1000),
                   MakeAltitude(3000), 2);
  equal_labels.Add(GeoPoint::Zero(), CLASSC, MakeAltitude(1000),
                   MakeAltitude(3000), 1);
  equal_labels.Sort(config);
  ok1(equal_labels[0].identity == 1 && equal_labels[1].identity == 2);
}

int
main()
{
  plan_tests(53);

  TestPreferredPlacement();
  TestFallbackPlacement();
  TestPreferredCandidate();
  TestCachedCandidateReuse();
  TestBlockedPreferredCandidate();
  TestPreferredCandidateGeometry();
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
