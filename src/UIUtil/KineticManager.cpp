/*
 * Copyright (C) 2011 Tobias Bieniek <Tobias.Bieniek@gmx.de>
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

#include "KineticManager.hpp"

void
KineticManager::MouseDown(int x)
{
  steady = false;
  last = x;
  clock.Update();
  v = 0;
}

void
KineticManager::MouseMove(int x)
{
  // Get time since last position update
  int dt = clock.Elapsed();

  // Filter fast updates to get a better velocity
  if (dt < 15)
    return;

  // Update clock for next event
  clock.Update();

  // Calculate value delta
  int dx = x - last;

  // Calculate value-based velocity
  v = double(dx) / dt;

  // Save value for next event
  last = x;
}

void
KineticManager::MouseUp(int x)
{
  // Calculate end position of the kinetic movement
  int dt = clock.Elapsed();
  if (dt > 200) {
    end = x;
    steady = true;
  } else
    end = last + (int)((v / 2) * stopping_time);
}

int
KineticManager::GetPosition()
{
  // Get time that has passed since the end of the manual movement
  int t = clock.Elapsed();

  // If more time has passed than allocated for the kinetic movement
  if (t >= stopping_time) {
    // Stop the kinetic movement and return the precalculated end position
    steady = true;
    return end;
  }

  // Calculate the current position of the kinetic movement
  int x = last + (int)(v * t - v * t * t / (2 * stopping_time));
  if (x == end)
    steady = true;

  return x;
}

bool
KineticManager::IsSteady()
{
  return steady;
}
