// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ASTPoint.hpp"

bool
ASTPoint::Equals(const OrderedTaskPoint &_other) const noexcept
{
  const ASTPoint &other = (const ASTPoint &)_other;

  return IntermediateTaskPoint::Equals(_other) &&
    other.GetScoreExit() == GetScoreExit();
}
