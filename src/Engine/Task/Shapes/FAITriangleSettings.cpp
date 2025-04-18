// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAITriangleSettings.hpp"
#include "FAITriangleRules.hpp"
#include "util/Compiler.h"

#include <cassert>

double
FAITriangleSettings::GetThreshold() const
{
  switch (threshold) {
  case Threshold::FAI:
    return FAITriangleRules::LARGE_THRESHOLD_FAI;

  case Threshold::KM500:
    return FAITriangleRules::LARGE_THRESHOLD_500;

  case Threshold::MAX:
    break;
  }

  assert(false);
  gcc_unreachable();
}
