// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SessionID.hpp"

#include <cstdlib>

namespace LiveTrack24 {

SessionID
GenerateSessionID() noexcept
{
  int r = rand();
  return (r & 0x7F000000) | 0x80000000;
}

SessionID
GenerateSessionID(UserID user_id) noexcept
{
  return GenerateSessionID() | (user_id & 0x00ffffff);
}

} // namespace Livetrack24
