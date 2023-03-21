// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClimbHistory.hpp"

#include <algorithm>

void
ClimbHistory::Clear()
{
  std::fill(vario.begin(), vario.end(), 0);
  std::fill(count.begin(), count.end(), 0);
}

void
ClimbHistory::Add(unsigned speed, double _vario)
{
  if (speed >= count.size())
    return;

  if (count[speed] >= 0x8000) {
    /* prevent integer overflow */
    vario[speed] /= 2;
    count[speed] /= 2;
  }

  vario[speed] += _vario;
  ++count[speed];
}
