// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "GliderLink/List.hpp"

/**
 * A container for all data received by GliderLink.
 */
struct GliderLinkData {
  GliderLinkTrafficList traffic;

  void Clear() noexcept {
    traffic.Clear();
  }

  void Complement(const GliderLinkData &add) {
    traffic.Replace(add.traffic);
  }

  void Expire(TimeStamp clock) noexcept {
    traffic.Expire(clock);
  }
};
