// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "Weather/MapOverlay/InputEvents.hpp"

// WeatherOverlay
// Adjusts the active map weather overlay cursor bar.
// time +/-, time auto on/off/toggle/show
// altitude +/-, altitude auto on/off/toggle/show
// level +/- and level auto … (EDL pressure level alias for altitude)
void
InputEvents::eventWeatherOverlay(const char *misc)
{
  WeatherMapOverlay::HandleInputEvent(misc);
}
