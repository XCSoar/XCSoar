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
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Android/Event.hpp"
#include "Simulator.hpp"
#include "Asset.hpp"
#include "Profile/Profile.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Java/Global.hpp"
#include "org_xcsoar_NativeView.h"

#include <assert.h>

NativeView *native_view;

EventQueue *event_queue;

JNIEXPORT jboolean JNICALL
Java_org_xcsoar_NativeView_initializeNative(JNIEnv *env, jobject obj,
                                            jint width, jint height)
{
  Java::Init(env);

  assert(native_view == NULL);
  native_view = new NativeView(env, obj, width, height);
  InitAsset();

  Profile::SetFiles(_T(""));

  event_queue = new EventQueue();

  return XCSoarInterface::Startup(NULL);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_runNative(JNIEnv *env, jobject obj)
{
  CommonInterface::main_window.event_loop();
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_deinitializeNative(JNIEnv *env, jobject obj)
{
  CommonInterface::main_window.reset();
  Fonts::Deinitialize();
  Graphics::Deinitialise();
  delete event_queue;
  delete native_view;
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_pauseNative(JNIEnv *env, jobject obj)
{
  event_queue->push(Event::PAUSE);

  CommonInterface::main_window.wait_paused();
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resumeNative(JNIEnv *env, jobject obj)
{
  event_queue->push(Event::RESUME);
}
