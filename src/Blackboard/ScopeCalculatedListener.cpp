// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScopeCalculatedListener.hpp"

void
ScopeCalculatedListener::OnCalculatedUpdate(const MoreData &basic,
                                            const DerivedInfo &calculated)
{
  function(basic, calculated);
}
