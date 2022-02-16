/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Main.hpp"
#include "Environment.hpp"
#include "Components.hpp"
#include "Context.hpp"
#include "NativeView.hpp"
#include "Bitmap.hpp"
#include "SoundUtil.hpp"
#include "Vibrator.hpp"
#include "InternalSensors.hpp"
#include "GliderLink.hpp"
#include "Sensor.hpp"
#include "PortBridge.hpp"
#include "BluetoothHelper.hpp"
#include "UsbSerialHelper.hpp"
#include "NativeDetectDeviceListener.hpp"
#include "NativePortListener.hpp"
#include "NativeInputListener.hpp"
#include "NativeSensorListener.hpp"
#include "TextUtil.hpp"
#include "TextEntryDialog.hpp"
#include "Product.hpp"
#include "Nook.hpp"
#include "Language/Language.hpp"
#include "Language/LanguageGlue.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Version.hpp"
#include "Screen/Debug.hpp"
#include "Look/GlobalFonts.hpp"
#include "ui/window/Init.hpp"
#include "ui/display/Display.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#include "Dialogs/Message.hpp"
#include "Profile/Profile.hpp"
#include "MainWindow.hpp"
#include "Startup.hpp"
#include "Interface.hpp"
#include "java/Global.hxx"
#include "java/File.hxx"
#include "java/InputStream.hxx"
#include "java/URL.hxx"
#include "java/Closeable.hxx"
#include "util/Compiler.h"
#include "org_xcsoar_NativeView.h"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "net/http/Init.hpp"
#include "thread/Debug.hpp"
#include "util/Exception.hxx"

#include "IOIOHelper.hpp"
#include "BMP085Device.hpp"
#include "I2CbaroDevice.hpp"
#include "NunchuckDevice.hpp"
#include "VoltageDevice.hpp"

#include <cassert>
#include <stdlib.h>

using namespace UI;

unsigned android_api_level;

Context *context;

NativeView *native_view;

Vibrator *vibrator;
bool os_haptic_feedback_enabled;

BluetoothHelper *bluetooth_helper;
UsbSerialHelper *usb_serial_helper;
IOIOHelper *ioio_helper;

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_runNative(JNIEnv *env, jobject obj,
                                     jobject _context,
                                     jint width, jint height,
                                     jint xdpi, jint ydpi,
                                     jint sdk_version, jstring product)
try {
  Java::Init(env);

  android_api_level = sdk_version;

  InitThreadDebug();

  const ScopeGlobalAsioThread global_asio_thread;
  const Net::ScopeInit net_init(asio_thread->GetEventLoop());

  Java::Object::Initialise(env);
  Java::File::Initialise(env);
  Java::InputStream::Initialise(env);
  Java::InitialiseCloseable(env);
  Java::URL::Initialise(env);
  Java::URLConnection::Initialise(env);

  NativeView::Initialise(env);
  Context::Initialise(env);
  Environment::Initialise(env);
  AndroidBitmap::Initialise(env);
  NativeSensorListener::Initialise(env);
  InternalSensors::Initialise(env);
  GliderLink::Initialise(env);
  NativePortListener::Initialise(env);
  NativeInputListener::Initialise(env);
  AndroidSensor::Initialise(env);
  PortBridge::Initialise(env);
  const bool have_bluetooth = BluetoothHelper::Initialise(env);
  const bool have_usb_serial = UsbSerialHelper::Initialise(env);
  NativeDetectDeviceListener::Initialise(env);
  const bool have_ioio = IOIOHelper::Initialise(env);
  BMP085Device::Initialise(env);
  I2CbaroDevice::Initialise(env);
  NunchuckDevice::Initialise(env);
  VoltageDevice::Initialise(env);
  AndroidTextEntryDialog::Initialise(env);

  context = new Context(env, _context);

  InitialiseDataPath();

  LogFormat(_T("Starting XCSoar %s"), XCSoar_ProductToken);

  TextUtil::Initialise(env);

  assert(native_view == nullptr);
  native_view = new NativeView(env, obj, width, height, xdpi, ydpi,
                               product);
#ifdef __arm__
  is_nook = StringIsEqual(native_view->GetProduct(), "NOOK");
#endif

  SoundUtil::Initialise(env);
  Vibrator::Initialise(env);
  vibrator = Vibrator::Create(env, *context);

  if (have_bluetooth) {
    try {
      bluetooth_helper = new BluetoothHelper(env, *context);
    } catch (...) {
      LogError(std::current_exception(), "Failed to initialise Bluetooth");
    }
  }

  if (have_usb_serial) {
    try {
      usb_serial_helper = new UsbSerialHelper(env, *context);
    } catch (...) {
      LogError(std::current_exception(), "Failed to initialise USB serial support");
    }
  }

  if (have_ioio) {
    try {
      ioio_helper = new IOIOHelper(env);
    } catch (...) {
      LogError(std::current_exception(), "Failed to initialise IOIO");
    }
  }

#ifdef __arm__
  if (IsNookSimpleTouch()) {
    is_dithered = Nook::EnterFastMode();

    /* enable USB host mode if this is a Nook */
    Nook::InitUsb();
  }
#endif

  ScreenGlobalInit screen_init;

  AllowLanguage();
  InitLanguage();

  if (Startup(screen_init.GetDisplay()))
    CommonInterface::main_window->RunEventLoop();

  Shutdown();

  if (IsNookSimpleTouch()) {
    Nook::ExitFastMode();
  }

  if (CommonInterface::main_window != nullptr) {
    CommonInterface::main_window->Destroy();
    delete CommonInterface::main_window;
    CommonInterface::main_window = nullptr;
  }

  DisallowLanguage();
  Fonts::Deinitialize();

  delete ioio_helper;
  ioio_helper = nullptr;

  delete usb_serial_helper;
  usb_serial_helper = nullptr;

  delete bluetooth_helper;
  bluetooth_helper = nullptr;

  delete vibrator;
  vibrator = nullptr;

  SoundUtil::Deinitialise(env);
  delete native_view;
  native_view = nullptr;

  TextUtil::Deinitialise(env);

  DeinitialiseDataPath();

  delete context;
  context = nullptr;

  AndroidTextEntryDialog::Deinitialise(env);
  BMP085Device::Deinitialise(env);
  I2CbaroDevice::Deinitialise(env);
  NunchuckDevice::Deinitialise(env);
  VoltageDevice::Deinitialise(env);
  IOIOHelper::Deinitialise(env);
  NativeDetectDeviceListener::Deinitialise(env);
  UsbSerialHelper::Deinitialise(env);
  BluetoothHelper::Deinitialise(env);
  NativeInputListener::Deinitialise(env);
  NativePortListener::Deinitialise(env);
  InternalSensors::Deinitialise(env);
  NativeSensorListener::Deinitialise(env);
  GliderLink::Deinitialise(env);
  AndroidBitmap::Deinitialise(env);
  Environment::Deinitialise(env);
  Context::Deinitialise(env);
  NativeView::Deinitialise(env);
  Java::URL::Deinitialise(env);
} catch (...) {
  /* if an error occurs, rethrow the C++ exception as Java exception,
     to be displayed by the Java glue code */
  const auto msg = GetFullMessage(std::current_exception());
  jclass Exception = env->FindClass("java/lang/Exception");
  env->ThrowNew(Exception, msg.c_str());
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resizedNative(JNIEnv *env, jobject obj,
                                         jint width, jint height)
{
  if (event_queue == nullptr)
    return;

  if (auto *main_window = NativeView::GetPointer(env, obj))
    main_window->AnnounceResize({width, height});

  event_queue->Purge(UI::Event::RESIZE);

  UI::Event event(UI::Event::RESIZE, PixelPoint(width, height));
  event_queue->Inject(event);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_pauseNative(JNIEnv *env, jobject obj)
{
  auto *main_window = NativeView::GetPointer(env, obj);
  if (event_queue == nullptr || main_window == nullptr)
    return;
    /* event subsystem is not initialized, there is nothing to pause */

  main_window->Pause();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resumeNative(JNIEnv *env, jobject obj)
{
  auto *main_window = NativeView::GetPointer(env, obj);
  if (event_queue == nullptr || main_window == nullptr)
    return;
    /* event subsystem is not initialized, there is nothing to resume */

  main_window->Resume();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_setHapticFeedback(JNIEnv *env, jobject obj,
                                             jboolean on)
{
  os_haptic_feedback_enabled = on;
}
