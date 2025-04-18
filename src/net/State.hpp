// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#define HAVE_NET_STATE
#define HAVE_NET_STATE_ROAMING
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
#else
constexpr NetState
GetNetState()
{
  return NetState::UNKNOWN;
}
#endif
