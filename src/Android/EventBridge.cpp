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

#include "org_xcsoar_EventBridge.h"

#include <SDL_events.h>

static SDL_keysym
translate_key_code(jint key_code)
{
  /* XXX implement */
  SDL_keysym keysym;
  keysym.sym = (SDLKey)key_code;
  return keysym;
}

void
Java_org_xcsoar_EventBridge_onKeyDown(JNIEnv *env, jclass cls, jint key_code)
{
  SDL_Event event;
  event.type = SDL_KEYDOWN;
  event.key.state = SDL_PRESSED;
  event.key.keysym = translate_key_code(key_code);
  SDL_PushEvent(&event);
}

void
Java_org_xcsoar_EventBridge_onKeyUp(JNIEnv *env, jclass cls, jint key_code)
{
  SDL_Event event;
  event.type = SDL_KEYUP;
  event.key.state = SDL_RELEASED;
  event.key.keysym = translate_key_code(key_code);
  SDL_PushEvent(&event);
}

void
Java_org_xcsoar_EventBridge_onMouseDown(JNIEnv *env, jclass cls,
                                        jint x, jint y)
{
  SDL_Event event;
  event.type = SDL_MOUSEBUTTONDOWN;
  event.button.button = SDL_BUTTON_LEFT;
  event.button.state = SDL_PRESSED;
  event.button.x = x;
  event.button.y = y;
  SDL_PushEvent(&event);
}

void
Java_org_xcsoar_EventBridge_onMouseUp(JNIEnv *env, jclass cls,
                                        jint x, jint y)
{
  SDL_Event event;
  event.type = SDL_MOUSEBUTTONUP;
  event.button.button = SDL_BUTTON_LEFT;
  event.button.state = SDL_RELEASED;
  event.button.x = x;
  event.button.y = y;
  SDL_PushEvent(&event);
}

void
Java_org_xcsoar_EventBridge_onMouseMove(JNIEnv *env, jclass cls,
                                        jint x, jint y)
{
  SDL_Event event;
  event.type = SDL_MOUSEMOTION;
  event.motion.x = x;
  event.motion.y = y;
  SDL_PushEvent(&event);
}
