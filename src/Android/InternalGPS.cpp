/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Android/InternalGPS.hpp"
#include "Android/Context.hpp"
#include "Java/Class.hpp"
#include "org_xcsoar_InternalGPS.h"
#include "DeviceBlackboard.hpp"
#include "OS/Clock.hpp"
#include "Geo/Geoid.hpp"

InternalGPS::InternalGPS(JNIEnv *env, jobject obj)
  :Java::Object(env, obj)
{
  Java::Class cls(env, env->GetObjectClass(obj));
  mid_setLocationProvider = env->GetMethodID(cls, "setLocationProvider",
                                             "(Ljava/lang/String;)V");
}

InternalGPS::~InternalGPS()
{
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(get(), mid_setLocationProvider, NULL);
}

InternalGPS *
InternalGPS::create(JNIEnv *env, Context *context, unsigned index)
{
  Java::Class cls(env, "org/xcsoar/InternalGPS");

  jmethodID cid = env->GetMethodID(cls, "<init>",
                                   "(Landroid/content/Context;I)V");
  assert(cid != NULL);

  jobject obj = env->NewObject(cls, cid, context->get(), index);
  assert(obj != NULL);

  InternalGPS *internal_gps = new InternalGPS(env, obj);
  env->DeleteLocalRef(obj);

  return internal_gps;
}

JNIEXPORT void JNICALL
Java_org_xcsoar_InternalGPS_setConnected(JNIEnv *env, jobject obj,
                                         jint connected)
{
  jfieldID fid_index = env->GetFieldID(env->GetObjectClass(obj),
                                       "index", "I");
  unsigned index = env->GetIntField(obj, fid_index);

  ScopeLock protect(device_blackboard.mutex);
  NMEA_INFO &basic = device_blackboard.SetRealState(index);

  switch (connected) {
  case 0: /* not connected */
    basic.Connected.Clear();
    basic.LocationAvailable.Clear();
    break;

  case 1: /* waiting for fix */
    basic.Connected.Update(fixed(MonotonicClockMS()) / 1000);
    basic.gps.android_internal_gps = true;
    basic.LocationAvailable.Clear();
    break;

  case 2: /* connected */
    basic.Connected.Update(fixed(MonotonicClockMS()) / 1000);
    basic.gps.android_internal_gps = true;
    break;
  }

  device_blackboard.ScheduleMerge();
}

JNIEXPORT void JNICALL
Java_org_xcsoar_InternalGPS_setLocation(JNIEnv *env, jobject obj,
                                        jlong time, jint n_satellites,
                                        jdouble longitude, jdouble latitude,
                                        jboolean hasAltitude, jdouble altitude,
                                        jboolean hasBearing, jdouble bearing,
                                        jboolean hasSpeed, jdouble speed,
                                        jboolean hasAccuracy, jdouble accuracy,
                                        jboolean hasAcceleration, jdouble acceleration)
{
  jfieldID fid_index = env->GetFieldID(env->GetObjectClass(obj),
                                       "index", "I");
  unsigned index = env->GetIntField(obj, fid_index);

  ScopeLock protect(device_blackboard.mutex);
  NMEA_INFO &basic = device_blackboard.SetRealState(index);
  basic.UpdateClock();
  basic.Connected.Update(basic.clock);

  BrokenDateTime date_time = BrokenDateTime::FromUnixTimeUTC(time / 1000);
  fixed second_of_day = fixed(date_time.GetSecondOfDay()) +
    /* add the millisecond fraction of the original timestamp for
       better accuracy */
    fixed((unsigned)(time % 1000)) / 1000u;

  if (second_of_day < basic.Time && date_time > basic.DateTime)
    /* don't wrap around when going past midnight in UTC */
    second_of_day += fixed(24u * 3600u);

  basic.Time = second_of_day;
  basic.DateTime = date_time;

  basic.gps.satellites_used = n_satellites;
  basic.gps.real = true;
  basic.gps.android_internal_gps = true;
  basic.Location = GeoPoint(Angle::degrees(fixed(longitude)),
                            Angle::degrees(fixed(latitude)));
  if (n_satellites > 0)
    basic.LocationAvailable.Update(basic.clock);
  else
    basic.LocationAvailable.Clear();

  if (hasAltitude) {
    fixed GeoidSeparation = LookupGeoidSeparation(basic.Location);
    basic.GPSAltitude = fixed(altitude) - GeoidSeparation;
    basic.GPSAltitudeAvailable.Update(basic.clock);
  } else
    basic.GPSAltitudeAvailable.Clear();

  if (hasBearing) {
    basic.track = Angle::degrees(fixed(bearing));
    basic.track_available.Update(basic.clock);
  } else
    basic.track_available.Clear();

  if (hasSpeed) {
    basic.GroundSpeed = fixed(speed);
    basic.GroundSpeedAvailable.Update(basic.clock);
  }

  if (hasAccuracy)
    basic.gps.hdop = fixed(accuracy);

  if (hasAcceleration) {
    // TODO: use ACCELERATION_STATE::complement() ?!?
    basic.acceleration.available = true;
    basic.acceleration.g_load = fixed(acceleration);
  }

  device_blackboard.ScheduleMerge();
}
