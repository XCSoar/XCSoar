// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "org_xcsoar_EventBridge.h"
#include "Product.hpp"
#include "ui/event/Queue.hpp"
#include "ui/event/Idle.hpp"
#include "ui/event/Globals.hpp"
#include "util/Compiler.h"

#include <atomic>

using namespace UI;

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
static constexpr unsigned KEYCODE_DPAD_LEFT = 0x15;
static constexpr unsigned KEYCODE_DPAD_RIGHT = 0x16;

static unsigned
TranslateKeyCode(unsigned key_code)
{
  if (key_code == KEYCODE_BACK)
    /* the "back" key acts as escape; on Android 16+ (target API 36)
       KEYCODE_BACK is no longer dispatched, so XCSoar.java injects
       this same code from OnBackInvokedCallback */
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
IndicatesCursorKeys(unsigned key_code)
{
  /* a real Escape key also means a keyboard or remote stick: it is
     the first key to select an overlay such as the airspace warning
     banner; the "back" button (KEYCODE_BACK) does not count */
  return key_code == KEYCODE_DPAD_UP || key_code == KEYCODE_DPAD_DOWN ||
    key_code == KEYCODE_DPAD_LEFT || key_code == KEYCODE_DPAD_RIGHT ||
    key_code == KEYCODE_ESCAPE;
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onKeyDown([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls, jint key_code)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  if (IndicatesCursorKeys(key_code)) {
    bool expected = false;
    /* enable this flag as soon as we see the first cursor event; used
       by HasCursorKeys() */
    has_cursor_keys.compare_exchange_strong(expected, true, 
                                            std::memory_order_relaxed);
  }

  event_queue->Inject(Event(Event::KEY_DOWN, TranslateKeyCode(key_code)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onKeyUp([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls, jint key_code)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Inject(Event(Event::KEY_UP, TranslateKeyCode(key_code)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onMouseDown([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls,
                                        jint x, jint y)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Inject(Event(Event::MOUSE_DOWN, PixelPoint(x, y)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onMouseUp([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls,
                                      jint x, jint y)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Inject(Event(Event::MOUSE_UP, PixelPoint(x, y)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onMouseMove([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls,
                                        jint x, jint y)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Purge(Event::MOUSE_MOTION);
  event_queue->Inject(Event(Event::MOUSE_MOTION, PixelPoint(x, y)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onMouseCancel([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Inject(Event::MOUSE_CANCEL);
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onPointerDown([[maybe_unused]] JNIEnv *env,
                                          [[maybe_unused]] jclass cls,
                                          jint x1, jint y1,
                                          jint x2, jint y2)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Inject(Event(Event::POINTER_DOWN,
                            PixelPoint(x1, y1),
                            PixelPoint(x2, y2)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onPointerMove([[maybe_unused]] JNIEnv *env,
                                          [[maybe_unused]] jclass cls,
                                          jint x1, jint y1,
                                          jint x2, jint y2)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Purge(Event::POINTER_MOVE);
  event_queue->Inject(Event(Event::POINTER_MOVE,
                            PixelPoint(x1, y1),
                            PixelPoint(x2, y2)));
  ResetUserIdle();
}

gcc_visibility_default
void
Java_org_xcsoar_EventBridge_onPointerUp([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls)
{
  if (event_queue == nullptr)
    /* XCSoar not yet initialised */
    return;

  event_queue->Inject(Event::POINTER_UP);
  ResetUserIdle();
}
