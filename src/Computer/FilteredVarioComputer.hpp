// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Util/VarioOutputFilter.hpp"

struct MoreData;

/**
 * Applies device-configured vario filtering to MoreData after raw
 * brutto/netto have been computed.
 */
class FilteredVarioComputer {
  VarioOutputFilter filter;
  double last_period = -1;
  bool sample_updated = false;

public:
  void Reset() noexcept {
    filter.Design(0);
    filter.Reset(0);
    last_period = -1;
    sample_updated = false;
  }

  void Compute(MoreData &data, double sink_rate) noexcept;

  [[gnu::pure]]
  bool SampleUpdated() const noexcept {
    return sample_updated;
  }

  [[gnu::pure]]
  bool FilterActive() const noexcept {
    return filter.IsActive();
  }
};
