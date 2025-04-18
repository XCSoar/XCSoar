// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SubOperationEnvironment.hpp"


void
SubOperationEnvironment::SetProgressRange(unsigned _range) noexcept
{
  range = _range;
}

void
SubOperationEnvironment::SetProgressPosition(unsigned position) noexcept
{
  ProxyOperationEnvironment::SetProgressPosition(parent_start + position * parent_range / range);
}
