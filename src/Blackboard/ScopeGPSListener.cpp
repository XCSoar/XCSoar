// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScopeGPSListener.hpp"

void
ScopeGPSListener::OnGPSUpdate(const MoreData &basic)
{
  function(basic);
}
