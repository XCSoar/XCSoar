/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_RATE_LIMITED_BLACKBOARD_LISTENER_HPP
#define XCSOAR_RATE_LIMITED_BLACKBOARD_LISTENER_HPP

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
                                unsigned period_ms, unsigned delay_ms)
    :ProxyBlackboardListener(_next),
     RateLimiter(period_ms, delay_ms),
     basic(nullptr), basic2(nullptr), calculated(nullptr) {}

  using RateLimiter::Cancel;

private:
  virtual void OnGPSUpdate(const MoreData &basic);

  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated);

  virtual void Run();
};

#endif
