/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "org_xcsoar_EventBridge.h"
#include "Product.hpp"
#include "Event/Queue.hpp"
#include "Event/Idle.hpp"
#include "Event/Globals.hpp"
#include "OS/Clock.hpp"
#include "Compiler.h"

/**
 * @see http://developer.android.com/reference/android/view/KeyEvent.html
 */
enum {
  KEYCODE_BACK = 0x04,
  KEYCODE_0 = 0x07,
  KEYCODE_9 = 0x10,
  KEYCODE_A = 0x1d,
  KEYCODE_Z = 0x36,
  KEYCODE_ESCAPE = 0x6f,
};

static constexpr unsigned KEYCODE_DPAD_UP = 0x13;
static constexpr unsigned KEYCODE_DPAD_DOWN = 0x14;

static unsigned
TranslateKeyCode(unsigned key_code)
{
  if (key_code == KEYCODE_BACK)
    /* the "back" key acts as escape */
    return KEYCODE_ESCAPE;

  if (key_code >= KEYCODE_0 && key_code <= KEYCODE_9)
    return '0' + (key_code - KEYCODE_0);

  if (key_code >= KEYCODE_A && key_code <= KEYCODE_Z)
    /* return upper-case character, because InputEvents::findKey()
       calls ToUpperASCII() */
    return 'A' + (key_code - KEYCODE_A);

  return key_code;
}

constexpr
static bool
IsCursorKey(unsigned key_code)
{
  return key_code == KEYCODE_DPAD_UP || key_code == KEYCODE_DPAD_DOWN;
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onKeyDown(JNIEnv *env, jclass cls, jint key_code)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  if (!has_cursor_keys && IsCursorKey(key_code))
    /* enable this flag as soon as we see the first cursor event; used
       by HasCursorKeys() */
    has_cursor_keys = true;

  event_queue->Push(Event(Event::KEY_DOWN, TranslateKeyCode(key_code)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onKeyUp(JNIEnv *env, jclass cls, jint key_code)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Push(Event(Event::KEY_UP, TranslateKeyCode(key_code)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onMouseDown(JNIEnv *env, jclass cls,
                                        jint x, jint y)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Push(Event(Event::MOUSE_DOWN, PixelPoint(x, y)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onMouseUp(JNIEnv *env, jclass cls,
                                      jint x, jint y)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Push(Event(Event::MOUSE_UP, PixelPoint(x, y)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onMouseMove(JNIEnv *env, jclass cls,
                                        jint x, jint y)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Purge(Event::MOUSE_MOTION);
  event_queue->Push(Event(Event::MOUSE_MOTION, PixelPoint(x, y)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onPointerDown(JNIEnv *env, jclass cls)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Push(Event(Event::POINTER_DOWN));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onPointerUp(JNIEnv *env, jclass cls)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Push(Event(Event::POINTER_UP));
  ResetUserIdle();
}
