/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; version 2
  of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "UsbSerialHelper.hpp"
#include "PortBridge.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"

namespace UsbSerialHelper {

static Java::TrivialClass cls;
static jmethodID isEnabled_method;
static jmethodID list_method;
static jmethodID connect_method;

bool
Initialise(JNIEnv *env) noexcept
{
  assert(!cls.IsDefined());
  assert(env != nullptr);

  if (!cls.FindOptional(env, "org/xcsoar/UsbSerialHelper")) {
    /* Android < 3.1 doesn't have Usb Host support */
    return false;
  }

  isEnabled_method = env->GetStaticMethodID(cls, "isEnabled", "()Z");
  list_method = env->GetStaticMethodID(cls, "list", "()[Ljava/lang/String;");
  connect_method = env->GetStaticMethodID(cls, "connect",
                                          "(Ljava/lang/String;I)Lorg/xcsoar/AndroidPort;");

  return true;
}

void
Deinitialise(JNIEnv *env) noexcept
{
  cls.ClearOptional(env);
}

bool
isEnabled(JNIEnv *env) noexcept
{
  return cls.IsDefined() &&
    env->CallStaticBooleanMethod(cls, isEnabled_method);
}

PortBridge *
connectDevice(JNIEnv *env, const char *name, unsigned baud) noexcept
{
  if (!cls.IsDefined())
    return nullptr;

  Java::String name2(env, name);
  jobject obj = env->CallStaticObjectMethod(cls, connect_method, name2.Get(), (int)baud);
  Java::RethrowException(env);
  if (obj == nullptr)
    return nullptr;

  PortBridge *bridge = new PortBridge(env, obj);
  env->DeleteLocalRef(obj);
  return bridge;
}

jobjectArray
list(JNIEnv *env) noexcept
{
  if (!cls.IsDefined())
    return nullptr;

  return (jobjectArray)env->CallStaticObjectMethod(cls, list_method);
}

} // namespace UsbSerialHelper
