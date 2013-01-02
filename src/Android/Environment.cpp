/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Android/Environment.hpp"
#include "Java/Class.hpp"
#include "Java/String.hpp"
#include "Java/File.hpp"
#include "Util/StringUtil.hpp"

namespace Environment {
  static Java::TrivialClass cls;
  static jmethodID getExternalStorageDirectory_method;
  static jmethodID getExternalStoragePublicDirectory_method;
};

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
  if (getExternalStoragePublicDirectory_method == NULL)
    /* needs API level 8 */
    env->ExceptionClear();
}

void
Environment::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

static jstring
ToAbsolutePathChecked(JNIEnv *env, jobject file)
{
  if (file == NULL)
    return NULL;

  jstring path = Java::File::getAbsolutePath(env, file);
  env->DeleteLocalRef(file);
  return path;
}

static jstring
getExternalStorageDirectory(JNIEnv *env)
{
  jobject file = env->CallStaticObjectMethod(Environment::cls,
                                             Environment::getExternalStorageDirectory_method);
  return ToAbsolutePathChecked(env, file);
}

char *
Environment::getExternalStorageDirectory(char *buffer, size_t max_size)
{
  JNIEnv *env = Java::GetEnv();

  jstring value = ::getExternalStorageDirectory(env);
  if (value == NULL)
    return NULL;

  Java::String value2(env, value);
  value2.CopyTo(env, buffer, max_size);
  return buffer;
}

static jstring
getExternalStoragePublicDirectory(JNIEnv *env, const char *type)
{
  if (Environment::getExternalStoragePublicDirectory_method == NULL)
    /* needs API level 8 */
    return NULL;

  Java::String type2(env, type);
  jobject file = env->CallStaticObjectMethod(Environment::cls,
                                             Environment::getExternalStoragePublicDirectory_method,
                                             type2.Get());
  return ToAbsolutePathChecked(env, file);
}

char *
Environment::getExternalStoragePublicDirectory(char *buffer, size_t max_size,
                                               const char *type)
{
  JNIEnv *env = Java::GetEnv();
  jstring path = ::getExternalStoragePublicDirectory(env, type);
  if (path == NULL)
    return NULL;

  Java::String path2(env, path);
  path2.CopyTo(env, buffer, max_size);
  return buffer;
}
