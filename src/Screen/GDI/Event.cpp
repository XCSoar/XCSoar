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

#include "Screen/GDI/Event.hpp"
#include "Screen/GDI/Key.h"
#include "Thread/Debug.hpp"
#include "Asset.hpp"

bool
EventLoop::get(MSG &msg)
{
  assert_none_locked();

  if (!::GetMessage(&msg, NULL, 0, 0))
    return false;

  return true;
}

void
EventLoop::dispatch(const MSG &msg)
{
  assert_none_locked();
  ::TranslateMessage(&msg);
  ::DispatchMessage(&msg);
  assert_none_locked();
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
     can't implement Window::on_key_check() */
  if (is_altair() && (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) &&
      (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
    return false;

  return true;
}

void
DialogEventLoop::dispatch(MSG &msg)
{
  assert_none_locked();

  if (AllowDialogMessage(msg) && ::IsDialogMessage(dialog, &msg)) {
    assert_none_locked();
    return;
  }

  if (is_altair() && msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
    /* the Windows CE dialog manager does not handle VK_ESCAPE, but
       the Altair needs it - let's roll our own */
    ::SendMessage(dialog, WM_COMMAND, IDCANCEL, 0);
    return;
  }

  EventLoop::dispatch(msg);
}

static void
HandleMessages(UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  MSG msg;
  while (::PeekMessage(&msg, NULL, wMsgFilterMin, wMsgFilterMax, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
}

void
EventQueue::HandlePaintMessages()
{
  assert_none_locked();

  HandleMessages(WM_PAINT, WM_PAINT);
}

unsigned
TranscodeKey(unsigned key_code)
{
#ifdef _WIN32_WCE
  /* VK_F23 is the "action" key on some iPaqs */
  if (key_code == VK_F23)
    return VK_RETURN;
#endif

  if (GlobalModelType == MODELTYPE_PNA_HP31X) {
    if (key_code == 0x7b)
      key_code = 0x1b;
  } else if (GlobalModelType == MODELTYPE_PNA_PN6000) {
    switch(key_code) {
    case 0x79: // Upper Silver key short press
      key_code = 0xc1; // F10 -> APP1
      break;
    case 0x7b: // Lower Silver key short press
      key_code = 0xc2; // F12 -> APP2
      break;
    case 0x72: // Back key plus
      key_code = 0xc3; // F3  -> APP3
      break;
    case 0x71: // Back key minus
      key_code = 0xc4; // F2  -> APP4
      break;
    case 0x7a: // Upper silver key LONG press
      key_code = 0x70; // F11 -> F1
      break;
    case 0x7c: // Lower silver key LONG press
      key_code = 0x71; // F13 -> F2
      break;
    }
  } else if (GlobalModelType == MODELTYPE_PNA_NOKIA_500) {
    switch(key_code) {
    case 0xc1:
      key_code = 0x0d; // middle key = enter
      break;
    case 0xc5:
      key_code = 0x26; // + key = pg Up
      break;
    case 0xc6:
      key_code = 0x28; // - key = pg Down
      break;
    }
  } else if (GlobalModelType == MODELTYPE_PNA_MEDION_P5) {
    switch(key_code) {
    case 0x79:
      key_code = 0x0d; // middle key = enter
      break;
    case 0x75:
      key_code = 0x26; // + key = pg Up
      break;
    case 0x76:
      key_code = 0x28; // - key = pg Down
      break;
    }
  } else if (is_altair()){  // handles new keypad driver button codes
    switch(key_code) {
    case VK_F1:
      return VK_APP1;

    case VK_F2:
      return VK_APP2;

    case VK_F3:
      return VK_APP3;

    case VK_F4:
      return VK_APP4;

    case VK_F5:
      key_code = '6';
      break;
    case VK_F6:
      key_code = '7';
      break;
    case VK_F7:
      key_code = '8';
      break;
    case VK_F8:
      key_code = '9';
      break;
    case VK_F9:
      key_code = '0';
      break;
    }
  }

  return key_code;
}
