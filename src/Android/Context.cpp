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

AllocatedPath
Context::GetExternalFilesDir(JNIEnv *env) noexcept
{
  Java::Class cls{env, env->GetObjectClass(Get())};
  jmethodID method = env->GetMethodID(cls, "getExternalFilesDir",
                                      "(Ljava/lang/String;)Ljava/io/File;");
  assert(method);

  Java::File dir{env, env->CallObjectMethod(Get(), method, nullptr)};
  return ToPathChecked(dir.GetAbsolutePathChecked());
}

AllocatedPath
Context::GetExternalCacheDir(JNIEnv *env) noexcept
{
  Java::Class cls{env, env->GetObjectClass(Get())};
  jmethodID method = env->GetMethodID(cls, "getExternalCacheDir",
                                      "()Ljava/io/File;");
  assert(method);

  Java::File dir{env, env->CallObjectMethod(Get(), method)};
  return ToPathChecked(dir.GetAbsolutePathChecked());
}

Java::LocalObject
Context::GetSystemService(JNIEnv *env, jstring name)
{
  assert(env != nullptr);
  assert(name != nullptr);

  Java::Class cls(env, env->GetObjectClass(Get()));
  jmethodID method = env->GetMethodID(cls, "getSystemService",
                                      "(Ljava/lang/String;)Ljava/lang/Object;");
  assert(method);

  return {env, env->CallObjectMethod(Get(), method, name)};
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
