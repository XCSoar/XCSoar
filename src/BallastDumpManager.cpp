// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BallastDumpManager.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "time/Cast.hxx"

void
BallastDumpManager::Start() noexcept
{
  assert(!IsEnabled());
  enabled = true;

  // Update ballast_clock for the next call to BallastDumpManager::Update()
  ballast_clock.Update();
}

void
BallastDumpManager::Stop() noexcept
{
  assert(IsEnabled());
  enabled = false;
}

void
BallastDumpManager::SetEnabled(bool _enabled) noexcept
{
  if (_enabled && !IsEnabled())
    Start();
  else if (!_enabled && IsEnabled())
    Stop();
}

bool
BallastDumpManager::Update(GlidePolar &glide_polar,
                           unsigned dump_time) noexcept
{
  assert(IsEnabled());

  // We don't know how fast the water is flowing so don't pretend that we do
  if (dump_time == 0) {
    Stop();
    return false;
  }

  // Milliseconds since last ballast_clock.Update() call
  const auto dt = ballast_clock.ElapsedUpdate();

  // Calculate the new ballast percentage
  auto ballast = glide_polar.GetBallast() - ToFloatSeconds(dt) / dump_time;

  // Check if the plane is dry now
  if (ballast < 0) {
    Stop();
    glide_polar.SetBallastLitres(0);
    return false;
  }

  // Set new ballast
  glide_polar.SetBallast(ballast);
  return true;
}
