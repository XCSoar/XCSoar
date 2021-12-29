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
