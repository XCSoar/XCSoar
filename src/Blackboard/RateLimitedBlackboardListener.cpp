// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RateLimitedBlackboardListener.hpp"

void
RateLimitedBlackboardListener::OnGPSUpdate(const MoreData &_basic)
{
  basic = &_basic;
  Trigger();
}

void
RateLimitedBlackboardListener::OnCalculatedUpdate(const MoreData &_basic,
                                                  const DerivedInfo &_calculated)
{
  basic2 = &_basic;
  calculated = &_calculated;
  Trigger();
}

void
RateLimitedBlackboardListener::Run()
{
  if (basic != nullptr) {
    next.OnGPSUpdate(*basic);
    basic = nullptr;
  }

  if (calculated != nullptr){
    next.OnCalculatedUpdate(*basic2, *calculated);
    basic2 = nullptr;
    calculated = nullptr;
  }
}
