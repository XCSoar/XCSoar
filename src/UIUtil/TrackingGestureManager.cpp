// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#include "TrackingGestureManager.hpp"

#include <cassert>

bool
TrackingGestureManager::Update(PixelPoint p)
{
  assert(!points.empty());

  points.back() = p;

  if (!GestureManager::Update(p))
    return false;

  points.emplace_back(p);
  return true;
}

void
TrackingGestureManager::Start(PixelPoint p, int threshold)
{
  // Start point
  points.emplace_back(p);

  // Next point that is changed by Update()
  points.emplace_back(p);

  GestureManager::Start(p, threshold);
}

const char*
TrackingGestureManager::Finish()
{
  points.clear();
  return GestureManager::Finish();
}
