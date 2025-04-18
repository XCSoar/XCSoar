// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/Flat/FlatProjection.hpp"
#include "FlatTriangleFanTree.hpp"

#include <optional>

class RoutePolars;
class RasterMap;
class GeoBounds;
struct ReachResult;

class ReachFan
{
  FlatProjection projection;
  FlatTriangleFanTree root;
  int terrain_base = 0;

public:
  friend class PrintHelper;

  bool IsEmpty() const noexcept {
    return root.IsEmpty();
  }

  const FlatProjection &GetProjection() const noexcept {
    return projection;
  }

  void Reset() noexcept;

  bool Solve(const AGeoPoint origin, const RoutePolars &rpolars,
             const RasterMap *terrain, const bool do_solve = true) noexcept;

  /**
   * Find arrival height at destination.
   *
   * Requires solve_reach() to have been called for positive results.
   *
   * @param dest Destination location
   * @param arrival_height_reach height at arrival (terrain reach) or -1 if out of reach
   * @param arrival_height_direct height at arrival (pure glide reach) or -1 if out of reach
   *
   * @return true if check was successful
   */
  [[gnu::pure]]
  std::optional<ReachResult> FindPositiveArrival(const AGeoPoint dest,
                                                 const RoutePolars &rpolars) const noexcept;

  /** Visit reach (working or terrain reach) */
  void AcceptInRange(const GeoBounds &bounds,
                     FlatTriangleFanVisitor &visitor) const noexcept;

  int GetTerrainBase() const noexcept {
    return terrain_base;
  }
};
