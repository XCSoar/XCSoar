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

#include "Android/Main.hpp"
#include "Android/Environment.hpp"
#include "Android/Context.hpp"
#include "Android/NativeView.hpp"
#include "Android/Timer.hpp"
#include "Android/SoundUtil.hpp"
#include "Android/Vibrator.hpp"
#include "Android/InternalSensors.hpp"
#include "Android/PortBridge.hpp"
#include "Android/BluetoothHelper.hpp"
#include "Android/NativeInputListener.hpp"
#include "Android/TextUtil.hpp"
#include "Android/LogCat.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Version.hpp"
#include "Screen/Debug.hpp"
#include "Look/Fonts.hpp"
#include "Event/Android/Queue.hpp"
#include "Screen/OpenGL/Init.hpp"
#include "Dialogs/Message.hpp"
#include "Simulator.hpp"
#include "Profile/Profile.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Java/Global.hpp"
#include "Java/File.hpp"
#include "Java/InputStream.hpp"
#include "Java/URL.hpp"
#include "Compiler.h"
#include "org_xcsoar_NativeView.h"
#include "IO/Async/GlobalIOThread.hpp"
#include "Thread/Debug.hpp"

#ifdef IOIOLIB
#include "Android/IOIOHelper.hpp"
#include "NativeBMP085Listener.hpp"
#include "BMP085Device.hpp"
#include "NativeMS5611Listener.hpp"
#include "MS5611Device.hpp"
#endif

#ifndef NDEBUG
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Buffer.hpp"
#endif

#include <assert.h>

Context *context;

NativeView *native_view;

EventQueue *event_queue;

Vibrator *vibrator;
bool os_haptic_feedback_enabled;

#ifdef IOIOLIB
IOIOHelper *ioio_helper;
#endif

extern "C" {
  /* workaround for
     http://code.google.com/p/android/issues/detail?id=23203 copied
     from https://bugzilla.mozilla.org/show_bug.cgi?id=734832 */
  __attribute__((weak)) void *__dso_handle;
}

gcc_visibility_default
JNIEXPORT jboolean JNICALL
Java_org_xcsoar_NativeView_initializeNative(JNIEnv *env, jobject obj,
                                            jobject _context,
                                            jint width, jint height,
                                            jint xdpi, jint ydpi,
                                            jint sdk_version, jstring product)
{
  InitThreadDebug();

  InitialiseIOThread();

  Java::Init(env);
  Java::File::Initialise(env);
  Java::InputStream::Initialise(env);
  Java::URL::Initialise(env);
  Java::URLConnection::Initialise(env);

  Environment::Initialise(env);
  AndroidTimer::Initialise(env);
  InternalSensors::Initialise(env);
  NativeInputListener::Initialise(env);
  PortBridge::Initialise(env);
  BluetoothHelper::Initialise(env);
#ifdef IOIOLIB
  IOIOHelper::Initialise(env);
  NativeBMP085Listener::Initialise(env);
  BMP085Device::Initialise(env);
  NativeMS5611Listener::Initialise(env);
  MS5611Device::Initialise(env);
#endif

  context = new Context(env, _context);

  InitialiseDataPath();

  LogStartUp(_T("Starting XCSoar %s"), XCSoar_ProductToken);

  OpenGL::Initialise();
  TextUtil::Initialise(env);

  assert(native_view == NULL);
  native_view = new NativeView(env, obj, width, height, xdpi, ydpi,
                               sdk_version, product);

  event_queue = new EventQueue();

  SoundUtil::Initialise(env);
  Vibrator::Initialise(env);
  vibrator = Vibrator::Create(env, *context);

#ifdef IOIOLIB
  ioio_helper = new IOIOHelper(env);
#endif

  ScreenInitialized();
  AllowLanguage();
  return XCSoarInterface::Startup();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_runNative(JNIEnv *env, jobject obj)
{
  InitThreadDebug();

  OpenGL::Initialise();

  if (CheckLogCat())
    ShowMessageBox(_T("How embarassing, we're terribly sorry!\n"
                      "Please submit a bug report and "
                      "include the file from the 'crash' directory.\n"
                      "http://www.xcsoar.org/trac/newticket\n"
                      "After your report, we'll fix it ASAP."),
                   _T("XCSoar has crashed recently"),
                   MB_OK|MB_ICONERROR);

  CommonInterface::main_window->RunEventLoop();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_deinitializeNative(JNIEnv *env, jobject obj)
{
  InitThreadDebug();

  delete CommonInterface::main_window;

  DisallowLanguage();
  Fonts::Deinitialize();

#ifdef IOIOLIB
  delete ioio_helper;
  ioio_helper = NULL;
#endif

  delete vibrator;
  vibrator = NULL;

  SoundUtil::Deinitialise(env);
  delete event_queue;
  event_queue = NULL;
  delete native_view;

  TextUtil::Deinitialise(env);
  OpenGL::Deinitialise();
  ScreenDeinitialized();
  DeinitialiseDataPath();

  delete context;

#ifdef IOIOLIB
  BMP085Device::Deinitialise(env);
  NativeBMP085Listener::Deinitialise(env);
  MS5611Device::Deinitialise(env);
  NativeMS5611Listener::Deinitialise(env);
  IOIOHelper::Deinitialise(env);
#endif
  BluetoothHelper::Deinitialise(env);
  NativeInputListener::Deinitialise(env);
  InternalSensors::Deinitialise(env);
  AndroidTimer::Deinitialise(env);
  Environment::Deinitialise(env);
  Java::URL::Deinitialise(env);

  DeinitialiseIOThread();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resizedNative(JNIEnv *env, jobject obj,
                                         jint width, jint height)
{
  if (event_queue == NULL)
    return;

  CommonInterface::main_window->AnnounceResize(width, height);

  event_queue->Purge(Event::RESIZE);

  Event event(Event::RESIZE, width, height);
  event_queue->Push(event);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_pauseNative(JNIEnv *env, jobject obj)
{
  if (event_queue == NULL)
    /* pause before we have initialized the event subsystem does not
       work - let's bail out, nothing is lost anyway */
    exit(0);

  CommonInterface::main_window->Pause();

  assert(num_textures == 0);
  assert(num_buffers == 0);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resumeNative(JNIEnv *env, jobject obj)
{
  if (event_queue == NULL)
    /* there is nothing here yet which can be resumed */
    exit(0);

  CommonInterface::main_window->Resume();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_setHapticFeedback(JNIEnv *env, jobject obj,
                                             jboolean on)
{
  os_haptic_feedback_enabled = on;
}
