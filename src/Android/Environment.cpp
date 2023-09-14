// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Environment.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "java/File.hxx"
#include "java/Path.hxx"
#include "util/StringUtil.hpp"

namespace Environment {

static Java::TrivialClass cls;
static jmethodID getExternalStoragePublicDirectory_method;

void
Initialise(JNIEnv *env) noexcept
{
  cls.Find(env, "android/os/Environment");

  getExternalStoragePublicDirectory_method =
    env->GetStaticMethodID(cls, "getExternalStoragePublicDirectory",
                           "(Ljava/lang/String;)Ljava/io/File;");
}

void
Deinitialise(JNIEnv *env) noexcept
{
  cls.Clear(env);
}

AllocatedPath
GetExternalStoragePublicDirectory(JNIEnv *env, const char *type) noexcept
{
  Java::String type2(env, type);
  Java::File file(env, env->CallStaticObjectMethod(cls,
                                                   getExternalStoragePublicDirectory_method,
                                                   type2.Get()));
  return Java::ToPathChecked(file.GetAbsolutePathChecked());
}

} // namespace Environment
