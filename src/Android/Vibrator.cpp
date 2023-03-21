// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vibrator.hpp"
#include "Main.hpp"
#include "Context.hpp"
#include "java/Class.hxx"

jmethodID Vibrator::cancel_method, Vibrator::vibrate_method;

void
Vibrator::Initialise(JNIEnv *env)
{
  Java::Class cls(env, "android/os/Vibrator");

  cancel_method = env->GetMethodID(cls, "cancel", "()V");
  vibrate_method = env->GetMethodID(cls, "vibrate", "(J)V");
}

Vibrator *
Vibrator::Create(JNIEnv *env, Context &context)
{
  const auto obj = context.GetVibrator(env);
  if (obj == nullptr)
    return nullptr;

  return new Vibrator(env, obj);
}

void
Vibrator::Cancel(JNIEnv *env)
{
  env->CallVoidMethod(object, cancel_method);
}

void
Vibrator::Vibrate(JNIEnv *env, unsigned duration_ms)
{
  env->CallVoidMethod(object, vibrate_method, (jlong)duration_ms);
}

bool Vibrator::IsOSHapticFeedbackEnabled()
{
  return os_haptic_feedback_enabled;
};
