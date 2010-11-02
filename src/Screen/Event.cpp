/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Screen/Event.hpp"
#include "Thread/Debug.hpp"
#include "Asset.hpp"

#ifdef ENABLE_SDL

#include "Screen/TopWindow.hpp"

bool
EventLoop::get(SDL_Event &event)
{
  if (event.type == SDL_QUIT)
    return false;

  if (bulk) {
    if (::SDL_PollEvent(&event))
      return true;

    /* that was the last event for now, refresh the screen now */
    top_window.refresh();
    bulk = false;
  }

  if (::SDL_WaitEvent(&event)) {
    bulk = true;
    return true;
  }

  return false;
}

void
EventLoop::dispatch(SDL_Event &event)
{
  if (event.type == Window::EVENT_USER && event.user.data1 != NULL) {
    Window *window = (Window *)event.user.data1;
    window->on_user(event.user.code);
  } else if (event.type == Window::EVENT_TIMER && event.user.data1 != NULL) {
    Window *window = (Window *)event.user.data1;
    SDLTimer *timer = (SDLTimer *)event.user.data2;
    window->on_timer(timer);
  } else
    ((Window &)top_window).on_event(event);
}

#else /* !ENABLE_SDL */

bool
EventLoop::get(MSG &msg)
{
  assert_none_locked();

  if (!::GetMessage(&msg, NULL, 0, 0))
    return false;

  ::TranslateMessage(&msg);
  return true;
}

void
EventLoop::dispatch(const MSG &msg)
{
  assert_none_locked();
  ::DispatchMessage(&msg);
  assert_none_locked();
}

#endif /* !ENABLE_SDL */

unsigned
TranscodeKey(unsigned key_code)
{
  // VENTA-ADDON HARDWARE KEYS TRANSCODING

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
  }

  return key_code;
}
