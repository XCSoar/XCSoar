/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_ANDROID_MS5611_DEVICE_HPP
#define XCSOAR_ANDROID_MS5611_DEVICE_HPP

#include "MS5611Listener.hpp"
#include "Java/Object.hpp"
#include "Math/SelfTimingKalmanFilter1d.hpp"
#include "Compiler.h"

#include <jni.h>

class MS5611Device : private MS5611Listener {
  unsigned index;
  Java::Object obj;

  /**
   * This Kalman filter is used to smooth the pressure input.
   */
  SelfTimingKalmanFilter1d kalman_filter;

public:
  static void Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

  MS5611Device(unsigned index,
               JNIEnv *env, jobject holder,
               unsigned twi_num, unsigned sleeptime);

  ~MS5611Device();

private:
  /* virtual methods from class MS5611Listener */
  virtual void onMS5611Values(AtmosphericPressure pressure);
  virtual void onMS5611Error() gcc_override;
};

#endif
