// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../ContestTrace.hpp"
#include "Trace/Point.hpp"

inline
ContestTracePoint::ContestTracePoint(const TracePoint &src) noexcept
  :time(src.GetTime()), location(src.GetLocation()) {}
