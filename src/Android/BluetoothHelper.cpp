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

#include "Android/BluetoothHelper.hpp"
#include "PortBridge.hpp"
#include "Java/String.hpp"
#include "Java/Class.hpp"

#include <map>
#include <string>

namespace BluetoothHelper {
  static Java::TrivialClass cls;
  static jmethodID isEnabled_method;
  static jmethodID getNameFromAddress_method;
  static jmethodID list_method, connect_method, createServer_method;

  static std::map<std::string, std::string> address_to_name;
}

bool
BluetoothHelper::Initialise(JNIEnv *env)
{
  assert(!cls.IsDefined());
  assert(env != NULL);

  if (!cls.FindOptional(env, "org/xcsoarte/BluetoothHelper"))
    /* Android < 2.0 doesn't have Bluetooth support */
    return false;

  isEnabled_method = env->GetStaticMethodID(cls, "isEnabled", "()Z");
  getNameFromAddress_method = env->GetStaticMethodID(cls, "getNameFromAddress",
                                                     "(Ljava/lang/String;)Ljava/lang/String;");
  list_method = env->GetStaticMethodID(cls, "list", "()[Ljava/lang/String;");
  connect_method = env->GetStaticMethodID(cls, "connect",
                                          "(Ljava/lang/String;)Lorg/xcsoarte/AndroidPort;");
  createServer_method = env->GetStaticMethodID(cls, "createServer",
                                               "()Lorg/xcsoarte/AndroidPort;");
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

  char name[256];
  Java::String::CopyTo(env, j_name, name, sizeof(name));
  env->DeleteLocalRef(j_name);

  auto j = address_to_name.insert(std::make_pair(x_address, name));
  return j.first->second.c_str();
}

jobjectArray
BluetoothHelper::list(JNIEnv *env)
{
  if (!cls.IsDefined())
    return NULL;

  /* call BluetoothHelper.connect() */

  return (jobjectArray)env->CallStaticObjectMethod(cls, list_method);
}

PortBridge *
BluetoothHelper::connect(JNIEnv *env, const char *address)
{
  if (!cls.IsDefined())
    return NULL;

  /* call BluetoothHelper.connect() */

  const Java::String address2(env, address);
  jobject obj = env->CallStaticObjectMethod(cls, connect_method,
                                            address2.Get());
  if (obj == NULL)
    return NULL;

  PortBridge *helper = new PortBridge(env, obj);
  env->DeleteLocalRef(obj);

  return helper;
}

PortBridge *
BluetoothHelper::createServer(JNIEnv *env)
{
  if (!cls.IsDefined())
    return NULL;

  jobject obj = env->CallStaticObjectMethod(cls, createServer_method);
  if (obj == NULL)
    return NULL;

  PortBridge *helper = new PortBridge(env, obj);
  env->DeleteLocalRef(obj);

  return helper;
}
