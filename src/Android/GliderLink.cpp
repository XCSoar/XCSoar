/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2018 The XCSoar Project
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

#include "GliderLink.hpp"
#include "org_xcsoar_GliderLinkReceiver.h"
#include "Compiler.h"
#include "Components.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Context.hpp"

Java::TrivialClass GliderLink::gl_cls;
jmethodID GliderLink::gl_ctor_id, GliderLink::close_method;


bool
GliderLink::Initialise(JNIEnv *env)
{
  assert(!gl_cls.IsDefined());
  assert(env != nullptr);

  gl_cls.Find(env, "org/xcsoar/GliderLinkReceiver");

  gl_ctor_id = env->GetMethodID(gl_cls, "<init>",
                                 "(Landroid/content/Context;I)V");
  close_method = env->GetMethodID(gl_cls, "close", "()V");

  return true;
}

void
GliderLink::Deinitialise(JNIEnv *env)
{
  gl_cls.Clear(env);
}

GliderLink* GliderLink::create(JNIEnv* env, Context* context,
                                         unsigned index) {
  assert(gl_cls != nullptr);

  // Construct GliderLinkReceiver object.
  jobject obj =
    env->NewObject(gl_cls, gl_ctor_id, context->Get(), index);
  assert(obj != nullptr);

  GliderLink *glider_link = new GliderLink(env, obj);
  env->DeleteLocalRef(obj);

  return glider_link;
}

GliderLink::GliderLink(JNIEnv* env, jobject obj)
    : obj(env, obj) {
}

GliderLink::~GliderLink() {
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj.Get(), close_method);
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_GliderLinkReceiver_setGliderLinkInfo(
    JNIEnv* env, jclass cls, jint index, jlong gid, jstring callsign,
    jdouble latitude, jdouble longitude, jdouble altitude,
    jdouble gspeed, jdouble vspeed, jint bearing) {

  // GliderLink uses these special values in case they don't have a real value  
  const double ALT_NONE = -10000.0;
  const double BEARING_NONE = 361.0;
  const double GSPEED_NONE = -1.0;
  const double VSPEED_NONE = -8675309.0;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  GliderLinkTrafficList &traffic_list = basic.glink_data.traffic;

  GliderLinkId id = GliderLinkId((uint32_t)gid);

  GliderLinkTraffic *traffic = traffic_list.FindTraffic(id);
  if (traffic == nullptr) {
    traffic = traffic_list.AllocateTraffic();
    if (traffic == nullptr)
      // no more slots available
      return;

    traffic->Clear();
    traffic->id = id;

    traffic_list.new_traffic.Update(basic.clock);
  }

  const char *nativeString = env->GetStringUTFChars(callsign, JNI_FALSE);
  traffic->name.SetASCII(nativeString);
  env->ReleaseStringUTFChars(callsign, nativeString);

  traffic->location = GeoPoint(Angle::Degrees(longitude),
                              Angle::Degrees(latitude));

  traffic->altitude_received = altitude > ALT_NONE;
  if (traffic->altitude_received)
    traffic->altitude = altitude;
  traffic->speed_received = gspeed >= GSPEED_NONE;
  if (traffic->speed_received)
    traffic->speed = gspeed;
  traffic->climb_rate_received = vspeed > VSPEED_NONE;
  if (traffic->climb_rate_received)
    traffic->climb_rate = vspeed;
  traffic->track_received = bearing < BEARING_NONE;
  if (traffic->track_received)
    traffic->track = Angle::Degrees(bearing);

  // set time of fix to current time
  traffic->valid.Update(basic.clock);

  device_blackboard->ScheduleMerge();
}
