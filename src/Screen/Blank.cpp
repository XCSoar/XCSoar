/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

/** timeout of display/battery mode in quarter seconds */
static const unsigned DISPLAYTIMEOUTMAX = 60 * 4;

unsigned DisplayTimeOut = 0;

static void
BlankDisplay(bool doblank)
{
  static bool oldblank = false;

  if (!XCSoarInterface::GetUISettings().enable_auto_blank)
    return;

  if (doblank == oldblank)
    return;

  if (!Display::BlankSupported())
    // can't do it, not supported
    return;

  UIState &ui_state = CommonInterface::SetUIState();

  if (doblank) {
    if (Power::External::Status == Power::External::OFF) {
      // Power off the display
      Display::Blank(true);
      oldblank = true;
      ui_state.screen_blanked = true;
    } else {
      ResetDisplayTimeOut();
    }
  } else {
    // was blanked
    // Power on the display
    Display::Blank(false);
    oldblank = false;
    ui_state.screen_blanked = false;
  }
}

void
CheckDisplayTimeOut(bool sticky)
{
#if defined(WIN32) && !defined(_WIN32_WCE)
  SystemIdleTimerReset();
#endif

  if (!sticky) {
    if (DisplayTimeOut < DISPLAYTIMEOUTMAX)
      DisplayTimeOut++;
  } else {
    // JMW don't let display timeout while a dialog is active
    ResetDisplayTimeOut();
    return;
  }
  if (DisplayTimeOut >= DISPLAYTIMEOUTMAX)
    BlankDisplay(true);
  else
    BlankDisplay(false);
}

#endif /* HAVE_BLANK */
