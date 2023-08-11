// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class Context;

class Vibrator {
  static jmethodID cancel_method, vibrate_method;

  Java::GlobalObject object;

public:
  /**
   * Global initialisation.  Looks up the methods of the
   * Vibrator Java class.
   */
  static void Initialise(JNIEnv *env) noexcept;

  Vibrator(JNIEnv *env, jobject obj) noexcept
    :object(env, obj) {}

  static Vibrator *Create(JNIEnv *env, Context &context) noexcept;

  void Cancel(JNIEnv *env) noexcept;
  void Vibrate(JNIEnv *env, unsigned duration_ms) noexcept;
};
