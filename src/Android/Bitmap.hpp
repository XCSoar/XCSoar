// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <jni.h>
#include <cassert>

class AndroidBitmap {
  static jmethodID recycle_method;
  static jmethodID getWidth_method, getHeight_method;

public:
  static void Initialise(JNIEnv *env);
  static void Deinitialise([[maybe_unused]] JNIEnv *env) {}

  [[gnu::pure]]
  static void Recycle(JNIEnv *env, jobject bitmap) {
    assert(env != nullptr);
    assert(bitmap != nullptr);

    return env->CallVoidMethod(bitmap, recycle_method);
  }

  [[gnu::pure]]
  static unsigned GetWidth(JNIEnv *env, jobject bitmap) {
    assert(env != nullptr);
    assert(bitmap != nullptr);

    return env->CallIntMethod(bitmap, getWidth_method);
  }

  [[gnu::pure]]
  static unsigned GetHeight(JNIEnv *env, jobject bitmap) {
    assert(env != nullptr);
    assert(bitmap != nullptr);

    return env->CallIntMethod(bitmap, getHeight_method);
  }
};
