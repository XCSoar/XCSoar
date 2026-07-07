// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Pan.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"

#include <cassert>

bool
IsPanning()
{
  const GlueMapWindow *map = UIGlobals::GetMapIfActive();
  return map != nullptr && map->IsPanning();
}

void
EnterPan()
{
  assert(CommonInterface::main_window != nullptr);

  const bool suspending_weather_page =
    PageActions::GetCurrentLayout().UsesWeatherOverlay();
  if (suspending_weather_page)
    PageActions::SuspendWeatherOverlaysForPan();

  GlueMapWindow *map = PageActions::ShowOnlyMap();
  if (map == nullptr || map->IsPanning()) {
    if (suspending_weather_page)
      PageActions::ResumeWeatherOverlaysAfterPan();
    return;
  }

  map->SetPan(true);

  InputEvents::setMode(InputEvents::MODE_DEFAULT);
  InputEvents::UpdatePan();
}

bool
PanTo(const GeoPoint &location)
{
  assert(CommonInterface::main_window != nullptr);

  const bool suspending_weather_page =
    PageActions::GetCurrentLayout().UsesWeatherOverlay();
  if (suspending_weather_page)
    PageActions::SuspendWeatherOverlaysForPan();

  GlueMapWindow *map = PageActions::ShowOnlyMap();
  if (map == nullptr) {
    if (suspending_weather_page)
      PageActions::ResumeWeatherOverlaysAfterPan();
    return false;
  }

  map->PanTo(location);

  InputEvents::setMode(InputEvents::MODE_DEFAULT);
  InputEvents::UpdatePan();
  return true;
}

void
DisablePan()
{
  GlueMapWindow *map = UIGlobals::GetMapIfActive();
  if (map == nullptr || !map->IsPanning())
    return;

  map->SetPan(false);

  InputEvents::UpdatePan();
  PageActions::ResumeWeatherOverlaysAfterPan();
}

void
LeavePan()
{
  GlueMapWindow *map = UIGlobals::GetMapIfActive();
  if (map == nullptr) {
    PageActions::ResumeWeatherOverlaysAfterPan();
    return;
  }

  if (map->IsPanning())
    map->SetPan(false);
  else if (!PageActions::IsStuckPanFullScreenLayout())
    return;

  InputEvents::UpdatePan();
  PageActions::Restore();
  PageActions::ResumeWeatherOverlaysAfterPan();
}

void
TogglePan()
{
  if (IsPanning())
    LeavePan();
  else
    EnterPan();
}
