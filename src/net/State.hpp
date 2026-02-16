// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#ifdef ANDROID
#define HAVE_NET_STATE
#define HAVE_NET_STATE_ROAMING
#elif defined(__linux__)
#define HAVE_NET_STATE
#elif defined(_WIN32)
#define HAVE_NET_STATE
#elif defined(__APPLE__)
#if !TARGET_OS_IPHONE
#define HAVE_NET_STATE
#endif
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
#elif defined(HAVE_NET_STATE)
NetState
GetNetState() noexcept;
#else
constexpr NetState
GetNetState() noexcept
{
  return NetState::UNKNOWN;
}
#endif
