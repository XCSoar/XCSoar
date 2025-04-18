// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ProxyBlackboardListener.hpp"
#include "RateLimiter.hpp"

/**
 * A proxy #BlackboardListener that limits the rate of GPS and
 * Calculated updates.
 */
class RateLimitedBlackboardListener
  : public ProxyBlackboardListener, private RateLimiter {
  const MoreData *basic, *basic2;
  const DerivedInfo *calculated;

public:
  RateLimitedBlackboardListener(BlackboardListener &_next,
                                std::chrono::steady_clock::duration period,
                                std::chrono::steady_clock::duration delay) noexcept
    :ProxyBlackboardListener(_next),
     RateLimiter(period, delay),
     basic(nullptr), basic2(nullptr), calculated(nullptr) {}

  using RateLimiter::Cancel;

private:
  virtual void OnGPSUpdate(const MoreData &basic);

  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated);

  virtual void Run();
};
