// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace VarioGeometry {

static constexpr unsigned VERTICAL_SCALE_PERCENT = 112;

static constexpr unsigned COMPACT_HEIGHT_PERCENT = 90;

static constexpr unsigned
GetMaximumWidth(unsigned height) noexcept
{
  return height * 100 / (2 * VERTICAL_SCALE_PERCENT);
}

static constexpr unsigned
GetCompactWidth(unsigned height) noexcept
{
  return GetMaximumWidth(height * COMPACT_HEIGHT_PERCENT / 100);
}

} // namespace VarioGeometry
