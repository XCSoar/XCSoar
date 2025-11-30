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

  // Get current ballast in litres
  auto ballast_litres = glide_polar.GetBallastLitres();
  
  const double max_ballast = glide_polar.GetMaxBallast();
  
  if (max_ballast > 0) {
    // Convert to fraction, subtract dump rate, convert back to litres
    double ballast_fraction = glide_polar.GetBallastFraction();
    ballast_fraction -= ToFloatSeconds(dt) / dump_time;
    
    // Check if the plane is dry now
    if (ballast_fraction < 0) {
      Stop();
      glide_polar.SetBallastLitres(0);
      return false;
    }
    
    // Set new ballast using fraction
    glide_polar.SetBallastFraction(ballast_fraction);
  } else {
    // No max ballast - dump directly from litres (assume 1L per second as fallback)
    ballast_litres -= ToFloatSeconds(dt);
    
    // Check if the plane is dry now
    if (ballast_litres < 0) {
      Stop();
      glide_polar.SetBallastLitres(0);
      return false;
    }
    
    // Set new ballast
    glide_polar.SetBallastLitres(ballast_litres);
  }
  return true;
}
