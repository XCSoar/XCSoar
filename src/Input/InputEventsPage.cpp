// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InputEvents.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "PageActions.hpp"
#include "Pan.hpp"
#include "UIState.hpp"
#include "util/StringAPI.hxx"

void
InputEvents::eventPage(const char *misc)
{
  if (!StringIsEqual(misc, "restore")) return;

  /* Escape on the map is this event.  A special page is something on
     the screen, so put the map back first.  Then go to the airspace
     warning banner, then to a question below it (task advance, MAT).
     Otherwise select the first InfoBox. */
  if (CommonInterface::GetUIState().pages.special_page.IsDefined()) {
    PageActions::Restore();
    return;
  }

  /* only where processKey() offers the overlays their keys (not
     while panning), so the highlight can be moved and dropped again */
  if (IsDefault() && !IsPanning() &&
      (CommonInterface::main_window->FocusBottomBanner() ||
       CommonInterface::main_window->FocusBottomQuestion()))
    return;

  InfoBoxWindow *first = InfoBoxManager::GetWindow(0);
  if (first != nullptr && first->IsVisible()) first->SetFocus();
}
