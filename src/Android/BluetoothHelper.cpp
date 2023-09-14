// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BluetoothHelper.hpp"
#include "Context.hpp"
#include "NativeDetectDeviceListener.hpp"
#include "NativeSensorListener.hpp"
#include "PortBridge.hpp"
#include "java/Env.hxx"
#include "java/String.hxx"
#include "java/Class.hxx"

#include <map>
#include <string>
#include <stdexcept>

static Java::TrivialClass cls;
static jmethodID ctor;
static jfieldID hasLe_field;
static jmethodID isEnabled_method;
static jmethodID getNameFromAddress_method;
static jmethodID connect_method, createServer_method;
static jmethodID hm10connect_method;
static jmethodID connectSensor_method;
static jmethodID addDetectDeviceListener_method;
static jmethodID removeDetectDeviceListener_method;

static std::map<std::string, std::string, std::less<>> address_to_name;

bool
BluetoothHelper::Initialise(JNIEnv *env) noexcept
{
  assert(env != nullptr);

  if (!cls.FindOptional(env, "org/xcsoar/BluetoothHelper"))
    /* Android < 2.0 doesn't have Bluetooth support */
    return false;


  ctor = env->GetMethodID(cls, "<init>",
                          "(Landroid/content/Context;"
                          "Lorg/xcsoar/PermissionManager;"
                          ")V");
  if (Java::DiscardException(env)) {
    /* need to check for Java exceptions again because the first
       method lookup initializes the Java class */
    cls.Clear(env);
    return false;
  }

  hasLe_field = env->GetFieldID(cls, "hasLe", "Z");
  isEnabled_method = env->GetMethodID(cls, "isEnabled", "()Z");
  getNameFromAddress_method = env->GetMethodID(cls, "getNameFromAddress",
                                               "(Ljava/lang/String;)Ljava/lang/String;");
  connectSensor_method = env->GetMethodID(cls, "connectSensor",
                                          "(Ljava/lang/String;Lorg/xcsoar/SensorListener;)"
                                          "Lorg/xcsoar/BluetoothSensor;");
  connect_method = env->GetMethodID(cls, "connect",
                                    "(Ljava/lang/String;)"
                                    "Lorg/xcsoar/AndroidPort;");
  createServer_method = env->GetMethodID(cls, "createServer",
                                         "()Lorg/xcsoar/AndroidPort;");

  hm10connect_method = env->GetMethodID(cls, "connectHM10",
                                        "(Ljava/lang/String;)"
                                        "Lorg/xcsoar/AndroidPort;");
  addDetectDeviceListener_method =
    env->GetMethodID(cls, "addDetectDeviceListener",
                     "(Lorg/xcsoar/DetectDeviceListener;)V");
  removeDetectDeviceListener_method =
    env->GetMethodID(cls, "removeDetectDeviceListener",
                     "(Lorg/xcsoar/DetectDeviceListener;)V");

  return true;
}

void
BluetoothHelper::Deinitialise(JNIEnv *env) noexcept
{
  cls.ClearOptional(env);
}

BluetoothHelper::BluetoothHelper(JNIEnv *env, Context &context,
                                 jobject permission_manager)
  :Java::GlobalObject(env,
                      Java::NewObjectRethrow(env, cls, ctor, context.Get(),
                                             permission_manager))
{
}

bool
BluetoothHelper::IsEnabled(JNIEnv *env) const noexcept
{
  return env->CallBooleanMethod(Get(), isEnabled_method);
}

const char *
BluetoothHelper::GetNameFromAddress(JNIEnv *env,
                                    const char *address) const noexcept
{
  assert(env != nullptr);
  assert(address != nullptr);

  const std::string_view x_address{address};
  if (auto i = address_to_name.find(x_address); i != address_to_name.end())
    return i->second.c_str();

  const Java::String j_address(env, address);
  jstring j_name = (jstring)
    env->CallObjectMethod(Get(), getNameFromAddress_method,
                          j_address.Get());
  if (j_name == nullptr)
    return nullptr;

  std::string name = Java::String(env, j_name).ToString();

  auto j = address_to_name.emplace(x_address, std::move(name));
  return j.first->second.c_str();
}

bool
BluetoothHelper::HasLe(JNIEnv *env) const noexcept
{
  return env->GetBooleanField(Get(), hasLe_field);
}

Java::LocalObject
BluetoothHelper::AddDetectDeviceListener(JNIEnv *env,
                                         DetectDeviceListener &_l) noexcept
{
  auto l = NativeDetectDeviceListener::Create(env, _l);
  env->CallVoidMethod(Get(), addDetectDeviceListener_method, l.Get());
  return l;
}

void
BluetoothHelper::RemoveDetectDeviceListener(JNIEnv *env, jobject l) noexcept
{
  env->CallVoidMethod(Get(), removeDetectDeviceListener_method, l);
}

Java::LocalObject
BluetoothHelper::connectSensor(JNIEnv *env, const char *_address,
                               SensorListener &_listener)
{
  const Java::String address{env, _address};
  auto listener = NativeSensorListener::Create(env, _listener);
  return Java::CallObjectMethodRethrow(env, Get(), connectSensor_method,
                                       address.Get(), listener.Get());
}

PortBridge *
BluetoothHelper::connect(JNIEnv *env, const char *address)
{
  /* call BluetoothHelper.connect() */

  const Java::String address2(env, address);
  auto obj = Java::CallObjectMethodRethrow(env, Get(), connect_method,
                                           address2.Get());
  assert(obj);

  return new PortBridge(env, obj);
}

PortBridge *
BluetoothHelper::createServer(JNIEnv *env)
{
  auto obj = Java::CallObjectMethodRethrow(env, Get(), createServer_method);
  assert(obj);

  return new PortBridge(env, obj);
}

PortBridge *
BluetoothHelper::connectHM10(JNIEnv *env, const char *address)
{
  /* call BluetoothHelper.connectHM10() */

  const Java::String address2(env, address);
  auto obj = Java::CallObjectMethodRethrow(env, Get(), hm10connect_method,
                                           address2.Get());
  assert(obj);

  return new PortBridge(env, obj);
}
