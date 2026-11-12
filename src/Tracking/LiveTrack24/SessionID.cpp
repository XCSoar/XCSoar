// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SessionID.hpp"

#include <random>

namespace LiveTrack24 {

SessionID
GenerateSessionID() noexcept
{
  static std::mt19937 generator{std::random_device{}()};
  std::uniform_int_distribution<SessionID> distribution{1};
  return distribution(generator);
}

} // namespace Livetrack24
