// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Pan.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"
#include "Weather/EDL/StateController.hpp"

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

  if (PageActions::GetCurrentLayout().main == PageLayout::Main::EDL_MAP)
    EDL::SuspendDedicatedPageForPan();

  GlueMapWindow *map = PageActions::ShowOnlyMap();
  if (map == nullptr || map->IsPanning())
    return;

  map->SetPan(true);

  InputEvents::setMode(InputEvents::MODE_DEFAULT);
  InputEvents::UpdatePan();
}

bool
PanTo(const GeoPoint &location)
{
  assert(CommonInterface::main_window != nullptr);

  if (PageActions::GetCurrentLayout().main == PageLayout::Main::EDL_MAP)
    EDL::SuspendDedicatedPageForPan();

  GlueMapWindow *map = PageActions::ShowOnlyMap();
  if (map == nullptr)
    return false;

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
}

void
LeavePan()
{
  GlueMapWindow *map = UIGlobals::GetMapIfActive();
  if (map == nullptr || !map->IsPanning())
    return;

  map->SetPan(false);

  InputEvents::UpdatePan();
  PageActions::Restore();
  EDL::ResumeDedicatedPageAfterPan();
}

void
TogglePan()
{
  if (IsPanning())
    LeavePan();
  else
    EnterPan();
}
