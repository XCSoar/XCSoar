// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InputEvents.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "UIState.hpp"
#include "util/StringAPI.hxx"

void
InputEvents::eventPage(const char *misc)
{
  if (!StringIsEqual(misc, "restore")) return;

  /* Escape on the map is this event. A special page is something
     on the screen, so put the map back. Otherwise select the first
     InfoBox. */
  if (CommonInterface::GetUIState().pages.special_page.IsDefined()) {
    PageActions::Restore();
    return;
  }

  InfoBoxWindow *first = InfoBoxManager::GetWindow(0);
  if (first != nullptr && first->IsVisible()) first->SetFocus();
}
