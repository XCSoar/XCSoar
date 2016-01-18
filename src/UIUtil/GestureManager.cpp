/*
 * Copyright (C) 2003-2010 Tobias Bieniek <Tobias.Bieniek@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "GestureManager.hpp"
#include "Math/FastMath.hpp"
#include "Compiler.h"

static gcc_const TCHAR
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
