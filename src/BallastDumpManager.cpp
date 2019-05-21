/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "BallastDumpManager.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Time/Cast.hxx"

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
