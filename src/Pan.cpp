// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Pan.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"
#include "Weather/Features.hpp"
#ifdef HAVE_EDL
#include "Weather/EDL/StateController.hpp"
#endif
#ifdef HAVE_HTTP
#include "Weather/xctherm/XCThermMapOverlay.hpp"
#endif

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

  const PageLayout &layout = PageActions::GetCurrentLayout();

#ifdef HAVE_EDL
  const bool suspending_edl_page = layout.UsesEdlOverlay();
  if (suspending_edl_page)
    EDL::SuspendDedicatedPageForPan();
#endif

#ifdef HAVE_HTTP
  const bool suspending_xctherm_page =
    XCTherm::ShouldSuspendPageOverlayForPan(layout);
  if (suspending_xctherm_page)
    XCTherm::SuspendPageOverlayForPan();
#endif

  GlueMapWindow *map = PageActions::ShowOnlyMap();
  if (map == nullptr || map->IsPanning()) {
#ifdef HAVE_EDL
    if (suspending_edl_page)
      EDL::ResumeDedicatedPageAfterPan();
#endif
#ifdef HAVE_HTTP
    if (suspending_xctherm_page)
      XCTherm::ResumePageOverlayAfterPan();
#endif
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

  const PageLayout &layout = PageActions::GetCurrentLayout();

#ifdef HAVE_EDL
  const bool suspending_edl_page = layout.UsesEdlOverlay();
  if (suspending_edl_page)
    EDL::SuspendDedicatedPageForPan();
#endif

#ifdef HAVE_HTTP
  const bool suspending_xctherm_page =
    XCTherm::ShouldSuspendPageOverlayForPan(layout);
  if (suspending_xctherm_page)
    XCTherm::SuspendPageOverlayForPan();
#endif

  GlueMapWindow *map = PageActions::ShowOnlyMap();
  if (map == nullptr) {
#ifdef HAVE_EDL
    if (suspending_edl_page)
      EDL::ResumeDedicatedPageAfterPan();
#endif
#ifdef HAVE_HTTP
    if (suspending_xctherm_page)
      XCTherm::ResumePageOverlayAfterPan();
#endif
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
#ifdef HAVE_EDL
  EDL::ResumeDedicatedPageAfterPan();
#endif
#ifdef HAVE_HTTP
  XCTherm::ResumePageOverlayAfterPan();
#endif
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
#ifdef HAVE_EDL
  EDL::ResumeDedicatedPageAfterPan();
#endif
#ifdef HAVE_HTTP
  XCTherm::ResumePageOverlayAfterPan();
#endif
}

void
TogglePan()
{
  if (IsPanning())
    LeavePan();
  else
    EnterPan();
}
