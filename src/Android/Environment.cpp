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

#include "Environment.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "java/File.hxx"
#include "java/Path.hxx"
#include "util/StringUtil.hpp"

namespace Environment {
static Java::TrivialClass cls;
static jmethodID getExternalStorageDirectory_method;
static jmethodID getExternalStoragePublicDirectory_method;
} // namespace Environment

void
Environment::Initialise(JNIEnv *env)
{
  cls.Find(env, "android/os/Environment");

  getExternalStorageDirectory_method =
    env->GetStaticMethodID(cls, "getExternalStorageDirectory",
                           "()Ljava/io/File;");

  getExternalStoragePublicDirectory_method =
    env->GetStaticMethodID(cls, "getExternalStoragePublicDirectory",
                           "(Ljava/lang/String;)Ljava/io/File;");
  if (getExternalStoragePublicDirectory_method == nullptr)
    /* needs API level 8 */
    env->ExceptionClear();
}

void
Environment::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

static Java::String
getExternalStorageDirectory(JNIEnv *env)
{
  Java::File file{
    env,
    env->CallStaticObjectMethod(Environment::cls,
                                Environment::getExternalStorageDirectory_method),
  };

  return file.GetAbsolutePathChecked();
}

AllocatedPath
Environment::getExternalStorageDirectory() noexcept
{
  JNIEnv *env = Java::GetEnv();
  return Java::ToPathChecked(::getExternalStorageDirectory(env));
}

static Java::String
getExternalStoragePublicDirectory(JNIEnv *env, const char *type)
{
  if (Environment::getExternalStoragePublicDirectory_method == nullptr)
    /* needs API level 8 */
    return nullptr;

  Java::String type2(env, type);
  Java::File file(env, env->CallStaticObjectMethod(Environment::cls,
                                                   Environment::getExternalStoragePublicDirectory_method,
                                                   type2.Get()));
  return file.GetAbsolutePathChecked();
}

AllocatedPath
Environment::getExternalStoragePublicDirectory(const char *type) noexcept
{
  JNIEnv *env = Java::GetEnv();
  return Java::ToPathChecked(::getExternalStoragePublicDirectory(env, type));
}
