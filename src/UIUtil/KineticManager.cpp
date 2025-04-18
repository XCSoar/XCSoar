// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#include "KineticManager.hpp"
#include "time/Cast.hxx"

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
  const auto dt = clock.Elapsed();

  // Filter fast updates to get a better velocity
  if (dt < std::chrono::milliseconds(15))
    return;

  // Update clock for next event
  clock.Update();

  // Calculate value delta
  int dx = x - last;

  // Calculate value-based velocity
  v = double(dx) / ToFloatSeconds(dt);

  // Save value for next event
  last = x;
}

void
KineticManager::MouseUp(int x)
{
  // Calculate end position of the kinetic movement
  const auto dt = clock.Elapsed();
  if (dt > std::chrono::milliseconds(200)) {
    end = x;
    steady = true;
  } else
    end = last + (int)((v / 2) * ToFloatSeconds(stopping_time));
}

int
KineticManager::GetPosition()
{
  // Get time that has passed since the end of the manual movement
  const auto t = clock.Elapsed();

  // If more time has passed than allocated for the kinetic movement
  if (t >= stopping_time) {
    // Stop the kinetic movement and return the precalculated end position
    steady = true;
    return end;
  }

  // Calculate the current position of the kinetic movement
  const auto ts = ToFloatSeconds(t);
  int x = last + (int)(v * ts - v * ts * ts / (2 * ToFloatSeconds(stopping_time)));
  if (x == end)
    steady = true;

  return x;
}

bool
KineticManager::IsSteady()
{
  return steady;
}
