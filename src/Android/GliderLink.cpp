/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Util/StaticString.hxx"
#include "org_xcsoar_GliderLinkReceiver.h"
#include "Compiler.h"
#include "Components.hpp"
#include "Blackboard/DeviceBlackboard.hpp"

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_GliderLinkReceiver_setGliderLinkInfo(
    JNIEnv* env, jclass cls, jlong gid, jstring callsign,
    jdouble latitude, jdouble longitude, jdouble altitude,
    jdouble gspeed, jdouble vspeed, jint bearing) {

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(0);
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

  traffic->location_available = true;
  traffic->location = GeoPoint(Angle::Degrees(longitude),
                              Angle::Degrees(latitude));

  traffic->altitude_available = fixed(altitude) > fixed(-10000);
  traffic->altitude = fixed(altitude);
  traffic->speed_received = fixed(gspeed) >= fixed(0.1);
  traffic->speed = fixed(gspeed);
  traffic->climb_rate_received = fixed(vspeed) > fixed(-8675309);
  traffic->climb_rate = fixed(vspeed);
  traffic->track_received = fixed(bearing) < fixed(361);
  traffic->track = Angle::Degrees(bearing);

  // set time of fix to current time
  traffic->valid.Update(basic.clock);

  device_blackboard->ScheduleMerge();
}
