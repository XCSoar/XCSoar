// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

/**
 * Prevent object from being copied directly
 */
class NonCopyable
{
protected:
  constexpr NonCopyable () {}
  /** Protected non-virtual destructor */
  ~NonCopyable () {}

private:
  NonCopyable(const NonCopyable &);
  NonCopyable &operator=(const NonCopyable &);
};
