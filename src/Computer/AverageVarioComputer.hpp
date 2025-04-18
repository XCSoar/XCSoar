// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/WindowFilter.hpp"
#include "time/DeltaTime.hpp"

struct MoreData;
struct VarioInfo;

class AverageVarioComputer {
  DeltaTime delta_time;

  WindowFilter<30> vario_30s_filter;
  WindowFilter<30> netto_30s_filter;

public:
  void Reset();

  void Compute(const MoreData &basic,
               bool circling, bool last_circling,
               VarioInfo &calculated);
};
