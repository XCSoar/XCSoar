// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AndroidStorageHotplugMonitor.hpp"
#include "Storage/StorageHotplugMonitor.hpp"
#include "Android/Main.hpp"
#include "Android/Context.hpp"
#include "Android/StorageHotplugBridge.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"
#include "java/Global.hxx"
#include "LogFile.hpp"

#include <cassert>

/**
 * Static JNI references for StorageHotplugReceiver.
 * These are looked up lazily on first use.
 */
static Java::TrivialClass receiver_cls;
static jmethodID receiver_ctor;
static jmethodID receiver_start_method;
static jmethodID receiver_stop_method;

static bool
EnsureReceiverClass(JNIEnv *env) noexcept
{
  if (receiver_cls.IsDefined())
    return true;

  if (!receiver_cls.FindOptional(env, "org/xcsoar/StorageHotplugReceiver"))
    return false;

  receiver_ctor = env->GetMethodID(
      receiver_cls, "<init>",
      "(Landroid/content/Context;)V");
  receiver_start_method = env->GetMethodID(
      receiver_cls, "start", "()V");
  receiver_stop_method = env->GetMethodID(
      receiver_cls, "stop", "()V");

  if (Java::DiscardException(env)) {
    receiver_cls.Clear(env);
    return false;
  }

  return true;
}

AndroidStorageHotplugMonitor::AndroidStorageHotplugMonitor(
    StorageHotplugHandler &handler) noexcept
  : handler_(handler) {}

AndroidStorageHotplugMonitor::~AndroidStorageHotplugMonitor() noexcept
{
  Stop();
}

void
AndroidStorageHotplugMonitor::Start() noexcept
{
  if (receiver_.IsDefined())
    return; // already running

  JNIEnv *env = Java::GetEnv();
  if (env == nullptr || context == nullptr)
    return;

  if (!EnsureReceiverClass(env)) {
    LogFormat("AndroidStorageHotplugMonitor: "
              "StorageHotplugReceiver class not found");
    return;
  }

  try {
    Java::LocalObject obj{env,
        env->NewObject(receiver_cls, receiver_ctor, context->Get())};
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
      return;
    }
    if (!obj)
      return;

    receiver_.Set(env, obj.Get());
    env->CallVoidMethod(receiver_.Get(), receiver_start_method);
    if (env->ExceptionCheck())
      env->ExceptionClear();

    // Register the global handler so JNI callbacks arrive here.
    SetAndroidStorageHotplugHandler(&handler_);
  } catch (...) {
    LogError(std::current_exception(),
             "AndroidStorageHotplugMonitor::Start failed");
  }
}

void
AndroidStorageHotplugMonitor::Stop() noexcept
{
  if (!receiver_.IsDefined())
    return;

  // Unregister the global handler first.
  SetAndroidStorageHotplugHandler(nullptr);

  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    return;

  env->CallVoidMethod(receiver_.Get(), receiver_stop_method);
  if (env->ExceptionCheck())
    env->ExceptionClear();

  receiver_.Clear(env);
}
