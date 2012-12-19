/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Screen/Blank.hpp"

#ifdef HAVE_BLANK

#include "Interface.hpp"
#include "Hardware/Battery.hpp"
#include "Hardware/Display.hpp"
#include "UIState.hpp"
#include "Event/Idle.hpp"

static void
BlankDisplay(bool doblank)
{
  if (!CommonInterface::GetUISettings().display.enable_auto_blank)
    return;

  UIState &ui_state = CommonInterface::SetUIState();

  if (doblank == ui_state.screen_blanked)
    return;

  if (!Display::BlankSupported())
    // can't do it, not supported
    return;

  if (doblank) {
    if (Power::External::Status == Power::External::OFF) {
      // Power off the display
      Display::Blank(true);
      ui_state.screen_blanked = true;
    }
  } else {
    // was blanked
    // Power on the display
    Display::Blank(false);
    ui_state.screen_blanked = false;
  }
}

void
CheckDisplayTimeOut(bool sticky)
{
#if defined(WIN32) && !defined(_WIN32_WCE)
  SystemIdleTimerReset();
#endif

  if (sticky) {
    // JMW don't let display timeout while a dialog is active
    return;
  }

  BlankDisplay(IsUserIdle(60000));
}

#endif /* HAVE_BLANK */
