// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Context.hpp"
#include "java/Class.hxx"
#include "java/File.hxx"
#include "java/Path.hxx"
#include "java/String.hxx"
#include "system/Path.hpp"

static Java::TrivialClass cls;
static jmethodID getExternalFilesDir_method,
  getExternalFilesDirs_method,
  getExternalMediaDirs_method,
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
  getExternalMediaDirs_method = env->GetMethodID(cls, "getExternalMediaDirs", "()[Ljava/io/File;");
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
    if (dir)
      previous = result.emplace_after(previous, ToPath(dir.GetAbsolutePath()));
  }

  return result;
}

std::forward_list<AllocatedPath>
Context::GetExternalMediaDirs(JNIEnv *env) const noexcept
{
  assert(env != nullptr);

  const Java::LocalRef<jobjectArray> array{
    env,
    (jobjectArray)env->CallObjectMethod(Get(), getExternalMediaDirs_method,
                                        nullptr),
  };

  assert(array);

  const jsize n = env->GetArrayLength(array);

  std::forward_list<AllocatedPath> result;
  auto previous = result.before_begin();

  for (jsize i = 0; i < n; ++i) {
    Java::File dir{env, env->GetObjectArrayElement(array, i)};
    if (dir)
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
