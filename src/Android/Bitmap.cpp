// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Bitmap.hpp"
#include "java/Class.hxx"

jmethodID AndroidBitmap::recycle_method;
jmethodID AndroidBitmap::getWidth_method;
jmethodID AndroidBitmap::getHeight_method;

void
AndroidBitmap::Initialise(JNIEnv *env)
{
  Java::Class cls(env, "android/graphics/Bitmap");

  recycle_method = env->GetMethodID(cls, "recycle", "()V");
  assert(recycle_method != nullptr);

  getWidth_method = env->GetMethodID(cls, "getWidth", "()I");
  assert(getWidth_method != nullptr);

  getHeight_method = env->GetMethodID(cls, "getHeight", "()I");
  assert(getHeight_method != nullptr);
}
