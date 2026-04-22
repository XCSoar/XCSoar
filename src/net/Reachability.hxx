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
 *  - PollInternetReachable() — throttled to one probe every few seconds.
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
 * thread).  Uses #GetNetState on every call.  The TCP connect probe is
 * throttled to one attempt every few seconds while the link is up, but
 * a change in #GetNetState resets the throttle.
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
