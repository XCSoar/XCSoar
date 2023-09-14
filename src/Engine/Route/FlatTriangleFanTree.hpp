// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/Flat/FlatBoundingBox.hpp"
#include "util/SliceAllocator.hxx"
#include "FlatTriangleFan.hpp"

#include <cstdint>
#include <forward_list>

class FlatProjection;
struct GeoPoint;
struct RouteLink;
struct AFlatGeoPoint;
struct ReachFanParms;
class FlatTriangleFanVisitor;

class FlatTriangleFanTree
{
  static constexpr unsigned BUFFER = 1;

  static constexpr unsigned MAX_DEPTH = 4;
  static constexpr unsigned MAX_VERTICES = 2000;

public:
  static constexpr unsigned MIN_STEP = 25;
  static constexpr unsigned MAX_FANS = 300;

private:
  FlatTriangleFan fan;

  using LeafVector =
    std::forward_list<FlatTriangleFanTree,
                      GlobalSliceAllocator<FlatTriangleFanTree, 128u>>;

  FlatBoundingBox bb_children;
  LeafVector children;
  uint_least8_t depth;
  bool gaps_filled = false;

public:
  friend class PrintHelper;

  explicit FlatTriangleFanTree(const uint_least8_t _depth = 0) noexcept
    :depth(_depth) {}

  void Clear() noexcept {
    fan.Clear();
    children.clear();
  }

  [[gnu::pure]]
  bool IsEmpty() const noexcept {
    return fan.IsEmpty();
  }

  [[gnu::pure]]
  auto GetHeight() const noexcept {
    return fan.GetHeight();
  }

  void FillReach(const AFlatGeoPoint &origin, ReachFanParms &parms) noexcept;
  void DummyReach(const AFlatGeoPoint &origin) noexcept;

  /**
   * Basic check for a state created by DummyReach().  If this method
   * returns true, then calls to FindPositiveArrival() are supposed to
   * be useless.
   */
  [[gnu::pure]]
  bool IsDummy() const noexcept {
    return fan.IsOnlyOrigin() && children.empty();
  }

  /**
   * Attempt to find a path to the specified #FlatGeoPoint higher than
   * the given #arrival_height.  If one is found, #arrival_height is
   * updated and the method returns true.
   */
  bool FindPositiveArrival(FlatGeoPoint n,
                           const ReachFanParms &parms,
                           int &arrival_height) const noexcept;

  void AcceptInRange(const FlatBoundingBox &bb,
                     FlatTriangleFanVisitor &visitor) const noexcept;

  void UpdateTerrainBase(FlatGeoPoint origin, ReachFanParms &parms) noexcept;

  [[gnu::pure]]
  int DirectArrival(FlatGeoPoint dest,
                    const ReachFanParms &parms) const noexcept;

private:
  bool IsRoot() const noexcept {
    return depth == 0;
  }

  const FlatBoundingBox &CalcBoundingBox() noexcept;

  /**
   * @return true if a valid fan has been filled, false to discard
   * this object
   */
  bool FillReach(const AFlatGeoPoint &origin,
                 const int index_low, const int index_high,
                 const ReachFanParms &parms) noexcept;

  bool FillDepth(const AFlatGeoPoint &origin, ReachFanParms &parms) noexcept;
  void FillGaps(const AFlatGeoPoint &origin, ReachFanParms &parms) noexcept;

  bool CheckGap(const AFlatGeoPoint &n, const RouteLink &e_1,
                const RouteLink &e_2, ReachFanParms &parms) noexcept;
};
