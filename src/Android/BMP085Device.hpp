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

#ifndef XCSOAR_ANDROID_BMP085_DEVICE_HPP
#define XCSOAR_ANDROID_BMP085_DEVICE_HPP

#include "BMP085Listener.hpp"
#include "Java/Object.hxx"
#include "Math/SelfTimingKalmanFilter1d.hpp"
#include "Compiler.h"

#include <jni.h>

class BMP085Device final : private BMP085Listener {
  unsigned index;
  Java::GlobalObject obj;

  /**
   * This Kalman filter is used to smooth the pressure input.
   */
  SelfTimingKalmanFilter1d kalman_filter;

public:
  static void Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

  BMP085Device(unsigned index,
               JNIEnv *env, jobject holder,
               unsigned twi_num, unsigned eoc_pin,
               unsigned oversampling);

  ~BMP085Device();

private:
  /* virtual methods from class BMP085Listener */
  virtual void onBMP085Values(double temperature,
                              AtmosphericPressure pressure) override;
  virtual void onBMP085Error() override;
};

#endif
