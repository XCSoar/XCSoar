// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#include "GestureManager.hpp"
#include "Math/FastMath.hpp"
#include "util/Compiler.h"

[[gnu::const]] static TCHAR
getDirection(int dx, int dy)
{
  if (dy < 0 && -dy >= abs(dx) * 2)
    return _T('U');
  if (dy > 0 && dy >= abs(dx) * 2)
    return _T('D');
  if (dx > 0 && dx >= abs(dy) * 2)
    return _T('R');
  if (dx < 0 && -dx >= abs(dy) * 2)
    return _T('L');

  return _T('\0');
}

bool
GestureManager::Update(PixelPoint p)
{
  // Calculate deltas
  auto d = p - drag_last;

  // See if we've reached the threshold already
  if (compare_squared(d.x, d.y, threshold) != 1)
    return false;

  // Save position for next call
  drag_last = p;

  // Get current dragging direction
  TCHAR direction = getDirection(d.x, d.y);

  // Return if we are in an unclear direction
  if (direction == '\0')
    return true;

  // Return if we are still in the same direction
  if (gesture.empty() || gesture.back() != direction)
    gesture.push_back(direction);

  return true;
}

void
GestureManager::Start(PixelPoint p, int _threshold)
{
  // Reset last position
  drag_last = p;

  // Reset gesture
  gesture.clear();

  // Set threshold
  threshold = _threshold;
}

const TCHAR*
GestureManager::Finish()
{
  return GetGesture();
}

const TCHAR*
GestureManager::GetGesture() const
{
  return gesture.empty()
    ? NULL
    : gesture.c_str();
}
