/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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
#include "Android/SoundUtil.hpp"
#include "Android/Vibrator.hpp"
#include "Android/InternalSensors.hpp"
#include "Android/PortBridge.hpp"
#include "Android/BluetoothHelper.hpp"
#include "Android/NativeInputListener.hpp"
#include "Android/TextUtil.hpp"
#include "Android/LogCat.hpp"
#include "Android/Product.hpp"
#include "Android/Nook.hpp"
#include "Language/Language.hpp"
#include "Language/LanguageGlue.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Version.hpp"
#include "Screen/Debug.hpp"
#include "Look/GlobalFonts.hpp"
#include "Event/Globals.hpp"
#include "Event/Queue.hpp"
#include "Screen/OpenGL/Init.hpp"
#include "Dialogs/Message.hpp"
#include "Simulator.hpp"
#include "Profile/Profile.hpp"
#include "MainWindow.hpp"
#include "Startup.hpp"
#include "Interface.hpp"
#include "Java/Global.hpp"
#include "Java/File.hpp"
#include "Java/InputStream.hpp"
#include "Java/URL.hpp"
#include "Compiler.h"
#include "org_xcsoar_NativeView.h"
#include "IO/Async/GlobalIOThread.hpp"
#include "Thread/Debug.hpp"

#include "IOIOHelper.hpp"
#include "NativeBMP085Listener.hpp"
#include "BMP085Device.hpp"
#include "NativeI2CbaroListener.hpp"
#include "NativeBaroListener.hpp"
#include "I2CbaroDevice.hpp"
#include "BaroDevice.hpp"
#include "NativeNunchuckListener.hpp"
#include "NunchuckDevice.hpp"
#include "NativeVoltageListener.hpp"
#include "VoltageDevice.hpp"

#ifndef NDEBUG
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Buffer.hpp"
#endif

#include <assert.h>
#include <stdlib.h>

Context *context;

NativeView *native_view;

Vibrator *vibrator;
bool os_haptic_feedback_enabled;

IOIOHelper *ioio_helper;

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
  InternalSensors::Initialise(env);
  NativeInputListener::Initialise(env);
  PortBridge::Initialise(env);
  BluetoothHelper::Initialise(env);
  IOIOHelper::Initialise(env);
  NativeBMP085Listener::Initialise(env);
  BMP085Device::Initialise(env);
  NativeI2CbaroListener::Initialise(env);
  I2CbaroDevice::Initialise(env);
  NativeBaroListener::Initialise(env);
  BaroDevice::Initialise(env);
  NativeNunchuckListener::Initialise(env);
  NunchuckDevice::Initialise(env);
  NativeVoltageListener::Initialise(env);
  VoltageDevice::Initialise(env);

  context = new Context(env, _context);

  InitialiseDataPath();

  LogFormat(_T("Starting XCSoar %s"), XCSoar_ProductToken);

  OpenGL::Initialise();
  TextUtil::Initialise(env);

  assert(native_view == NULL);
  native_view = new NativeView(env, obj, width, height, xdpi, ydpi,
                               sdk_version, product);
#ifdef __arm__
  is_nook = strcmp(native_view->GetProduct(), "NOOK") == 0;
#endif

  event_queue = new EventQueue();

  SoundUtil::Initialise(env);
  Vibrator::Initialise(env);
  vibrator = Vibrator::Create(env, *context);

  ioio_helper = new IOIOHelper(env);

#ifdef __arm__
  if (IsNookSimpleTouch()) {
    is_dithered = Nook::EnterFastMode();

    /* enable USB host mode if this is a Nook */
    Nook::InitUsb();
  }
#endif

  ScreenInitialized();
  AllowLanguage();
  InitLanguage();
  return Startup();
}

void
OnLogCatFinished(bool crash_found)
{
  if (crash_found)
    CommonInterface::main_window->SendCrash();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_runNative(JNIEnv *env, jobject obj)
{
  InitThreadDebug();

  OpenGL::Initialise();

  CheckLogCat(*io_thread);

  CommonInterface::main_window->RunEventLoop();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_deinitializeNative(JNIEnv *env, jobject obj)
{
  if (IsNookSimpleTouch()) {
    Nook::ExitFastMode();
  }

  StopLogCat();

  InitThreadDebug();

  if (CommonInterface::main_window != nullptr) {
    CommonInterface::main_window->Destroy();
    delete CommonInterface::main_window;
    CommonInterface::main_window = nullptr;
  }

  DisallowLanguage();
  Fonts::Deinitialize();

  delete ioio_helper;
  ioio_helper = NULL;

  delete vibrator;
  vibrator = NULL;

  SoundUtil::Deinitialise(env);
  delete event_queue;
  event_queue = NULL;
  delete native_view;
  native_view = nullptr;

  TextUtil::Deinitialise(env);
  OpenGL::Deinitialise();
  ScreenDeinitialized();
  DeinitialiseDataPath();

  delete context;
  context = nullptr;

  BMP085Device::Deinitialise(env);
  NativeBMP085Listener::Deinitialise(env);
  I2CbaroDevice::Deinitialise(env);
  NativeI2CbaroListener::Deinitialise(env);
  BaroDevice::Deinitialise(env);
  NativeBaroListener::Deinitialise(env);
  NunchuckDevice::Deinitialise(env);
  NativeNunchuckListener::Deinitialise(env);
  VoltageDevice::Deinitialise(env);
  NativeVoltageListener::Deinitialise(env);
  IOIOHelper::Deinitialise(env);
  BluetoothHelper::Deinitialise(env);
  NativeInputListener::Deinitialise(env);
  InternalSensors::Deinitialise(env);
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

  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->AnnounceResize(width, height);

  event_queue->Purge(Event::RESIZE);

  Event event(Event::RESIZE, width, height);
  event_queue->Push(event);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_pauseNative(JNIEnv *env, jobject obj)
{
  if (event_queue == nullptr || CommonInterface::main_window == nullptr)
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
  if (event_queue == nullptr || CommonInterface::main_window == nullptr)
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
