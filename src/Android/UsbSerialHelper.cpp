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
#include "Context.hpp"
#include "NativeDetectDeviceListener.hpp"
#include "PortBridge.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"
#include "java/String.hxx"

static Java::TrivialClass cls;
static jmethodID ctor;
static jmethodID close_method;
static jmethodID connect_method;
static jmethodID addDetectDeviceListener_method;
static jmethodID removeDetectDeviceListener_method;

bool
UsbSerialHelper::Initialise(JNIEnv *env) noexcept
{
  assert(!cls.IsDefined());
  assert(env != nullptr);

  if (!cls.FindOptional(env, "org/xcsoar/UsbSerialHelper")) {
    /* Android < 3.1 doesn't have Usb Host support */
    return false;
  }

  ctor = env->GetMethodID(cls, "<init>",
                          "(Landroid/content/Context;)V");
  if (Java::DiscardException(env)) {
    /* need to check for Java exceptions again because the first
       method lookup initializes the Java class */
    cls.Clear(env);
    return false;
  }

  close_method = env->GetMethodID(cls, "close", "()V");
  connect_method = env->GetMethodID(cls, "connect",
                                    "(Ljava/lang/String;I)Lorg/xcsoar/AndroidPort;");

  addDetectDeviceListener_method =
    env->GetMethodID(cls, "addDetectDeviceListener",
                     "(Lorg/xcsoar/DetectDeviceListener;)V");
  removeDetectDeviceListener_method =
    env->GetMethodID(cls, "removeDetectDeviceListener",
                     "(Lorg/xcsoar/DetectDeviceListener;)V");

  return true;
}

void
UsbSerialHelper::Deinitialise(JNIEnv *env) noexcept
{
  cls.ClearOptional(env);
}

UsbSerialHelper::UsbSerialHelper(JNIEnv *env, Context &context)
  :Java::GlobalObject(env,
                      Java::NewObjectRethrow(env, cls, ctor, context.Get()))
{
}

UsbSerialHelper::~UsbSerialHelper() noexcept
{
  Java::GetEnv()->CallVoidMethod(Get(), close_method);
}

Java::LocalObject
UsbSerialHelper::AddDetectDeviceListener(JNIEnv *env,
                                         DetectDeviceListener &_l) noexcept
{
  auto l = NativeDetectDeviceListener::Create(env, _l);
  env->CallVoidMethod(Get(), addDetectDeviceListener_method, l.Get());
  return l;
}

void
UsbSerialHelper::RemoveDetectDeviceListener(JNIEnv *env, jobject l) noexcept
{
  env->CallVoidMethod(Get(), removeDetectDeviceListener_method, l);
}

PortBridge *
UsbSerialHelper::Connect(JNIEnv *env, const char *name, unsigned baud)
{
  Java::String name2(env, name);
  auto obj = Java::CallObjectMethodRethrow(env, Get(), connect_method,
                                           name2.Get(), (int)baud);
  assert(obj);

  return new PortBridge(env, obj);
}
