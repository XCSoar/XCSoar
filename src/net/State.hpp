// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#define HAVE_NET_STATE
#define HAVE_NET_STATE_ROAMING
#elif defined(__linux__)
#define HAVE_NET_STATE
#endif

enum class NetState {
  UNKNOWN,
  DISCONNECTED,
  CONNECTED,
  ROAMING,
};

/**
 * Do we have an internet connection?
 */
#ifdef ANDROID
NetState
GetNetState();
#elif defined(__linux__)
NetState
GetNetState() noexcept;
#else
constexpr NetState
GetNetState() noexcept
{
  return NetState::UNKNOWN;
}
#endif
