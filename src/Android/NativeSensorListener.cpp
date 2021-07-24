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

#include "NativeSensorListener.hpp"
#include "SensorListener.hpp"
#include "Main.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "org_xcsoar_NativeSensorListener.h"

namespace NativeSensorListener {
static Java::TrivialClass cls;
static jmethodID ctor;
static jfieldID ptr_field;
} // namespace NativeSensorListener

void
NativeSensorListener::Initialise(JNIEnv *env) noexcept
{
  cls.Find(env, "org/xcsoar/NativeSensorListener");
  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativeSensorListener::Deinitialise(JNIEnv *env) noexcept
{
  cls.Clear(env);
}

Java::LocalObject
NativeSensorListener::Create(JNIEnv *env,
                             SensorListener &cb) noexcept
{
  return {env, env->NewObject(cls, ctor, (jlong)&cb)};
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeSensorListener_onConnected(JNIEnv *env, jobject obj,
                                                 jint connected)
{
  jlong ptr = env->GetLongField(obj, NativeSensorListener::ptr_field);
  if (ptr == 0)
    return;

  auto &listener = *(SensorListener *)ptr;
  listener.OnConnected(connected);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeSensorListener_onLocationSensor(JNIEnv *env, jobject obj,
                                                      jlong time, jint n_satellites,
                                                      jdouble longitude, jdouble latitude,
                                                      jboolean hasAltitude, jdouble altitude,
                                                      jboolean hasBearing, jdouble bearing,
                                                      jboolean hasSpeed, jdouble ground_speed,
                                                      jboolean hasAccuracy, jdouble accuracy,
                                                      jboolean hasAcceleration, jdouble acceleration)
{
  jlong ptr = env->GetLongField(obj, NativeSensorListener::ptr_field);
  if (ptr == 0)
    return;

  auto &listener = *(SensorListener *)ptr;
  listener.OnLocationSensor(time, n_satellites, longitude, latitude,
                            hasAltitude, altitude,
                            hasBearing, bearing,
                            hasSpeed, ground_speed,
                            hasAccuracy, accuracy,
                            hasAcceleration, acceleration);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeSensorListener_onAccelerationSensor(JNIEnv *env, jobject obj,
                                                          jfloat ddx, jfloat ddy,
                                                          jfloat ddz)
{
  jlong ptr = env->GetLongField(obj, NativeSensorListener::ptr_field);
  if (ptr == 0)
    return;

  auto &listener = *(SensorListener *)ptr;
  listener.OnAccelerationSensor(ddx, ddy, ddz);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeSensorListener_onRotationSensor(JNIEnv *env, jobject obj,
                                                      jfloat dtheta_x, jfloat dtheta_y,
                                                      jfloat dtheta_z)
{
  jlong ptr = env->GetLongField(obj, NativeSensorListener::ptr_field);
  if (ptr == 0)
    return;

  auto &listener = *(SensorListener *)ptr;
  listener.OnRotationSensor(dtheta_x, dtheta_y, dtheta_z);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeSensorListener_onMagneticFieldSensor(JNIEnv *env, jobject obj,
                                                           jfloat h_x, jfloat h_y,
                                                           jfloat h_z)
{
  jlong ptr = env->GetLongField(obj, NativeSensorListener::ptr_field);
  if (ptr == 0)
    return;

  auto &listener = *(SensorListener *)ptr;
  listener.OnMagneticFieldSensor(h_x, h_y, h_z);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeSensorListener_onBarometricPressureSensor(JNIEnv *env,
                                                                jobject obj,
                                                                jfloat pressure,
                                                                jfloat sensor_noise_variance)
{
  jlong ptr = env->GetLongField(obj, NativeSensorListener::ptr_field);
  if (ptr == 0)
    return;

  auto &listener = *(SensorListener *)ptr;
  listener.OnBarometricPressureSensor(pressure, sensor_noise_variance);
}
