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

#include "Context.hpp"
#include "java/Class.hxx"
#include "java/File.hxx"
#include "java/Path.hxx"
#include "java/String.hxx"
#include "system/Path.hpp"

static Java::TrivialClass cls;
static jmethodID getExternalFilesDir_method,
  getExternalFilesDirs_method,
  getExternalCacheDir_method,
  getSystemService_method;

void
Context::Initialise(JNIEnv *env) noexcept
{
  cls.Find(env, "android/content/Context");

  getExternalFilesDir_method = env->GetMethodID(cls, "getExternalFilesDir",
                                                "(Ljava/lang/String;)Ljava/io/File;");
  getExternalFilesDirs_method = env->GetMethodID(cls, "getExternalFilesDirs",
                                                 "(Ljava/lang/String;)[Ljava/io/File;");
  getExternalCacheDir_method = env->GetMethodID(cls, "getExternalCacheDir",
                                                "()Ljava/io/File;");
  getSystemService_method = env->GetMethodID(cls, "getSystemService",
                                             "(Ljava/lang/String;)Ljava/lang/Object;");
}

void
Context::Deinitialise(JNIEnv *env) noexcept
{
  cls.Clear(env);
}

AllocatedPath
Context::GetExternalFilesDir(JNIEnv *env) noexcept
{
  Java::File dir{env, env->CallObjectMethod(Get(), getExternalFilesDir_method, nullptr)};
  return ToPathChecked(dir.GetAbsolutePathChecked());
}

std::forward_list<AllocatedPath>
Context::GetExternalFilesDirs(JNIEnv *env) const noexcept
{
  assert(env != nullptr);

  const Java::LocalRef<jobjectArray> array{
    env,
    (jobjectArray)env->CallObjectMethod(Get(), getExternalFilesDirs_method,
                                        nullptr),
  };

  assert(array);

  const jsize n = env->GetArrayLength(array);

  std::forward_list<AllocatedPath> result;
  auto previous = result.before_begin();

  for (jsize i = 0; i < n; ++i) {
    Java::File dir{env, env->GetObjectArrayElement(array, i)};
    previous = result.emplace_after(previous, ToPath(dir.GetAbsolutePath()));
  }

  return result;
}

AllocatedPath
Context::GetExternalCacheDir(JNIEnv *env) noexcept
{
  Java::File dir{env, env->CallObjectMethod(Get(), getExternalCacheDir_method)};
  return ToPathChecked(dir.GetAbsolutePathChecked());
}

Java::LocalObject
Context::GetSystemService(JNIEnv *env, jstring name)
{
  assert(env != nullptr);
  assert(name != nullptr);

  return {env, env->CallObjectMethod(Get(), getSystemService_method, name)};
}

Java::LocalObject
Context::GetSystemService(JNIEnv *env, const char *name)
{
  assert(env != nullptr);
  assert(name != nullptr);

  Java::String name2(env, name);
  return GetSystemService(env, name2);
}

Java::LocalObject
Context::GetVibrator(JNIEnv *env)
{
  return GetSystemService(env, "vibrator");
}
