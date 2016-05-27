/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "BluetoothHelper.hpp"
#include "Context.hpp"
#include "Main.hpp"
#include "NativeLeScanCallback.hpp"
#include "PortBridge.hpp"
#include "Java/String.hxx"
#include "Java/Class.hxx"
#include "Java/Exception.hxx"

#include <map>
#include <string>
#include <stdexcept>

namespace BluetoothHelper {
  static Java::TrivialClass cls;
  static jfieldID hasLe_field;
  static jmethodID isEnabled_method;
  static jmethodID getNameFromAddress_method;
  static jmethodID list_method, connect_method, createServer_method;
  static jmethodID startLeScan_method, stopLeScan_method;

  static std::map<std::string, std::string> address_to_name;
}

bool
BluetoothHelper::Initialise(JNIEnv *env)
{
  assert(!cls.IsDefined());
  assert(env != nullptr);

  if (android_api_level < 5 ||
      !cls.FindOptional(env, "org/xcsoar/BluetoothHelper"))
    /* Android < 2.0 doesn't have Bluetooth support */
    return false;

  hasLe_field = env->GetStaticFieldID(cls, "hasLe", "Z");
  isEnabled_method = env->GetStaticMethodID(cls, "isEnabled", "()Z");
  getNameFromAddress_method = env->GetStaticMethodID(cls, "getNameFromAddress",
                                                     "(Ljava/lang/String;)Ljava/lang/String;");
  list_method = env->GetStaticMethodID(cls, "list", "()[Ljava/lang/String;");
  connect_method = env->GetStaticMethodID(cls, "connect",
                                          "(Landroid/content/Context;"
                                          "Ljava/lang/String;)"
                                          "Lorg/xcsoar/AndroidPort;");
  createServer_method = env->GetStaticMethodID(cls, "createServer",
                                               "()Lorg/xcsoar/AndroidPort;");

  startLeScan_method = env->GetStaticMethodID(cls, "startLeScan",
                                              "(Landroid/bluetooth/BluetoothAdapter$LeScanCallback;)Z");
  stopLeScan_method = env->GetStaticMethodID(cls, "stopLeScan",
                                             "(Landroid/bluetooth/BluetoothAdapter$LeScanCallback;)V");

  return true;
}

void
BluetoothHelper::Deinitialise(JNIEnv *env)
{
  cls.ClearOptional(env);
}

bool
BluetoothHelper::isEnabled(JNIEnv *env)
{
  return cls.IsDefined() &&
    env->CallStaticBooleanMethod(cls, isEnabled_method);
}

const char *
BluetoothHelper::GetNameFromAddress(JNIEnv *env, const char *address)
{
  assert(env != nullptr);
  assert(address != nullptr);

  if (!cls.IsDefined())
    return nullptr;

  std::string x_address(address);
  auto i = address_to_name.find(x_address);
  if (i != address_to_name.end())
    return i->second.c_str();

  const Java::String j_address(env, address);
  jstring j_name = (jstring)
    env->CallStaticObjectMethod(cls, getNameFromAddress_method,
                                j_address.Get());
  if (j_name == nullptr)
    return nullptr;

  std::string name = Java::String(env, j_name).ToString();

  auto j = address_to_name.insert(std::make_pair(std::move(x_address),
                                                 std::move(name)));
  return j.first->second.c_str();
}

jobjectArray
BluetoothHelper::list(JNIEnv *env)
{
  if (!cls.IsDefined())
    return nullptr;

  /* call BluetoothHelper.connect() */

  return (jobjectArray)env->CallStaticObjectMethod(cls, list_method);
}

bool
BluetoothHelper::HasLe(JNIEnv *env)
{
  return cls.IsDefined() && env->GetStaticBooleanField(cls, hasLe_field);
}

jobject
BluetoothHelper::StartLeScan(JNIEnv *env, LeScanCallback &_cb)
{
  assert(HasLe(env));

  jobject cb = NativeLeScanCallback::Create(env, _cb);
  if (cb == nullptr) {
    env->ExceptionClear();
    return nullptr;
  }

  if (!env->CallStaticBooleanMethod(cls, startLeScan_method, cb)) {
    env->ExceptionClear();
    env->DeleteLocalRef(cb);
    return nullptr;
  }

  return cb;
}

void
BluetoothHelper::StopLeScan(JNIEnv *env, jobject cb)
{
  assert(HasLe(env));

  env->CallStaticVoidMethod(cls, stopLeScan_method, cb);
  env->DeleteLocalRef(cb);
}

PortBridge *
BluetoothHelper::connect(JNIEnv *env, const char *address)
{
  if (!cls.IsDefined())
    throw std::runtime_error("Bluetooth not available");

  /* call BluetoothHelper.connect() */

  const Java::String address2(env, address);
  jobject obj = env->CallStaticObjectMethod(cls, connect_method,
                                            context->Get(), address2.Get());
  Java::RethrowException(env);
  if (obj == nullptr)
    return nullptr;

  PortBridge *helper = new PortBridge(env, obj);
  env->DeleteLocalRef(obj);

  return helper;
}

PortBridge *
BluetoothHelper::createServer(JNIEnv *env)
{
  if (!cls.IsDefined())
    throw std::runtime_error("Bluetooth not available");

  jobject obj = env->CallStaticObjectMethod(cls, createServer_method);
  Java::RethrowException(env);
  if (obj == nullptr)
    return nullptr;

  PortBridge *helper = new PortBridge(env, obj);
  env->DeleteLocalRef(obj);

  return helper;
}
