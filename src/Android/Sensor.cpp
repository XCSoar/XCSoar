// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "Sensor.hpp"
#include "java/Class.hxx"

namespace AndroidSensor {

static jmethodID getState_method;

void
Initialise(JNIEnv *env) noexcept
{
	Java::Class cls(env, "org/xcsoar/AndroidSensor");
	getState_method = env->GetMethodID(cls, "getState", "()I");
}

PortState
GetState(JNIEnv *env, jobject object) noexcept
{
	return (PortState)env->CallIntMethod(object, getState_method);
}

} // namespace Java
