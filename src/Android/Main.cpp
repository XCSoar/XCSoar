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

#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "Android/SoundUtil.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Android/Event.hpp"
#include "Screen/OpenGL/Debug.hpp"
#include "Simulator.hpp"
#include "Asset.hpp"
#include "Profile/Profile.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Java/Global.hpp"
#include "org_xcsoar_NativeView.h"

#ifndef NDEBUG
#include "Screen/OpenGL/Texture.hpp"
#endif

#include <assert.h>

NativeView *native_view;

EventQueue *event_queue;

SoundUtil *sound_util;

JNIEXPORT jboolean JNICALL
Java_org_xcsoar_NativeView_initializeNative(JNIEnv *env, jobject obj,
                                            jint width, jint height)
{
  Java::Init(env);

#ifndef NDEBUG
  OpenGL::thread = pthread_self();
#endif

  assert(native_view == NULL);
  native_view = new NativeView(env, obj, width, height);
  InitAsset();

  Profile::SetFiles(_T(""));

  event_queue = new EventQueue();

  sound_util = new SoundUtil(env);

  return XCSoarInterface::Startup(NULL);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_runNative(JNIEnv *env, jobject obj)
{
#ifndef NDEBUG
  OpenGL::thread = pthread_self();
#endif

  CommonInterface::main_window.event_loop();
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_deinitializeNative(JNIEnv *env, jobject obj)
{
  CommonInterface::main_window.reset();
  Fonts::Deinitialize();
  Graphics::Deinitialise();
  delete sound_util;
  delete event_queue;
  delete native_view;
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resizedNative(JNIEnv *env, jobject obj,
                                         jint width, jint height)
{
  if (event_queue == NULL)
    return;

  event_queue->purge(Event::RESIZE);

  Event event(Event::RESIZE, width, height);
  event_queue->push(event);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_pauseNative(JNIEnv *env, jobject obj)
{
  if (event_queue == NULL)
    /* pause before we have initialized the event subsystem does not
       work - let's bail out, nothing is lost anyway */
    exit(0);

  CommonInterface::main_window.pause();

  assert(num_textures == 0);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resumeNative(JNIEnv *env, jobject obj)
{
  if (event_queue == NULL)
    /* there is nothing here yet which can be resumed */
    exit(0);

  CommonInterface::main_window.resume();
}
