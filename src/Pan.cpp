// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Pan.hpp"
#include "Simulator.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "util/ScopeExit.hxx"

#include <cassert>

namespace {

/**
 * Suspend weather overlays if needed and show the fullscreen map.
 * On failure, resumes weather if it was suspended.
 *
 * @param abort_if_already_panning  if true, treat an already-panning
 *                                  map as failure (EnterPan)
 */
GlueMapWindow *
PreparePanFullscreen(bool abort_if_already_panning) noexcept
{
  const bool suspending_weather =
    PageActions::GetCurrentLayout().UsesWeatherOverlay();
  if (suspending_weather)
    PageActions::SuspendWeatherOverlaysForPan();

  GlueMapWindow *map = PageActions::ShowOnlyMap();
  if (map == nullptr ||
      (abort_if_already_panning && map->IsPanning())) {
    if (suspending_weather)
      PageActions::ResumeWeatherOverlaysAfterPan();
    return nullptr;
  }

  return map;
}

void
FinishEnterPan() noexcept
{
  InputEvents::setMode(InputEvents::MODE_DEFAULT);
  InputEvents::UpdatePan();
}

} // anonymous namespace

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

  GlueMapWindow *map = PreparePanFullscreen(true);
  if (map == nullptr)
    return;

  map->SetPan(true);
  FinishEnterPan();
}

bool
PanTo(const GeoPoint &location)
{
  assert(CommonInterface::main_window != nullptr);

  GlueMapWindow *map = PreparePanFullscreen(false);
  if (map == nullptr)
    return false;

  map->PanTo(location);
  FinishEnterPan();
  return true;
}

bool
SimJumpTo(const GeoPoint &location)
{
#ifdef SIMULATOR_AVAILABLE
  assert(CommonInterface::main_window != nullptr);

  if (!is_simulator() || backend_components == nullptr ||
      backend_components->device_blackboard == nullptr)
    return false;

  backend_components->device_blackboard->SetSimulatorLocation(location);

  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->FullRedraw();

  return true;
#else
  (void)location;
  return false;
#endif
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

  if (!map->IsPanning() && !PageActions::IsStuckPanFullScreenLayout())
    return;

  /* Coalesce leaving follow-pan with the page restore so the map is
     not painted once at fullscreen with FOLLOW_SELF and again after
     InfoBoxes return. */
  auto &main_window = *CommonInterface::main_window;
  main_window.BeginCoalesceMapLayout();
  AtScopeExit(&main_window) { main_window.EndCoalesceMapLayout(); };

  if (map->IsPanning())
    map->SetPan(false);

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
