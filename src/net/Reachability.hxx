// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "State.hpp"

/**
 * Minimal, low-overhead internet reachability check.
 *
 * A background thread performs a short non-blocking TCP connect to a
 * well-known IPv4 address and caches the boolean result.
 *
 * Two entry points:
 *  - IsInternetReachable()  — always triggers a probe (if none pending).
 *  - PollInternetReachable() — throttled to one probe every 30 s.
 */

#ifdef HAVE_NET_STATE

/**
 * Trigger an immediate background probe (if none is already pending)
 * and return the last cached result.
 */
bool
IsInternetReachable() noexcept;

/**
 * Throttled variant for callers that poll every frame (e.g. render
 * thread).  Only triggers a probe every 30 seconds.
 */
bool
PollInternetReachable() noexcept;

#else

constexpr bool
IsInternetReachable() noexcept
{
  return false;
}

constexpr bool
PollInternetReachable() noexcept
{
  return false;
}

#endif
