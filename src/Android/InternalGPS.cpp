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
#include "Android/NativeView.hpp"
#include "org_xcsoar_InternalGPS.h"
#include "DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "OS/Clock.hpp"
#include "Device/Geoid.h"

InternalGPS *
InternalGPS::create(JNIEnv *env, NativeView *native_view)
{
  jobject context = native_view->get_context();

  Java::Class cls(env, "org/xcsoar/InternalGPS");

  jmethodID cid = env->GetMethodID(cls, "<init>",
                                   "(Landroid/content/Context;)V");
  assert(cid != NULL);

  jobject obj = env->NewObject(cls, cid, context);
  assert(obj != NULL);
  env->DeleteLocalRef(context);

  InternalGPS *internal_gps = new InternalGPS(env, obj);
  env->DeleteLocalRef(obj);

  return internal_gps;
}

JNIEXPORT void JNICALL
Java_org_xcsoar_InternalGPS_setConnected(JNIEnv *env, jobject obj,
                                         jint connected)
{
  mutexBlackboard.Lock();
  NMEA_INFO &basic = device_blackboard.SetRealState();

  switch (connected) {
  case 0: /* not connected */
    basic.Connected.clear();
    basic.LocationAvailable.clear();
    break;

  case 1: /* waiting for fix */
    basic.Connected.update(fixed(MonotonicClockMS()) / 1000);
    basic.LocationAvailable.clear();
    break;

  case 2: /* connected */
    basic.Connected.update(fixed(MonotonicClockMS()) / 1000);
    break;
  }

  device_blackboard.Merge();
  mutexBlackboard.Unlock();
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
  mutexBlackboard.Lock();

  NMEA_INFO &basic = device_blackboard.SetRealState();
  basic.Connected.update(fixed(MonotonicClockMS()) / 1000);

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

  basic.gps.SatellitesUsed = n_satellites;
  basic.gps.real = true;
  basic.gps.AndroidInternalGPS = true;
  basic.Location = GeoPoint(Angle::degrees(fixed(longitude)),
                            Angle::degrees(fixed(latitude)));
  if (n_satellites > 0)
    basic.LocationAvailable.update(basic.Time);
  else
    basic.LocationAvailable.clear();

  if (hasAltitude) {
    fixed GeoidSeparation = LookupGeoidSeparation(basic.Location);
    basic.GPSAltitude = fixed(altitude) - GeoidSeparation;
    basic.GPSAltitudeAvailable.update(basic.Time);
  } else
    basic.GPSAltitudeAvailable.clear();

  if (hasBearing) {
    basic.TrackBearing = Angle::degrees(fixed(bearing));
    basic.TrackBearingAvailable.update(basic.Time);
  } else
    basic.TrackBearingAvailable.clear();

  if (hasSpeed) {
    basic.GroundSpeed = fixed(speed);
    basic.GroundSpeedAvailable.update(basic.Time);
  }

  if (hasAccuracy)
    basic.gps.HDOP = fixed(accuracy);

  if (hasAcceleration) {
    // TODO: use ACCELERATION_STATE::complement() ?!?
    basic.acceleration.Available = true;
    basic.acceleration.Gload = fixed(acceleration);
  }

  device_blackboard.Merge();
  mutexBlackboard.Unlock();
}
