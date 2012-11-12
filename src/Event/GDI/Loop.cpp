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

#include "Loop.hpp"
#include "Transcode.hpp"
#include "Screen/GDI/Key.h"
#include "Thread/Debug.hpp"
#include "Asset.hpp"

static bool
IsKeyMessage(const MSG &msg)
{
  return msg.message == WM_KEYDOWN || msg.message == WM_KEYUP;
}

bool
EventLoop::Get(MSG &msg)
{
  AssertNoneLocked();

  if (!::GetMessage(&msg, NULL, 0, 0))
    return false;

  if (IsOldWindowsCE() && IsKeyMessage(msg) &&
      msg.wParam >= KEY_APP1 && msg.wParam <= KEY_APP4) {
    /* kludge for iPaq 3xxx: the VK_APPx buttons emit a WM_KEYUP
       instead of WM_KEYDOWN when the user presses the button */

    static bool seen_app_down = false;
    if (msg.message == WM_KEYDOWN)
      /* everything seems ok, disable the kludge */
      seen_app_down = true;
    else if (!seen_app_down && msg.lParam == (LPARAM)0x80000001)
      msg.message = WM_KEYDOWN;
  }

  if (IsKeyMessage(msg))
    msg.wParam = TranscodeKey(msg.wParam);

  return true;
}

void
EventLoop::Dispatch(const MSG &msg)
{
  AssertNoneLocked();
  ::TranslateMessage(&msg);
  ::DispatchMessage(&msg);
  AssertNoneLocked();
}

/**
 * Checks if we should pass this message to the WIN32 dialog manager.
 */
gcc_pure
static bool
AllowDialogMessage(const MSG &msg)
{
  /* this hack disallows the dialog manager to handle VK_LEFT/VK_RIGHT
     on the Altair; some dialogs use the knob as a hot key, and they
     can't implement Window::OnKeyCheck() */
  if (IsAltair() && (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) &&
      (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
    return false;

  return true;
}

void
DialogEventLoop::Dispatch(MSG &msg)
{
  AssertNoneLocked();

  if (AllowDialogMessage(msg) && ::IsDialogMessage(dialog, &msg)) {
    AssertNoneLocked();
    return;
  }

  if (IsAltair() && msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
    /* the Windows CE dialog manager does not handle VK_ESCAPE, but
       the Altair needs it - let's roll our own */
    ::SendMessage(dialog, WM_COMMAND, IDCANCEL, 0);
    return;
  }

  EventLoop::Dispatch(msg);
}
