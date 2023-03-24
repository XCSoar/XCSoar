// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

extern bool has_cursor_keys;

#ifdef __arm__
extern bool is_nook, is_dithered;
#endif

/**
 * Returns whether the application is running on Nook Simple Touch
 */
#ifdef __arm__
[[gnu::const]]
#else
constexpr
#endif
static inline bool
IsNookSimpleTouch()
{
#ifdef __arm__
  return is_nook;
#else
  return false;
#endif
}
