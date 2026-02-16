// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

#include <tchar.h>

struct PolarShape;
struct PolarInfo;

namespace PolarStore {

struct Item {
  /**< Name of the glider type */
  const char *name;

  // Using doubles here to simplify the code in PolarStore.cpp

  /** Reference mass of the polar (kg) */
  double reference_mass;

  /** Max water ballast (l) */
  double max_ballast;

  /** Speed (kph) of point 1 */
  double v1;
  /** Sink rate (negative, m/s) of point 1  */
  double w1;
  /** Speed (kph) of point 2 */
  double v2;
  /** Sink rate (negative, m/s) of point 2  */
  double w2;
  /** Speed (kph) of point 3 */
  double v3;
  /** Sink rate (negative, m/s) of point 3  */
  double w3;

  /** Reference wing area (m^2), 0.0 if unknown */
  double wing_area;

  /** Maximum speed for normal operations (m/s), 0.0 if unknown */
  double v_no;

  /** Contest handicap, 0 if unknown */
  unsigned contest_handicap;

  /** empty rigged glider mass (kg), make the polar reference mass independent of the lift of weight sum */
  unsigned empty_mass;

  [[gnu::pure]]
  PolarShape ToPolarShape() const noexcept;

  [[gnu::pure]]
  PolarInfo ToPolarInfo() const noexcept;
};

using const_iterator = const struct Item *;

[[gnu::const]]
const Item &
GetDefault() noexcept;

[[gnu::const]]
std::span<const Item>
GetAll() noexcept;

} // namespace PolarStore
