// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Main.hpp"
#include "ReceiveTask.hpp"
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
#include "util/ScopeExit.hxx"
#include "GlobalSettings.hpp"

#include "IOIOHelper.hpp"
#include "BMP085Device.hpp"
#include "I2CbaroDevice.hpp"
#include "NunchuckDevice.hpp"
#include "VoltageDevice.hpp"

#include <cassert>
#include <mutex>

#include <stdlib.h>

using namespace UI;

Context *context;

NativeView *native_view;

Vibrator *vibrator;

BluetoothHelper *bluetooth_helper;
UsbSerialHelper *usb_serial_helper;
IOIOHelper *ioio_helper;

/**
 * This mutex protects shutdown against other JNI calls, to avoid
 * races between shutdown (destruction of MainWindow and NativeView)
 * and new events being received on the Android main thread.
 */
static Mutex shutdown_mutex;

static void
InitNative(JNIEnv *env) noexcept
{
  Java::Init(env);
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

  NativeDetectDeviceListener::Initialise(env);
  BMP085Device::Initialise(env);
  I2CbaroDevice::Initialise(env);
  NunchuckDevice::Initialise(env);
  VoltageDevice::Initialise(env);
  AndroidTextEntryDialog::Initialise(env);
}

gcc_visibility_default
void
Java_org_xcsoar_NativeView_initNative(JNIEnv *env, [[maybe_unused]] jclass cls)
{
  static std::once_flag init_native_flag;

  std::call_once(init_native_flag, InitNative, env);
}

gcc_visibility_default
void
Java_org_xcsoar_NativeView_deinitNative(JNIEnv *env,
                                        [[maybe_unused]] jclass cls)
{
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
}

gcc_visibility_default
void
Java_org_xcsoar_NativeView_onConfigurationChangedNative([[maybe_unused]] JNIEnv *env,
                                                        [[maybe_unused]] jclass cls,
                                                        jboolean night_mode)
{
  if (night_mode == GlobalSettings::dark_mode)
    // no change
    return;

  GlobalSettings::dark_mode = night_mode;

  const std::scoped_lock shutdown_lock{shutdown_mutex};

  if (event_queue == nullptr)
    return;

  event_queue->Purge(UI::Event::LOOK);
  event_queue->Inject(UI::Event::LOOK);
}

gcc_visibility_default
JNIEXPORT jstring JNICALL
Java_org_xcsoar_NativeView_onReceiveXCTrackTask(JNIEnv *env,
                                                [[maybe_unused]] jclass cls,
                                                jstring data)
try {
  ReceiveXCTrackTask(Java::String::GetUTFChars(env, data).c_str());
  return nullptr;
} catch (...) {
  return env->NewStringUTF(GetFullMessage(std::current_exception()).c_str());
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_runNative(JNIEnv *env, jobject obj,
                                     jobject _context,
                                     jobject _permission_manager,
                                     jint width, jint height,
                                     jint xdpi, jint ydpi,
                                     jstring product)
try {
  const std::scoped_lock shutdown_lock{shutdown_mutex};

  InitThreadDebug();

  const bool have_bluetooth = BluetoothHelper::Initialise(env);
  const bool have_usb_serial = UsbSerialHelper::Initialise(env);
  const bool have_ioio = IOIOHelper::Initialise(env);

  context = new Context(env, _context);
  AtScopeExit() {
    delete context;
    context = nullptr;
  };

  permission_manager = env->NewGlobalRef(_permission_manager);
  AtScopeExit(env) { env->DeleteGlobalRef(permission_manager); };

  const ScopeGlobalAsioThread global_asio_thread;
  const Net::ScopeInit net_init(asio_thread->GetEventLoop());

  InitialiseDataPath();
  AtScopeExit() { DeinitialiseDataPath(); };

  LogFormat(_T("Starting %s"), XCSoar_ProductToken);

  TextUtil::Initialise(env);
  AtScopeExit(env) { TextUtil::Deinitialise(env); };

  assert(native_view == nullptr);
  native_view = new NativeView(env, obj, width, height, xdpi, ydpi,
                               product);
  AtScopeExit() {
    delete native_view;
    native_view = nullptr;
  };

  SoundUtil::Initialise(env);
  AtScopeExit(env) { SoundUtil::Deinitialise(env); };

  Vibrator::Initialise(env);
  vibrator = Vibrator::Create(env, *context);

  AtScopeExit() {
    delete vibrator;
    vibrator = nullptr;
  };

  if (have_bluetooth) {
    try {
      bluetooth_helper = new BluetoothHelper(env, *context,
                                             permission_manager);
    } catch (...) {
      LogError(std::current_exception(), "Failed to initialise Bluetooth");
    }
  }

  AtScopeExit() {
    delete bluetooth_helper;
    bluetooth_helper = nullptr;
  };

  if (have_usb_serial) {
    try {
      usb_serial_helper = new UsbSerialHelper(env, *context);
    } catch (...) {
      LogError(std::current_exception(), "Failed to initialise USB serial support");
    }
  }

  AtScopeExit() {
    delete usb_serial_helper;
    usb_serial_helper = nullptr;
  };

  if (have_ioio) {
    try {
      ioio_helper = new IOIOHelper(env);
    } catch (...) {
      LogError(std::current_exception(), "Failed to initialise IOIO");
    }
  }

  AtScopeExit() {
    delete ioio_helper;
    ioio_helper = nullptr;
  };

  ScreenGlobalInit screen_init;
  AtScopeExit() { Fonts::Deinitialize(); };

  AllowLanguage();
  AtScopeExit() { DisallowLanguage(); };

  InitLanguage();

  AtScopeExit() {
    if (CommonInterface::main_window != nullptr) {
      CommonInterface::main_window->Destroy();
      delete CommonInterface::main_window;
      CommonInterface::main_window = nullptr;
    }
  };

  {
    const ScopeUnlock shutdown_unlock{shutdown_mutex};

    if (Startup(screen_init.GetDisplay()))
      CommonInterface::main_window->RunEventLoop();
  }

  Shutdown();
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
  const std::scoped_lock shutdown_lock{shutdown_mutex};

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
Java_org_xcsoar_NativeView_surfaceDestroyedNative(JNIEnv *env, jobject obj)
{
  const std::scoped_lock shutdown_lock{shutdown_mutex};

  if (auto *main_window = NativeView::GetPointer(env, obj))
    main_window->InvokeSurfaceDestroyed();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_pauseNative(JNIEnv *env, jobject obj)
{
  const std::scoped_lock shutdown_lock{shutdown_mutex};

  if (event_queue == nullptr)
    /* event subsystem is not initialized, there is nothing to pause */
    return;

  auto *main_window = NativeView::GetPointer(env, obj);
  if (main_window == nullptr)
    return;

  main_window->Pause();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_resumeNative(JNIEnv *env, jobject obj)
{
  const std::scoped_lock shutdown_lock{shutdown_mutex};

  if (event_queue == nullptr)
    /* event subsystem is not initialized, there is nothing to pause */
    return;

  auto *main_window = NativeView::GetPointer(env, obj);
  if (main_window == nullptr)
    return;

  main_window->Resume();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_setHapticFeedback([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject obj,
                                             jboolean on)
{
  GlobalSettings::haptic_feedback = on;
}
