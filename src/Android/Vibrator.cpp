// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vibrator.hpp"
#include "Context.hpp"
#include "java/Class.hxx"

jmethodID Vibrator::cancel_method, Vibrator::vibrate_method;

void
Vibrator::Initialise(JNIEnv *env) noexcept
{
  Java::Class cls(env, "android/os/Vibrator");

  cancel_method = env->GetMethodID(cls, "cancel", "()V");
  vibrate_method = env->GetMethodID(cls, "vibrate", "(J)V");
}

Vibrator *
Vibrator::Create(JNIEnv *env, Context &context) noexcept
{
  const auto obj = context.GetVibrator(env);
  if (obj == nullptr)
    return nullptr;

  return new Vibrator(env, obj);
}

void
Vibrator::Cancel(JNIEnv *env) noexcept
{
  env->CallVoidMethod(object, cancel_method);
}

void
Vibrator::Vibrate(JNIEnv *env, unsigned duration_ms) noexcept
{
  env->CallVoidMethod(object, vibrate_method, (jlong)duration_ms);
}
